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
  ZONE_ON,
  ZONE_OFF,
  ZONE_ON,
  ZONE_ON,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
  ZONE_OFF,
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
//  Serial.print("zone ");
//  Serial.print(zone);
//  Serial.print("offset ");
//  Serial.print(offset);
//  Serial.print("  x: ");
//  Serial.print(x);
//  Serial.print("  y: ");
//  Serial.println(y);
  
  display.drawLine(x + offset,      y,               x + 10 - offset,  y + 10,      WHITE);
  display.drawLine(x,               y + 10 - offset, x + 10,           y + offset,  WHITE);
}

// draw running sprinklers offset per animation
void draw_running_orig(int x, int y, int offset) {
  display.drawLine(x + offset,      y,               x + 10 - offset,  y + 10,      WHITE);
  display.drawLine(x,               y + 10 - offset, x + 10,           y + offset,  WHITE);
}

void print_to_oled(String str) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(str);

  //  display.startscrollleft(0x00, 0x0F);

}


// TODO: only turn on sprinklers running in zone_state



// animate running sprinklers
void animate_sprinklers() {
  while (1) {
    // cycle through each animation step, skip 11 since this
    // is the corner same as 0
    for (int phase = 0; phase < 10; phase++) {
      draw_display();
      // cycle through all running sprinklers
      for (int i = 0; i < sizeof(zone_state) / sizeof(zone_state[0]); i++) {
        if (zone_state[i]) {
          draw_running(i, phase);
        }
      }
//      delay(10000);
//            for (int x = 0; x + 11 < 65; x += 13) {
//              draw_running_orig(x, YLINE + 2, phase);
//              draw_running_orig(x, YLINE + 14, phase);
//            }
      display.display();
    }
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

  print_to_oled("Running");
  display.display();

  draw_display();
  display.display();
  delay(2000);


  // draw the first ~12 characters in the font
  //  testdrawchar();
  //  display.display();
  //  delay(2000);
  //  display.clearDisplay();

  // draw scrolling text
  //  testscrolltext();
  //  delay(2000);
  //  display.clearDisplay();

  // text display tests
  //  display.setTextSize(1);
  //  display.setTextColor(WHITE);
  //  display.setCursor(0, 0);
  //  display.println("Hello, world!");
  //  display.setTextColor(BLACK, WHITE); // 'inverted' text
  //  display.println(3.141592);
  //  display.setTextSize(2);
  //  display.setTextColor(WHITE);
  //  display.print("0x");
  //  display.println(0xDEADBEEF, HEX);
  //  display.display();
  //  delay(2000);
  //  display.clearDisplay();

}

void loop() {
  animate_sprinklers();
}

void testdrawchar(void) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  for (uint8_t i = 0; i < 168; i++) {
    if (i == '\n') continue;
    display.write(i);
    if ((i > 0) && (i % 21 == 0))
      display.println();
  }
  display.display();
  delay(1);
}

void testdrawcircle(void) {
  for (int16_t i = 0; i < display.height(); i += 2) {
    display.drawCircle(display.width() / 2, display.height() / 2, i, WHITE);
    display.display();
    delay(1);
  }
}

void testfillrect(void) {
  uint8_t color = 1;
  for (int16_t i = 0; i < display.height() / 2; i += 3) {
    // alternate colors
    display.fillRect(i, i, display.width() - i * 2, display.height() - i * 2, color % 2);
    display.display();
    delay(1);
    color++;
  }
}

void testdrawtriangle(void) {
  for (int16_t i = 0; i < min(display.width(), display.height()) / 2; i += 5) {
    display.drawTriangle(display.width() / 2, display.height() / 2 - i,
                         display.width() / 2 - i, display.height() / 2 + i,
                         display.width() / 2 + i, display.height() / 2 + i, WHITE);
    display.display();
    delay(1);
  }
}

void testfilltriangle(void) {
  uint8_t color = WHITE;
  for (int16_t i = min(display.width(), display.height()) / 2; i > 0; i -= 5) {
    display.fillTriangle(display.width() / 2, display.height() / 2 - i,
                         display.width() / 2 - i, display.height() / 2 + i,
                         display.width() / 2 + i, display.height() / 2 + i, WHITE);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
    delay(1);
  }
}

void testdrawroundrect(void) {
  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
    display.drawRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, WHITE);
    display.display();
    delay(1);
  }
}

void testfillroundrect(void) {
  uint8_t color = WHITE;
  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
    display.fillRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, color);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
    delay(1);
  }
}

void testdrawrect(void) {
  for (int16_t i = 0; i < display.height() / 2; i += 2) {
    display.drawRect(i, i, display.width() - 2 * i, display.height() - 2 * i, WHITE);
    display.display();
    delay(1);
  }
}


//void 2(void) {
//  display.setTextSize(2);
//  display.setTextColor(WHITE);
//  display.setCursor(10, 0);
//  //  display.clearDisplay();
//  display.println("scroll");
//  display.display();
//  delay(1);
//
//  display.startscrollright(0x00, 0x0F);
//  delay(2000);
//  display.stopscroll();
//  delay(1000);
//  display.startscrollleft(0x00, 0x0F);
//  delay(2000);
//  display.stopscroll();
//  delay(1000);
//  display.startscrolldiagright(0x00, 0x07);
//  delay(2000);
//  display.startscrolldiagleft(0x00, 0x07);
//  delay(2000);
//  display.stopscroll();
//}
