#include "DHT.h"
DHT dht;  //instance of DHT11 sample code
byte DHT11pin = 4;  //digital 4
byte number_samples = 0;   //current number of samples 
unsigned int humidity = 0; //outside humidity
unsigned int temperature = 0;  //outside temp
byte windsensorIN = 1;  //analog 1 
unsigned long windspeed = 0;  //wind speed
volatile byte rainfall = 0;  //count bucket tips
byte int0 = 2; //using INT0
volatile byte flashes = 0; //count lightning flashes
byte int1 = 3; //using INT1
byte data[5];  //the data samples
//humidity,temperature,windspeed,rainfall,lightning flashes
byte h_data[] = {40,40,40};  //running copy humidity
byte t_data[] = {24,24,24};  //running copy temperature
byte sample = 10;  //the sample display LED

void setup() {
  Serial.begin(1200); // opens serial port, sets data rate to 1200 bps
  dht.setup(DHT11pin); // setup DHT11
  delay(2000);  //allow to settle
  float h = dht.getHumidity();
  float t = dht.getTemperature();
  //only use sample if it is valid
  if (!isnan(h) && !isnan(t)) {
  h_data[0]=(byte)h;  h_data[1]=(byte)h;  h_data[2]=(byte)h;
  t_data[0]=(byte)t;  t_data[1]=(byte)t;  t_data[2]=(byte)t;
  }//end of get initial H/T
  analogReference(EXTERNAL); //setup reference = VCC/2
  pinMode(int0,INPUT); //INT0 for rain sensor
  attachInterrupt(digitalPinToInterrupt(int0),count_rain,FALLING);
  pinMode(int1,INPUT); //INT1 for lightning sensor
  attachInterrupt(digitalPinToInterrupt(int1),count_flashes,FALLING);
  pinMode(sample, OUTPUT);
}//end of setup

void loop() {
  // send data only when you receive a go data: 
  if (Serial.available() > 0) {          
      byte gobyte = Serial.read(); // read & clear the go byte:
      if(gobyte == 99){ // the go byte "c"
          data[0] =  h_data[0];  //use first in line
          h_data[0]= h_data[1];  h_data[1]= h_data[2];
          h_data[2] =  humidity/number_samples;//rotate data in
          if((h_data[0]-h_data[1]>5)&&(h_data[2]-h_data[1]>5)){
            h_data[1] =( h_data[0]+h_data[2])/2; //set the average
          }//end of we have a low error data
          data[1] =  t_data[0];  //use first in line
          t_data[0]= t_data[1];  t_data[1]= t_data[2];
          t_data[2] =  temperature/number_samples;//rotate data in
          if((t_data[0]-t_data[1]>5)&&(t_data[2]-t_data[1]>5)){
            t_data[1] =( t_data[0]+t_data[2])/2; //set the average
          }//end of we have a low error data
          data[2] = ((windspeed/(number_samples*10))*110)/512; // 5v Vref
          data[3] = rainfall;
          data[4] = flashes;
          Serial.write(data,5);
          number_samples = 0;  //reset the numer of samples
          humidity = 0; //reset humidity
          temperature = 0;  //reset temp
          windspeed = 0;  //reset wind speed
          rainfall = 0;  //reset rainfall 
          flashes = 0;  //reset lightning flashes
          }//end of if its a 99 character            
  }//end of check for any serial data
  else if(number_samples < 60) {   //continue to sample data
      for(byte i=0;i<10;i+=1){  //sample 10 of wind speed
      windspeed += analogRead(windsensorIN);    // read the input pin
      delay(200);  //total of up to 600 samples
      }//end of 10 windspeed samples   
      float h = dht.getHumidity();  //only allow 60 lots of 2 second samples
      float t = dht.getTemperature();
      //only save sample if it is valid
      if (!isnan(h) && !isnan(t)) {
      humidity += (byte)h;
      temperature += (byte)t;
      number_samples += 1;  //one more
      if(number_samples < 30) {
      digitalWrite(sample, HIGH);  
      }//end of less than 30
      else{                     //switch LED on/off
      digitalWrite(sample, LOW);
      }//end of more than 30
      }//end of good sample
    }//end of continue to sample data
  }//end of loop 

void count_rain(){  //INT0 count bucket tips
rainfall += 1;
}//end of count rain

void count_flashes(){  //INT1 count lightning flashes
flashes += 1;
}//end of count flashes
