#include <Wire.h>
#include "DHT.h"
DHT dht;  //instance of dh11 reader class

#define IRpin_PIN      PIND
#define IRpin          2
#define IRQpin         13 // raspberry pi IR command
#define IRQlog         12 // raspberry pi data logging

unsigned int time[66];  //counter for pulse length
boolean valid = true; //valid ir code ?
byte IRaddress = 0;  //the ir address byte
byte IRcommand = 0;  //the ir command byte
long timeS;  //time counter
byte last_humid = 40;  //save last good copy
byte last_temp = 25;   //save last good copy

#define SLAVE_ADDRESS 0x04
byte address = 0; //one byte only
byte data[11] = {0,0,0,0,0,0,0,0,0,0,0};
//Pi zero only receives least 7 bits in i2c read from arduino
//Pi A and B series receives full 8 bits.
//if byte is > 127 use two bytes as in IR commands
//0,1 for MSB IR. 2,3 for LSB IR. 4,5 for in.humid and in.temp
//6,7 for out.humidity, out.temperature, 8 for windspeed
//9 for rainfall, 10 for lightning flashes
byte N = 0; //working byte
byte extdata[5];  //the external weather data

void setup() {
Serial.begin(1200);  //only for communication with out door arduino
TCCR1A = 0x00;  //set up timer 1 as 16 bit counter 
TCCR1B = 0x02;  // normal mode counts to 65535 and resets
//speed = 2 Mhz
 // initialize i2c as slave
Wire.begin(SLAVE_ADDRESS);
// define callbacks for i2c communication
Wire.onReceive(receiveData);
Wire.onRequest(sendData);
pinMode(IRQpin, OUTPUT);//high edge to interupt
digitalWrite(IRQpin, LOW);//set low to start for IR command
pinMode(IRQlog, OUTPUT);//high edge to interupt
digitalWrite(IRQlog, LOW);//set low to start for data logging
dht.setup(3); // data pin 3 for dht11 sensor
timeS = millis();  //initialise counter
}//end of setup

//datalogging program can be up to 2 seconds 
//during this time IR commands wont work-not a problem
void datalog(){
  float h = dht.getHumidity();  //only allow 60 lots of 2 second samples
  float t = dht.getTemperature();
  //only save sample if it is valid
  if (!isnan(h) && !isnan(t)) {
    data[4] = (byte)h;
    data[5] = (byte)t;
    last_humid = (byte)h;
    last_temp = (byte)t;  //save this good sample
    }//end of good sample
  else{    //use old good sample
     data[4] = last_humid;
     data[5] = last_temp;
  }//end of have a bad sample
 //quiz the external arduino weather data
 Serial.write(99);  //the go character "c"
 Serial.readBytes(extdata,5); // read external data    
 //end of get data from external. 1 second time out
 data[6] = extdata[4];
 data[7] = extdata[3];
 data[8] = extdata[2];  //copy over the external data
 data[9] = extdata[1];
 data[10] = extdata[0];  
 digitalWrite(IRQlog, HIGH);//interupt raspberry pi
 delayMicroseconds(1000);//1 ms pulse for raspberry IRQ 
 digitalWrite(IRQlog, LOW);//reset for next time
 timeS = millis();  //reset counter 
}//end of data logging

void loop() {
while (IRpin_PIN & (1 << IRpin)) {
   if(millis()-timeS > 120000){
    datalog();  //run logger if reach interval
  }//end of wait for sample interval
  TCNT1 = 0x00;  //set start value of counter  
}//end of wait out low
while (! (IRpin_PIN & _BV(IRpin))) {
    time[0] = TCNT1;
    if(TCNT1 > 64000){
    time[0] = 0;
    break; //prevent overflow
    }//end of prevent overflow
}//end of find length of high

if((time[0] < 19000) & (time[0] > 17000)){
    TCNT1 = 0x00;  //reset the counter //look for low pulse
    while (IRpin_PIN & (1 << IRpin)) {
     time[1] = TCNT1;
     if(TCNT1 > 64000){
     time[1] = 0;
     break; //prevent overflow
     }//end of prevent overflow
    }//end of start low pulse 
    
     if((time[1] < 9500) && (time[1] > 8500)){       
       //we have a valid start sequence 9000ms followed by 4500ms
       
       valid = true; //start with high hopes
       for(int i = 0; i < 64; i +=2){ //find the 64 data bits
       TCNT1 = 0x00;  //reset the counter //look for high pulse 
       while (! (IRpin_PIN & _BV(IRpin))) {
        time[2+i] = TCNT1;
        if(TCNT1 > 64000){
        time[2+i] = 0;
        break; //prevent overflow
        }//end of prevent overflow
       }//end of find length of high
       if(time[2+i] < 900 || time[2+i] > 4000){
        valid = false;
        break; //end this pair search
       }//end of valid length ?
       TCNT1 = 0x00;  //reset the counter //look for low pulse
       while (IRpin_PIN & (1 << IRpin)) {
       time[3+i] = TCNT1;
        if(TCNT1 > 64000){
        time[3+i] = 0;
        break; //prevent overflow
        }//end of prevent overflow
       }//end of find length of low
       if(time[2+i] < 900 || time[2+i] > 4000){
        valid = false;
        break; //end this pair search
       }//end of valid length ?
       }//end of find pairs
    }//end of we have a start sequence
    else{
       valid = false;
    }//end of not start sequence
}//end of we have a start pulse
else {
  valid = false;
}//not valid start pulse

if(valid){
  for(byte i = 0; i < 16; i += 2){
    IRaddress = IRaddress << 1; //move bit along 
    byte bit = 0; //is the pair a 1 or 0
    if((time[2+i]< 2000) && (time[3+i]<2000)){bit = 1;}
    IRaddress = IRaddress | bit;  //add the bit value
  }//end of get address byte 

 for(byte i = 0; i < 16; i += 2){
    IRcommand = IRcommand << 1; //move bit along 
    byte bit = 0; //is the pair a 1 or 0
    if((time[34+i]< 2000) && (time[35+i]<2000)){bit = 1;}
    IRcommand = IRcommand | bit;  //add the bit value
  }//end of get address byte 
 //for display only
 unsigned int disp = IRaddress;
 data[0] = IRaddress;//MSB of IR
 N = data[0]; //working byte
 data[1] = data[0]&127;//remove msbit 
 data[0] = N>>7;//move msbit to lsbit
 data[2] = IRcommand;//LSB of IR
 N = data[2]; //working byte
 data[3] = data[2]&127;//remove msbit 
 data[2] = N>>7;//move msbit to lsbit
 digitalWrite(IRQpin, HIGH);//interupt raspberry pi
 delayMicroseconds(1000);//1 ms pulse for raspberry IRQ 
 digitalWrite(IRQpin, LOW);//reset for next time
   
}//end of if valid
}//end of loop

// callback for sending data
 void sendData(){
 Wire.write(data[address]);
 }//end of send data

// callback for received data
void receiveData(int byteCount){
while(Wire.available()) {
address = Wire.read();
}//end of while data
}//end of receive data
