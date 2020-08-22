#include <Arduino.h>
#include <TM1637Display.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <EEPROM.h>


#define CLK1 4
#define DIO1 5

#define CLK2 6
#define DIO2 7

#define LED1 8
#define LED2 9
#define LED3 11

#define F_CPU 16000000UL
// Calculate the value needed for 
// the CTC match value in OCR1A.
#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 8) 

TM1637Display display(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);

unsigned int timer = 0;
unsigned int record = 9999;
short milli = 0;

byte led_state = 0;

bool game_on = false;

void write_record() {
  EEPROM.write(0, record & 0xff);
  EEPROM.write(1, (record & (0xff << 8)) >> 8);
}

void finish() {
  if(digitalRead(2)) {
    if(!game_on) {
      return;
    }
    Serial.println("finish");
    led_state = 2;
  
    if(timer > 300) {
      if(timer < record) {
        record = timer;
        write_record();
      }
    }
    game_on = false;
  } else {
    Serial.println("start");
    led_state = 3;
    if(game_on) {
      return;
    }
  
    timer = 0;
    milli = 0;
    game_on = true;
  }

}

void stop_game() {
  Serial.println("stop");
  led_state = 1;
  game_on = false;
  digitalWrite(10, HIGH);
  delay(100);
  digitalWrite(10, LOW);
}


void setup() {
  Serial.begin(9600);  
  //Timer setup
  TCCR1B |= (1 << WGM12) | (1 << CS11);
  OCR1AH = (CTC_MATCH_OVERFLOW >> 8);
  OCR1AL = CTC_MATCH_OVERFLOW;
  TIMSK1 |= (1 << OCIE1A);
  DDRC |= (1 << PC0);

  record = EEPROM.read(0) | (EEPROM.read(1) << 8);

  PCMSK2 |= _BV(PCINT0);
  PCICR |= _BV(PCIE2);

  //change interrupt pins
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  //led
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  //buzzer
  pinMode(10, OUTPUT);

  //reset record
  pinMode(13, INPUT_PULLUP);
  
  digitalWrite(10, LOW);
  attachInterrupt(digitalPinToInterrupt(2), finish, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), stop_game, LOW);

  display.setBrightness(0xff);
  display2.setBrightness(0xff);
  sei();
}

ISR (TIMER1_COMPA_vect)
{
  milli++;
  if(milli > 9 && timer < 9999 && game_on) {
    timer++;
    milli = 0;
  }
}


void loop() {
  if(!digitalRead(13)) {
    record = 9999;
    write_record();
  }
  if(led_state == 1) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);
    
  } else if(led_state == 2) {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, LOW);
    
  } else if(led_state == 3) {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, HIGH);
    
  }
  display.showNumberDecEx(timer, 0b01000000, true);
  display2.showNumberDecEx(record, 0b01000000, true);
}
