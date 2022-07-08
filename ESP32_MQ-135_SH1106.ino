//Include the library
#include <MQUnifiedsensor.h>
#include "SH1106Wire.h"
#include <Wire.h>
//Definitions
#define board ("ESP-32")
#define Voltage_Resolution 3.3
#define pin 15 //Analog input 0 of your arduino
#define type "MQ-135" //MQ135
#define ADC_Bit_Resolution 12 // For arduino UNO/MEGA/NANO
#define RatioMQ135CleanAir 3.6//RS / R0 = 3.6 ppm  
//#define calibration_button 13 //Pin to calibrate your sensor

//Declare Sensor
MQUnifiedsensor MQ135(board, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
SH1106Wire display(0x3c, 21, 22); //oled instance with address and SDA and SCL pins

void setup() {
  //Init the serial port communication - to debug the library
  Serial.begin(9600); //Init serial port
  Wire.begin();
  display.init();
  display.setFont(ArialMT_Plain_10);
  //Set math model to calculate the PPM concentration and the value of constants
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  // MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to to calculate C02 concentration
  // MQ135.setA(102.2); MQ135.setB(-2.473); // Configure the equation to to calculate NH4 concentration
  // MQ135.setA(77.255); MQ135.setB(-3.18); // Configure the equation to to calculate Alcohol concentration

  /*
    Exponential regression:
  GAS      | a      | b
  CO       | 605.18 | -3.937  
  Alcohol  | 77.255 | -3.18 
  CO2      | 110.47 | -2.862
  Toluen  | 44.947 | -3.445
  NH4      | 102.2  | -2.473
  Aceton  | 34.668 | -3.369
  */
  
  /*****************************  MQ Init ********************************************/ 
  //Remarks: Configure the pin of arduino as input.
  /************************************************************************************/ 
  MQ135.init(); 
  /* 
    //If the RL value is different from 10K please assign your RL value with the following method:
    MQ135.setRL(10);
  */
  /*****************************  MQ CAlibration ********************************************/ 
  // Explanation: 
   // In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
  // and on clean air (Calibration conditions), setting up R0 value.
  // We recomend executing this routine only on setup in laboratory conditions.
  // This routine does not need to be executed on each restart, you can load your R0 value from eeprom.
  // Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
  MQ135.serialDebug(true);
}
void loop() {
  printDisplay();
  delay(250);
}


void drawProgressBarDemo() {
float NH4, CO2, ALC;
  for(int i = 0; i<=3; i++){
      switch (i){
      case 1:
        MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to to calculate C02 concentration
        MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
        MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
        CO2 = MQ135.ppmprint(1);
        Serial.print("CO2 : ");
      break;
      case 2:
        MQ135.setA(102.2); MQ135.setB(-2.473); // Configure the equation to to calculate NH4 concentration
        MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
        MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
        NH4 = MQ135.ppmprint(1);
        Serial.print("NH4 : ");
      break;
      case 3:
        MQ135.setA(77.255); MQ135.setB(-3.18); // Configure the equation to to calculate Alcohol concentration
        MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
        MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
        ALC = MQ135.ppmprint(1);
        Serial.print("Alcohol : ");
      break;
    }
  }
  
  // MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
  // MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

  // float h = lightMeter.readLightLevel();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "CO2 : "+ String(CO2) + " PPM" );
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 14, "NH4 : " + String(NH4) + " PPM" );
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 28, "ALCOHOL : " + String(ALC) + " PPM" );
    
}

void printDisplay(){
  display.clear();
  drawProgressBarDemo();
  display.display();
}