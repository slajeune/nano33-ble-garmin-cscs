/* This code and use of a nano 33 BLE sens is directly adapted from Tom Schucker code over at       */
        https://teaandtechtime.com/arduino-ble-cycling-power-service/                               */
/* The code here was modified to make the board as CSCS sensor (speed sensor) instead of a power    */
/*     sensor as I wanted to track speed and distance on my garmin                                  */
/* The algorithm to detect one wheel revolution was also changed and it should be generic           */
/* The sensor was tested on a garmin Forerunner 245M and confirmed as working properly              */
/* For some reason, updating once every other revolutions (3 in this case) seem required for the    */
/*     Garmin. Without it, the speed was very jumpy and it showed in garmin connect                 */
/* This was installed on a Xterra FB150: http://xterrafitness.ca/FB150-cycle.html                   */
/* Wheel size of 1565mm matches speed on console which is in KM/h on my bike and not miles/h        */
/*    based on a Keiser M3i I used, I can sustain 23km/h at a specific HR point                     */
/*    and this matches the 24 (km/h) I'm seeing on my bike                                          */
/*    1565mm wheel size matches this speed (24km/h) on my garmin watch                              */
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
short c_spin = 0;

double i_prev = 0;
double i_curr = 0;
double i_diff = 0;
double counter = 0;

//Configurable values
double mag_samps_per_sec = 16;

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
        counter = counter + 1;
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
          i_curr = counter-1;
          i_diff = i_curr - i_prev;

          revolutions = revolutions + 1;
          timestamp = timestamp + (unsigned short)(i_diff*(1024/mag_samps_per_sec));
          i_prev = i_curr;
          
          /* For crank data */
          cum_cranks = revolutions*0.32;
          crank_rev_period = ((50/16)*1024*timestamp);
          last_crank_event += crank_rev_period;

          /* add data buff portion!!*/ 
          data_buf[0] |= CSC_FEATURE_WHEEL_REV_DATA;
          
          /* fill the measurement data */
          memcpy(&data_buf[1], &revolutions, 4);
          memcpy(&data_buf[5], &timestamp, 5);
          memcpy(&data_buf[7], &cum_cranks, 6);
          memcpy(&data_buf[9], &last_crank_event, 7);

          c_spin = c_spin + 1;

          /* update sensor every 3 spins to smooth the data spike caused by the fudge factor (3 for 1.33, 2 for 1.5, etc */
          /* updating every 3 revoultions also seems to be required for garmin even with fudge = 1.0! No clue why        */
          if ( c_spin == 3) 
             {
          
               /* Publish measurements */
               CycleCSCMeasurement.writeValue(data_buf, sizeof(data_buf));
               n_spin = false;
               c_spin = 0;
             }
        }
      }
    }
  }
}

