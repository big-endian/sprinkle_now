#include <ESP8266WiFi.h>
#include "form_html.h"
#include "form_css.h"

#ifndef STASSID
#define STASSID "myssid"
#define STAPSK  "mypasswd"
#endif

#define ZONE_OFF 0
#define ZONE_ON 1


int zone_state[10 ] = {
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

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

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
  if (!client) {
    return;
  }



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

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  if (req.indexOf("/form.css") != -1) {
    client.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
    client.print(form_css);
  } else if (req.indexOf("/favicon.ico") != -1) {
    client.print("HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n");
  } else {
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
)====="
);
    
    client.print(form_html_tail);
  }


  client.flush();
  delay(1);
  Serial.println("Client disconnected");
}
