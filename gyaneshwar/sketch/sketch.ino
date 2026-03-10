#include <Wire.h>
#include <si5351.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Si5351 si5351;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Encoder pins
#define ENC_CLK 2
#define ENC_DT 3
#define BUTTON_PIN 4

volatile long rf_freq_hz = 7100000UL;
const long IF_FREQ = 9000000UL;
volatile bool update_needed = false;
unsigned long last_interrupt_time = 0;

void setup() {
  Serial.begin(115200);
  
  // Give the UNO Q Bridge time to wake up
  delay(2000);

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_freq((rf_freq_hz + IF_FREQ) * 100ULL, SI5351_CLK0);
  
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
  }
  
  update_display();
  
  // STM32 specific: Attach interrupt to the CLK pin
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), handle_encoder, FALLING);
}

void loop() {
  if (update_needed) {
    si5351.set_freq((rf_freq_hz + IF_FREQ) * 100ULL, SI5351_CLK0);
    update_display();
    update_needed = false;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    rf_freq_hz = 7100000UL;
    update_needed = true;
    delay(200); 
  }
}

// Optimized ISR for STM32
void handle_encoder() {
  unsigned long interrupt_time = millis();
  // Debounce: If interrupts come faster than 5ms, ignore them
  if (interrupt_time - last_interrupt_time > 5) {
    if (digitalRead(ENC_DT) == LOW) {
      rf_freq_hz += 1000; // Step up
    } else {
      rf_freq_hz -= 1000; // Step down
    }
    update_needed = true;
  }
  last_interrupt_time = interrupt_time;
}

void update_display() {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print(rf_freq_hz / 1000000.0, 3);
  oled.print(" MHz");
  oled.display();
}