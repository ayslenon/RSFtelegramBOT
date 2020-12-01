# RSFtelegramBOT

## Overview
This project is a simple use of a telegram bot, created with BOTFather using ESP32.
It was made in 2019 as a final project.

It has some issues and not optimized parts.

ESP32 connects to a wifi and waits to new chats on telegram, you can set the max number os users that can chat with esp32 and set the commands that esp32 answers. In my case I did some functions to turn on and off two valves to irrigate plants, and set a timer to this valves to be turned on. It has functions to read soil moisure sensors, and functions to force stop before the valves timer is done.

You can make the changes for your specific use. I am not working on this project anymore, but if you need some help you can talk to me.

## Characteristics 
- Developed in Arduino IDE for esp32
- Resets system after 6h, to avoid loss of data
- Has a watchdog timer, and if the esp32 stops in any point it will reset
- You can change the code to use telegram BOT as you want
- Code commented in portuguese-brazilian