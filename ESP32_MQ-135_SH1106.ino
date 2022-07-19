//Include the library
#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp
#include <Ticker.h>

#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

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
DHTesp dht;
void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();
bool initTemp();

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;
/** Comfort profile */
ComfortState cf;
/** Flag if task should run */
bool tasksEnabled = false;
/** Pin number for DHT11 data pin */
int dhtPin = 17;

/**
 * initTemp
 * Setup DHT library
 * Setup task and timer for repeated measurement
 * @return bool
 *    true if task and timer are started
 *    false if task or timer couldn't be started
 */
bool initTemp() {
  byte resultValue = 0;
  // Initialize temperature sensor
	dht.setup(dhtPin, DHTesp::DHT11);
	Serial.println("DHT initiated");

  // Start task to get temperature
	xTaskCreatePinnedToCore(
			tempTask,                       /* Function to implement the task */
			"tempTask ",                    /* Name of the task */
			4000,                           /* Stack size in words */
			NULL,                           /* Task input parameter */
			5,                              /* Priority of the task */
			&tempTaskHandle,                /* Task handle. */
			1);                             /* Core where the task should run */

  if (tempTaskHandle == NULL) {
    Serial.println("Failed to start task for temperature update");
    return false;
  } else {
    // Start update of environment data every 20 seconds
    tempTicker.attach(20, triggerGetTemp);
  }
  return true;
}

/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker getTempTimer
 */
void triggerGetTemp() {
  if (tempTaskHandle != NULL) {
	   xTaskResumeFromISR(tempTaskHandle);
  }
}

/**
 * Task to reads temperature from DHT11 sensor
 * @param pvParameters
 *    pointer to task parameters
 */
void tempTask(void *pvParameters) {
	Serial.println("tempTask loop started");
	while (1) // tempTask loop
  {
    if (tasksEnabled) {
      // Get temperature values
			getTemperature();
		}
    // Got sleep again
		vTaskSuspend(NULL);
	}
}

/**
 * getTemperature
 * Reads temperature from DHT11 sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
*/
bool getTemperature() {
	// Reading temperature for humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
	// Check if any reads failed and exit early (to try again).
	if (dht.getStatus() != 0) {
		Serial.println("DHT11 error status: " + String(dht.getStatusString()));
		return false;
	}

	float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  String comfortStatus;
  switch(cf) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      break;
    default:
      comfortStatus = "Unknown:";
      break;
  };

  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
  
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "TEMP : " + String(newValues.temperature) + "  C" );
  display.display();
  delay(2000);
  return true;
}

void setup() {
  //Init the serial port communication - to debug the library
  Serial.begin(115200);
  Wire.begin();
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.invertDisplay();
  display.flipScreenVertically();
  //Set math model to calculate the PPM concentration and the value of constants
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to to calculate C02 concentration
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

  
  Serial.println();
  Serial.println("DHT ESP32 example with tasks");
  initTemp();
  // Signal end of setup() to tasks
  tasksEnabled = true;
}
void loop() {
  printDisplay();
  if (!tasksEnabled) {
    // Wait 2 seconds to let system settle down
    delay(2000);
    
    // Enable task that will read values from the DHT sensor
    tasksEnabled = true;
    if (tempTaskHandle != NULL) {
			vTaskResume(tempTaskHandle);
		}
    
  }
  yield();
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
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 0, "AIR QUALITY" );
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 14, "CO2 : "+ String(CO2) + " PPM" );
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 28, "NH4 : " + String(NH4) + " PPM" );
  // display.setTextAlignment(TEXT_ALIGN_LEFT);
  // display.drawString(0, 42, "TEMP : " + String(ALC) + " PPM" );
    
}

void printDisplay(){
  display.clear();
  drawProgressBarDemo();
  display.display();  
}