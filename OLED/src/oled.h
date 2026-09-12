/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of OLED
  MYOSA Platform consists of a beautiful OLED Display Board. It is equipped with SSD1306 IC.
  It is a very small display, about 1" in diagonal but still very readable due to high contrast. 
  This display is made of 128x64 individual white OLED pixels, each one is turned on or off by the controller chip.
  I2C Address of the board = 0x3C.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an 
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  10 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)
  
  Contact Team MYOSA for any kind of feedback/issues pertaining to performance or any update request.
  Email: myosa.event@gmail.com
*/

#ifndef __OLED_H__
#define __OLED_H__

#include <Wire.h>
#include <math.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "logo_array.h"

#define OLED_I2C_ADDRESS    0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

/*
 * Drawing and text helpers modify the framebuffer; call display() to send it to the screen.
 * Coordinates and dimensions are pixels; cube angles are degrees.
 * begin() includes a blocking welcome sequence.
 */
class oLed : public Adafruit_SSD1306
{
  public:
    oLed(uint8_t w, uint8_t h, TwoWire *twi=&Wire, int8_t rst_pin=-1,uint32_t clkDuring=400000UL, uint32_t clkAfter=100000UL);
    oLed(uint8_t w, uint8_t h, int8_t mosi_pin, int8_t sclk_pin,int8_t dc_pin, int8_t rst_pin, int8_t cs_pin);
    oLed(uint8_t w, uint8_t h, SPIClass *spi,int8_t dc_pin, int8_t rst_pin, int8_t cs_pin, uint32_t bitrate=8000000UL);
    bool begin(void);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
    void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color, uint16_t bg);
    void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
    void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg);
    void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
    void drawCube(float xAngle=0u,float yAngle=0u, float zAngle=0u);
    void setCursor(int16_t x, int16_t y);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t bg);
    void clearDisplay(void);
  private:
    typedef struct {
        float x;
        float y;
        float z;
    }point3D_t;
    point3D_t _vertices[8];
    point3D_t _mV[8];
    uint8_t _edges[12][2];
    void displayLogo(void);
    void rotateXYZ(point3D_t *self, float xAngle,float yAngle,float zAngle);
    void project3Dto2D(point3D_t *self, uint16_t win_width=128, uint16_t win_height=64, uint16_t fov=64, uint16_t viewer_distance=4);
};

#endif
