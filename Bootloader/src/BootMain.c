/**
 * @file      BootMain.c
 * @brief     Main file for the ESE5160 bootloader. Handles updating the main application
 * @details   Main file for the ESE5160 bootloader. Handles updating the main application
 * @author    Eduardo Garcia
 * @author    Nick M-G
 * @date      2024-03-03
 * @version   2.0
 * @copyright Copyright University of Pennsylvania
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "conf_example.h"
#include "sd_mmc_spi.h"
#include <asf.h>
#include <string.h>

#include "ASF/sam0/drivers/dsu/crc32/crc32.h"
#include "SD Card/SdCard.h"
#include "SerialConsole/SerialConsole.h"
#include "Systick/Systick.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define APP_START_ADDRESS           ((uint32_t) 0x12000)                    ///< Start of main application. Must be address of start of main application
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS + (uint32_t) 0x04)   ///< Main application reset vector address

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/

struct usart_module cdc_uart_module;   ///< Structure for UART module connected to EDBG (used for unit test output)

/******************************************************************************
 * Local Function Declaration
 ******************************************************************************/
static void jumpToApplication(void);
static bool StartFilesystemAndTest(void);
static void configure_nvm(void);
static void BootloaderUpdate(void);

static bool Firmware_Check(char * filename);
static bool verifyFirmwareCRC32(uint32_t addr, uint32_t len, uint32_t expected_crc);

/******************************************************************************
 * Global Variables
 ******************************************************************************/
// INITIALIZE VARIABLES
char test_file_name[] = "0:sd_mmc_test.txt";   ///< Test TEXT File name
char test_bin_file[] = "0:sd_binary.bin";      ///< Test BINARY File name
char flag_file_name[] = "0:Flag.txt";		      ///< Firmware TEXT(Flag) File name
char firmware_bin_file[] = "0:Application.bin";   ///< Firmware BINARY file name
char gold_bin_file[] = "0:g_application.bin";   ///< Firmware BINARY file name

bool Update_Flag = false;


Ctrl_status status;                            ///< Holds the status of a system initialization
FRESULT res;                                   // Holds the result of the FATFS functions done on the SD CARD TEST
FATFS fs;                                      // Holds the File System of the SD CARD
FIL file_object;                               // FILE OBJECT used on main for the SD Card Test

/******************************************************************************
 * Global Functions
 ******************************************************************************/

/**
* @fn		int main(void)
* @brief	Main function for ESE5160 Bootloader Application

* @return	Unused (ANSI-C compatibility).
* @note		Bootloader code initiates here.
*****************************************************************************/

int main(void) {

    /*1.) INIT SYSTEM PERIPHERALS INITIALIZATION*/
    system_init();
    delay_init();
    InitializeSerialConsole();
    system_interrupt_enable_global();

    /* Initialize SD MMC stack */
    sd_mmc_init();

    // Initialize the NVM driver
    configure_nvm();

    irq_initialize_vectors();
    cpu_irq_enable();

    // Configure CRC32
    dsu_crc32_init();

    SerialConsoleWriteString("ESE5160 - ENTER BOOTLOADER");   // Order to add string to TX Buffer

    /*END SYSTEM PERIPHERALS INITIALIZATION*/

    /*2.) STARTS SIMPLE SD CARD MOUNTING AND TEST!*/

    // EXAMPLE CODE ON MOUNTING THE SD CARD AND WRITING TO A FILE
    // See function inside to see how to open a file
    SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");

    if (StartFilesystemAndTest() == false) {
        SerialConsoleWriteString("SD CARD failed! Check your connections. System will restart in 5 seconds...");
        delay_cycles_ms(5000);
        system_reset();
    } else {
        SerialConsoleWriteString("SD CARD mount success! Filesystem also mounted. \r\n");
    }

    /*END SIMPLE SD CARD MOUNTING AND TEST!*/

    /*3.) STARTS BOOTLOADER HERE!*/

    // Students - this is your mission!
	//BootloaderUpdate(); //A08G
	
	//A10G OTAFU
	flag_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	firmware_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';

	res = f_open(&file_object, flag_file_name, FA_READ);
	if (res == FR_OK) {
		SerialConsoleWriteString("Flag.txt detected, proceeding with firmware update...\r\n");
		Update_Flag = true;
		} else {
		SerialConsoleWriteString("Flag.txt is not detected...\r\n");
		LogMessage(LOG_INFO_LVL, "[INFO] Flag.txt not found. Use current flash.\r\n");
	}
	
	//A10G Golden Image		
	//// Check the CRC
	if (!Firmware_Check(firmware_bin_file)) 
	{ // Failed in CRC check, use golden image
		SerialConsoleWriteString("\n\rCRC mismatch. Overwriting with golden image........\r\n\n");
				
		gold_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
		Update_Flag = true;
		//overwrite application with golden image
		if(!Firmware_Check(gold_bin_file))
		{ //Golden image failed form CRC check again, need download firmware from internet
			SerialConsoleWriteString("\n\rGolden image failed! Use command 'fw' to get latest firmware.\r\n\n");
		}
		else
		{
			SerialConsoleWriteString("Replaced Application.bin with Golden image.\r\n");
		}
	} 
	else 
	{ // Success from CRC check, move to application
		SerialConsoleWriteString("\r\nNo issue found in firmware.\r\n");
		f_unlink(flag_file_name); // delete the flag to avoid re-upload
	}

    /* END BOOTLOADER HERE!*/

    // 4.) DEINITIALIZE HW AND JUMP TO MAIN APPLICATION!
    SerialConsoleWriteString("ESE5160 - EXIT BOOTLOADER");   // Order to add string to TX Buffer
    delay_cycles_ms(100);                                    // Delay to allow print

    // Deinitialize HW - deinitialize started HW here!
    DeinitializeSerialConsole();   // Deinitializes UART
    sd_mmc_deinit();               // Deinitialize SD CARD

    // Jump to application
    jumpToApplication();
	

    // Should not reach here! The device should have jumped to the main FW.
}

/******************************************************************************
 * Static Functions
 ******************************************************************************/

/**
 * function      static void StartFilesystemAndTest()
 * @brief        Starts the filesystem and tests it. Sets the filesystem to the global variable fs
 * @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
 *				before performing the jump!
 * @return       Returns true is SD card and file system test passed. False otherwise.
 ******************************************************************************/
static bool StartFilesystemAndTest(void) {
    bool sdCardPass = true;
    uint8_t binbuff[256];

    // Before we begin - fill buffer for binary write test
    // Fill binbuff with values 0x00 - 0xFF
    for (int i = 0; i < 256; i++) {
        binbuff[i] = i;
    }

    // MOUNT SD CARD
    Ctrl_status sdStatus = SdCard_Initiate();
    if (sdStatus == CTRL_GOOD)   // If the SD card is good we continue mounting the system!
    {
        SerialConsoleWriteString("SD Card initiated correctly!\n\r");

        // Attempt to mount a FAT file system on the SD Card using FATFS
        SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
        memset(&fs, 0, sizeof(FATFS));
        res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);   // Order FATFS Mount
        if (FR_INVALID_DRIVE == res) {
            LogMessage(LOG_INFO_LVL, "[FAIL] res %d\r\n", res);
            sdCardPass = false;
            goto main_end_of_test;
        }
        SerialConsoleWriteString("[OK]\r\n");

        // Create and open a file
        SerialConsoleWriteString("Create a file (f_open)...\r\n");

        test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
        res = f_open(&file_object, (char const *) test_file_name, FA_CREATE_ALWAYS | FA_WRITE);

        if (res != FR_OK) {
            LogMessage(LOG_INFO_LVL, "[FAIL] res %d\r\n", res);
            sdCardPass = false;
            goto main_end_of_test;
        }

        SerialConsoleWriteString("[OK]\r\n");

        // Write to a file
        SerialConsoleWriteString("Write to test file (f_puts)...\r\n");

        if (0 == f_puts("Test SD/MMC stack\n", &file_object)) {
            f_close(&file_object);
            LogMessage(LOG_INFO_LVL, "[FAIL]\r\n");
            sdCardPass = false;
            goto main_end_of_test;
        }

        SerialConsoleWriteString("[OK]\r\n");
        f_close(&file_object);   // Close file
        SerialConsoleWriteString("Test is successful.\n\r");

        // Write binary file
        // Read SD Card File
        test_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
        res = f_open(&file_object, (char const *) test_bin_file, FA_WRITE | FA_CREATE_ALWAYS);

        if (res != FR_OK) {
            SerialConsoleWriteString("Could not open binary file!\r\n");
            LogMessage(LOG_INFO_LVL, "[FAIL] res %d\r\n", res);
            sdCardPass = false;
            goto main_end_of_test;
        }

        // Write to a binaryfile
        SerialConsoleWriteString("Write to test file (f_write)...\r\n");
        uint32_t varWrite = 0;
        if (0 != f_write(&file_object, binbuff, 256, &varWrite)) {
            f_close(&file_object);
            LogMessage(LOG_INFO_LVL, "[FAIL]\r\n");
            sdCardPass = false;
            goto main_end_of_test;
        }

        SerialConsoleWriteString("[OK]\r\n");
        f_close(&file_object);   // Close file
        SerialConsoleWriteString("Test is successful.\n\r");

    main_end_of_test:
        SerialConsoleWriteString("End of Test.\n\r");

    } else {
        SerialConsoleWriteString("SD Card failed initiation! Check connections!\n\r");
        sdCardPass = false;
    }

    return sdCardPass;
}

/**
 * function      static void jumpToApplication(void)
 * @brief        Jumps to main application
 * @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
 *				before performing the jump!
 * @return
 ******************************************************************************/
static void jumpToApplication(void) {
    // Function pointer to application section
    void (*applicationCodeEntry)(void);

    // Rebase stack pointer
    __set_MSP(*(uint32_t *) APP_START_ADDRESS);

    // Rebase vector table
    SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

    // Set pointer to application section
    applicationCodeEntry = (void (*)(void))(unsigned *) (*(unsigned *) (APP_START_RESET_VEC_ADDRESS));

    // Jump to application. By calling applicationCodeEntry() as a function we move the PC to the point in memory pointed by applicationCodeEntry,
    // which should be the start of the main FW.
    applicationCodeEntry();
}

/**
 * function      static void configure_nvm(void)
 * @brief        Configures the NVM driver
 * @details
 * @return
 ******************************************************************************/
static void configure_nvm(void) {
    struct nvm_config config_nvm;
    nvm_get_config_defaults(&config_nvm);
    config_nvm.manual_page_write = false;
    nvm_set_config(&config_nvm);
}

/**
 * function      static void BootloaderUpdate(void)
 * @brief        Update Bootloader
 * @details		 Check file in SD card -> Open Firmware -> Update Firmware -> Check CRC -> Delete Flag
 * @return
 ******************************************************************************/
static void BootloaderUpdate(void){
	#define CRC32_TESTA 0x1EF640AF  /**< CRC32 checksum for TestA.bin firmware */
	#define CRC32_TESTB 0xFFECCCF0  /**< CRC32 checksum for TestB.bin firmware */
	FRESULT fr;				//Result of file operation
	FIL firmware_file;		//Firmware file
	char* flag_name = NULL;		//Flag of File detected
	char* firmware_name = NULL;		//Name of File detected
	uint32_t crc_value = 0;		//crc of TextA and TextB

	char flagA_path[] = "0:FlagA.txt";  /**< Path to FlagA.txt indicating TestA.bin should be loaded */
	char flagB_path[] = "0:FlagB.txt";  /**< Path to FlagB.txt indicating TestB.bin should be loaded */
	
	flagA_path[0] = LUN_ID_SD_MMC_0_MEM + '0';
	flagB_path[0] = LUN_ID_SD_MMC_0_MEM + '0';
	
	
	//1. Check File in SD Card
	if(	f_open(&file_object, (char const *)flagA_path, FA_READ) == FR_OK){
		f_close(&file_object);
		flag_name = "0:FlagA.txt";
		firmware_name = "0:TestA.bin";
		crc_value = 0x1EF640AF;
		SerialConsoleWriteString("FlagA.txt is detected, loading TestA.bin... \r\n");
		f_unlink(flag_name);
	}
	else if(f_open(&file_object, (char const *)flagB_path, FA_READ) == FR_OK){
		f_close(&file_object);
		flag_name = "0:FlagB.txt";
		firmware_name = "0:TestB.bin";
		crc_value = 0xFFECCCF0;
		SerialConsoleWriteString("FlagB.txt is detected, loading TestB.bin... \r\n");
		f_unlink(flag_name);
	}
	else{
		SerialConsoleWriteString("Nothing detected, going to application... \r\n");
		return;
	}
	
	//2. Open Firmware
	fr = f_open(&firmware_file, firmware_name,FA_READ);
	if(fr != FR_OK){
		SerialConsoleWriteString("Firmware open unsuccessfully... \r\n");
		return;
	}
	
	//3. Update Firmware
	uint32_t address = APP_START_ADDRESS;
	uint32_t br;
	uint8_t buffer[256];				//256 Byte Per Row
	
	while (f_read(&firmware_file, buffer, 256, &br) == FR_OK && br > 0)
	{
		if (nvm_erase_row(address) != STATUS_OK)
		{
			SerialConsoleWriteString("Flash Erase Failed... \r\n");
			f_close(&firmware_file);
			return;
		}
		for (int i = 0; i < 4 && (i * 64 < br); i++ )
		{
			if (nvm_write_buffer(address + i*64, buffer + i*64, 64) != STATUS_OK)
			{
				SerialConsoleWriteString("Flash Write Failed... \r\n");
				f_close(&firmware_file);
				return;
			}
		}
		address += 256;
	}
	
	f_close(&firmware_file);			//Close File after the work of file done
	
	//4. Check CRC
	uint32_t real_crc = 0xFFFFFFFF;
//	uint32_t image_size = address - APP_START_ADDRESS;
	DWORD firmware_size = f_size(&firmware_file);
	if (dsu_crc32_cal(APP_START_ADDRESS, firmware_size, &real_crc) != STATUS_OK)
	{
		SerialConsoleWriteString("CRC Calculation Failed... \r\n");
		return;
	}
	else {
		SerialConsoleWriteString("CRC Calculation Succeed... \r\n");
	}
	
	char crc_msg[100];
	sprintf(crc_msg, "Calculated CRC32: 0x%08lX, Expected CRC32: 0x%08lX\r\n",
	(unsigned long)~real_crc, (unsigned long)crc_value);
	SerialConsoleWriteString(crc_msg);
	
	if (~real_crc != crc_value)
	{
		SerialConsoleWriteString("CRC mismatch after write... \r\n");
		return;
	}
	else {
		SerialConsoleWriteString("CRC matches after write... \r\n");
	}
	
	//5. Delete Flag
	//f_unlink(flag_name); //Get rid of existing two flags
	SerialConsoleWriteString("Firmware update success! Rebooting into app... \r\n");
}

/**
 * function      void Firmware_Check(char * filename, uint32_t expected_crc)
 * @brief        Update firmware from SD Card to MCU Flash
 * @details
 * @return
 ******************************************************************************/
static bool Firmware_Check(char * filename) {
	FIL file;
	uint32_t file_size;
	uint32_t expected_crc;
	uint8_t buffer[FLASH_PAGE_SIZE];
	UINT bytesRead;
	uint32_t flash_address = APP_START_ADDRESS;
	char bufferPrint[64];// store test

	// Open file
	if (f_open(&file, filename, FA_READ) != FR_OK) {
		SerialConsoleWriteString("Failed to open firmware file!\n");
		return false;
	}

	// Get file size
	file_size = f_size(&file);
	if (file_size < 4) {
		SerialConsoleWriteString("Firmware file too small!\n");
		f_close(&file);
		return false;
	}

	// Move to last 4 bytes and read CRC
	if (f_lseek(&file, file_size - 4) != FR_OK || f_read(&file, &expected_crc, 4, &bytesRead) != FR_OK || bytesRead != 4) {
		SerialConsoleWriteString("Failed to read expected CRC!\n");
		f_close(&file);
		return false;
	}
	LogMessage(LOG_INFO_LVL, "Expected CRC from file: %#010x\r\n", expected_crc);

	// Write firmware (without CRC part)
	if(Update_Flag){ // It should be named as Flag(include OTAFU and Golden)
		SerialConsoleWriteString("Firmware updating approved by Update_Flag"); //OTAFU or Golden(if OTAFU
		if (f_lseek(&file, 0) != FR_OK) {
			SerialConsoleWriteString("Failed to reset file pointer!\r\n");
			f_close(&file);
			return false;
		}

		uint32_t remaining = file_size - 4;
		while (remaining > 0) {
			UINT to_read = (remaining > FLASH_PAGE_SIZE) ? FLASH_PAGE_SIZE : remaining;
			if (f_read(&file, buffer, to_read, &bytesRead) != FR_OK || bytesRead == 0) {
				SerialConsoleWriteString("Failed to read firmware data!\r\n");
				f_close(&file);
				return false;
			}
			if (flash_address % NVMCTRL_ROW_SIZE == 0) {
				nvm_erase_row(flash_address);
			}
			nvm_write_buffer(flash_address, buffer, bytesRead);
			flash_address += bytesRead;
			remaining -= bytesRead;
		}
	}

	f_close(&file);

	// Validate CRC
	return verifyFirmwareCRC32(APP_START_ADDRESS, file_size - 4, expected_crc);
}


/**
 * function      static bool verifyFirmwareCRC32(uint32_t addr, uint32_t len, uint32_t expected_crc)
 * @brief        Calculate CRC32 of firmware and compare with expected_crc
 * @details
 * @return
 ******************************************************************************/
static bool verifyFirmwareCRC32(uint32_t addr, uint32_t len, uint32_t expected_crc) {
	uint32_t calculated_crc = 0xFFFFFFFF;
	char bufferPrint[64];
	if (dsu_crc32_cal(addr, len, &calculated_crc) != STATUS_OK) {
		SerialConsoleWriteString("CRC32 calculation failed.\r\n");
		return false;
	}
	calculated_crc ^= 0xFFFFFFFF;
	LogMessage(LOG_INFO_LVL, "Calculated CRC is: %#010x \r\n", calculated_crc);
	
	//CRC
	snprintf(bufferPrint, 64, "expected CRC CLI: 0x%08lX\r\n",(unsigned long)expected_crc);
	SerialConsoleWriteString(bufferPrint);
	//CRC
	snprintf(bufferPrint, 64, "calculated CRC CLI: 0x%08lX\r\n",(unsigned long)calculated_crc);
	SerialConsoleWriteString(bufferPrint);
	
	return (calculated_crc == expected_crc);
}