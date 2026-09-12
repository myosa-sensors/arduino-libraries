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

#include "oled.h"

/*
 * Pass the selected bus and pin configuration to SSD1306; initialization happens in begin().
 */
oLed::oLed(uint8_t w, uint8_t h, TwoWire *twi, int8_t rst_pin, uint32_t clkDuring, uint32_t clkAfter)
:Adafruit_SSD1306(w, h, twi, rst_pin, clkDuring, clkAfter)
{
  }

/*
 * Pass the selected bus and pin configuration to SSD1306; initialization happens in begin().
 */
oLed::oLed(uint8_t w, uint8_t h, int8_t mosi_pin, int8_t sclk_pin, int8_t dc_pin, int8_t rst_pin, int8_t cs_pin)
:Adafruit_SSD1306(w, h, mosi_pin, sclk_pin, dc_pin, rst_pin, cs_pin)
{
  }

/*
 * Pass the selected bus and pin configuration to SSD1306; initialization happens in begin().
 */
oLed::oLed(uint8_t w, uint8_t h, SPIClass *spi,int8_t dc_pin, int8_t rst_pin, int8_t cs_pin, uint32_t bitrate)
:Adafruit_SSD1306(w, h, spi, dc_pin, rst_pin, cs_pin, bitrate)
{
  }

/*
 * Initialize cube geometry and the display, then show the blocking welcome sequence.
 */
bool oLed::begin(void)
{
    /* update the vertices */
    _vertices[0].x = -1;_vertices[0].y =  1;_vertices[0].z = -1;
    _vertices[1].x =  1;_vertices[1].y =  1;_vertices[1].z = -1;
    _vertices[2].x =  1;_vertices[2].y = -1;_vertices[2].z = -1;
    _vertices[3].x = -1;_vertices[3].y = -1;_vertices[3].z = -1;
    _vertices[4].x = -1;_vertices[4].y =  1;_vertices[4].z =  1;
    _vertices[5].x =  1;_vertices[5].y =  1;_vertices[5].z =  1;
    _vertices[6].x =  1;_vertices[6].y = -1;_vertices[6].z =  1;
    _vertices[7].x = -1;_vertices[7].y = -1;_vertices[7].z =  1;
    /* update the back edges */
    _edges[0][0] = 0; _edges[0][1] = 1; _edges[1][0] = 1; _edges[1][1] = 2;
    _edges[2][0] = 2; _edges[2][1] = 3; _edges[3][0] = 3; _edges[3][1] = 0;
    /* update the Front edges */
    _edges[4][0] = 5; _edges[4][1] = 4; _edges[5][0] = 4; _edges[5][1] = 7;
    _edges[6][0] = 7; _edges[6][1] = 6; _edges[7][0] = 6; _edges[7][1] = 5;
    /* update the Front-to-back edges */
    _edges[8][0] = 0; _edges[8][1] = 4; _edges[9][0] = 1; _edges[9][1] = 5;
    _edges[10][0] = 2; _edges[10][1] = 6; _edges[11][0] = 3; _edges[11][1] = 7;
    /* begin OLED display */
    if(Adafruit_SSD1306::begin(SSD1306_SWITCHCAPVCC,OLED_I2C_ADDRESS) == false)
    {
      return false;
    }
    clearDisplay();
    displayLogo();
    return true;
}

/*
 * Show the startup screens with delays, then clear the framebuffer.
 */
void oLed::displayLogo(void)
{
    Adafruit_GFX::setRotation(0);
    clearDisplay();
    setTextColor(WHITE); // or BLACK);
    Adafruit_GFX::setTextSize(2);      // printable sizes from 1 to 8; typical use is 1, 2 or 4
    setCursor(6, 6);     // begin text at this location
    Adafruit_GFX::print("Welcome!!!");
    Adafruit_SSD1306::display();
    delay(2000);

    Adafruit_GFX::setTextSize(3);
    setCursor(5, 33);     // begin text at this location
    Adafruit_GFX::print("MYOSA");
    Adafruit_SSD1306::display();
    delay(1000);
    
    Adafruit_GFX::setTextSize(1);
    setCursor(98, 48);     // begin text at this location
    Adafruit_GFX::print("v3.0");
    Adafruit_SSD1306::display();
    delay(2500);
    //drawBitmap(0, 32, logo16_glcd_bmp, 256, 64, 1); //draw logo
    //Adafruit_SSD1306::display();
    //delay(2000);
    clearDisplay();
    //setCursor(0,0);
    //Adafruit_SSD1306::display();
}

/*
 * Rotate by angles in degrees, project the vertices, and send the completed cube to the display.
 */
void oLed::drawCube(float xAngle,float yAngle, float zAngle)
{
    uint8_t e[2];

	/* clear the display */
    clearDisplay();
    /* Calculate the points */
    for(uint8_t nVertex=0u; nVertex < 8u; nVertex++)
    {
        _mV[nVertex].x = _vertices[nVertex].x;
        _mV[nVertex].y = _vertices[nVertex].y;
        _mV[nVertex].z = _vertices[nVertex].z;
        /* Calulate the 3D points */
        rotateXYZ(&_mV[nVertex],xAngle,yAngle,zAngle);
        /* project 3D to 2D */
        project3Dto2D(&_mV[nVertex]);
    }
	/* Plot the line to make cube */
    for(uint8_t nEdge=0u; nEdge < 12u; nEdge++)
    {
        e[0] = _edges[nEdge][0u];
        e[1] = _edges[nEdge][1u];
        drawLine((uint16_t)_mV[e[0]].x, (uint16_t)_mV[e[0]].y, (uint16_t)_mV[e[1]].x, (uint16_t)_mV[e[1]].y,1);
    }
    Adafruit_SSD1306::display();
}

/*
 * Rotate the supplied 3D point in place around X, Y, then Z; angles are in degrees.
 */
void oLed::rotateXYZ(point3D_t *self, float xAngle,float yAngle,float zAngle)
{
    float rad, cosa, sina ;
    float x, y, z;

    /* Rotates this point around the X axis the given number of degrees */
    rad     = xAngle * M_PI / 180.f;
    cosa    = cos(rad);
    sina    = sin(rad);
    y = self->y * cosa - self->z * sina;
    z = self->y * sina + self->z * cosa;
    self->y = y;
    self->z = z;

    /* Rotates this point around the Y axis the given number of degrees */
    rad     = yAngle * M_PI / 180.f;
    cosa    = cos(rad);
    sina    = sin(rad);
    z = self->z * cosa - self->x * sina;
    x = self->z * sina + self->x * cosa;
    self->z = z;
    self->x = x;

    /* Rotates this point around the Z axis the given number of degrees */
    rad     = zAngle * M_PI / 180.f;
    cosa    = cos(rad);
    sina    = sin(rad);
    x = self->x * cosa - self->y * sina;
    y = self->x * sina + self->y * cosa;
    self->x = x;
    self->y = y;
}

/*
 * Project a point in place into screen coordinates; viewer_distance + z must be nonzero.
 */
void oLed::project3Dto2D(point3D_t *self, uint16_t win_width, uint16_t win_height, uint16_t fov, uint16_t viewer_distance)
{
    float x, y;
    float factor;

    factor  =  fov / (viewer_distance + self->z);
    x       =  self->x * factor + win_width / 2.f;
    y       = -self->y * factor + win_height / 2.f;

    /* update the axes */
    self->x = x;
    self->y = y;
}


/*
 * Write to the framebuffer; call display() after drawing to update the physical screen.
 */
void oLed::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  Adafruit_SSD1306::drawPixel(x,y,color);
}

void oLed::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  Adafruit_GFX::drawLine(x0,y0,x1,y1,color);
}

void oLed::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  Adafruit_GFX::drawRect(x,y,w,h,color);
}

void oLed::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  Adafruit_GFX::fillRect(x,y,w,h,color);
}

void oLed::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  Adafruit_GFX::drawCircle(x0,y0,r,color);
}

void oLed::drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
  Adafruit_GFX::drawCircleHelper(x0, y0, r, cornername, color);
}

void oLed::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  Adafruit_GFX::fillCircle(x0,y0,r,color);
}

void oLed::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color)
{
  Adafruit_GFX::fillCircleHelper(x0,y0,r,cornername,delta,color);
}

void oLed::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  Adafruit_GFX::drawTriangle(x0,y0,x1,y1,x2,y2,color);
}

void oLed::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  Adafruit_GFX::fillTriangle(x0,y0,x1,y1,x2,y2,color);
}

void oLed::drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color)
{
  Adafruit_GFX::drawRoundRect(x0,y0,w,h,radius,color);
}

void oLed::fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color)
{
  Adafruit_GFX::fillRoundRect(x0,y0,w,h,radius,color);
}

void oLed::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color)
{
  Adafruit_GFX::drawBitmap(x,y,bitmap,w,h,color);
}

void oLed::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color, uint16_t bg)
{
  Adafruit_GFX::drawBitmap(x,y,bitmap,w,h,color,bg);
}

void oLed::drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color)
{
  Adafruit_GFX::drawBitmap(x,y,bitmap,w,h,color);
}

void oLed::drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg)
{
  Adafruit_GFX::drawBitmap(x,y,bitmap,w,h,color, bg);
}

void oLed::drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color)
{
  Adafruit_GFX::drawXBitmap(x,y,bitmap,w,h,color);
}

void oLed::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size)
{
  Adafruit_GFX::drawChar(x,y,c,color,bg,size);
}

void oLed::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y)
{
  Adafruit_GFX::drawChar(x,y,c,color,bg,size_x,size_y);
}

/*
 * Set the framebuffer text cursor in pixel coordinates.
 */
void oLed::setCursor(int16_t x, int16_t y)
{
  Adafruit_GFX::setCursor(x,y);
}

/*
 * Set text foreground color; the one-argument form uses a transparent background.
 */
void oLed::setTextColor(uint16_t c)
{
  Adafruit_GFX::setTextColor(c);
}

/*
 * Set text foreground and background colors.
 */
void oLed::setTextColor(uint16_t c, uint16_t bg)
{
  Adafruit_GFX::setTextColor(c,bg);
}

void oLed::clearDisplay(void)
{
  Adafruit_SSD1306::clearDisplay();
}
