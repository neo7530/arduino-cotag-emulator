#include <GyverPWM.h>

uint8_t packet_buf[10];
uint8_t bit_pos = 0;
uint8_t invert = 0;
uint8_t bits = 64;
bool lsb = 1;
uint8_t parity = 0;
int del = 2850; /* initial delay b4 syncing (should be 2910 µsec) */ 
#define DETECT_PIN  2
volatile bool TX = false;
#define led_on digitalWrite(13,HIGH)
#define led_off digitalWrite(13,LOW)
uint32_t last_change = 0;
uint32_t cur_timestamp = 0;
uint32_t pulse_duration = 0;


int get_bit() {
  int ret;
  int byte_pos;
  int byte_bit_pos;
  if(lsb)
  {
    byte_pos     = (bits/8)-(bit_pos / 8);
    byte_bit_pos = (bit_pos % 8);     // send lsb first
  } else {
    byte_pos     = bit_pos / 8;
    byte_bit_pos = 7 - (bit_pos % 8);     // reverse indexing to send the bits msb
  }
  bit_pos++;
  ret = (packet_buf[byte_pos] & (1<<byte_bit_pos)) ? 1 : 0;
  return ret^invert;
}



void setup() {
  Serial.begin(115200);
  pinMode(DETECT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(DETECT_PIN), DET, RISING); //PIN 2 = INT 0  pinMode(10, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(13,OUTPUT);
  PWM_frequency(10, 66000, FAST_PWM);
  PWM_set(10, 128);
  PWM_detach(10); //!!!

  packet_buf[0] = 0x00; //always 0x00
  packet_buf[1] = 0x81; //saved data MSB
  packet_buf[2] = 0xb2;
  packet_buf[3] = 0x00;
  packet_buf[4] = 0x00;
  packet_buf[5] = 0x00;
  packet_buf[6] = 0x0e;
  packet_buf[7] = 0x01;
  packet_buf[8] = 0x23; //saved data LSB
  parity = 0;
 Serial.println(F("Cotag-Emu V0.01"));
  for(int i=0;i<5;i++){
    led_on;
    delay(10);
    led_off;
    delay(200);  
  }  
 Serial.println(F("ready..."));
 Serial.print("Delay: ");
 Serial.print(del);
 Serial.println("µsec.");
}

void loop() {

  PWM_detach(10);

while(!TX){}

  bit_pos=0;
  led_on;


  delayMicroseconds(del);
  PWM_attach(10);
  delayMicroseconds(del);  

if(TX){
    
  if(parity)
  {
    PWM_attach(10);
    delayMicroseconds(del);
    PWM_detach(10);
  } else {
    PWM_detach(10);
    delayMicroseconds(del);
    PWM_attach(10);    
  }

  for(int i =bit_pos;i<bits;i++)
  {
    int bit = get_bit();
    if(bit)
    {
      delayMicroseconds(del);
      PWM_attach(10);
      delayMicroseconds(del);
      PWM_detach(10);      
    } else {
      delayMicroseconds(del);
      PWM_detach(10);      
      delayMicroseconds(del);
      PWM_attach(10);
    }
    /* self-synchronizing function (pulse-pause-delay) */
    if(!TX){
      del += 5;
      if(del>=2950){
        del = 2850;
      }
      break;
    }    
  }
  if(!TX){goto exit;}  /* avoid further delay */
  delayMicroseconds(del);
  PWM_detach(10);
}
exit:
  led_off;

  TX=false;
  attachInterrupt(digitalPinToInterrupt(DETECT_PIN), DET, RISING);

}

void BRK(){
  detachInterrupt(digitalPinToInterrupt(DETECT_PIN));
  PWM_detach(10);
  TX=false;
  attachInterrupt(digitalPinToInterrupt(DETECT_PIN), DET, RISING);
}


void DET(){

 detachInterrupt(digitalPinToInterrupt(DETECT_PIN));
  cur_timestamp = micros();
  pulse_duration = cur_timestamp - last_change;
  last_change     = cur_timestamp;

  if((pulse_duration >= 2000) && (pulse_duration <= 5000))
  {
    attachInterrupt(digitalPinToInterrupt(DETECT_PIN), BRK, FALLING);
    TX = true;
    } else {
      attachInterrupt(digitalPinToInterrupt(DETECT_PIN), DET, RISING);
  }
}
