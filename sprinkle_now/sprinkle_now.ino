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

//#define ZONE_OFF 0
//#define ZONE_ON 1

enum zone_states {
  ZONE_OFF,
  ZONE_ON
};

const String get_zone_state_name(zone_states state) {
  switch (state) {
    case ZONE_OFF: return String("ZONE_OFF");
    case ZONE_ON: return String("ZONE_ON");
  }
}

zone_states zone_state[] = {
  ZONE_OFF,
  ZONE_ON,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
};

zone_states new_zone_state[] = {
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_ON,
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
void set_zone_pin(int zone, zone_states state) {
  Serial.println(String("Setting zone " + String(zone) + " to state " + get_zone_state_name(state)));
  //  digitalWrite(zone_pin[zone], state);
}

// set zone_state to match new states and
// update corresponding pins to match
void update_zone_states() {
  for (int zone = 0; zone < sizeof(new_zone_state) / sizeof(new_zone_state[0]); zone++) {
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
int val;

void setup() {
  val = 0;
  Serial.begin(115200);

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);

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

  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
    if (val) {
      print_to_oled(String("Connecting"), String(ssid));
      toggle();
    } else {
      print_to_oled(String("Connecting"), String(ssid) + String("."));
      toggle();
    }
    display.display();
  }
  print_to_oled("WiFi", "Connected");
  Serial.println("");
  Serial.println("WiFi connected");
  delay(2000);

  clear_oled_text();

  // Start the server
  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());

}

void toggle() {
  // toggle led each request
  if (val == 0) {
    val = 1;
  } else {
    val = 0;
  }
  // Set GPIO2 to toggle value
  digitalWrite(2, val);
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

    //element_1_1=1&element_1_4=1&submit=Submit
    // Read the first line of the request
    //    String req = client.readStringUntil('\r');
    String req = client.readStringUntil('%');
    //    String param = req.readStringUntil('%');
    Serial.print("req is '");
    Serial.print(req);
    Serial.print("'");
    //    Serial.print("param is '");
    //    Serial.print(param);
    //    Serial.println("'");
    Serial.println("");

    client.flush();

    if (req.indexOf("/form.css") != -1) {
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
      client.print(form_css);
    } else if (req.indexOf("/favicon.ico") != -1) {
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
    } else {
      // read form elements
      if (req.indexOf("/activate") != -1) {
        for (int zone = 0; zone < sizeof(zone_state) / sizeof(zone_state[0]); zone++) {
          // todo process form elements, save in new_zone_state
          ;
        }
        update_zone_states();
      }
      toggle();
      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      client.print(form_html_head);
      client.print(R"=====(
        <input id="element_1_1" name="element_1_1" class="element checkbox" type="checkbox" value="1" checked/>
        <label class="choice" for="element_1_1">1 Back  Lower Shrubs</label>
        <input id="element_1_2" name="element_1_2" class="element checkbox" type="checkbox" value="1" />
        <label class="choice" for="element_1_2">2 Back Lower Main</label>
        <input id="element_1_3" name="element_1_3" class="element checkbox" type="checkbox" value="1" />
        <label class="choice" for="element_1_3">3 Back Wildlife Garden</label>
        <input id="element_1_4" name="element_1_4" class="element checkbox" type="checkbox" value="1" checked/>
        <label class="choice" for="element_1_4">4 Back Lawn</label>
        <input id="element_1_5" name="element_1_5" class="element checkbox" type="checkbox" value="1" />
        <label class="choice" for="element_1_5">5 Back Shrubs</label>
        <input id="element_1_6" name="element_1_6" class="element checkbox" type="checkbox" value="1" />
        <label class="choice" for="element_1_6">6 Front Lawn</label>
        <input id="element_1_7" name="element_1_7" class="element checkbox" type="checkbox" value="1" />
        <label class="choice" for="element_1_7">7 Front House Shrubs</label>
        <input id="element_1_8" name="element_1_8" class="element checkbox" type="checkbox" value="1" />
        <label class="choice" for="element_1_8">8 Front South Beds</label>
        <input id="element_1_9" name="element_1_9" class="element checkbox" type="checkbox" value="1" />
        <label class="choice" for="element_1_9">9 Front North Beds</label>
        <input id="element_1_10" name="element_1_10" class="element checkbox" type="checkbox" value="1" />
        <label class="choice" for="element_1_10">10 Front North Wall Drip</label>
        )=====");
      client.print(form_html_tail);
    }

    client.flush();
    delay(1);
    Serial.println("Client disconnected");
  } else {
    update_display();
  }
}
