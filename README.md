# Air Quality Monitoring MQ-135 | ESP32 

Air Quality Monitoring System using MQ135 Sensor.

## Installation

Use the Library manager to install : 

```c
#include <MQUnifiedsensor.h>
#include "SH1106Wire.h"
#include <Wire.h>
```

## Define MQ-135 pins and ESP32 Board

```c
#define board ("ESP-32")
#define Voltage_Resolution 3.3
#define pin 15 //Analog input 0 of your arduino
#define type "MQ-135" //MQ135
#define ADC_Bit_Resolution 12 // For arduino UNO/MEGA/NANO
#define RatioMQ135CleanAir 3.6//RS / R0 = 3.6 ppm  
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
[MIT](https://choosealicense.com/licenses/mit/)