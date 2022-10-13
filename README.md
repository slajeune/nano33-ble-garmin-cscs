# nano33-ble-garmin-cscs
A speed sensor (CSCS) to use on stationary bikes with magnetized wheels (such as Xterra FB150)

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

