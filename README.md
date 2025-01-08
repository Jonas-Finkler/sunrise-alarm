# Sunrise Alarm 
This project is and alarm clock, inspired by Danish winters and powered by an ESP32, that controlls Wiz lights via WiFi. 
It turns on the lights in the room gently, simulating a sunrise before the alarm goes off. 
![Image of the alarm clock](https://github.com/Jonas-Finkler/sunrise-alarm/blob/main/images/alarm.jpg)

## Functionality
The alarm clock features three buttons on top. 
When pressed once, each button toggles on of three lights in the room. 
A double press of the buttons allows to adjust the brightness using the rotary encoder on the right side of the alarm. 
By pressing a button three times the temperature of the light can be adjusted too. 
In the adjustment modes, other lights can be selected by pressing the corresponding buttons. 
The selection is indicated by decimal dots in the LED display. 

By pressing the button included in the rotary encoder, the alarm can be set. 
When the alarm goes off (a buzzer is integrated inside the case) it can be snoozed by a single press of any button on top or turned of by a double press. 

## Case
The case is lasercut out of 3mm thick MDF and was designed using inkscape. 
![Lasercutting of the case](https://github.com/Jonas-Finkler/sunrise-alarm/blob/main/images/case-cut.svg)
![Empty case](https://github.com/Jonas-Finkler/sunrise-alarm/blob/main/images/case_empty.jpg)

It is designed to be assembled without glue. 
All parts, including the ESP32, a buzzer and an seven segment LED display are held in place by friction fits.
![Case with parts](https://github.com/Jonas-Finkler/sunrise-alarm/blob/main/images/front_assembled.jpg)

## Software
To interact with the Wiz light bulbs, the [EspWizLight library](https://github.com/Jonas-Finkler/EspWizLight) is used.
