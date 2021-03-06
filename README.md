# doorman
Doorman is an ESP8266 based mqtt bridge for TCS door control systems (https://www.tcsag.de/). 

![doorman opener](doc/doorman.gif)

## Main Features
 * Mqtt integration for all messages read from the bus and for writing messages to the bus
 * Button push pattern detection - you can define a pattern and assign a code that is written to the bus if the pattern is detected (Example: automatically open the door if the doorbell is pressed x times in a certain way). A successfully detected pattern is also published via mqtt. 
 * Party mode (When this is enabled, the door opener automatically opens if the door bell is pressed)


## Wiring
![wiring](doc/wiring.svg)
If you open your phone in your flat (check TCS website for manuals), you will probably find the following screw terminal lines: A, B, E, P. 

A and B are the bus lines. You have to make sure which one is 24V+ and wire this to A of the setup. The other one is GND. 

The P line can be used for power supply. You can also skip the part and power through the USB port of the Wemos D1. 

### Part list
 * 1x Wemos D1 Mini
 * 1x ULN2003A (to send commands to the bus)
 * 1x Traco TSR1 2405 (dc/dc 24V to 5V as power supply for Wemos)
 * 1x tripple screw terminal (to connect to the bus)
 * 2x double screw terminal (optional, to connect switch and led)
 * 1x 1 MOhm resistor
 * 1x 1 kOhm resistor
 * 1x 147 kOhm
 * 1x 10k Ohm resistor (only needed to wire a button)
 * 1x 330 Ohm resistor (only needed to wire a led)
 * 2x 1.2 Ohm resistors

## Credits
Doorman is heavily built on the code and the information of the following two projects: 

**TCSIntercomArduino** different methods to read from and write to TCS bus<br />
Reverse Engineering video: <https://www.youtube.com/watch?v=xFLoauqj9yA&t=11s><br />
<https://github.com/atc1441/TCSintercomArduino>

**tcs-monitor** an mqtt monitor for listening to the TCS bus<br />
Blog Post: <https://blog.syralist.de/posts/smarthome/klingel/><br />
<https://github.com/Syralist/tcs-monitor>