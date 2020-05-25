#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define YLINE 23

#define ZONE_ON 1
#define ZONE_OFF 0

int zone_state[10] = {
  ZONE_ON,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_ON,
  ZONE_OFF,
};

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

// draw running sprinklers offset per animation
void draw_running_orig(int x, int y, int offset) {
  display.drawLine(x + offset,      y,               x + 10 - offset,  y + 10,      WHITE);
  display.drawLine(x,               y + 10 - offset, x + 10,           y + offset,  WHITE);
}

void print_to_oled_at_y(String text, int yoffset) {
  int x = 0;
  display.setCursor(x, yoffset);
  display.println(text);
}

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
    draw_display();
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
void draw_display() {
  for (int x = 0; x + 11 < 65; x += 13) {
    draw_sprinkler(x, YLINE + 2);
    draw_sprinkler(x, YLINE + 14);
  }
}

void setup()   {
  Serial.begin(9600);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  delay(100);

  print_to_oled("Sprinkle", "Now!");
  display.display();

  draw_display();
  display.display();
  delay(2000);
}

void loop() {
  print_to_oled("Running", "");
  animate_sprinklers();
}
