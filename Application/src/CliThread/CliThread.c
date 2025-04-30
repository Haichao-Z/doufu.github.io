/**************************************************************************/ /**
 * @file      CliThread.c
 * @brief     File for the CLI Thread handler. Uses FREERTOS + CLI
 * @author    Eduardo Garcia
 * @date      2020-02-15

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "CliThread.h"

#include "I2cDriver/I2cDriver.h"
#include "WifiHandlerThread/WifiHandler.h"
/******************************************************************************
 * Defines
 ******************************************************************************/
#define FIRMWARE_VERSION "0.0.1"

/******************************************************************************
 * Variables
 ******************************************************************************/
static int8_t *const pcWelcomeMessage =
    "FreeRTOS CLI.\r\nType Help to view a list of registered commands.\r\n";

static const CLI_Command_Definition_t xOTAUCommand = {"fw", "fw: Download a file and perform an FW update\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_OTAU, 0};
static const CLI_Command_Definition_t xI2cScan = {"i2c", "i2c: Scans I2C bus\r\n", (const pdCOMMAND_LINE_CALLBACK)CLI_i2cScan, 0};
static const CLI_Command_Definition_t xGoldCommand = {"gold", "gold:\r\n Create golden image of current firmware as g_application.bin\r\n", CLI_Gold, 0};
// Clear screen command
const CLI_Command_Definition_t xClearScreen =
    {
        CLI_COMMAND_CLEAR_SCREEN,
        CLI_HELP_CLEAR_SCREEN,
        CLI_CALLBACK_CLEAR_SCREEN,
        CLI_PARAMS_CLEAR_SCREEN};

static const CLI_Command_Definition_t xResetCommand =
    {
        "reset",
        "reset: Resets the device\r\n",
        (const pdCOMMAND_LINE_CALLBACK)CLI_ResetDevice,
        0};

static const CLI_Command_Definition_t xVersionCommand =
	{
		"version",
		"version: Shows the firmware version\r\n",
		CLI_ShowVersion,
		0};

static const CLI_Command_Definition_t xTicksCommand =
	{
		"ticks",
		"ticks: Shows FreeRTOS ticks since startup\r\n",
		CLI_ShowTicks,
		0};

SemaphoreHandle_t xRxSemaphore; // Semaphore for CLI

/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static void FreeRTOS_read(char *character);
/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/******************************************************************************
 * CLI Thread
 ******************************************************************************/

void vCommandConsoleTask(void *pvParameters)
{
    // REGISTER COMMANDS HERE
    FreeRTOS_CLIRegisterCommand(&xOTAUCommand);
    FreeRTOS_CLIRegisterCommand(&xClearScreen);
    FreeRTOS_CLIRegisterCommand(&xResetCommand);
    FreeRTOS_CLIRegisterCommand(&xI2cScan);
	FreeRTOS_CLIRegisterCommand(&xVersionCommand);
	FreeRTOS_CLIRegisterCommand(&xTicksCommand);
	FreeRTOS_CLIRegisterCommand(&xGoldCommand);

    uint8_t cRxedChar[2], cInputIndex = 0;
    BaseType_t xMoreDataToFollow;
    /* The input and output buffers are declared static to keep them off the stack. */
    static char pcOutputString[MAX_OUTPUT_LENGTH_CLI], pcInputString[MAX_INPUT_LENGTH_CLI];
    static char pcLastCommand[MAX_INPUT_LENGTH_CLI];
    static bool isEscapeCode = false;
    static char pcEscapeCodes[4];
    static uint8_t pcEscapeCodePos = 0;

    // Any semaphores/mutexes/etc you needed to be initialized, you can do them here
	xRxSemaphore = xSemaphoreCreateBinary();
	if (xRxSemaphore == NULL)
	{
		SerialConsoleWriteString("Failed to create xRxSemaphore!\r\n");
	}
	
    /* This code assumes the peripheral being used as the console has already
    been opened and configured, and is passed into the task as the task
    parameter.  Cast the task parameter to the correct type. */

    /* Send a welcome message to the user knows they are connected. */
    SerialConsoleWriteString(pcWelcomeMessage);
    char rxChar;
    for (;;)
    {
        /* This implementation reads a single character at a time.  Wait in the
        Blocked state until a character is received. */

        FreeRTOS_read(&cRxedChar);

        if (cRxedChar[0] == '\n' || cRxedChar[0] == '\r')
        {
            /* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */
            SerialConsoleWriteString("\r\n");
            // Copy for last command
            isEscapeCode = false;
            pcEscapeCodePos = 0;
            strncpy(pcLastCommand, pcInputString, MAX_INPUT_LENGTH_CLI - 1);
            pcLastCommand[MAX_INPUT_LENGTH_CLI - 1] = 0; // Ensure null termination

            /* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            explanation of why this is. */
            do
            {
                /* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand(
                    pcInputString,        /* The command string.*/
                    pcOutputString,       /* The output buffer. */
                    MAX_OUTPUT_LENGTH_CLI /* The size of the output buffer. */
                );

                /* Write the output generated by the command interpreter to the
                console. */
                // Ensure it is null terminated
                pcOutputString[MAX_OUTPUT_LENGTH_CLI - 1] = 0;
                SerialConsoleWriteString(pcOutputString);

            } while (xMoreDataToFollow != pdFALSE);

            /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
            cInputIndex = 0;
            memset(pcInputString, 0x00, MAX_INPUT_LENGTH_CLI);
			memset(pcOutputString, 0, MAX_OUTPUT_LENGTH_CLI);
        }
        else
        {
            /* The if() clause performs the processing after a newline character
    is received.  This else clause performs the processing if any other
    character is received. */

            if (true == isEscapeCode)
            {

                if (pcEscapeCodePos < CLI_PC_ESCAPE_CODE_SIZE)
                {
                    pcEscapeCodes[pcEscapeCodePos++] = cRxedChar[0];
                }
                else
                {
                    isEscapeCode = false;
                    pcEscapeCodePos = 0;
                }

                if (pcEscapeCodePos >= CLI_PC_MIN_ESCAPE_CODE_SIZE)
                {

                    // UP ARROW SHOW LAST COMMAND
                    if (strcasecmp(pcEscapeCodes, "oa"))
                    {
                        /// Delete current line and add prompt (">")
                        sprintf(pcInputString, "%c[2K\r>", 27);
                        SerialConsoleWriteString(pcInputString);
                        /// Clear input buffer
                        cInputIndex = 0;
                        memset(pcInputString, 0x00, MAX_INPUT_LENGTH_CLI);
                        /// Send last command
                        strncpy(pcInputString, pcLastCommand, MAX_INPUT_LENGTH_CLI - 1);
                        cInputIndex = (strlen(pcInputString) < MAX_INPUT_LENGTH_CLI - 1) ? strlen(pcLastCommand) : MAX_INPUT_LENGTH_CLI - 1;
                        SerialConsoleWriteString(pcInputString);
                    }

                    isEscapeCode = false;
                    pcEscapeCodePos = 0;
                }
            }
            /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

            else if (cRxedChar[0] == '\r')
            {
                /* Ignore carriage returns. */
            }
            else if (cRxedChar[0] == ASCII_BACKSPACE || cRxedChar[0] == ASCII_DELETE)
            {
                char erase[4] = {0x08, 0x20, 0x08, 0x00};
                SerialConsoleWriteString(erase);
                /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
                if (cInputIndex > 0)
                {
                    cInputIndex--;
                    pcInputString[cInputIndex] = 0;
                }
            }
            // ESC
            else if (cRxedChar[0] == ASCII_ESC)
            {
                isEscapeCode = true; // Next characters will be code arguments
                pcEscapeCodePos = 0;
            }
            else
            {
                /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a n is entered the complete
                string will be passed to the command interpreter. */
                if (cInputIndex < MAX_INPUT_LENGTH_CLI)
                {
                    pcInputString[cInputIndex] = cRxedChar[0];
                    cInputIndex++;
                }

                // Order Echo
                cRxedChar[1] = 0;
                SerialConsoleWriteString(&cRxedChar[0]);
            }
        }
    }
}

/**************************************************************************/ /**
 * @fn			void FreeRTOS_read(char* character)
 * @brief		STUDENTS TO COMPLETE. This function block the thread unless we received a character. How can we do this?
                 There are multiple solutions! Check all the inter-thread communications available! See https://www.freertos.org/a00113.html
 * @details		STUDENTS TO COMPLETE.
 * @note
 *****************************************************************************/
static void FreeRTOS_read(char *character)
{
    // ToDo: Complete this function
	uint8_t rxChar;
	
	//Wait for Signal and stop to block the thread when receiving data.
	if (xSemaphoreTake(xRxSemaphore,portMAX_DELAY) == pdTRUE)
	{
		//Read Character
		if (SerialConsoleReadCharacter(&rxChar) != -1)
		{
			*character = rxChar;
		}
	}
//    vTaskSuspend(NULL); // We suspend ourselves. Please remove this when doing your code
}

/******************************************************************************
 * CLI Functions - Define here
 ******************************************************************************/

// THIS COMMAND USES vt100 TERMINAL COMMANDS TO CLEAR THE SCREEN ON A TERMINAL PROGRAM LIKE TERA TERM
// SEE http://www.csie.ntu.edu.tw/~r92094/c++/VT100.html for more info
// CLI SPECIFIC COMMANDS
static char bufCli[CLI_MSG_LEN];
BaseType_t xCliClearTerminalScreen(char *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    char clearScreen = ASCII_ESC;
    snprintf(bufCli, CLI_MSG_LEN - 1, "%c[2J", clearScreen);
    snprintf(pcWriteBuffer, xWriteBufferLen, bufCli);
    return pdFALSE;
}

// Example CLI Command. Resets system.
BaseType_t CLI_ResetDevice(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    system_reset();
    return pdFALSE;
}

// Print a Firmware Version
BaseType_t CLI_ShowVersion(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Firmware version: %s\r\n", FIRMWARE_VERSION);	
	return pdFALSE;
}

// Print the number of ticks
BaseType_t CLI_ShowTicks(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "System ticks: %lu\r\n", xTaskGetTickCount());
	return pdFALSE;
} 

// Example CLI Command. Reads from the IMU and returns data.
BaseType_t CLI_OTAU(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	const char *source_path = "0:/Application.bin";
	f_unlink(source_path);
	WifiHandlerSetState(WIFI_DOWNLOAD_INIT);

	snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Starting OTA firmware download from:\r\n%s\r\n", MAIN_HTTP_FILE_URL);
	return pdFALSE;
}

/**
 * @brief    Scans fot connected i2c devices
 * @param    p_cli
 * @param    argc
 * @param    argv
 ******************************************************************************/
BaseType_t CLI_i2cScan(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    I2C_Data i2cDevice;
    uint8_t address;
    // Send 0 command byte
    uint8_t dataOut[2] = {0, 0};
    uint8_t dataIn[2];
    dataOut[0] = 0;
    dataOut[1] = 0;
    i2cDevice.address = 0;
    i2cDevice.msgIn = (uint8_t *)&dataIn[0];
    i2cDevice.lenOut = 1;
    i2cDevice.msgOut = (const uint8_t *)&dataOut[0];
    i2cDevice.lenIn = 1;

    SerialConsoleWriteString("0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) {
        snprintf(bufCli, CLI_MSG_LEN - 1, "%02x: ", i);
        SerialConsoleWriteString(bufCli);

        for (int j = 0; j < 16; j++) {
            i2cDevice.address = (i + j) << 1;

            int32_t ret = I2cWriteDataWait(&i2cDevice, 100);
            if (ret == 0) {
                snprintf(bufCli, CLI_MSG_LEN - 1, "%02x: ", i2cDevice.address);
                SerialConsoleWriteString(bufCli);
            } else {
                snprintf(bufCli, CLI_MSG_LEN - 1, "X ");
                SerialConsoleWriteString(bufCli);
            }
        }
        SerialConsoleWriteString("\r\n");
    }
    SerialConsoleWriteString("\r\n");
    return pdFALSE;
}

BaseType_t CLI_Gold(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	const char *source_path = "0:/Application.bin";
	const char *dest_path = "0:/g_application.bin";
	
	FIL src_file, dst_file;
	FRESULT res;
	UINT bytes_read, bytes_written;
	uint8_t buffer[256];
	
	SerialConsoleWriteString("Enter gold cmd. \r\n");
	
	// Step 1: Open source file
	res = f_open(&src_file, source_path, FA_READ);
	if (res != FR_OK) {
		snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Failed to open %s (err %d)\r\n", source_path, res);
		return pdFALSE;
	}
	else snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Success to open %s (err %d)\r\n", source_path, res);

	// Step 2: Create destination file
	res = f_open(&dst_file, dest_path, FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK) {
		f_close(&src_file);
		snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Failed to create %s (err %d)\r\n", dest_path, res);
		return pdFALSE;
	}
	else snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Success to open %s (err %d)\r\n", dest_path, res);
	
	// Step 3: Copy contents
	do {
		res = f_read(&src_file, buffer, sizeof(buffer), &bytes_read);
		if (res != FR_OK) break;

		res = f_write(&dst_file, buffer, bytes_read, &bytes_written);
		if (res != FR_OK || bytes_read != bytes_written) break;
	} while (bytes_read > 0);

	f_close(&src_file);
	f_close(&dst_file);

	if (res == FR_OK) {
		snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Golden copy created: g_application.bin\r\n");
		} else {
		snprintf((char *)pcWriteBuffer, xWriteBufferLen, "Copy failed (err %d)\r\n", res);
	}

	return pdFALSE;
}