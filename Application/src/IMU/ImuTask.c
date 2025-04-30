/*
 * ImuTask.c
 *
 * Created: 2025/4/7
 *  Author: Zeng Li
 */ 
#include "lsm6dso_reg.h"
#include "I2cDriver/I2cDriver.h"
#include "SerialConsole/SerialConsole.h"
#include "asf.h"
#include "LED/LED.h"
#include "flag.h"

#define IMU_I2C_ADDR 0x6B
#define LIFT_THRESHOLD 0.4f

static stmdev_ctx_t imu_ctx;
static uint8_t imu_address = IMU_I2C_ADDR;

static char bufferPrint[64];

void vIMUTask(void *pvParameters)
{
	SerialConsoleWriteString("IMU task started\r\n");
	// Set up STMdev context
	imu_ctx.write_reg = platform_write;
	imu_ctx.read_reg = platform_read;
	imu_ctx.handle = &imu_address;

	// Check device ID
	uint8_t whoamI = 0;
	lsm6dso_device_id_get(&imu_ctx, &whoamI);
	if (whoamI != LSM6DSO_ID) {
		SerialConsoleWriteString("IMU not detected!\r\n");
		vTaskDelete(NULL);
		} else {
		SerialConsoleWriteString("IMU detected!\r\n");
	}

	//// Reset IMU and enable Block Data Update
	lsm6dso_reset_set(&imu_ctx, PROPERTY_ENABLE);
	uint8_t rst;
	uint32_t resetTimeout = 50; // 50 ¡Á 10ms = 500ms timeout
	
	do {
		lsm6dso_reset_get(&imu_ctx, &rst);
		vTaskDelay(pdMS_TO_TICKS(10));
		resetTimeout--;
	} while (rst && resetTimeout > 0);

	if (resetTimeout == 0) {
		SerialConsoleWriteString("IMU reset timeout!\r\n");
		vTaskDelete(NULL);
	}

	// Set Output Data Rate and Full Scale
	lsm6dso_xl_data_rate_set(&imu_ctx, LSM6DSO_XL_ODR_104Hz);
	lsm6dso_xl_full_scale_set(&imu_ctx, LSM6DSO_2g);

	lsm6dso_gy_data_rate_set(&imu_ctx, LSM6DSO_GY_ODR_104Hz);
	lsm6dso_gy_full_scale_set(&imu_ctx,  LSM6DSO_250dps);
	
	//Main loop
	//bool button_prev = true; // Assume not pressed at startup (active low)

	while (1) {
		int16_t acc_raw[3];
		lsm6dso_acceleration_raw_get(&imu_ctx, acc_raw);

		float az_mg = lsm6dso_from_fs2_to_mg(acc_raw[2]);

		if (abs(az_mg - 980.665f) > (LIFT_THRESHOLD * 1000)) {
			SerialConsoleWriteString("IMU: Pet lifted detected!\r\n");
			explosion_effect();
			meteor_effect(255, 0, 0);
			meteor_effect(0, 255, 0);
			meteor_effect(0, 0, 255);
			rainbow_swirl();
			SK6812_Clear();
			
			vTaskDelay(pdMS_TO_TICKS(1000)); 
		}
		
		vTaskDelay(pdMS_TO_TICKS(100)); // Sample every 100ms
	}
}

