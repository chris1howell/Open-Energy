#define emonTxV3
#define ONE_WIRE_BUS 5
#define ASYNC_DELAY 375

#include "EmonLib.h"
#include <OneWire.h>                                                  //http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h> 

EnergyMonitor ct1, ct2, ct3;

unsigned long lastpost = 0;
unsigned long pulsetime = 0;

const byte LEDpin = 6;
const byte DS18B20_PWR = 19;
const byte DIP_switch1 = 8; // Voltage selection 230 / 120 V AC (default switch off 230V)  - 
const byte DIP_switch2 = 9; // node ID 9/10 (default 10)                               
const byte pulse_countINT = 1; // INT 1 / Dig 3 Terminal Block / RJ45 Pulse c
const byte pulse_count_pin = 3;
const byte min_pulsewidth = 110;
const byte MaxOnewire = 6;
const byte Vrms = 230;
byte CT_count = 0;
byte nodeID = 10;
volatile byte pulseCount = 0;

const int no_of_samples = 1480; 
const int no_of_half_wavelengths = 20;
const int timeout = 2000;                               //emonLib timeout 
const int ACAC_DETECTION_LEVEL = 3000;
const int TEMPERATURE_PRECISION = 11;


float phase_shift = 1.7;
float Ical = 90.9;
float Vcal = 268.97;             // (230V x 13) / (9V x 1.2) = 276.9 Calibration for UK AC-AC adapter 77DB-06-09 
float Vcal_USA = 130.0;

boolean CT1, CT2, CT3, ACAC, DS18B20_STATUS; 

typedef struct { 
int power1, power2, power3, power4, Vrms, temp[MaxOnewire]; 
int pulseCount; 
} 
PayloadTX;     // create structure - a neat way of packaging data for RF comms
PayloadTX emontx;
  
//Setup DS128B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
byte allAddress [MaxOnewire][8];  // 8 bytes per address
byte numSensors;


void setup() {    
  pinMode(LEDpin, OUTPUT);
  pinMode(DS18B20_PWR, OUTPUT);
  pinMode(pulse_count_pin, INPUT);
  pinMode(DIP_switch1, INPUT_PULLUP);
  pinMode(DIP_switch2, INPUT_PULLUP);
  attachInterrupt(pulse_countINT, onPulse, FALLING);
  emontx.pulseCount=0;
  
  digitalWrite(LEDpin,HIGH);
  digitalWrite(DS18B20_PWR, HIGH); 
  delay(100); 
  
  Serial.begin(115200);
  
  
  if (digitalRead(DIP_switch1)==LOW) nodeID--;                            //IF DIP switch 1 is switched on then subtract 1 from nodeID
  if (digitalRead(DIP_switch2)==LOW) Vcal = Vcal_USA;                       //IF DIP switch 2 is switched on then activate USA mode
    
 sensors.begin();
 sensors.setWaitForConversion(false);             //disable automatic temperature conversion to reduce time spent awake, conversion will be implemented manually in sleeping http://harizanov.com/2013/07/optimizing-ds18b20-code-for-low-power-applications/ 
 numSensors=(sensors.getDeviceCount());
  if (numSensors > MaxOnewire) numSensors=MaxOnewire;   //Limit number of sensors to max number of sensors 
    byte j=0;                                        // search for one wire devices and
  while ((j < numSensors) && (oneWire.search(allAddress[j])))
    j++;
 delay(500);
 digitalWrite(DS18B20_PWR, LOW);
 
 if (numSensors==0) DS18B20_STATUS=0; 
    else DS18B20_STATUS=1; 
 
//delay(10000);            //wait for settle
  
  
  // Calculate if there is an ACAC adapter on analog input 0
  
  double vrms = calc_rms(0,1780) * 0.87;
  if (vrms>90) ACAC = 1; else ACAC=0;
  
  
  // CT Current calibration 
  // (2000 turns / 22 Ohm burden resistor = 90.909)
  ct1.current(1, Ical); 
  ct2.current(2, Ical);
  ct3.current(3, Ical);
 if (ACAC)
   { 
   // Calibration, phase_shift
    ct1.voltage(0, Vcal, phase_shift);                   
    ct2.voltage(0, Vcal, phase_shift);
    ct3.voltage(0, Vcal, phase_shift);
   }
 
  
  lastpost = 0;
  digitalWrite(LEDpin,LOW); 
  
}

void loop() {
  while(Serial.available()) {
    digitalWrite(LEDpin,HIGH); 
    String apiString = Serial.readStringUntil('\r');
      if (apiString.startsWith("$GP")) {
        Serial.print("$OK ");
        if (ACAC) {
          Serial.print(ct1.realPower); Serial.print(' '); 
          Serial.print(ct2.realPower); Serial.print(' '); 
          Serial.print(ct3.realPower);  
        }
        else {
          Serial.print(emontx.power1); Serial.print(' '); 
          Serial.print(emontx.power2); Serial.print(' '); 
          Serial.print(emontx.power3); 
        }
      }  
      if (apiString.startsWith("$GV")) {
        Serial.print("$OK ");
        Serial.println(emontx.Vrms);
      }
      if (apiString.startsWith("$GT")) {
        Serial.print("$OK ");
        if (DS18B20_STATUS==1){
          for(byte j=0;j<numSensors;j++){
            Serial.print(emontx.temp[j]);
            Serial.print(" ");
        } 
      }  
    }
    else { 
      Serial.flush();
    }
   digitalWrite(LEDpin,LOW);  
  }
  
  if ((millis()-lastpost)>=10000) {
    lastpost = millis();
    
    if (ACAC) {
      delay(200);                         //if powering from AC-AC allow time for power supply to settle    
      emontx.Vrms=0;                      //Set Vrms to zero, this will be overwirtten by CT 1-4
      
      ct1.calcVI(no_of_half_wavelengths,timeout); 
      emontx.power1=ct1.realPower;
      emontx.Vrms=ct1.Vrms*100;
    
      ct2.calcVI(no_of_half_wavelengths,timeout); 
      emontx.power2=ct2.realPower;
      emontx.Vrms=ct2.Vrms*100;
    
      ct3.calcVI(no_of_half_wavelengths,timeout); 
      emontx.power3=ct3.realPower;
      emontx.Vrms=ct3.Vrms*100;
 
    }
    else {
      emontx.power1 = ct1.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of sample
      emontx.power2 = ct2.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of samples
      emontx.power3 = ct3.calcIrms(no_of_samples)*Vrms;                               // Calculate Apparent Power 1  1480 is  number of samples
    }
    
    if (DS18B20_STATUS==1) {
      digitalWrite(DS18B20_PWR, HIGH); 
      delay(50); 
      for(int j=0;j<numSensors;j++) sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);      // and set the a to d conversion resolution of each.
      sensors.requestTemperatures();
      delay(ASYNC_DELAY); //Must wait for conversion, since we use ASYNC mode 
      for(byte j=0;j<numSensors;j++) emontx.temp[j]=get_temperature(j); 
      digitalWrite(DS18B20_PWR, LOW);
    } 
  }
}  

void onPulse() {  
  if ( (millis() - pulsetime) > min_pulsewidth) {
    pulseCount++;					//calculate wh elapsed from time between pulses
    pulsetime=millis(); 
  }	
}

int get_temperature(byte sensor) {
  float temp=(sensors.getTempC(allAddress[sensor]));
  if ((temp<125.0) && (temp>-55.0)) return(temp*10);            //if reading is within range for the sensor convert float to int ready to send via RF
}

double calc_rms(int pin, int samples)
{
  unsigned long sum = 0;
  for (int i=0; i<samples; i++) // 178 samples takes about 20ms
  {
    int raw = (analogRead(0)-512);
    sum += (unsigned long)raw * raw;
  }
  double rms = sqrt((double)sum / samples);
  return rms;
}
