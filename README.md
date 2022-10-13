# nano33-ble-garmin-cscs
A speed sensor (CSCS) to use on stationary bikes with magnetized wheels (such as Xterra FB150)

This code and use of a nano 33 BLE sense is directly adapted from Tom Schucker code over at 
https://teaandtechtime.com/arduino-ble-cycling-power-service/ 

The code here was modified to make the board as CSCS sensor (speed sensor) instead of a power sensor as I wanted to track speed and distance on my garmin   
The algorithm to detect one wheel revolution was also changed and it should be generic

The sensor was tested on a garmin Forerunner 245M and confirmed as working properly
For some reason, updating once every other revolutions (3 in this case) seem required for the Garmin. Without it, the speed was very jumpy and it showed in garmin connect

This was installed on a Xterra FB150: http://xterrafitness.ca/FB150-cycle.html

This should work on all such bikes that use a magnetized wheel without modifications. You could adapt the code to use the data from the 3.5mm jack. Please see Tom's article for details on what data is produced from the wheel.

For the Xterra FB150, the wheel size is 1565mm. Wheel size of 1565mm matches speed on console which is in KM/h on my bike and not miles/h based on a Keiser M3i I used, I can sustain 23km/h at a specific HR point and this matches the 24 (km/h) I'm seeing on my bike. Garmin watch also reports proper speed with the sensor and 1565mm wheel size. Do note that wheel size is specific to each stationnary bike and you will need to adjust accordingly on your watch / app. The sensor works on watches and apps that can use a bluetooth CSCS sensor.

To compile, you need Arduino Mbed OS boards package 1.1.6 (deprecated) Failure to do so will result in an ospriority missing error
