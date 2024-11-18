/*
  Spacetime using ESP32

  Modified by Tiffany Tseng for esp32 Arduino Board Definition 3.0+ 
  Originally created by Mark Santolucito for Barnard COMS 3930
  Based on DroneBot Workshop 2022 ESP-NOW Multi Unit Demo
*/

// Include Libraries
#include <WiFi.h>
#include <esp_now.h>
#include <TFT_eSPI.h>  // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include "image.h"

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite sprite = TFT_eSprite(&tft); // for background

String cmd1 = "";
String cmd2 = "";
volatile bool scheduleCmd1Send = false;
volatile bool scheduleCmd2Send = false;

String cmdRecvd = "";
const String waitingCmd = "Waiting...";
bool redrawCmdRecvd = false;

// for drawing progress bars
int progress = 0;
bool redrawProgress = true;
int lastRedrawTime = 0;

//we could also use xSemaphoreGiveFromISR and its associated fxns, but this is fine
volatile bool scheduleCmdAsk = true;
hw_timer_t *askRequestTimer = NULL;
volatile bool askExpired = false;
hw_timer_t *askExpireTimer = NULL;
int expireLength = 25;

#define ARRAY_SIZE 10
#define MAC_ADDR_SIZE 100
const String commandVerbs[ARRAY_SIZE] = { "Buzz", "Engage", "Floop", "Bother", "Twist", "Jingle", "Jangle", "Yank", "Press", "Play" };
const String commandNounsFirst[ARRAY_SIZE] = { "foo", "dev", "bobby", "jaw", "tooty", "wu", "fizz", "rot", "tea", "bee" };
const String commandNounsSecond[ARRAY_SIZE] = { "bars", "ices", "pins", "nobs", "zops", "tangs", "bells", "wels", "pops", "bops" };

const String multiCommandVerbs[ARRAY_SIZE] = { "VISIT", "PROTECT", "ATTACK", "DEFEND", "SHIELD", "TICKLE", "ROTATE", "DIVERT", "MULTIPLY", "SURRENDER" };
const String multiCommandNouns[ARRAY_SIZE] = { "ZOO", "HOTTUB", "ENGINE-BAY", "MISSLES", "NUKE-CORE", "PENGUINS", "ELECTRICAL", "ADMIN", "SPECIMINS", "EGGS" };
bool receivedMulti = false;
uint8_t multiMacAddrs[MAC_ADDR_SIZE] = {0};
int recvdFrom = 0;
int lastFlash = 0;
bool flashing = true;

uint8_t macAddrs[MAC_ADDR_SIZE] = {0};
int currentAddrs = 0;


int lineHeight = 30;

// Define LED and pushbutton pins
#define BUTTON_LEFT 0
#define BUTTON_RIGHT 35


// Variables for the background
const int imageW = 200;
const int imageH = 300;
const int screenW = 240;
const int screenH = 135;
unsigned short imageS[screenW * screenH] = {0}; 
const int screen_middle_x = (imageW - screenW) / 2;
const int screen_middle_y = (imageH - screenH) / 2;

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
// Formats MAC Address
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void addMacAddr(const uint8_t *macAddr) {
  for(int i = 0; i < MAC_ADDR_SIZE; i++) {
    if(macAddrs[i] == 0) {
      macAddrs[i] = macAddr[3];
      currentAddrs++;
      break;
    } else if(macAddrs[i] == macAddr[3]) {
      break;
    }
  }
}

void addMultiMacAddr(const uint8_t *macAddr) {
  for(int i = 0; i < MAC_ADDR_SIZE; i++) {
    if(multiMacAddrs[i] == 0) {
      multiMacAddrs[i] = macAddr[3];
      recvdFrom++;
      break;
    } else if(multiMacAddrs[i] == macAddr[3]) {
      break;
    }
  }
}

void clearMultis() {
  for(int i = 0; i < MAC_ADDR_SIZE; i++) {
    multiMacAddrs[i] = 0;
  }
}

void receiveCallback(const esp_now_recv_info_t *macAddr, const uint8_t *data, int dataLen)
/* Called when data is received
   You can receive 5 types of messages
   1) a "ASK" message, which indicates that your device should display the cmd if the device is free
   2) a "DONE" message, which indicates the current ASK? cmd has been executed
   3) a "PROGRESS" message, indicating a change in the progress of the spaceship
   4) a "MULTI" message, indicating a user has sent a multiMessage
   5) a "COMPLETED" message, indicating a user has completed a multi message
   
   Messages are formatted as follows:
   [A/D]: cmd
   For example, an ASK message for "Twist the wutangs":
   A: Twist the wutangs
   For example, a DONE message for "Engage the devnobs":
   D: Engage the devnobs
   For example, a PROGESS message for 75% progress
   P: 75
   For example, a MULTI message for "PROTECT ZOO"
   M: PROTECT ZOO
   For example, a COMPLETED MESSAGE
   C
*/

{
  // add their mac addr
  addMacAddr(macAddr->src_addr);

  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);

  // Make sure we are null terminated
  buffer[msgLen] = 0;
  String recvd = String(buffer);
  Serial.println(recvd);
  // Format the MAC address
  char macStr[18];
  // formatMacAddress(macAddr, macStr, 18);

  // Send Debug log message to the serial port
  Serial.printf("Received message from: %s \n%s\n", macStr, buffer);
  if (recvd[0] == 'A' && cmdRecvd == waitingCmd && random(100) < 30)  //only take an ask if you don't have an ask already and only take it XX% of the time
  {
    recvd.remove(0, 3);
    cmdRecvd = recvd;
    redrawCmdRecvd = true;
    timerStart(askExpireTimer);  //once you get an ask, a timer starts
  } else if (recvd[0] == 'D' && recvd.substring(3) == cmdRecvd) {
    timerWrite(askExpireTimer, 0);
    timerStop(askExpireTimer);
    cmdRecvd = waitingCmd;
    progress = progress + 1;
    broadcast("P: " + String(progress));
    redrawCmdRecvd = true;
  } else if (recvd[0] == 'P') {
    recvd.remove(0, 3);
    progress = recvd.toInt();
    redrawProgress = true;
  } else if (recvd[0] == 'M' && cmdRecvd == waitingCmd && random(100) < 30) {
    recvd.remove(0, 3);
    cmdRecvd = recvd;
    redrawCmdRecvd = true;
    receivedMulti = true;
    recvdFrom = 0;
    clearMultis();
    timerStart(askExpireTimer);  //once you get an ask, a timer starts
  } else if(recvd[0] == 'C' && receivedMulti) {
    addMultiMacAddr(macAddr->src_addr);
    if(recvdFrom == currentAddrs) {
      timerWrite(askExpireTimer, 0);
      timerStop(askExpireTimer);
      cmdRecvd = waitingCmd;
      progress = progress + 1;
      broadcast("P: " + String(progress));
      redrawCmdRecvd = true;
      receivedMulti = false;
      flashing = false;
      tft.drawRect(3, 3, tft.width()-3, tft.height()-3, TFT_BLACK);
    }
  }
}

void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
// Called when data is sent
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void broadcast(const String &message)
// Emulates a broadcast
{
  // Broadcast a message to every device in range
  uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress)) {
    esp_now_add_peer(&peerInfo);
  }
  // Send message
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
}

void IRAM_ATTR sendCmd1() {
  scheduleCmd1Send = true;
  if(!digitalRead(BUTTON_RIGHT))
      scheduleCmd2Send = true;
}

void IRAM_ATTR sendCmd2() {
  scheduleCmd2Send = true;
  if(!digitalRead(BUTTON_LEFT))
      scheduleCmd1Send = true;
}

void IRAM_ATTR onAskReqTimer() {
  scheduleCmdAsk = true;
}

void IRAM_ATTR onAskExpireTimer() {
  askExpired = true;
  timerStop(askExpireTimer);
  timerWrite(askExpireTimer, 0);
}

void espnowSetup() {
  // Set ESP32 in STA mode to begin with
  delay(500);
  WiFi.mode(WIFI_STA);
  Serial.println("ESP-NOW Broadcast Demo");

  // Print MAC address
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Disconnect from WiFi
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESP-NOW Init Success");
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  } else {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    ESP.restart();
  }
}

void buttonSetup() {
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), sendCmd1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), sendCmd2, FALLING);
}

void textSetup() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
  drawControls();

  cmdRecvd = waitingCmd;
  redrawCmdRecvd = true;
}

void timerSetup() {
  // https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html
  askRequestTimer = timerBegin(1000000); // 1MHz
  timerAttachInterrupt(askRequestTimer, &onAskReqTimer);
  timerAlarm(askRequestTimer, 5 * 1000000, true, 0);  //send out an ask every 5 secs

  askExpireTimer = timerBegin(80000000);
  timerAttachInterrupt(askExpireTimer, &onAskExpireTimer);
  timerAlarm(askExpireTimer, expireLength * 1000000, true, 0);
  timerStop(askExpireTimer);
}
void setup() {
  Serial.begin(115200);
  textSetup();
  buttonSetup();
  espnowSetup();
  timerSetup();

  //background
  drawBackground();
}

String genCommand() {
  String verb = commandVerbs[random(ARRAY_SIZE)];
  String noun1 = commandNounsFirst[random(ARRAY_SIZE)];
  String noun2 = commandNounsSecond[random(ARRAY_SIZE)];
  return verb + " " + noun1 + noun2;
}

String genMulti() {
  String verb = multiCommandVerbs[random(ARRAY_SIZE)];
  String noun = multiCommandNouns[random(ARRAY_SIZE)];

  return verb + " " + noun;
}

void drawControls() {

  cmd1 = genCommand();
  cmd2 = genCommand();
  cmd1.indexOf(' ');

  int firstBox = 115;
  int secondBox = firstBox + 50;
  tft.drawRect(0, firstBox, tft.width(), 45, TFT_WHITE);
  tft.drawString("B1: " + cmd1.substring(0, cmd1.indexOf(' ')), 7, firstBox+5, 1.75);
  tft.drawString(cmd1.substring(cmd1.indexOf(' ') + 1), 7, firstBox+5 + lineHeight/2, 1.75);
  tft.drawRect(0, secondBox, tft.width(), 45, TFT_WHITE);
  tft.drawString("B2: " + cmd2.substring(0, cmd2.indexOf(' ')), 7, secondBox+5, 1.75);
  tft.drawString(cmd2.substring(cmd2.indexOf(' ') + 1), 7, secondBox+5 + lineHeight/2, 1.75);

  tft.drawString("B1", 5, tft.height()-20, 1.5);
  tft.drawString("B2", tft.width()-30, tft.height()-20, 1.5);
}

void loop() {
  if(scheduleCmd1Send && scheduleCmd2Send) {
    broadcast("C");
  }
  if (scheduleCmd1Send) {
    broadcast("D: " + cmd1);
    scheduleCmd1Send = false;
  }
  if (scheduleCmd2Send) {
    broadcast("D: " + cmd2);
    scheduleCmd2Send = false;
  }
  if (scheduleCmdAsk) {
    String cmdAsk;
    if(random(ARRAY_SIZE * ARRAY_SIZE/2) == 7) {
      cmdAsk = genMulti();
      broadcast("M: " + cmdAsk);
    } else {
      cmdAsk = random(2) ? cmd1 : cmd2;
      broadcast("A: " + cmdAsk);
    }

    scheduleCmdAsk = false;
  }
  if (askExpired) {
    progress = max(0, progress - 1);
    broadcast(String(progress));
    //tft.fillRect(0, 0, 135, 90, TFT_RED);
    cmdRecvd = waitingCmd;
    redrawCmdRecvd = true;
    askExpired = false;
  }

  if ((millis() - lastRedrawTime) > 50) {
    tft.fillRect(15, lineHeight * 2 + 14, 100, 6, TFT_WHITE);
    tft.fillRect(16, lineHeight * 2 + 14 + 1, (((expireLength * 1000000.0) - timerRead(askExpireTimer)) / (expireLength * 1000000.0)) * 98, 4, TFT_RED);
    lastRedrawTime = millis();
  }

  if(receivedMulti) {
    if(flashing) {
      tft.drawRect(3, 3, tft.width()-3, tft.height()-3, TFT_RED);
      if(millis() - 300 > lastFlash) {
        lastFlash = millis();
        flashing = false;
        tft.drawRect(3, 3, tft.width()-3, tft.height()-3, TFT_BLACK);
      }
    } else {
      if(millis() > lastFlash + 1000) {
        flashing = true;
        tft.drawRect(3, 3, tft.width()-3, tft.height()-3, TFT_RED);
      }
    }
  }

  if (redrawCmdRecvd || redrawProgress) {
    tft.fillRect(0, 0, 135, 90, TFT_BLACK);
    tft.drawString(cmdRecvd.substring(0, cmdRecvd.indexOf(' ')), 5, 5, 1);
    if(cmdRecvd != waitingCmd)
      tft.drawString(cmdRecvd.substring(cmdRecvd.indexOf(' ') + 1), 5, 5 + lineHeight/2, 1);
    redrawCmdRecvd = false;

    if (progress >= 100) {
      tft.fillScreen(TFT_BLUE);
      tft.setTextSize(3);
      tft.setTextColor(TFT_WHITE, TFT_BLUE);
      tft.drawString("GO", 45, 20, 2);
      tft.drawString("COMS", 20, 80, 2);
      tft.drawString("3930!", 18, 130, 2);
      delay(6000);
      ESP.restart();
    } else {
      tft.fillRect(15, lineHeight * 2 + 5, 100, 6, TFT_WHITE);
      tft.fillRect(16, lineHeight * 2 + 5 + 1, progress, 4, TFT_BLUE);
    }
    redrawProgress = false;
  }
   updateDigitalTimer();
}

void drawBackground() {
  int pos = screen_middle_x + imageW * screen_middle_y;
  int start = pos;
  int m = screenW + pos;

  // Load the portion of the image into the buffer
  for (int i = 0; i < screenW * screenH; i++) {
    if (start % m == 0) {
      start += (imageW - screenW);
      m += imageW;
    }
    imageS[i] = picture[start];
    start++;
  }

  // Display the static image on the screen
  tft.pushImage(0, 0, screenW, screenH, imageS);
}

void updateDigitalTimer() {
  // Calculate remaining time
  unsigned long remainingTime = expireLength * 1000000 - timerRead(askExpireTimer); 
  if (remainingTime < 0) remainingTime = 0; // Avoid negative values

  unsigned long totalSeconds = remainingTime / 1000000;
  unsigned long minutes = totalSeconds / 60;
  unsigned long seconds = totalSeconds % 60;

  // Clear the previous timer text
  tft.fillRect(10, 10, 120, 30, TFT_BLACK); 

  // Display time on the screen with a smaller text size
  tft.setTextSize(1); 
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(100, 2);
  tft.printf("%02lu:%02lu", minutes, seconds);

  tft.setTextSize(2); // Restore default text size for other text
}