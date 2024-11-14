*
  Display a static image (from image.h) on a TFT
*/

#include "TFT_eSPI.h"
#include "image.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

const int imageW = 200;
const int imageH = 300;

const int screenW = 240;
const int screenH = 135;

unsigned short imageS[screenW * screenH] = {0}; // Screen buffer for displayed portion

// Set `x` and `y` to center the image on the screen
const int x = (imageW - screenW) / 2;
const int y = (imageH - screenH) / 2;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);

  int pos = x + imageW * y;
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

void loop() {
}
