#include <ESP8266WiFi.h>
#include "form_html.h"
#include "form_css.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define STASSID "myssid"
#define STAPSK  "password"

#define UNUSED "UNUSED"
const String zone_id_tmpl = "##ZONE_ID##";
const String zone_name_tmpl = "##ZONE_NAME##";
const String zone_checked_tmpl = "##CHECKED##";
const String msg_tmpl = "##USER_MSG##";

const String element_template = (
                                  "<input id=\"zone_##ZONE_ID##\" name=\"zone_##ZONE_ID##\" class=\"element checkbox\" type=\"checkbox\" value=1 ##CHECKED##/ color=red>"
                                  "<label class=\"choice\" for=\"zone_##ZONE_ID##\">##ZONE_ID## ##ZONE_NAME##</label>"
                                  "\n"
                                );

const String runtime_template = (
  "<label for=\"run_minutes\">Set Run Minutes:</label>"
  "<input type=\"text\" id=\"run_minutes_id\" name=\"run_minutes_name\" value=\"10\">"
);

const String msg_template = ("<div class=\"user_msg\">##USER_MSG##</div>");

#define ZONE_OFF 0
#define ZONE_ON 1
#define ZONE_UNUSED 2
#define MAX_ACTIVE_ZONES 1
#define ZONES_POSSIBLE 16
#define MAX_RUN_TIME_MINUTES 30

const String get_zone_state_name(int state) {
  switch (state) {
    case ZONE_OFF: return String("ZONE_OFF");
    case ZONE_ON: return String("ZONE_ON");
  }
}

int zone_state[ZONES_POSSIBLE];
int new_zone_state[ZONES_POSSIBLE];

// pin_zone_map keys are the shift register pins
// values are the zones on the particular order
// are wired to relays on the PCB
int pin_zone_map[ZONES_POSSIBLE] = {
  14,
  12,
  10,
  8,
  6,
  4,
  2,
  0,
  1,
  3,
  5,
  7,
  9,
  11,
  13,
  15,
};

// order should align with zone state order
String zone_name[] = {
  "Back Lower Shrubs",
  "Back Lower Main",
  "Back Wildlife Garden",
  "Back Lawn",
  "Back Shrubs",
  "Front Lawn",
  "Front House Shrubs",
  "Front South Beds",
  "Front North Beds",
  "Front North Wall Drip",
  UNUSED,
  UNUSED,
  UNUSED,
  UNUSED,
  UNUSED,
  UNUSED,
};

int clockPin = D7;
int latchPin = D6;
int dataPin = D5;
int OEPin = D4; // output enable
int SCLRPin = D3; // clear output
unsigned long start_run_marker_ms = 0; // time zones started
unsigned long run_interval_ms = 0; // zone run time
int run_seconds_remaining = 0; // global tracking of running count down seconds

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

#define YLINE 23

#define ZONE_OFF 0
#define ZONE_ON 1

#define LINE_ONE 0
#define LINE_TWO 1

#define CLIENT_READ_TIME 300

int display_offset[2] = {
  0,
  10,
};

void initialize_zones_off(int *arr, int size) {
    for (int x = 0; x < size; x++) {
        arr[x] = ZONE_OFF;
//        Serial.println("set zone: " + String(x) + " to value: " + String(arr[x]));
    }
}

void print_zones_to_serial(int *arr, int size) {
    printf("printing: %d elements\n", size);
    for (int x = 0; x < size; x++) {
      Serial.println("zone: " + String(x) + " has value: " + String(arr[x]));
    }
}

#define MSG_RUNNING Running

String user_msg = "";

// draw a single non-running sprinkler starting at the
// position specified
void draw_sprinkler(int x, int y) {
  display.fillRect(x, y, 11, 11, BLACK);
  display.drawRect(x, y, 11, 11, WHITE);
  display.drawPixel(x + 5, y + 5, WHITE);
}

// draw running lines across the sprinkler for the
// specified zone slanted according to offset
void draw_running(int zone, int offset) {
  int x = zone * 13;
  int y = YLINE + 2;
  // wrap to next row over 5 zones
  if (zone > 4) {
    x = (zone - 5) * 13;
    y += 12;
  }
  display.drawLine(x + offset,      y,               x + 10 - offset,  y + 10,      WHITE);
  display.drawLine(x,               y + 10 - offset, x + 10,           y + offset,  WHITE);
}

// print line of text on oled display at
// specified y-offset
void print_to_oled_at_y(String text, int yoffset) {
  // TODO: check for (text height + yoffset) < YLINE
  int x = 0;
  display.setCursor(x, yoffset);
  display.println(text);
}

// clear and update two-line oled display
// max 10 characters per line
void print_to_oled(String line_one, String line_two) {
  display.fillRect(0, 0, display.width(), YLINE, BLACK);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  print_to_oled_at_y(line_one, display_offset[LINE_ONE]);
  print_to_oled_at_y(line_two, display_offset[LINE_TWO]);
}

void print_to_oled(char *line) {
  display.setCursor(0, 0);
  display.println(line);
}

// animate running sprinklers
void animate_sprinklers() {
  // cycle through each animation step, skip 11 since this
  // is the corner same as 0
  for (int phase = 0; phase < 10; phase++) {
    draw_base_display();
    // cycle through all running sprinklers
    for (int zone = 0; zone < sizeof(zone_state) / sizeof(zone_state[0]); zone++) {
      if (zone_state[zone]) {
        draw_running(zone, phase);
      }
    }
    display.display();
  }
}

// draw all ten non-running sprinklers
// animation can be layered on top
void draw_base_display() {
  for (int x = 0; x + 11 < 65; x += 13) {
    draw_sprinkler(x, YLINE + 2);
    draw_sprinkler(x, YLINE + 14);
  }
}

// set zone_state to match new states and
// update corresponding pins to match
void output_zone_states() {
  digitalWrite(latchPin, LOW);
  for (int relay_pin = ZONES_POSSIBLE - 1; relay_pin >= 0; relay_pin--) {
    int zone = pin_zone_map[relay_pin];
    int state = new_zone_state[zone];
    zone_state[zone] = state;
    Serial.println(String("Setting zone " + String(zone) + " to state (" + state + ") " + get_zone_state_name(state)));
    digitalWrite(clockPin, LOW);
    if (state)
      digitalWrite(dataPin, HIGH);
    else
      digitalWrite(dataPin, LOW);
    digitalWrite(clockPin, HIGH);
  }
  digitalWrite(latchPin, HIGH);
  initialize_zones_off(new_zone_state, ZONES_POSSIBLE);
}

/**
 * Return true if any zones are active, false otherwise
 */
bool zones_active() {
  for (int zone = 0; zone < sizeof(zone_state) / sizeof(zone_state[0]); zone++) {
    if (zone_state[zone]) {
      return true;
    }
  }
  return false;
}

/** 
 * update display to match current run mode
 * if any zones are on:
 *   - update display text for current state
 *   - animate corresponding zones on display
 */
void update_display() {
  if (zones_active()) {
      int mins = run_seconds_remaining / 60;
      int secs = run_seconds_remaining % 60;
      String time_display = String(mins) + "m " + String(secs) + "s";
      print_to_oled("Sprinkling", time_display);
      animate_sprinklers();
  } else {
    print_to_oled("Sprinklers", "Idle");
    draw_base_display();
    display.display();
  }
}

void clear_oled_text() {
  print_to_oled("", "");
}

const char* ssid = STASSID;
const char* password = STAPSK;

WiFiServer server(80);

void setup() {
  Serial.begin(115200);

  
  server.setNoDelay(true);

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(OEPin, OUTPUT);
  pinMode(SCLRPin, OUTPUT);

  // initialize output pins to off
  //   SCLR pin (shift register clear) is provided so that the
  //   outputs may be put into a known state which should be
  //   done prior to enabling the outputs
  // Disable outputs
  digitalWrite(OEPin, HIGH);
  // Clear output register by driving SCLR low
  digitalWrite(SCLRPin, LOW);

  initialize_zones_off(zone_state, ZONES_POSSIBLE);
  print_zones_to_serial(zone_state, ZONES_POSSIBLE);
  initialize_zones_off(new_zone_state, ZONES_POSSIBLE);
  print_zones_to_serial(new_zone_state, ZONES_POSSIBLE);
  delay(100);
  // Enable output register to recieve new values
  digitalWrite(SCLRPin, HIGH);

  delay(100);
  output_zone_states();

  // Enable outputs for all operations going forward
  digitalWrite(OEPin, LOW);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  delay(100);
  draw_base_display();
  display.display();

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  print_to_oled("WiFi", "Connecting");
  display.display();

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  print_to_oled("WiFi", "Connected");
  display.display();
  Serial.println("");
  Serial.println("WiFi connected");
  delay(500);

//  print_to_oled(WiFi.localIP());
  clear_oled_text();
  display.setCursor(0, 0);
  display.println(WiFi.localIP());
  display.display();
  delay(2000);

  // Start the server
  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());

  print_to_oled("Web server", "started");
  display.display();
  delay(1000);


  // Just in case, initialize outputs to ensure everything is off
  output_zone_states();

  Serial.println("Listening for connections...");
}

String get_html_msg() {
  String html = msg_template.substring(0);
  html.replace(msg_tmpl, user_msg);
  return html;
}

String get_html_form_elements() {
  String elements_html;
  for (int zone = 0; zone < sizeof(zone_state) / sizeof(zone_state[0]); zone++) {
    if (zone_name[zone] == UNUSED) {
      Serial.println("Skipping unused zone for web UI: " + String(zone + 1));
      continue;
    }
    int zone_pretty_id = zone + 1;
    int zone_value = zone_state[zone];
    String string_html = element_template.substring(0);
    string_html.replace(zone_id_tmpl, String(zone_pretty_id));
    string_html.replace(zone_name_tmpl, zone_name[zone]);
    if (zone_state[zone]) {
      string_html.replace(zone_checked_tmpl, String("checked"));
    } else {
      string_html.replace(zone_checked_tmpl, String(""));
    }
    elements_html += string_html;
  }
  elements_html += runtime_template;
  return elements_html;
}

void print_ms(int ms, String text) { 
  int run_ms = millis() - ms; 
  Serial.print("***" + text + "***  ms: "); 
  Serial.println(run_ms);
}

/**
 * Check if spinkler run time is over
 * If so, turn off all zones
 */
void check_run_time() {
  if (zones_active()) {
    unsigned long time_since_start_ms = (unsigned long)(millis() - start_run_marker_ms);
    int ms_left = (int)run_interval_ms - (int)time_since_start_ms;
    int seconds_left = ms_left/1000;
    if (ms_left < 0) {
      Serial.println("Disabling zones");
      initialize_zones_off(zone_state, ZONES_POSSIBLE);
      output_zone_states();
    } else {
      user_msg = String(run_seconds_remaining) + "s remaining";
      if (run_seconds_remaining != seconds_left) {
        run_seconds_remaining = seconds_left;
        Serial.println("Run time remaining: " + String(run_seconds_remaining) + "s");
      }
    }
  }
}

/**
 * Get client request as string and return if client connected
 */
String get_request(WiFiClient client) {
  String req = "";
  if (client) {
    client.setTimeout(CLIENT_READ_TIME);
    print_to_oled("Processing", "request...");
    display.display();
    // Wait until the client sends some data
    Serial.println("web client connected");
    unsigned long timeout = millis() + 3000;
    Serial.print("waiting for client.available() ");
    while (!client.available() && millis() < timeout) {
      Serial.print(".");
      delay(1);
    }
    Serial.println("");
    if (millis() > timeout) {
      Serial.println("timeout");
      client.flush();
      client.stop();
      Serial.println("Client Timeout");
      return req;
    }
    req = client.readString();
    Serial.println("req is '" + req + "'");
    client.flush();
  }
  return req;
}

void process_request(String req, String path) {
  // read form elements
//  int activate_index = req.indexOf("/activate");
  if (path == "/activate") {
    // Serial.println("Processing /activate, index is '" + String(activate_index) + "'");
    // Serial.println("***************\n" + req + "\n****************");
    int params_index = req.indexOf("\r\n\r\n");
    if (params_index == -1) {
      Serial.println("/activate called, but no header/body break found");
    } else {
      String body = req.substring(params_index + 4);
      Serial.println(String("Body: '" + body + "'"));
      int last_index = 0;
      int param_index = 0;
      int zone_on_count = 0;
      do {
        param_index = body.indexOf("&", last_index);
        if (param_index > -1) {
          String param = body.substring(last_index, param_index);
          Serial.println("Got param substring: '" + param + "'");
          int equals_index = param.indexOf("=");
          String field_id = param.substring(0, equals_index);
          String field_value = param.substring(equals_index + 1);
          // Serial.println("DEBUG field_id = '" + String(field_id) + "'");
          // Serial.println("DEBUG field_value = '" + String(field_value) + "'");
          if (field_id == "run_minutes_name") {
            int run_time_minutes = field_value.toInt();
            if (run_time_minutes > MAX_RUN_TIME_MINUTES) {
              Serial.println("run time selected is too long: '" + String(run_time_minutes) + "'");
              run_time_minutes = 0;
            }
            run_interval_ms = (unsigned long)(run_time_minutes * 60 * 1000);
            start_run_marker_ms = millis();
        
            Serial.println("new stop run interval is " + String(run_interval_ms));
            Serial.println("new stop run marker is " + String(start_run_marker_ms));
          } else if (field_id.startsWith("zone_")) {
            int input_index = param.indexOf("_");
            String zone_id = param.substring(input_index + 1, equals_index);
            String zone_value = param.substring(equals_index + 1);
            int int_zone_id = zone_id.toInt() - 1;
            if (zone_on_count < MAX_ACTIVE_ZONES) {
              Serial.println("Zone '" + zone_id + "'(" + String(int_zone_id) + "): '" + zone_value + "'");
              new_zone_state[int_zone_id] = ZONE_ON;
              ++zone_on_count;
            } else {
              Serial.println(
                "ZONE index " + String(int_zone_id) + 
                " excluded due to max zones. Current count: " + String(zone_on_count)
              );
              user_msg = "Too Many Zones Selected. Max is " + String(MAX_ACTIVE_ZONES);
            }
          }
          last_index = param_index + 1;
        }
      } while (param_index > -1);
      output_zone_states();
    }
  } else {
    Serial.println("No processing for request: '" + req + "'");
  }
}

void loop() {
  int start_loop_ms = millis();
  user_msg = "";
  check_run_time();
  WiFiClient client = server.available();
  String req = get_request(client);
  if (req.length()) {
    int first_space_index = req.indexOf(" ");
    int proto_space_index = req.indexOf(" HTTP/");
    String path = req.substring(first_space_index + 1, proto_space_index);
    Serial.println("path is '" + path + "'");
    if (path == "/form.css") {
      Serial.println("Got request for /form.css");
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
      client.print(form_css);
    } else if (path == "/favicon.ico") {
      Serial.println("Got request for /favicon.ico");
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
    } else {
      process_request(req, path);
      check_run_time(); // hack to print time remaining after form submission processed
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      client.print(form_html_head);
      client.print(get_html_msg());
      client.print(get_html_form_elements());
      client.print(form_html_tail);
    }
    client.flush();
    delay(1);
    Serial.println("Client disconnected");
  }
  update_display();
}
