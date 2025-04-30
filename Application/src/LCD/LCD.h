//-----------------------------------------------------------------------------------
// LCD Define
//-----------------------------------------------------------------------------------
#define LCD_CS_PIN      PIN_PA11
#define LCD_MOSI_PIN    PIN_PB02//PIN_PA04
#define LCD_SCK_PIN     PIN_PB03//PIN_PA05
#define LCD_DC_PIN      PIN_PA25
#define LCD_RST_PIN     PIN_PA24
#define LCD_LITE_PIN    PIN_PB22

/**
  * @name SPI Interface
  * @{
  */
 /** SPI master module instance */
 extern struct spi_module spi_master_instance;
 /** SPI slave device instance */
 extern struct spi_slave_inst lcd_slave;
 /** @} */
 
 /**
  * @name ST7735 Command Set
  * @{
  */
 #define ST7735_NOP     0x00    /**< No operation */
 #define ST7735_SWRESET 0x01    /**< Software reset */
 #define ST7735_SLPOUT  0x11    /**< Sleep out */
 #define ST7735_NORON   0x13    /**< Normal display mode on */
 #define ST7735_INVOFF  0x20    /**< Display inversion off */
 #define ST7735_DISPON  0x29    /**< Display on */
 #define ST7735_CASET   0x2A    /**< Column address set */
 #define ST7735_RASET   0x2B    /**< Row address set */
 #define ST7735_RAMWR   0x2C    /**< Memory write */
 #define ST7735_MADCTL  0x36    /**< Memory access control */
 #define ST7735_COLMOD  0x3A    /**< Interface pixel format */
 /** @} */
 
 /**
  * @name Basic Color Definitions (RGB565 format)
  * @{
  */
 #define ST7735_BLACK   0x0000    /**< Black color */
 #define ST7735_WHITE   0xFFFF    /**< White color */
 #define ST7735_BLUE    0xF800    /**< Blue color */
 #define ST7735_GREEN   0x07E0    /**< Green color */
 #define ST7735_RED     0x001F    /**< Red color */
 #define ST7735_CYAN    0x07FF    /**< Cyan color (Green + Blue) */
 #define ST7735_MAGENTA 0xF81F    /**< Magenta color (Red + Blue) */
 #define ST7735_YELLOW  0xFFE0    /**< Yellow color (Red + Green) */
 /** @} */
 
 /**
  * @name Extended Color Set - Red-Green Combinations
  * @{
  */
 #define ST7735_FIRE_ENGINE_RED  0xD000    /**< Fire engine red */
 #define ST7735_LIME_GREEN       0x2FE0    /**< Lime green */
 /** @} */
 
 /**
  * @name Extended Color Set - Blue-Orange Combinations
  * @{
  */
 #define ST7735_NAVY_BLUE        0x000F    /**< Navy blue */
 #define ST7735_PUMPKIN_ORANGE   0xFC08    /**< Pumpkin orange */
 /** @} */
 
 /**
  * @name Extended Color Set - Purple-Yellow Combinations
  * @{
  */
 #define ST7735_ROYAL_PURPLE     0x8010    /**< Royal purple */
 #define ST7735_GOLDEN_YELLOW    0xFE60    /**< Golden yellow */
 /** @} */
 
 /**
  * @name Additional Vibrant Colors
  * @{
  */
 #define ST7735_VIVID_RED        0xF800    /**< Vivid red */
 #define ST7735_ELECTRIC_BLUE    0x001F    /**< Electric blue */
 #define ST7735_NEON_GREEN       0x07E0    /**< Neon green */
 #define ST7735_PURPLE           0xF81F    /**< Purple */
 #define ST7735_SUNFLOWER_YELLOW 0xFFE0    /**< Sunflower yellow */
 #define ST7735_ORANGE           0xFD20    /**< Orange */
 /** @} */
 
 /**
  * @name Display Dimensions
  * @{
  */
 #define ST7735_WIDTH  160    /**< Display width in pixels */
 #define ST7735_HEIGHT 128    /**< Display height in pixels */
 /** @} */

void lcd_init(void);
void lcd_write_command(uint8_t cmd);
void lcd_write_data(uint8_t data);
void lcd_write_data16(uint16_t data);
void lcd_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void lcd_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void lcd_display_task();


// Global variables for DMA
struct dma_resource spi_dma_resource;
struct dma_descriptor_config dma_config;
DmacDescriptor dmac_descriptor_section[1] __attribute__((aligned(16)));
DmacDescriptor dmac_descriptor_writeback_section[1] __attribute__((aligned(16)));

void dma_transfer_complete_callback(struct dma_resource *const resource);

// Setup DMA for SPI transfers
void setup_spi_dma(void);

// Write data to LCD using DMA
void lcd_write_data_dma(uint8_t* data, uint16_t length);

// Alternative version with wait for completion
void lcd_write_data_dma_wait(uint8_t* data, uint16_t length);

// Fill a rectangle with color using DMA
void lcd_fill_rect_dma(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

void lcd_write_data16_dma(uint16_t color, uint16_t count);