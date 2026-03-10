#include <Wire.h>
#include <si5351.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Si5351 si5351;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Encoder pins (KY-040)
#define ENC_CLK 2
#define ENC_DT 3
#define BUTTON_PIN 4

unsigned long rf_freq_hz = 7100000UL;
const unsigned long IF_FREQ = 9000000UL;
const unsigned long MIN_FREQ = 7000000UL;
const unsigned long MAX_FREQ = 7300000UL;

volatile int enc_state = 0;
int last_enc_a = HIGH;
int last_enc_b = HIGH;
unsigned long last_change = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_freq((rf_freq_hz + IF_FREQ) * 100ULL, SI5351_CLK0);
  si5351.update_status();
  
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while(1);
  }
  oled.clearDisplay();
  oled.display();
  
  Serial.println("40m VFO Ready!");
  update_display();
  
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), handle_encoder, CHANGE);
}

void loop() {
  // Button reset
  if (digitalRead(BUTTON_PIN) == LOW) {
    rf_freq_hz = 7100000UL;
    si5351.set_freq((rf_freq_hz + IF_FREQ) * 100ULL, SI5351_CLK0);
    si5351.update_status();
    update_display();
    delay(250);
  }
}

void handle_encoder() {
  int enc_a = digitalRead(ENC_CLK);
  int enc_b = digitalRead(ENC_DT);
  
  if (enc_a != last_enc_a) {
    unsigned long now = millis();
    unsigned long delta = now - last_change;
    
    long step = (delta < 100) ? 5000 : (delta < 500) ? 1000 : 100;
    
    if (enc_b != enc_a) {
      rf_freq_hz += step;  // CW
    } else {
      rf_freq_hz -= step;  // CCW
    }
    
    rf_freq_hz = constrain(rf_freq_hz, MIN_FREQ, MAX_FREQ);
    si5351.set_freq((rf_freq_hz + IF_FREQ) * 100ULL, SI5351_CLK0);
    si5351.update_status();
    
    last_change = now;
    update_display();
  }
  
  last_enc_a = enc_a;
  last_enc_b = enc_b;
}

void update_display() {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print(rf_freq_hz / 1000000);
  oled.print(".");
  if ((rf_freq_hz % 1000000) < 100000) oled.print("0");
  oled.print((rf_freq_hz % 1000000) / 1000);
  oled.println(" MHz");
  
  oled.setTextSize(1);
  oled.setCursor(0, 25);
  oled.print("LO: ");
  oled.print((float)(rf_freq_hz + IF_FREQ) / 1000000, 3);
  oled.println(" MHz");
  
  oled.display();
}
