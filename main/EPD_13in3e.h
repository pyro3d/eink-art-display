/*****************************************************************************
* | File      	:	EPD_12in48.h
* | Author      :   Waveshare team
* | Function    :   Electronic paper driver
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2018-11-29
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#ifndef _EPD_13IN3E_H_
#define _EPD_13IN3E_H_

#include <stdint.h>
#include <string.h>
#include <cJSON.h>

#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t
/**
 * GPIO config
**/
#define EPD_MOSI_PIN    1
#define EPD_SCK_PIN     2
#define EPD_CS_M_PIN    3
#define EPD_CS_S_PIN    4
#define EPD_DC_PIN      5
#define EPD_RST_PIN     6
#define EPD_BUSY_PIN    7
#define EPD_PWR_PIN     8

// M/S 控制区域 600*1600
#define EPD_13IN3E_WIDTH        1200
#define EPD_13IN3E_HEIGHT       1600    


#define EPD_13IN3E_BLACK        0x0
#define EPD_13IN3E_WHITE        0x1
#define EPD_13IN3E_YELLOW       0x2
#define EPD_13IN3E_RED          0x3
#define EPD_13IN3E_BLUE         0x5
#define EPD_13IN3E_GREEN        0x6

#define PSR             0x00
#define PWR_epd         0x01
#define POF             0x02
#define PON             0x04
#define BTST_N          0x05
#define BTST_P          0x06
#define DTM             0x10
#define DRF             0x12
#define CDI             0x50
#define TCON            0x60
#define TRES            0x61
#define AN_TM           0x74
#define AGID            0x86
#define BUCK_BOOST_VDDN 0xB0
#define TFT_VCOM_POWER  0xB1
#define EN_BUF          0xB6
#define BOOST_VDDP_EN   0xB7
#define CCSET           0xE0
#define PWS             0xE3
#define CMD66           0xF0




void EPD_13IN3E_Init(void);
void EPD_13IN3E_Clear(UBYTE color);
void EPD_13IN3E_Display(const UBYTE *Image);
void EPD_13IN3E_Display_Buffers();
void EPD_13IN3E_Display2(const UBYTE *Image1, const UBYTE *Image2);
void copy_cjson_array(cJSON *array, int s_select);
void EPD_13IN3E_DisplayPart(const UBYTE *Image, UWORD xstart, UWORD ystart, UWORD image_width, UWORD image_heigh);
void EPD_13IN3E_Show6Block(void);
void EPD_13IN3E_Sleep(void);
void copy_to_s(char * src);
void power_on();
void power_off();
void init_spi();

#endif

