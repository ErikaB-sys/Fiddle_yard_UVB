// LED Test für Testplatine
// D2, D3, D4 + Built-in LED

const uint8_t LED1 = 2;
const uint8_t LED2 = 3;
const uint8_t LED3 = 4;
const uint8_t LED_BUILTIN_PIN = LED_BUILTIN;

void setup()
{
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
uint16_t PWM=F_min; 
void loop()
{ 
 if (PWM <300)
 {PWM+=10;}
else 
 {PWM=F_min;}
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