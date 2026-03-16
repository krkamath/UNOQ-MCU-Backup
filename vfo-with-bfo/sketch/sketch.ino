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

// --- Frequencies ---
volatile long rf_freq_hz = 7150000UL;      // RF tuning
const long IF_FREQ  = 9000000UL;           // IF
const long BFO_FREQ = 8998500UL;           // example BFO on CLK1

// --- Variables (Volatile for ISR safety) ---
volatile bool update_needed = false;
volatile int lastEncoded = 0;
volatile int pulseCounter = 0;  

void setup() 
{
  Serial.begin(115200);

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // --- Si5351 init (one time) ---
  bool ok = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  if (!ok) 
  {
    Serial.println("Si5351 not found – check I2C wiring.");
    while (1) { delay(1000); }
  }

  // CLK0: VFO (RF+IF)
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  si5351.output_enable(SI5351_CLK0, 1);

  // CLK1: BFO (fixed, or later you can vary it)
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);
  si5351.output_enable(SI5351_CLK1, 1);

  update_hardware();   // set both clocks

  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  oled.clearDisplay();
  update_display();

  attachInterrupt(digitalPinToInterrupt(ENC_CLK), handle_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT),  handle_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handle_button, CHANGE);
}

void loop() 
{
  if (update_needed) 
  {   
    update_hardware();
    update_display();
    update_needed = false;
  }
}

void handle_button() 
{
  if (digitalRead(BUTTON_PIN) == LOW) 
  { 
    rf_freq_hz = 7150000UL;
    update_needed = true;
  }
}

void handle_encoder()
{
  int MSB = digitalRead(ENC_CLK);
  int LSB = digitalRead(ENC_DT);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) pulseCounter++;
  else if (sum == 0b1110 || sum == 0b1000 || sum == 0b0001 || sum == 0b0111) pulseCounter--;

  if (abs(pulseCounter) >= 4)
  {
    if (pulseCounter > 0) rf_freq_hz += 1000L;
    else                  rf_freq_hz -= 1000L;

    rf_freq_hz = constrain(rf_freq_hz, 7000000L, 7300000L);
    pulseCounter = 0;
    update_needed = true;
  }

  lastEncoded = encoded;
}

void update_hardware() 
{
  // CLK0: RF + IF (VFO)
  uint64_t vfo = (uint64_t)(rf_freq_hz + IF_FREQ) * 100ULL;
  si5351.set_freq(vfo, SI5351_CLK0);

  // CLK1: fixed BFO (here around IF)
  uint64_t bfo = (uint64_t)BFO_FREQ * 100ULL;
  si5351.set_freq(bfo, SI5351_CLK1);
}

void update_display() 
{
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 5);
  oled.print(rf_freq_hz / 1000000.0, 3);
  oled.print(" MHz");
  oled.display();
}
