/**************************************************************************/
/**
 * @file        SerialConsole.h
 * @ingroup 	Serial Console
 * @brief       This file has the code necessary to run the CLI and Serial Debugger. 
 * 				It initializes an UART channel and uses it to receive command from the user
 *				as well as print debug information.
 * @details     This file has the code necessary to run the CLI and Serial Debugger. 
 * 				It initializes an UART channel and uses it to receive command from the user
 *				as well as print debug information.
 *
 *				The code in this file will:
 *				--Initialize a SERCOM port (SERCOM # ) to be an UART channel operating at 115200 baud/second, 8N1
 *				--Register callbacks for the device to read and write characters asycnhronously as required by the CLI
 *				--Initialize the CLI and Debug Logger datastructures
 *
 *				Usage:
 *
 *
 * @copyright
 * @author
 * @date        January 26, 2019
 * @version		0.1
 *****************************************************************************/

 #ifndef SERIAL_CONSOLE_H
 #define SERIAL_CONSOLE_H
 
 /******************************************************************************
  * Includes
  ******************************************************************************/
 #include <asf.h>
 #include <string.h>
 #include <stdarg.h>
 #include "circular_buffer.h"
 
 /******************************************************************************
  * Enumerations
  ******************************************************************************/
 enum eDebugLogLevels {
	 LOG_INFO_LVL    = 0, /**< Logs an INFO message */
	 LOG_DEBUG_LVL   = 1, /**< Logs a DEBUG message */
	 LOG_WARNING_LVL = 2, /**< Logs a WARNING message */
	 LOG_ERROR_LVL   = 3, /**< Logs an ERROR message */
	 LOG_FATAL_LVL   = 4, /**< Logs a FATAL message (non-recoverable error) */
	 LOG_OFF_LVL     = 5, /**< Disables logging */
	 N_DEBUG_LEVELS  = 6  /**< Maximum number of log levels */
 };

/******************************************************************************
* Global Function Declarations
******************************************************************************/
/**
 * @fn			void InitializeSerialConsole(void)
 * @brief		Initializes the UART - sets up the SERCOM to act as UART and registers the callbacks for
 *				asynchronous reads and writes.
 * @details		Initializes the UART - sets up the SERCOM to act as UART and registers the callbacks for
 *				asynchronous reads and writes.
 * @note			Call from main once to initialize Hardware.
 *****************************************************************************/
void InitializeSerialConsole(void);

/**************************************************************************/
/**
 * @fn			void DeinitializeSerialConsole(void)
 * @brief		Deinitlaises the UART
 * @note
 *****************************************************************************/
void DeinitializeSerialConsole(void);

/**
 * @fn			void SerialConsoleWriteString(char * string)
 * @brief		Writes a string to be written to the uart. Copies the string to a ring buffer that 
 * 				is used to hold the text send to the uart
 * @details		Uses the ringbuffer 'cbufTx', which in turn uses the array 'txCharacterBuffer'
 * @note			Use to send a string of characters to the user via UART
 *****************************************************************************/
void SerialConsoleWriteString(char * string);

/**
 * @fn			int SerialConsoleReadCharacter(uint8_t *rxChar)
 * @brief		Reads a character from the RX ring buffer and stores it on the pointer given as an argument.
 *				Also, returns -1 if there is no characters on the buffer
 *				This buffer has values added to it when the UART receives ASCII characters from the terminal
 * @details		Uses the ringbuffer 'cbufTx', which in turn uses the array 'txCharacterBuffer'
 * @param[in]	Pointer to a character. This function will return the character from the RX buffer into this pointer
 * @return		Returns -1 if there are no characters in the buffer
 * @note			Use to receive characters from the RX buffer (FIFO)
 *****************************************************************************/
int SerialConsoleReadCharacter(uint8_t *rxChar);

/**
 * @fn			LogMessage
 * @brief		Logs a message at the specified debug level.
 * @param   	level  Determines the log levels of the message to output. If the level is smaller than 
 * 					   the current “logLevel” it is not printed.
 * @param   	format Pointer to a array of characters to be printed.
 * @param   	...    The “...” in C denotes a variable list. Please refer to https://www.cprogramming.com/tutorial/c/lesson17.html 
 * 					   for more information. In this argument, we expect the variables that you would normally use in a vsprintf 
 * 					   (please see example on https://www.tutorialspoint.com/c_standard_library/c_function_vsprintf.htm). 
 * @note
 *****************************************************************************/
void LogMessage(enum eDebugLogLevels level, const char *format, ...);

/**
 * @fn			eDebugLogLevels getLogLevel(void)
 * @brief		Sets the level of debug to print to the console to the given argument.
 *				Debug logs below the given level will not be allowed to be printed on the system
 * @param[in]   debugLevel The debug level to be set for the debug logger
 * @note
 *****************************************************************************/
void setLogLevel(enum eDebugLogLevels debugLevel);

/**
 * @fn			eDebugLogLevels getLogLevel(void)
 * @brief		Gets the level of debug to print to the console to the given argument.
 *				Debug logs below the given level will not be allowed to be printed on the system
 * @return		Returns the current debug level of the system.
 * @note
 *****************************************************************************/
enum eDebugLogLevels getLogLevel(void);


/******************************************************************************
* Local Functions
******************************************************************************/
void LogMessageDebug(const char *format, ...);

#endif /* SERIAL_CONSOLE_H */
