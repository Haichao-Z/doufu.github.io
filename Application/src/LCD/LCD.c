#include <errno.h>

#include "CliThread/CliThread.h"
#include "FreeRTOS.h"
#include "I2cDriver\I2cDriver.h"
#include "SerialConsole.h"
#include "WifiHandlerThread/WifiHandler.h"
#include "asf.h"
#include "driver/include/m2m_wifi.h"
#include "main.h"
#include "stdio_serial.h"
#include "ASF\common2\services\delay\sam0\systick_counter.h"
#include "LCD/LCD.h"
#include "ASF\sam0\drivers\dma\dma.h"

void lcd_write_command(uint8_t cmd);
void lcd_write_data(uint8_t data);
void lcd_write_data16(uint16_t data);
void lcd_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void lcd_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);


// ST7735 Initialization
void lcd_init(void)
{
	SerialConsoleWriteString("Initializing LCD...\r\n");
	
	// Configure control pins
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LCD_CS_PIN, &pin_conf);
	port_pin_set_config(LCD_DC_PIN, &pin_conf);
	port_pin_set_config(LCD_RST_PIN, &pin_conf);
	port_pin_set_config(LCD_LITE_PIN, &pin_conf);
	
	// Set initial levels
	port_pin_set_output_level(LCD_CS_PIN, true);   // Default CS high
	port_pin_set_output_level(LCD_DC_PIN, false);  // Command mode
	port_pin_set_output_level(LCD_LITE_PIN, true); // Backlight on
	
	// Configure SPI port
	struct spi_config config_spi_master;
	struct spi_slave_inst_config slave_config;
	
	spi_get_config_defaults(&config_spi_master);

	config_spi_master.mux_setting = SPI_SIGNAL_MUX_SETTING_D; //C
	config_spi_master.pinmux_pad0 = PINMUX_PB02D_SERCOM5_PAD0;//PINMUX_PA04D_SERCOM0_PAD0; // MOSI
	config_spi_master.pinmux_pad1 = PINMUX_PB03D_SERCOM5_PAD1;//PINMUX_PA05D_SERCOM0_PAD1; // SCK
	config_spi_master.pinmux_pad2 = PINMUX_UNUSED;
	config_spi_master.pinmux_pad3 = PINMUX_UNUSED;
	config_spi_master.mode_specific.master.baudrate = 24000000; // 12MHz
	
	spi_init(&spi_master_instance, SERCOM5, &config_spi_master);
	spi_enable(&spi_master_instance);
	
	spi_slave_inst_get_config_defaults(&slave_config);
	slave_config.ss_pin = LCD_CS_PIN;
	spi_attach_slave(&lcd_slave, &slave_config);
	
	// Hardware reset LCD
	SerialConsoleWriteString("Resetting LCD hardware...\r\n");
	port_pin_set_output_level(LCD_RST_PIN, true);
	vTaskDelay(10);
	port_pin_set_output_level(LCD_RST_PIN, false);
	vTaskDelay(20);
	port_pin_set_output_level(LCD_RST_PIN, true);
	vTaskDelay(120);
	
	// Simple non-DMA write for initialization commands to ensure reliability
	SerialConsoleWriteString("Sending LCD initialization commands...\r\n");
	
	// LCD initialization sequence
	lcd_write_command(ST7735_SWRESET);  // Software reset
	vTaskDelay(150);
	
	lcd_write_command(ST7735_SLPOUT);   // Exit sleep mode
	vTaskDelay(120);
	
	lcd_write_command(ST7735_COLMOD);   // Set color format
	lcd_write_data(0x05);               // 16-bit color RGB565
	
	lcd_write_command(ST7735_MADCTL);   // Set display orientation
	lcd_write_data(0xA8);               // Row/Column addressing, refresh bottom to top
	
	lcd_write_command(ST7735_INVOFF);   // Disable display inversion
	
	lcd_write_command(ST7735_NORON);    // Enable display
	vTaskDelay(10);
	
	lcd_write_command(ST7735_DISPON);   // Turn on display
	vTaskDelay(100);
	
	// Clear screen to a visible color (red) to verify functionality
	SerialConsoleWriteString("Clearing screen...\r\n");

	SerialConsoleWriteString("LCD initialization complete\r\n");
}

// Write Command
void lcd_write_command(uint8_t cmd)
{
	port_pin_set_output_level(LCD_DC_PIN, false); // Order Mode
	spi_select_slave(&spi_master_instance, &lcd_slave, true);
	spi_write_buffer_wait(&spi_master_instance, &cmd, 1);
	spi_select_slave(&spi_master_instance, &lcd_slave, false);
}

// Write Data
void lcd_write_data(uint8_t data)
{
	port_pin_set_output_level(LCD_DC_PIN, true); // Data Mode
	spi_select_slave(&spi_master_instance, &lcd_slave, true);
	spi_write_buffer_wait(&spi_master_instance, &data, 1);
	spi_select_slave(&spi_master_instance, &lcd_slave, false);
}

// Write 16bit Data
void lcd_write_data16(uint16_t data)
{
	uint8_t data_array[2];
	data_array[0] = (data >> 8) & 0xFF; 
	data_array[1] = data & 0xFF;        
	
	port_pin_set_output_level(LCD_DC_PIN, true); 
	spi_select_slave(&spi_master_instance, &lcd_slave, true);
	spi_write_buffer_wait(&spi_master_instance, data_array, 2);
	spi_select_slave(&spi_master_instance, &lcd_slave, false);
}

// Set Address Window
void lcd_set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
	lcd_write_command(ST7735_CASET);  // column
	lcd_write_data(0x00);
	lcd_write_data(x0);
	lcd_write_data(0x00);
	lcd_write_data(x1);
	
	lcd_write_command(ST7735_RASET);  // row
	lcd_write_data(0x00);
	lcd_write_data(y0);
	lcd_write_data(0x00);
	lcd_write_data(y1);
	
	lcd_write_command(ST7735_RAMWR);  // Write into RAM
}

// Rectangular
void lcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
	
	lcd_set_addr_window(x, y, x+w-1, y+h-1);
	
	port_pin_set_output_level(LCD_DC_PIN, true); 
	spi_select_slave(&spi_master_instance, &lcd_slave, true);
	
	for (uint16_t i = 0; i < w * h; i++) {
		lcd_write_data16(color);
	}
	
	spi_select_slave(&spi_master_instance, &lcd_slave, false);
}

// Draw Circle
void lcd_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	lcd_fill_rect(x0-r, y0, 2*r+1, 1, color);
	
	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		
		lcd_fill_rect(x0-x, y0+y, 2*x+1, 1, color); 
		lcd_fill_rect(x0-x, y0-y, 2*x+1, 1, color); 
		lcd_fill_rect(x0-y, y0+x, 2*y+1, 1, color);
		lcd_fill_rect(x0-y, y0-x, 2*y+1, 1, color); 
	}
}

// LCD Display Task
void lcd_display_task()
{
	// Initial
	lcd_init();
	
	// Clear Screen to White
	lcd_fill_rect(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_WHITE);
	
	// Animation
	lcd_fill_circle(ST7735_WIDTH/2, ST7735_HEIGHT/4, 18, ST7735_BLACK);
	lcd_fill_circle(ST7735_WIDTH/2, 3*ST7735_HEIGHT/4, 18, ST7735_BLACK);
	lcd_fill_rect(ST7735_WIDTH/2, ST7735_HEIGHT/4, 4, ST7735_HEIGHT/2, ST7735_BLACK);
	
	while (1) {
		vTaskDelay(1000);
	}
}


volatile bool transfer_complete = false;

// DMA callback function
void dma_transfer_complete_callback(struct dma_resource *const resource)
{
	// Important: Set flag BEFORE releasing CS
	transfer_complete = true;
	
	// Add a small delay before releasing CS to ensure data is latched
	// This can be done with a small loop instead of a delay function
	for(volatile int i = 0; i < 10; i++);
	
	// Release the CS pin in interrupt - be careful with timing here
	spi_select_slave(&spi_master_instance, &lcd_slave, false);
}

DmacDescriptor dmac_descriptor_section[1] __attribute__((aligned(16)));
DmacDescriptor dmac_descriptor_writeback_section[1] __attribute__((aligned(16)));

// Setup DMA for SPI transfers
void setup_spi_dma(void)
{
	SerialConsoleWriteString("Setting up SPI DMA...\r\n");
	
	// DMA
	struct dma_resource_config config;
	dma_get_config_defaults(&config);
	
	// SPI
	config.peripheral_trigger = SERCOM5_DMAC_ID_TX;
	config.trigger_action = DMA_TRIGGER_ACTION_BEAT;
	
	// Allocate DMA resources
	enum status_code dma_status = dma_allocate(&spi_dma_resource, &config);
	if (dma_status != STATUS_OK) {
		SerialConsoleWriteString("Failed to allocate DMA resource!\r\n");
		return;
	}
	
	// Register and Enable Callback Function
	dma_register_callback(&spi_dma_resource, dma_transfer_complete_callback,
	DMA_CALLBACK_TRANSFER_DONE);
	dma_enable_callback(&spi_dma_resource, DMA_CALLBACK_TRANSFER_DONE);
	
	struct dma_descriptor_config descriptor_config;
	dma_descriptor_get_config_defaults(&descriptor_config);
	
	descriptor_config.beat_size = DMA_BEAT_SIZE_BYTE;
	descriptor_config.dst_increment_enable = false;
	descriptor_config.src_increment_enable = true;
	
	static DmacDescriptor descriptor;
	dma_descriptor_create(&descriptor, &descriptor_config);
	
	spi_dma_resource.descriptor = &descriptor;
	
	// After Transmission, it will be status true
	transfer_complete = true;
	
	SerialConsoleWriteString("DMA setup complete\r\n");
}

void lcd_write_data_dma(uint8_t* data, uint16_t length)
{
	if (!transfer_complete) {
		while (!transfer_complete) {
		}
	}
	
	port_pin_set_output_level(LCD_DC_PIN, true); 
	spi_select_slave(&spi_master_instance, &lcd_slave, true);
	
	transfer_complete = false;
	
	spi_dma_resource.descriptor->BTCNT.reg = length;
	spi_dma_resource.descriptor->SRCADDR.reg = (uint32_t)data + length;
	
	spi_dma_resource.descriptor->DSTADDR.reg = (uint32_t)&SERCOM5->SPI.DATA.reg;
	
	
	dma_start_transfer_job(&spi_dma_resource);
}

// Alternative version with wait for completion
void lcd_write_data_dma_wait(uint8_t* data, uint16_t length)
{
	lcd_write_data_dma(data, length);
	
	// Wait for transfer completion
	while (!transfer_complete) {
		// Could yield to other tasks here if using RTOS
		// vTaskDelay(1);
	}
}

void lcd_write_data16_dma(uint16_t color, uint16_t count)
{
	#define MAX_CHUNK 32  
	uint8_t buffer[MAX_CHUNK * 2];
	
	uint32_t remaining = count;
	
	while (remaining > 0) {
		uint32_t chunk_size = (remaining > MAX_CHUNK) ? MAX_CHUNK : remaining;
		
		for (uint32_t i = 0; i < chunk_size; i++) {
			buffer[i*2] = (color >> 8) & 0xFF;  
			buffer[i*2+1] = color & 0xFF;      
		}
		
		lcd_write_data_dma(buffer, chunk_size * 2);
		
		while (!transfer_complete) {
			vTaskDelay(1);
		}
		
		remaining -= chunk_size;
	}
}

void lcd_write_data16_dma_wait(uint16_t color, uint16_t count)
{
	lcd_write_data16_dma(color, count);
	
	while (!transfer_complete) {
		vTaskDelay(1);
	}
}


// Fill a rectangle with color using DMA
void lcd_fill_rect_dma(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
	// Calculate total pixels
	uint32_t pixel_count = w * h;
	
	// Set address window
	lcd_set_addr_window(x, y, x+w-1, y+h-1);
	
	// For small areas, use simpler approach
	if (pixel_count <= 64) {
		// Set data mode
		port_pin_set_output_level(LCD_DC_PIN, true);
		spi_select_slave(&spi_master_instance, &lcd_slave, true);
		
		// Prepare color bytes
		uint8_t color_high = (color >> 8) & 0xFF;
		uint8_t color_low = color & 0xFF;
		
		// Send pixels one by one
		for (uint32_t i = 0; i < pixel_count; i++) {
			spi_write_buffer_wait(&spi_master_instance, &color_high, 1);
			spi_write_buffer_wait(&spi_master_instance, &color_low, 1);
		}
		
		spi_select_slave(&spi_master_instance, &lcd_slave, false);
		return;
	}
	
	// For larger areas, use DMA in chunks
	#define BUFFER_SIZE 32  // Smaller buffer for reliability
	uint8_t buffer[BUFFER_SIZE * 2];
	
	// Fill buffer with color values
	for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
		buffer[i*2] = (color >> 8) & 0xFF;    // High byte
		buffer[i*2+1] = color & 0xFF;         // Low byte
	}
	
	// Send data in chunks
	uint32_t remaining = pixel_count;
	
	// Set data mode once
	port_pin_set_output_level(LCD_DC_PIN, true);
	
	while (remaining > 0) {
		uint32_t chunk = (remaining > BUFFER_SIZE) ? BUFFER_SIZE : remaining;
		
		// Make sure previous transfer is complete
		while (!transfer_complete);
		
		// Start new transfer
		lcd_write_data_dma(buffer, chunk * 2);
		
		// Wait for transfer completion when we're on the last chunk
		if (remaining <= BUFFER_SIZE) {
			while (!transfer_complete);
		}
		
		remaining -= chunk;
	}
}