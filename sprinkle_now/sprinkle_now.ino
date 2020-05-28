#include <ESP8266WiFi.h>
#include "form_html.h"
#include "form_css.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifndef STASSID
#define STASSID "myssid"
#define STAPSK  "mypasswd"
#endif

const String zone_id_tmpl = "##ZONE_ID##";
const String zone_name_tmpl = "##ZONE_NAME##";
const String zone_checked_tmpl = "##CHECKED##";

const String element_template = (
                                  "<input id=\"zone_##ZONE_ID##\" name=\"zone_##ZONE_ID##\" class=\"element checkbox\" type=\"checkbox\" value=1 ##CHECKED##/>"
                                  "<label class=\"choice\" for=\"zone_##ZONE_ID##\">##ZONE_ID## ##ZONE_NAME##</label>"
                                  "\n"
                                );

#define ZONE_OFF 0
#define ZONE_ON 1

const String get_zone_state_name(int state) {
  switch (state) {
    case ZONE_OFF: return String("ZONE_OFF");
    case ZONE_ON: return String("ZONE_ON");
  }
}

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
};

int zone_pin[] = {
  D2,
  D5,
  D6,
  D7,
  D8,
  D9,
  D10,
  D11,
  D12,
  D13,
};

int zone_state[] = {
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
};

int new_zone_state[] = {
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
};

#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

#define YLINE 23

#define ZONE_OFF 0
#define ZONE_ON 1

#define LINE_ONE 0
#define LINE_TWO 1

int display_offset[2] = {
  0,
  10,
};

#define MSG_RUNNING Running

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

// Set digital output pin state for zone to match
// zone run state
void set_zone_pin(int zone, int state) {
  Serial.println(String("Setting zone " + String(zone) + " to state " + get_zone_state_name(state)));
  digitalWrite(zone_pin[zone], state);
}

// set zone_state to match new states and
// update corresponding pins to match
void update_zone_states() {
  for (int zone = 0; zone < sizeof(zone_state) / sizeof(zone_state[0]); zone++) {
    zone_state[zone] = new_zone_state[zone];
    set_zone_pin(zone, zone_state[zone]);
  }
}

// update display to match current run mode
// if any zones are on:
//   - update display text for current state
//   - animate corresponding zones on display
void update_display() {
  print_to_oled("Sprinkling", "");
  animate_sprinklers();
}

void clear_oled_text() {
  print_to_oled("", "");
}

const char* ssid = STASSID;
const char* password = STAPSK;

WiFiServer server(80);

void setup() {
  Serial.begin(115200);

  // prepare GPIO
  for (int pin_id = 0; pin_id < sizeof(zone_pin) / sizeof(zone_pin[0]); pin_id++) {
    int pin = zone_pin[pin_id];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
  }

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
  delay(2000);

  clear_oled_text();

  // Start the server
  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());

  print_to_oled("Server", "started");
  display.display();
  delay(2000);
}

String get_html_form_elements() {
  String elements_html;
  for (int zone = 0; zone < sizeof(zone_state) / sizeof(zone_state[0]); zone++) {
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
  return elements_html;
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (client) {

    print_to_oled("Processing", "request...");
    display.display();

    // Wait until the client sends some data
    Serial.println("web client connected");
    unsigned long timeout = millis() + 3000;
    while (!client.available() && millis() < timeout) {
      delay(1);
    }
    if (millis() > timeout) {
      Serial.println("timeout");
      client.flush();
      client.stop();
      return;
    }

    String req = client.readString();
    Serial.println("req is '" + req + "'");
    client.flush();

    if (req.indexOf("/form.css") != -1) {
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
      client.print(form_css);
    } else if (req.indexOf("/favicon.ico") != -1) {
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
    } else {
      // read form elements
      if (req.indexOf("/activate") != -1) {
        int params_index = req.indexOf("\r\n\r\n");
        if (params_index == -1) {
          Serial.println("uh on, no header/body break found");
        } else {
          Serial.println(String("Body index is ") + String(params_index));
          String body = req.substring(params_index + 4);
          Serial.println(String("Body: " + body));
          // initialize all states to zero since form only provides active states
          for (int zone = 0; zone < sizeof(zone_state) / sizeof(zone_state[0]); zone++) {
            new_zone_state[zone] = ZONE_OFF;
          }
          int last_index = 0;
          int param_index = 0;
          do {
            param_index = body.indexOf("&", last_index);
            if (param_index > -1) {
              String param = body.substring(last_index, param_index);
              Serial.println("Got substring: '" + param + "'");
              // get zone id
              int zone_id_index = param.indexOf("_");
              int equals_index = param.indexOf("=");

              String zone_id = param.substring(zone_id_index + 1, equals_index);
              String zone_value = param.substring(equals_index + 1);
              int int_zone_id = zone_id.toInt() - 1;

              Serial.println("Zone '" + zone_id + "': '" + zone_value + "'");
              new_zone_state[int_zone_id] = ZONE_ON;

              // get zone value
              last_index = param_index + 1;
            }
          } while (param_index > -1);
        }
        update_zone_states();
      }
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      client.print(form_html_head);
      client.print(get_html_form_elements());
      client.print(form_html_tail);
    }

    client.flush();
    delay(1);
    Serial.println("Client disconnected");
  } else {
    update_display();
  }
}
