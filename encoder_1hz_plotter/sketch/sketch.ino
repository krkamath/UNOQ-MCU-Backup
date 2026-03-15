#include <Wire.h>
#include <si5351.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Pin Definitions
#define ENC_CLK 2
#define ENC_DT 3
#define BUTTON_PIN 4

// --- Objects ---
Si5351 si5351;
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Variables (Volatile for ISR safety) ---
volatile long rf_freq_hz = 7100000UL;
const long IF_FREQ = 9000000UL;
volatile bool update_needed = false;
volatile int lastEncoded = 0;
volatile int currentSum = 0;      // Global to share with Serial Plotter
volatile int pulseCounter = 0;  

// Timer variables for Display
unsigned long lastDisplayUpdate = 0;
const int displayRefreshRate = 100;



void setup() {
  // High baud rate is essential for smooth Plotter lines
  Monitor.begin(115200);

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0)) {
    Serial.println("Si5351 sync failed!");
  }

  update_hardware();

  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  oled.clearDisplay();
  update_display();

  attachInterrupt(digitalPinToInterrupt(ENC_CLK), handle_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT), handle_encoder, CHANGE);
}

void loop() {
  int rawA = digitalRead(ENC_CLK);
  int rawB = digitalRead(ENC_DT);

  Monitor.print("CLK:");       Monitor.print(rawA);
  Monitor.print(",");
  Monitor.print("DT_Offset:"); Monitor.print(rawB + 2);
  Monitor.print(",");
  Monitor.print("PackedSum:"); Monitor.print(currentSum + 4);
  Monitor.print(",");
  Monitor.print("Freq_kHz:");  Monitor.println(rf_freq_hz / 1000);


  // --- SERIAL PLOTTER VISUALIZATION ---
  // We read the pins here just for the graph to see the "Quadrature" offset
  // int rawA = digitalRead(ENC_CLK);
  // int rawB = digitalRead(ENC_DT);
  
  // Serial.print("CLK:");          Serial.print(rawA);
  // Serial.print(",");
  // Serial.print("DT_Offset:");    Serial.print(rawB + 2); // Offset by 2 to separate lines
  // Serial.print(",");
  // Serial.print("PackedSum:");    Serial.print(currentSum + 4); // Offset by 4
  // Serial.print(",");
  // Serial.print("Freq_kHz:");     Serial.println(rf_freq_hz / 1000); 

  // Handle hardware and display updates
  if (update_needed && (millis() - lastDisplayUpdate >= displayRefreshRate)) {
    update_hardware();
    update_display();
    lastDisplayUpdate = millis();
    update_needed = false;
  }

  // Frequency Reset Button
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    if (digitalRead(BUTTON_PIN) == LOW) {
      rf_freq_hz = 7100000UL;
      update_needed = true;
      while (digitalRead(BUTTON_PIN) == LOW);
    }
  }
}

// --- High Precision ISR ---
void handle_encoder() {
  int MSB = digitalRead(ENC_CLK);
  int LSB = digitalRead(ENC_DT);

  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;
  currentSum = sum; // Save to global for the plotter

  // Identify Directional Pulses
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    pulseCounter++;
  } else if (sum == 0b1110 || sum == 0b1000 || sum == 0b0001 || sum == 0b0111) {
    pulseCounter--;
  }

  // Step Logic (4 pulses = 1 detent click on most encoders)
  if (abs(pulseCounter) >= 4) {
    long stepSize = 1000L; 
    if (pulseCounter > 0) rf_freq_hz += stepSize;
    else rf_freq_hz -= stepSize;

    pulseCounter = 0;
    update_needed = true;
  }

  rf_freq_hz = constrain(rf_freq_hz, 100000L, 30000000L);
  lastEncoded = encoded;
}

void update_hardware() {
  si5351.set_freq((rf_freq_hz + IF_FREQ) * 100ULL, SI5351_CLK0);
}

void update_display() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.print("BHAIRAVA VFO");

  oled.setTextSize(2);
  oled.setCursor(0, 25);
  oled.print(rf_freq_hz / 1000000.0, 3);
  oled.setTextSize(1);
  oled.print(" MHz");

  oled.setCursor(0, 55);
  oled.print("Step: 1 kHz [FIXED]");
  oled.display();
}