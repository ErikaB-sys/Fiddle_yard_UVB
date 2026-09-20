#include <Arduino.h>
#include <Wire.h>

// LED Test für Testplatine
// D2, D3, D4 + Built-in LED

const uint8_t LED1 = 2;
const uint8_t LED2 = 3;
const uint8_t LED3 = 4;
const uint8_t LED_BUILTIN_PIN = LED_BUILTIN;

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

void setup()
{
  Serial.begin(115200);

  Wire.begin();
  delay(100);

  scanI2C();

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
