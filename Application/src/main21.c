/**
 * @file      main.c
 * @brief     Main application entry point
 * @author    Eduardo Garcia
 * @author    Nick M-G
 * @date      2022-04-14
 ******************************************************************************/

/****
 * Includes
 ******************************************************************************/
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

#include "Motor.h"
#include "LCD/LCD.h"
#include "RTC_LCD/rtc_lcd.h"
#include "LED/LED.h"
#include "Health_Reminder/Health_Reminder.h"
#include "AI_voice_control/voice_control.h"
#include "IMU/lsm6dso_reg.h"
#include "IMU/ImuTask.h"

#include "flag.h"

/****
 * Defines and Types
 ******************************************************************************/
#define APP_TASK_ID 0 /**< @brief ID for the application task */
#define CLI_TASK_ID 1 /**< @brief ID for the command line interface task */
#define WAIT_TX_COMPLETE_MS 1000

/****
 * Local Function Declaration
 ******************************************************************************/
void vApplicationIdleHook(void);
//!< Initial task used to initialize HW before other tasks are initialized
static void StartTasks(void);
void vApplicationDaemonTaskStartupHook(void);

void vApplicationStackOverflowHook(void);
void vApplicationMallocFailedHook(void);
void vApplicationTickHook(void);

/****
 * Variables
 ******************************************************************************/
static TaskHandle_t cliTaskHandle = NULL;       //!< CLI task handle
static TaskHandle_t daemonTaskHandle = NULL;    //!< Daemon task handle
static TaskHandle_t wifiTaskHandle = NULL;      //!< Wifi task handle
static TaskHandle_t uiTaskHandle = NULL;        //!< UI task handle
static TaskHandle_t controlTaskHandle = NULL;   //!< Control task handle
static TaskHandle_t healthTaskHandle = NULL;    //!< Health Monitor task handle
static TaskHandle_t lcdTaskHandle = NULL;		//!< LCD task handle
static TaskHandle_t ledTaskHandle = NULL;		//!< LED task handle
static TaskHandle_t voicecontrolTaskHandle = NULL;//!< Voice Control task Handle
static TaskHandle_t imuTaskHandle = NULL;

char bufferPrint[64];   ///< Buffer for daemon task

SystemState system_state = counting_down;

SemaphoreHandle_t xHealthMonitorSemaphore = NULL; // For Health Monitor

volatile int stop_rtc_show_flag = 0;
volatile int led_flag = 0;
volatile bool health_monitor_flag = false;
volatile bool stop_all_flag = false;
volatile int rtc_mode = 0;
volatile int voice_control_flag = 11; //0 is no task   11 for test    12 nothing

volatile rtc_time_t current_time;

/**
 * @brief Main application function.
 * Application entry point.
 * @return int
 */
int main(void) {
    /* Initialize the board. */
    system_init();
	SK6812_Init();
	configure_tcc0();
	servo_control_init();
	delay_init();	
	Command_Detect();
    // Initialize trace capabilities
    vTraceEnable(TRC_START);
    // Start FreeRTOS scheduler
    vTaskStartScheduler();
	
	
    return 0;   // Will not get here
}

/**
 * function          vApplicationDaemonTaskStartupHook
 * @brief            Initialization code for all subsystems that require FreeRToS
 * @details			This function is called from the FreeRToS timer task. Any code
 *					here will be called before other tasks are initilized.
 * @param[in]        None
 * @return           None
 */
void vApplicationDaemonTaskStartupHook(void) {
	InitializeSerialConsole();
    SerialConsoleWriteString("\r\n\r\n-----ESE516 Main Program-----\r\n");

     //Initialize HW that needs FreeRTOS Initialization
    SerialConsoleWriteString("\r\n\r\nInitialize HW...\r\n");
    if (I2cInitializeDriver() != STATUS_OK) {
	    SerialConsoleWriteString("Error initializing I2C Driver!\r\n");
	    } else {
	    SerialConsoleWriteString("Initialized I2C Driver!\r\n");
    }
	//I2CScanBus();
	xHealthMonitorSemaphore = xSemaphoreCreateBinary();
	if (xHealthMonitorSemaphore == NULL) {
		SerialConsoleWriteString("ERR: Failed to create HealthMonitorsemaphore!\r\n");
	}
	
	
    StartTasks();

    vTaskSuspend(daemonTaskHandle);
}

/**
 * function          StartTasks
 * @brief            Initialize application tasks
 * @details
 * @param[in]        None
 * @return           None
 */
static void StartTasks(void) {
    snprintf(bufferPrint, 64, "Heap before starting tasks: %d\r\n", xPortGetFreeHeapSize());
    SerialConsoleWriteString(bufferPrint);

    // Initialize Tasks here (Change Stack Size)
	
	//CLI Task
    if (xTaskCreate(vCommandConsoleTask, "CLI_TASK", CLI_TASK_SIZE, NULL, CLI_PRIORITY, &cliTaskHandle) != pdPASS) {
        SerialConsoleWriteString("ERR: CLI task could not be initialized!\r\n");
    }

    snprintf(bufferPrint, 64, "Heap after starting CLI: %d\r\n", xPortGetFreeHeapSize());
    SerialConsoleWriteString(bufferPrint);
	
	////LCD Task(1200 -> 512 -> 256)
	if (xTaskCreate(rtc_lcd_display_task, "LCD_TASK", 512-128, NULL, 4, &lcdTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERROR: LCD task could not be initialized!\r\n");
		} else {
		SerialConsoleWriteString("LCD task created successfully\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting LCD: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);

	////Health Monitor Task
	if (xTaskCreate(vHealthMonitorTask, "Health_Monitor_TASK", 128, NULL, 3, &healthTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: Health Monitor task could not be initialized!\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting Health Monitor: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);

	//WIFI Task(1024 -> 512)
	if (xTaskCreate(vWifiTask, "WIFI_TASK", 1024-128, NULL, WIFI_PRIORITY, &wifiTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: WIFI task could not be initialized!\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting WIFI: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);

	////////Voice Control Task
	if (xTaskCreate(vVoiceControlTask, "Voice_Control_TASK", 128, NULL, 3, &voicecontrolTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: Voice Control task could not be initialized!\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting Voice Control: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);
	
	//////IMU
	if (xTaskCreate(vIMUTask, "IMU_TASK", 128, NULL, 3, &imuTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: IMU Initialization Failed£¡\r\n");
		} else {
		SerialConsoleWriteString("IMU Created Successfully\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting IMU: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);
	


//LED Task(512 -> 256)
	if (xTaskCreate(LED_Task, "LED_TASK", 128, NULL, 3, &ledTaskHandle) != pdPASS) {
		SerialConsoleWriteString("ERR: LED Initialization Failed£¡\r\n");
		} else {
		SerialConsoleWriteString("LED Created Successfully\r\n");
	}
	snprintf(bufferPrint, 64, "Heap after starting LED: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);
}

void vApplicationMallocFailedHook(void) {
    SerialConsoleWriteString("Error on memory allocation on FREERTOS!\r\n");
    while (1)
        ;
}

void vApplicationStackOverflowHook(void) {
    SerialConsoleWriteString("Error on stack overflow on FREERTOS!\r\n");
    while (1)
        ;
}

#include "MCHP_ATWx.h"
void vApplicationTickHook(void) { SysTick_Handler_MQTT(); }