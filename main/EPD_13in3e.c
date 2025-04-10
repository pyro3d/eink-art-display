/*****************************************************************************
* | File        :   EPD_12in48.c
* | Author      :   Waveshare team
* | Function    :   Electronic paper driver
* | Info     :
*----------------
* | This version:   V1.0
* | Date     :   2018-11-29
* | Info     :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files(the "Software"), to deal
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
#include "EPD_13in3e.h"

#include "driver/spi_master.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_cache.h"

static char *TAG = "EPD";
static spi_device_handle_t spi;

EXT_RAM_BSS_ATTR static UBYTE m_buf[EPD_13IN3E_HEIGHT * EPD_13IN3E_WIDTH / 4  ] = { 0 };
EXT_RAM_BSS_ATTR static UBYTE s_buf[EPD_13IN3E_HEIGHT * EPD_13IN3E_WIDTH / 4  ] = { 0 };
void copy_cjson_array(cJSON *array, int s_select) {
    cJSON *item = array ? array->child : 0;
    int i = 0;
    char * arr = cJSON_Print(item);
    free(arr);
    for (i = 0; item && i < EPD_13IN3E_HEIGHT * EPD_13IN3E_WIDTH / 4 ; item=item->next, i++) {
        if (s_select) { 
            s_buf[i] = item->valueint;
        }
        else {
            m_buf[i] = item->valueint;
        }
    }
    esp_cache_msync(&m_buf, EPD_13IN3E_HEIGHT * EPD_13IN3E_WIDTH /4, ESP_CACHE_MSYNC_FLAG_DIR_C2M| ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    esp_cache_msync(&s_buf, EPD_13IN3E_HEIGHT * EPD_13IN3E_WIDTH /4, ESP_CACHE_MSYNC_FLAG_DIR_C2M  | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
}
void copy_to_s(char* src) {
    memcpy(m_buf, src, EPD_13IN3E_HEIGHT * EPD_13IN3E_WIDTH / 4  );
    memcpy(s_buf, src+EPD_13IN3E_HEIGHT * EPD_13IN3E_WIDTH / 4, EPD_13IN3E_HEIGHT * EPD_13IN3E_WIDTH / 4  );
}
void DEV_Delay_ms(int ms) {
    vTaskDelay(ms/ portTICK_PERIOD_MS);
}

void Debug(char * text) {
    ESP_LOGI(TAG, "%s", text);
}
int DEV_Digital_Read(int pin){
    return  gpio_get_level(pin);
}
void DEV_SPI_WriteByte(UBYTE Data) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); 
    t.length = 8;    
    t.tx_buffer = &Data;
    t.user = (void*)1;
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

void epaper_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(EPD_DC_PIN, 0);
}
void init_spi() {
 spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = EPD_MOSI_PIN,
        .sclk_io_num = EPD_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };
    spi_device_interface_config_t devcfg = {
         .clock_speed_hz = 10 * 1000 * 1000, 
          .mode = 0,                              //SPI mode 0
         .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
        .pre_cb = epaper_spi_pre_transfer_callback
    };
    gpio_set_direction(EPD_DC_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_CS_M_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_CS_S_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_PWR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(EPD_BUSY_PIN, GPIO_MODE_INPUT);

    gpio_set_level(EPD_CS_M_PIN, 0);
    gpio_set_level(EPD_CS_S_PIN, 0);
//    gpio_set_level(EPD_PWR_PIN, 1);
    
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));
}

void power_on() {
    gpio_set_level(EPD_PWR_PIN, 1);
}
void power_off() {
    gpio_set_level(EPD_PWR_PIN, 0);
}
void DEV_SPI_Write_nByte(UBYTE * buf, int len) {
    
//   for (int i = 0; i < len; i++)  {
        spi_transaction_t t;
        memset(&t, 0, sizeof(t)); 
        t.length = 8*len;    
        t.tx_buffer = buf;
        t.user = (void*)1;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
    
 //  }
}
void DEV_Digital_Write(int pin, int level)
{
    ESP_ERROR_CHECK(gpio_set_level(pin,level));
}
// const UBYTE spiCsPin[2] = {
// 		SPI_CS0, SPI_CS1
// };
const UBYTE PSR_V[2] = {
	0xDF, 0x69
};
const UBYTE PWR_V[6] = {
	0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38
};
const UBYTE POF_V[1] = {
	0x00
};
const UBYTE DRF_V[1] = {
	0x00
};
const UBYTE CDI_V[1] = {
	0xF7
};
const UBYTE TCON_V[2] = {
	0x03, 0x03
};
const UBYTE TRES_V[4] = {
	0x04, 0xB0, 0x03, 0x20
};
const UBYTE CMD66_V[6] = {
	0x49, 0x55, 0x13, 0x5D, 0x05, 0x10
};
const UBYTE EN_BUF_V[1] = {
	0x07
};
const UBYTE CCSET_V[1] = {
	0x01
};
const UBYTE PWS_V[1] = {
	0x22
};
const UBYTE AN_TM_V[9] = {
	0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55
};


const UBYTE AGID_V[1] = {
	0x10
};

const UBYTE BTST_P_V[2] = {
	0xE8, 0x28
};
const UBYTE BOOST_VDDP_EN_V[1] = {
	0x01
};
const UBYTE BTST_N_V[2] = {
	0xE8, 0x28
};
const UBYTE BUCK_BOOST_VDDN_V[1] = {
	0x01
};
const UBYTE TFT_VCOM_POWER_V[1] = {
	0x02
};


static void EPD_13IN3E_CS_ALL(UBYTE Value)
{
    DEV_Digital_Write(EPD_CS_M_PIN, Value);
    DEV_Digital_Write(EPD_CS_S_PIN, Value);
}


static void EPD_13IN3E_SPI_Sand(UBYTE Cmd, const UBYTE *buf, UDOUBLE Len)
{
    DEV_SPI_WriteByte(Cmd);
    DEV_SPI_Write_nByte((UBYTE *)buf,Len);
}


/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_13IN3E_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(30);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(30);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/

static void EPD_13IN3E_SendCommand(UBYTE Reg)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); 
    t.length = 8;    
    t.tx_buffer = &Reg;
    t.user = (void*)0;

   ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_13IN3E_SendData(UBYTE Reg)
{
    DEV_SPI_WriteByte(Reg);
}
static void EPD_13IN3E_SendData2(const UBYTE *buf, uint32_t Len)
{
    DEV_SPI_Write_nByte((UBYTE *)buf,Len);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static void EPD_13IN3E_ReadBusyH(void)
{
    Debug("e-Paper busy\r\n");
	while(!DEV_Digital_Read(EPD_BUSY_PIN)) {      //LOW: busy, HIGH: idle
        DEV_Delay_ms(100);
        // Debug("e-Paper busy release\r\n");
    }
	DEV_Delay_ms(200);
    Debug("e-Paper busy release\r\n");
}


/******************************************************************************
function :  Turn On Display
parameter:
******************************************************************************/
static void EPD_13IN3E_TurnOnDisplay(void)
{
    ESP_LOGI(TAG,"Write PON \r\n");
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SendCommand(0x04); // POWER_ON
    EPD_13IN3E_CS_ALL(1);
    EPD_13IN3E_ReadBusyH();

    ESP_LOGI(TAG,"Write DRF \r\n");
    DEV_Delay_ms(50);
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(DRF, DRF_V, sizeof(DRF_V));
    EPD_13IN3E_CS_ALL(1);
    EPD_13IN3E_ReadBusyH();

    ESP_LOGI(TAG,"Write POF \r\n");
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SPI_Sand(POF, POF_V, sizeof(POF_V));
    EPD_13IN3E_CS_ALL(1);
    // EPD_13IN3E_ReadBusyH();
    ESP_LOGI(TAG,"Display Done!! \r\n");
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_13IN3E_Init(void)
{
	EPD_13IN3E_Reset();
//    EPD_13IN3E_ReadBusyH();

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(AN_TM, AN_TM_V, sizeof(AN_TM_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(CMD66, CMD66_V, sizeof(CMD66_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(PSR, PSR_V, sizeof(PSR_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(CDI, CDI_V, sizeof(CDI_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(TCON, TCON_V, sizeof(TCON_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(AGID, AGID_V, sizeof(AGID_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(PWS, PWS_V, sizeof(PWS_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(CCSET, CCSET_V, sizeof(CCSET_V));
    EPD_13IN3E_CS_ALL(1);

    EPD_13IN3E_CS_ALL(0);
	EPD_13IN3E_SPI_Sand(TRES, TRES_V, sizeof(TRES_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(PWR_epd, PWR_V, sizeof(PWR_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(EN_BUF, EN_BUF_V, sizeof(EN_BUF_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(BTST_P, BTST_P_V, sizeof(BTST_P_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(BOOST_VDDP_EN, BOOST_VDDP_EN_V, sizeof(BOOST_VDDP_EN_V));
    EPD_13IN3E_CS_ALL(1);
	
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(BTST_N, BTST_N_V, sizeof(BTST_N_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(BUCK_BOOST_VDDN, BUCK_BOOST_VDDN_V, sizeof(BUCK_BOOST_VDDN_V));
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_M_PIN, 0);
	EPD_13IN3E_SPI_Sand(TFT_VCOM_POWER, TFT_VCOM_POWER_V, sizeof(TFT_VCOM_POWER_V));
    EPD_13IN3E_CS_ALL(1);
    
}

/******************************************************************************
function :  Clear screen
parameter:
******************************************************************************/
void EPD_13IN3E_Clear(UBYTE color)
{
    UDOUBLE Width, Height;
    UBYTE Color;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    Color = (color<<4)|color;
    
    UBYTE buf[Width/2];
    
    for (UDOUBLE j = 0; j < Width/2; j++) {
        buf[j] = Color;
    }
    
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for (UDOUBLE j = 0; j < EPD_13IN3E_HEIGHT; j++) {
        EPD_13IN3E_SendData2(buf, Width/2);
        DEV_Delay_ms(10);
    }
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_S_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for (UDOUBLE j = 0; j < EPD_13IN3E_HEIGHT; j++) {
        EPD_13IN3E_SendData2(buf, Width/2);
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);
    
    EPD_13IN3E_TurnOnDisplay();
}


void EPD_13IN3E_Display(const UBYTE *Image)
{
    UDOUBLE Width, Width1, Height;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Width1 = (Width % 2 == 0)? (Width / 2 ): (Width / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for(UDOUBLE i=0; i<Height; i++ )
    {
        EPD_13IN3E_SendData2(Image + i*Width,Width1);
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_S_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for(UDOUBLE i=0; i<Height; i++ )
    {
        EPD_13IN3E_SendData2(Image + i*Width + Width1,Width1);
        DEV_Delay_ms(1);
    }
       
    EPD_13IN3E_CS_ALL(1);
    
    EPD_13IN3E_TurnOnDisplay();
}

void EPD_13IN3E_Display2(const UBYTE *Image1, const UBYTE *Image2)
{
    UDOUBLE Width, Width1, Height;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Width1 = (Width % 2 == 0)? (Width / 2 ): (Width / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for(UDOUBLE i=0; i<Height; i++ )
    {
        EPD_13IN3E_SendData2(Image1 + i*Width1,Width1);
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_S_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for(UDOUBLE i=0; i<Height; i++ )
    {
        EPD_13IN3E_SendData2(Image2 + i*Width1, Width1);
        DEV_Delay_ms(1);
    }
       
    EPD_13IN3E_CS_ALL(1);
    
    EPD_13IN3E_TurnOnDisplay();
}
void EPD_13IN3E_Display_Buffers()
{
    UDOUBLE Width, Width1, Height;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Width1 = (Width % 2 == 0)? (Width / 2 ): (Width / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    ESP_LOGW("epd", "First element of s_buf is %d", s_buf[0]) ;
    ESP_LOGW("epd", "Second element of s_buf is %d", s_buf[1]) ;
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    int Color = (6<<4)|6;
    
    UBYTE buf[Width/2];
    
    for (UDOUBLE j = 0; j < Width/2; j++) {
        buf[j] = Color;
    }
    DEV_Delay_ms(10);
    for(UDOUBLE i=0; i<Height; i++ )
    {
        EPD_13IN3E_SendData2(m_buf + i*Width1, Width1);
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);

    DEV_Digital_Write(EPD_CS_S_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for(UDOUBLE i=0; i<Height; i++ )
    {
        EPD_13IN3E_SendData2(s_buf + i*Width1, Width1);
        DEV_Delay_ms(1);
    }
       
    EPD_13IN3E_CS_ALL(1);
    
    EPD_13IN3E_TurnOnDisplay();
}


void EPD_13IN3E_DisplayPart(const UBYTE *Image, UWORD xstart, UWORD ystart, UWORD image_width, UWORD image_heigh)
{
    UDOUBLE Width, Width1, Height;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Width1 = (Width % 2 == 0)? (Width / 2 ): (Width / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    
    UWORD Xend = ((xstart + image_width)%2 == 0)?((xstart + image_width) / 2 - 1): ((xstart + image_width) / 2 );
    UWORD Yend = ystart + image_heigh-1;
    xstart = xstart / 2;
    
    if(xstart > 300 )
    {
        Xend = Xend - 300;
        xstart = xstart - 300;
        DEV_Digital_Write(EPD_CS_M_PIN, 0);
        EPD_13IN3E_SendCommand(0x10);
        for (UDOUBLE i = 0; i < Height; i++) {
            for (UDOUBLE j = 0; j < Width1; j++) {
                EPD_13IN3E_SendData(0x11);
            }
            DEV_Delay_ms(1);
        }
        EPD_13IN3E_CS_ALL(1);
        
        
        DEV_Digital_Write(EPD_CS_S_PIN, 0);
        EPD_13IN3E_SendCommand(0x10);
        for (UDOUBLE i = 0; i < Height; i++) {
            for (UDOUBLE j = 0; j < Width1; j++) {
                if((i<Yend) && (i>=ystart) && (j<Xend) && (j>=xstart)) {
                    EPD_13IN3E_SendData(Image[(j-xstart) + (image_width/2*(i-ystart))]);
                }
                else
                    EPD_13IN3E_SendData(0x11);
            }
            DEV_Delay_ms(1);
        }
        EPD_13IN3E_CS_ALL(1);
    }
    else if(Xend < 300 )
    {
        DEV_Digital_Write(EPD_CS_M_PIN, 0);
        EPD_13IN3E_SendCommand(0x10);
        for (UDOUBLE i = 0; i < Height; i++) {
            for (UDOUBLE j = 0; j < Width1; j++) {
                if((i<Yend) && (i>=ystart) && (j<Xend) && (j>=xstart)) {
                    EPD_13IN3E_SendData(Image[(j-xstart) + (image_width/2*(i-ystart))]);
                }
                else
                    EPD_13IN3E_SendData(0x11);
            }
            DEV_Delay_ms(1);
        }
        EPD_13IN3E_CS_ALL(1);
        
        
        DEV_Digital_Write(EPD_CS_S_PIN, 0);
        EPD_13IN3E_SendCommand(0x10);
        for (UDOUBLE i = 0; i < Height; i++) {
            for (UDOUBLE j = 0; j < Width1; j++) {
                EPD_13IN3E_SendData(0x11);
            }
            DEV_Delay_ms(1);
        }
        EPD_13IN3E_CS_ALL(1);
    }
    else
    {
        DEV_Digital_Write(EPD_CS_M_PIN, 0);
        EPD_13IN3E_SendCommand(0x10);
        for (UDOUBLE i = 0; i < Height; i++) {
            for (UDOUBLE j = 0; j < Width1; j++) {
                if((i<Yend) && (i>=ystart) && (j>=xstart)) {
                    EPD_13IN3E_SendData(Image[(j-xstart) + (image_width/2*(i-ystart))]);
                }
                else
                    EPD_13IN3E_SendData(0x11);
            }
            DEV_Delay_ms(1);
        }
        EPD_13IN3E_CS_ALL(1);
        
        
        DEV_Digital_Write(EPD_CS_S_PIN, 0);
        EPD_13IN3E_SendCommand(0x10);
        for (UDOUBLE i = 0; i < Height; i++) {
            for (UDOUBLE j = 0; j < Width1; j++) {
                if((i<Yend) && (i>=ystart) && (j<Xend-300)) {
                    EPD_13IN3E_SendData(Image[(j+300-xstart) + (image_width/2*(i-ystart))]);
                }
                else
                    EPD_13IN3E_SendData(0x11);
            }
            DEV_Delay_ms(1);
        }
        EPD_13IN3E_CS_ALL(1);
    }

    EPD_13IN3E_TurnOnDisplay();
}



void EPD_13IN3E_Show6Block(void)
{
    unsigned long i, j, k;
    UWORD Width, Height;
    Width = (EPD_13IN3E_WIDTH % 2 == 0)? (EPD_13IN3E_WIDTH / 2 ): (EPD_13IN3E_WIDTH / 2 + 1);
    Height = EPD_13IN3E_HEIGHT;
    unsigned char const Color_seven[6] = 
    {EPD_13IN3E_BLACK, EPD_13IN3E_BLUE, EPD_13IN3E_GREEN,
    EPD_13IN3E_RED, EPD_13IN3E_YELLOW, EPD_13IN3E_WHITE};
    
    DEV_Digital_Write(EPD_CS_M_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for (k = 0; k < 6; k++) {
        for (j = 0; j < Height/6; j++) {
            for (i = 0; i < Width/2; i++) {
                EPD_13IN3E_SendData(Color_seven[k]|(Color_seven[k]<<4));
            }
        }
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);
    
    DEV_Digital_Write(EPD_CS_S_PIN, 0);
    EPD_13IN3E_SendCommand(0x10);
    for (k = 0; k < 6; k++) {
        for (j = 0; j < Height/6; j++) {
            for (i = 0; i < Width/2; i++) {
                EPD_13IN3E_SendData(Color_seven[k]|(Color_seven[k]<<4));
            }
        }
        DEV_Delay_ms(1);
    }
    EPD_13IN3E_CS_ALL(1);
    
    EPD_13IN3E_TurnOnDisplay();
}


/******************************************************************************
function :  Enter sleep mode
parameter:
******************************************************************************/
void EPD_13IN3E_Sleep(void)
{
    EPD_13IN3E_CS_ALL(0);
    EPD_13IN3E_SendCommand(0x07); // DEEP_SLEEP
    EPD_13IN3E_SendData(0XA5);
    EPD_13IN3E_CS_ALL(1);
}





