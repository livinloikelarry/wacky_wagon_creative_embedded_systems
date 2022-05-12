# wacky_wagon_creative_embedded_systems

## Materials

 - ESP32 
 - computer
 - USB-C 
 - 4x AA battery box with wires
 - battery 
 - 2 DC Brush motors 
 - L293D Driver
 - DC Brush motor wheel attachments
 - car base 
 - breadboard 
 - female to female wires 
 - distance sensor 


### Setup Arduino 

- Download and open the Arduino IDE ([Download](https://www.arduino.cc/en/software))
- Open Preferences 
- Copy and paste the following link ([https://dl.espressif.com/dl/package_esp32_index.json](https://dl.espressif.com/dl/package_esp32_index.json)) into `Additonal Boards Manager URLs` to add the ESP package
- Select `Tools > Boards > Boards Manager`, then search for `esp32` and install the most recent version
- Select `Tools > Boards > ESP32 Arduino > TTGO T1`
- Select `Tools > Manage Libraries`, then search for `TFT_eSPI` and install the most recent version
- In your file storage system, find the Arduino folder
- Open `Arduino/libraries/TFT_eSPI/User_Setup_Select.h`
- Comment out the line that says `#include <User_Setup.h>`
- Uncomment the line that says `#include <User_Setups/Setup25_TTGO_T_Display.h>`

### build this project 

