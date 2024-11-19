#  🪐 SPACE TEAM  🪐
COMS 3930 - Module 3

<div align="center">
 
![Demo GIF](ezgif.com-video-to-gif-converter.gif)

</div>


### 🙌🤝 Team 🙌🤝 
- Jake Torres 
- Mori Liu  
- Mila Lin Tabach 
- Sushmita Debnath 

### 🚀 Overview  🚀
This project is a modification of a space game, originally created by Mark Santolucito for Barnard COMS 3930. 
The Original Game is based on Spacetime, which is an interactive multiplayer game in which the team has to complete task collectives. 
Each player has an instruction and a set of actions that they can do. Players have to read their instructions out loud so the other players can execute the right actions. 


### 🔧 Hardware  🔧 
This code was written and tested in esp 32 (Lilly TTGO). 

### ✨ Added Features  ✨
- Change in the layout of the screen (added boxes, made the text smaller, added B1 and B2 so the user knows which buttons are which, added a background photo, changed the color of the text and the progress bar)
- Add the command that all players need to press both buttons at the same time. When any command in ALL CAPS appears, it means that the other players should press both buttons. The player that received the command does not have to press the buttons.  
-  Fixed the timer and added a number timer;

### 📋  Contents 📋
- arduinospaceteam.ino : the Arduino IDE sketch to be uploaded to the esp32 (the main code)
- ezgif.com-video-to-gif-converter.gif: demo gif of the game (sorry about the name)
- image.h : the image file generated through http://rinkydinkelectronics.com/_t_doimageconverter565.php#google_vignette 
- \background: in this folder, you can encounter a sketch to upload the image static on the esp32, as well as a copy of the image.h and a jpeg of the image. 

