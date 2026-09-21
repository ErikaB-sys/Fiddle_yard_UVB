#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PCF8574.h>



// LED Test für Testplatine
// D2, D3, D4 + Built-in LED

const uint8_t LED1 = 2;
const uint8_t LED2 = 3;
const uint8_t LED3 = 4;
const uint8_t LED_BUILTIN_PIN = LED_BUILTIN;

// Lichtschranken am Arduino
const uint8_t LS1 = 13;
const uint8_t LS2 = 12;
const uint8_t LS3 = 11;
const uint8_t LS4 = 10;

#define ON  1
#define OFF 0

Adafruit_SSD1306 display(128, 32, &Wire, -1);  //OLED dispaly 
PCF8574 expander(0x27);                        // LED und Tasten


void scanI2C()
{
  Serial.println(F("I2C Scanner"));
  uint8_t found = 0;

  for (uint8_t address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0)
    {
      Serial.print(F("I2C device: 0x"));
      if (address < 0x10)
        Serial.print('0');
      Serial.println(address, HEX);
      found++;
    }
  }

  if (found == 0)
    Serial.println(F("No I2C devices found"));

  Serial.print(F("I2C devices found: "));
  Serial.println(found);
}


//Start Oled 
void InitOLED()
{
  // Initialize the SSD1306 OLED display.
// I2C address: 0x3C
// Resolution: 128 x 32 pixels
if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
{
    Serial.println(F("Error initializing OLED!"));
    while (true)
    {
        // OLED initialization failed.
    }
}
display.setRotation(2);
// Clear display buffer
display.clearDisplay();

// Text configuration
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 0);

// First simple display test
display.println(F("FY HW TEST"));
display.println(F("OLED: OK"));
display.println(F("I2C: 0x3C"));

// Send buffer to OLED
display.display();
}


void init_extendeder()
{
    expander.pinMode(P7, INPUT);
    expander.pinMode(P6, INPUT);
    expander.pinMode(P5, INPUT);
    expander.pinMode(P4, INPUT);
    expander.pinMode(P0, OUTPUT);
    expander.pinMode(P1, OUTPUT);
    expander.pinMode(P2, OUTPUT);
    expander.pinMode(P3, OUTPUT);
    
    if (!expander.begin())
    {
        Serial.println(F("Error initializing PCF8574!"));
        while (true)
        {
        }
    }
}

void read_buttons()
{
    bool key1 = expander.digitalRead(P7) == LOW ? ON : OFF;
    bool key2 = expander.digitalRead(P6) == LOW ? ON : OFF;
    bool key3 = expander.digitalRead(P5) == LOW ? ON : OFF;
    bool key4 = expander.digitalRead(P4) == LOW ? ON : OFF;

    // Lichtschranken direkt als Rohzustand lesen.
    // INPUT ohne internen Pull-up: die Beschaltung der LS liefert den Pegel.
    bool ls1 = digitalRead(LS1);
    bool ls2 = digitalRead(LS2);
    bool ls3 = digitalRead(LS3);
    bool ls4 = digitalRead(LS4);

    display.clearDisplay();
    display.setCursor(0, 0);

    display.print(F("K1:"));
    display.print(key1 ? F("ON ") : F("OFF"));
    display.print(F(" LS13:"));
    display.println(ls1 ? F("1") : F("0"));
    expander.digitalWrite(P3, !key1);

    display.print(F("K2:"));
    display.print(key2 ? F("ON ") : F("OFF"));
    display.print(F(" LS12:"));
    display.println(ls2 ? F("1") : F("0"));
    expander.digitalWrite(P2, !key2);

    display.print(F("K3:"));
    display.print(key3 ? F("ON ") : F("OFF"));
    display.print(F(" LS11:"));
    display.println(ls3 ? F("1") : F("0"));
    expander.digitalWrite(P1, !key3);

    display.print(F("K4:"));
    display.print(key4 ? F("ON ") : F("OFF"));
    display.print(F(" LS10:"));
    display.println(ls4 ? F("1") : F("0"));
    expander.digitalWrite(P0, !key4);

    display.display();
}


void setup()
{
  Serial.begin(9600);

  Wire.begin();
  delay(100);

  scanI2C();

  InitOLED();
  init_extendeder();

  pinMode(LS1, INPUT);
  pinMode(LS2, INPUT);
  pinMode(LS3, INPUT);
  pinMode(LS4, INPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED_BUILTIN_PIN, OUTPUT);
}

void setPWM(uint16_t frequency)
{
  // Timer2 CTC, Prescaler 64
  // f = 16 MHz / (64 * (OCR2A + 1))

  uint16_t ocr = (16000000UL / (64UL * frequency)) - 1;

  if (ocr > 255)
    ocr = 255;

  OCR2A = ocr;

  // 50 % Duty Cycle
  OCR2B = ocr / 2;

  // Timer2: CTC, Prescaler 64
  TCCR2A = (1 << WGM21) | (1 << COM2B1);
  TCCR2B = (1 << CS22);
}

#define F_min 65
uint16_t PWM = F_min;

void loop()
{
  if (PWM < 300)
  {
    PWM += 10;
  }
  else
  {
    PWM = F_min;
  }
   read_buttons();


  digitalWrite(LED1, LOW);
  digitalWrite(LED3, LOW);
  setPWM(PWM);
  digitalWrite(LED_BUILTIN_PIN, LOW);
  delay(1000 / PWM * 10);

  digitalWrite(LED1, LOW);
  digitalWrite(LED3, HIGH);
  digitalWrite(LED_BUILTIN_PIN, LOW);
  delay(1000 / PWM * 10);

  digitalWrite(LED1, HIGH);
  digitalWrite(LED3, LOW);
  digitalWrite(LED_BUILTIN_PIN, LOW);
  delay(1000 / PWM * 10);

  digitalWrite(LED1, HIGH);
  digitalWrite(LED3, HIGH);
  digitalWrite(LED_BUILTIN_PIN, HIGH);
  delay(1000 / PWM * 10);
}
