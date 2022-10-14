/* This code and use of a nano 33 BLE sens is directly adapted from Tom Schucker code over at       */
/*      https://teaandtechtime.com/arduino-ble-cycling-power-service/                               */
/* The code here was modified to make the board as CSCS sensor (speed sensor) instead of a power    */
/*     sensor as I wanted to track speed and distance on my garmin                                  */
/* The algorithm to detect one wheel revolution was also changed and it should be generic           */
/* The sensor was tested on a garmin Forerunner 245M and confirmed as working properly              */
/* For some reason, updating once every other revolutions (3 in this case) seem required for the    */
/*     Garmin. Without it, the speed was very jumpy and it showed in garmin connect                 */
/* This was installed on a Xterra FB150: http://xterrafitness.ca/FB150-cycle.html                   */
/* Wheel size of 1800mm matches speed on console which is in KM/h on my bike and not miles/h        */
/*    based on a Keiser M3i I used, I can sustain 23km/h at a specific HR point                     */
/*    and this matches the 24 (km/h) I'm seeing on my bike                                          */
/*    1800mm wheel size matches this speed (24km/h) on my garmin watch                              */
/* To compile, you need Arduino Mbed OS boards package 1.1.6 (deprecated)                           */
/* Failure to do so will result in an ospriority missing error                                      */

#include "Arduino.h"
/* For the bluetooth funcionality */
#include <ArduinoBLE.h>
/* For the use of the IMU sensor */
#include "Nano33BLEMagnetic.h"

/* Device name which can be scene in BLE scanning software. */
#define BLE_DEVICE_NAME               "Arduino Nano 33 BLE Sense"
/* Local name which should pop up when scanning for BLE devices. */
#define BLE_LOCAL_NAME                "Cycle CSC BLE"

Nano33BLEMagneticData magneticData;

BLEService CycleCSCService("1816");
BLECharacteristic CycleCSCFeature("2A5C", BLERead, 4);
BLECharacteristic CycleCSCMeasurement("2A5B", BLERead | BLENotify, 8);
BLECharacteristic CycleCSCSensorLocation("2A5D", BLERead, 1);
BLECharacteristic CycleCSCControlPoint("2A55", BLEWrite | BLEIndicate, 8);

#define     CSC_MEASUREMENT_WHEEL_REV_PRESENT       0x01
#define     CSC_MEASUREMENT_CRANK_REV_PRESENT       0x02

#define     CSC_FEATURE_WHEEL_REV_DATA              0x01
#define     CSC_FEATURE_CRANK_REV_DATA              0x02

uint8_t data_buf[11];

uint32_t revolutions = 0;
uint32_t f_revolutions = 0;
unsigned short cum_cranks = 0;
unsigned short timestamp = 0;
byte     sensorlocation = 0x04;
uint16_t last_crank_event = 0;
uint16_t crank_rev_period;

float tm2 = 0;
float tm1 = 0;
float tm0 = 0;
/* For algo to detect spin */
/* direction = 1 is up and direction = 0 is down */
short old_direction = 1;
short direction = 1;
bool n_spin = false;
/* Might need to adjust for your bike, this is required for the FB150 */
float fudge = 2;

/* Update sensor every 2 seconds as it makes the reading smoother with my garmin watch */
double update_interval = 2000;
unsigned long cur_millis = 0;
unsigned long old_millis = 0;

void setup() 
{
  // put your setup code here, to run once:
  /* BLE Setup. For information, search for the many ArduinoBLE examples.*/
  if (!BLE.begin()) 
  {
    while (1);    
  }
  else
  {
    BLE.setDeviceName(BLE_DEVICE_NAME);
    BLE.setLocalName(BLE_LOCAL_NAME);
    BLE.setAdvertisedService(CycleCSCService);
    CycleCSCService.addCharacteristic(CycleCSCFeature);
    CycleCSCService.addCharacteristic(CycleCSCMeasurement);
    CycleCSCService.addCharacteristic(CycleCSCSensorLocation);
    CycleCSCService.addCharacteristic(CycleCSCControlPoint);

    /* initliaze location and features */
    CycleCSCSensorLocation.writeValue((byte)sensorlocation);
    CycleCSCFeature.writeValue((uint16_t)0x0001);
    
    BLE.addService(CycleCSCService);
    BLE.advertise();

    Magnetic.begin();
   }       
}

void loop() 
{
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  if(central)
  {     
    /* 
    If a BLE device is connected, magnetic data will start being read, 
    and the data will be processed
    */
    while(central.connected())
    {            
      if(Magnetic.pop(magneticData))
      {
        /* 
        process magnetic to detect revolution and publish data to sensor
        */
        tm0 = tm1;
        tm1 = tm2;
        tm2 = magneticData.z;
        
        /* Algo to find peak                                              */
        /* three downs after change from up to down direction = 1 rotation */

        if ((tm2 > tm1) && (tm1 > tm0))
        {
          /* Three ups in a row makes direction go up */
          direction = 1;
        }

        if ((tm0 > tm1) && (tm1 > tm2))
        {
          /* Three downs in a row makes direction go down */
          direction = 0;
        }


        /* change of direction */
        if (old_direction != direction)
        {
          /* change of direction */
          if ((old_direction == 1) && ( direction == 0))
             {
                /* Change from up to down means 1 revolution */
                n_spin = true;
             }
             
             old_direction = direction;
        }
        
        
        if(n_spin)
        {
          n_spin = false;
          cur_millis = millis();

          revolutions = revolutions + 1;
          f_revolutions = revolutions * fudge;
          timestamp = cur_millis;
          
          /* For crank data */
          cum_cranks = f_revolutions*0.32;
          crank_rev_period = ((50/16)*1024*timestamp);
          last_crank_event += crank_rev_period;

          /* add data buff portion!!*/ 
          data_buf[0] |= CSC_FEATURE_WHEEL_REV_DATA;
          
          /* fill the measurement data */
          memcpy(&data_buf[1], &f_revolutions, 4);
          memcpy(&data_buf[5], &timestamp, 5);
          memcpy(&data_buf[7], &cum_cranks, 6);
          memcpy(&data_buf[9], &last_crank_event, 7);

          /* update sensor every second as per specs */
          if ( (cur_millis - old_millis) >= update_interval) 
             {
               old_millis = cur_millis;
               /* Publish measurements */
               CycleCSCMeasurement.writeValue(data_buf, sizeof(data_buf));
             }
        }
      }
    }
  }
}

