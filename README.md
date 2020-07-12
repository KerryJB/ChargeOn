# ChargeOn
Windows Laptop Battery Conditioner (GUI version)

## Description
This project is designed to "condition" a Windows laptop battery by continually
* charging it up to a user-defined maximum and then
* allowing it to discharge to a user-defined minimum

The inspiration for this project came from the Arduino-based **Smartphone Charger** project described on [Ralph S Bacon's YouTube channel](https://www.youtube.com/c/RalphBacon/videos) (see videos 179, 182 and 193).

A search for existing laptop-based solutions only turned up network-based approaches using Alexa, Google Assistant, etc. I don't own any such devices, and it's not clear whether I ever will. This project utilizes a 433MHz remote-controlled outlet instead, along with an Arduino module that is connected to a USB port on the laptop.

## Features
* Continuous display of battery charge percentage, and whether the battery is charging or discharging
* User-configurable MINimum charging range, from 20 to 95%
* User-configurable MAXimum charging range, from 25 to 100%
* On-screen button for manually turning the outlet ON (when discharging) or OFF (when charging)
* User-configurable update interval
* User-configurable remote outlet settings, as used by the Arduino **RCSwitch** library
  * ON code
  * OFF code
  * Bit length of ON/OFF codes
  * Protocol
  * Pulse length
* "Learn" function to capture the above settings for an outlet, as tranmitted by its dedicated remote control
* User-configurable number of "pulse repeats", to improve reliability by sending each code a specified number of times
* User-configurable setting to turn the outlet ON (if necessary) before exiting the program
* Menu-driven installation of CH340 driver (if needed) for certain Arduino Nano clone variants
* Menu-driven updating of Arduino firmware from a downloaded *.hex file (as updates are made available)
* Ability to monitor message traffic between the Windows program and Arduino sketch. This requires
  * A **ChargeOn** hardware module with an exposed connection to pin D9 on the Arduino Nano
  * A separate serial-to-USB adaptor
    * I used a (formerly) cheap Prolific 4-wire USB-to-TTL cable with an embedded PL2303 circuit but prices for these seem to have gone crazy
    * If I were starting fresh, I would buy a (still) cheap CP2102 module instead
  * A separate terminal program (I used [NinjaTerm](http://gbmhunter.github.io/NinjaTerm/) for my testing but there are MANY others including
    * [Br@y's Terminal](https://sites.google.com/site/terminalbpp/)
    * [CoolTerm](http://freeware.the-meiers.org/)
    * [RealTerm](https://sourceforge.net/projects/realterm/)
    * [Termite](https://www.compuphase.com/software_termite.htm)

## Components
* Arduino Nano board with 433MHz transmitter module and (optional) 433MHz receiver module
  * ![Arduino hardware module](http://kerryburton.com/Images/ChargeOnModule/ChargeOn_ArduinoHardwareModule.jpg)

* 433MHz remote outlet (tested with Dewenwils and Etekcity ZAP)
  * ![Arduino hardware module](http://kerryburton.com/Images/ChargeOnModule/ChargeOn_433MHzRemoteOutlets.jpg)

* Windows program
  * ![Windows dialog interface](http://kerryburton.com/Images/ChargeOnModule/ChargeOn_WindowsDialogInterface.jpg)
  
  * ![Windows Tools Menu](http://kerryburton.com/Images/ChargeOnModule/ChargeOn_WindowsToolsMenu.jpg)
  
  * ![Windows dialog interface](http://kerryburton.com/Images/ChargeOnModule/ChargeOn_WindowsOutletSettings.jpg)
  
* Arduino sketch
  * ![Arduino sketch](http://kerryburton.com/Images/ChargeOnModule/ChargeOn_ArduinoSketch.jpg)
