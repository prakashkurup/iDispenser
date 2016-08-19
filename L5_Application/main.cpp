/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */

/**
 * @file
 * @brief This is the application entry point.
 * 			FreeRTOS and stdio printf is pre-configured to use uart0_min.h before main() enters.
 * 			@see L0_LowLevel/lpc_sys.h if you wish to override printf/scanf functions.
 *
 */

/* iDISPENSER EMBEDDED APPLICATION VERSION 1.0
 *
 * AUTHOR: Prakash Kurup
 * DATE: June 2, 2016
 *
 * DESCRIPTION: The application is developed using C, C++ and FreeRTOS. There are three main tasks running in
 * FreeRTOS namely, WiFi, Bluetooth BLE and servo motor for mechanical spraying. The WiFi task connects to the
 * wireless network assigned and, establishes TCP connection to the cloud server for sending current
 * temperature data. Depending on the response received from the server, the application either initiates
 * dispenser action or continues sending data. Also, the application allows Bluetooth connection via
 * iOS mobile app to test the dispenser spray manually.
 *
 * Hardware names for reference:
 * ESP8266 - WiFi Module
 * Adafruit Bluefruit BLE - Bluetooth Module
 *
 * P.S. Made with 'sleepless nights' code :)
 *
 * */

#include "tasks.hpp"
#include "examples/examples.hpp"
#include "esp8266.hpp"				// ESP8266 WiFi
#include "utilities.h"
#include "LPC17xx.h"
#include "io.hpp"
#include "rtc.h"					// RTC Timer
#include "printf_lib.h"				// Debug print immediately
#include "lpc_sys.h"
#include "storage.hpp"				// SD Card
#include "uart2.hpp"
#include "dispenser.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define DELAY 5000 					// Needs 5 sec delay between each WiFi functions
#define RUN_TASK_DURATION 60000		// Frequency of WiFi run() method called

SemaphoreHandle_t gDispenserSemaphore = NULL; // Semaphore Signal Global


/*************************************** BLUETOOTH BLE TASK ***************************************/

class bleTask: public scheduler_task
{
	Uart2 &ble = Uart2::getInstance(); 	// UART2 Initialization for BLE
	const int bleBaud 			= 9600;	// UART baud rate for BLE Module
	const int bleRx 			= 1000; // RX Queue size for BLE UART
	const int bleTx 			= 1000;	// TX Queue size for BLE UART
	char bleBufferRx[256];				// Temporary stores data received over Bluetooth
	uint32_t bleTaskStackSize 	= 2000;	// Stack size for BLE task

public:
	bleTask(int priority): scheduler_task("BLE Task", bleTaskStackSize, priority) {
		/* Nothing to init */
	}

	bool init(void)
	{
		gDispenserSemaphore = xSemaphoreCreateMutex();

		ble.init(bleBaud, bleRx, bleTx);

		ble.put("************ DISPENSER READY *************\r\n");

		return true;
	}

	bool run(void *p)
	{
		if (ble.getBLE(bleBufferRx, 5, portMAX_DELAY)) {

			if (!strcmp(bleBufferRx, "hello")) {
				ble.put("\r\nInitializing...\r\n");
				xSemaphoreGive(gDispenserSemaphore); // Giving semaphore signal to Dispenser Task
				vTaskDelay(4000);
				ble.put("Task completed!\r\n");
			} else {
				ble.put("WRONG INPUT!\r\n");
			}
		}

		return true;
	}
}; // end of class

/*************************************** ESP8266 WiFi TASK ***************************************/
class espTask : public scheduler_task
{
	esp8266 &esp = esp8266::getInstance(); // WiFi ESP8266 Class object

	/* WiFi SSID and Password */
//	const char* ssid 	= "Lannisters";
//	const char* pwd 	= "JohnSnow@321";
	const char* ssid 	= "EvenPrime2";
	const char* pwd 	= "EP2Hanumayamma1)";

	/* TCP IP Address and Port number */
	const char* ipAddr 	= "52.22.106.58";
	unsigned int port 	= 8090;

	/* URL end points for testing */
	const char* getURL 	= "GET //inteliidispenserSvc/api/users/hello HTTP/1.1\r\nHost: 52.22.106.58:8090\r\n\r\n";
	const char* postURL = "POST /inteliidispenserSvc/api/users/receivecsv HTTP/1.1\r\n"
			"Host: 52.22.106.58:8090\r\nContent-Length:12\r\n\r\nhello,server\r\n\r\n";

	char wifiBufferData[512] 	= {0}; 					// Temporary stores sensor data and time
	char wifiBufferTx[512] 		= {0};					// Temporary stores data to be sent over WiFi
	double temperature_celsius 	= 0.0; 					// Stores temperature sensor values
	const char* auth 			= "HANUEMBEDSJ1758";	// Authorization key for POST request
	const int baud 				= 115200;				// Baud rate for ESP8266 ESP-01
	const int wifiRx 			= 1000;					// UART RX Queue size
	const int wifiTx 			= 1000;					// UART TX Queue size
	const int mode 				= 1;					// ESP8266 Mode: 1 = Client, 2 = Station, 3 = Both
	uint32_t wifiTaskStackSize 	= 5000;					// WiFi Task stack size

//	rtc_t timeStamp; 									// LPC176x/5x Real Time Clock (RTC) structure type
//	bool sdCardPresent 			= false;

public:
	espTask(int priority) : scheduler_task("espTask", wifiTaskStackSize, priority) {
		/* Nothing to init */
	}

	bool init(void)
	{
//		gDispenserSemaphore = xSemaphoreCreateBinary();
		gDispenserSemaphore = xSemaphoreCreateMutex();

		LD.setNumber(00); // Setting on-board LED Display to 0

		u0_dbg_put("\r\n********* Intelligent Dispenser *********\r\n");

		u0_dbg_put("\r\nWifi ESP8266 Initialization\r\n");
		esp.esp8266_init(baud, wifiRx, wifiTx); // Initialized using UART3

		delay_ms(DELAY);

		u0_dbg_put("\r\nReset the WiFi module\r\n"); // AT+RST command
		if(!esp.esp8266_reset()) {
			u0_dbg_put("RESET ERROR!\r\n");
			return false;
		}

		delay_ms(DELAY);

		u0_dbg_put("\r\nDisabling echo mode\r\n"); // AT+E0 command
		if(!esp.esp8266_echo()) {
			u0_dbg_put("ECHO ERROR!\r\n");
			return false;
		}

		delay_ms(DELAY);

		u0_dbg_put("\r\nSetting the WiFi module to client mode\r\n"); // AT+CWMODE command
		if(!esp.esp8266_mode(mode)) {
			u0_dbg_put("MODE ERROR!\r\n");
			return false;
		}

		delay_ms(DELAY);

		u0_dbg_put("\r\nConnecting to the WiFi network\r\n"); // AT+CWJAP command
		if(!esp.esp8266_wifi_connect(ssid, pwd)) {
			u0_dbg_put("WIFI CONNECTION ERROR!\r\n");
			return false;
		}

		delay_ms(DELAY);

		setRunDuration(RUN_TASK_DURATION); // Setting frequency of run() method being called

		return true;
	}

	bool run(void *p)
	{
		u0_dbg_put("\r\nStarting TCP connection\r\n"); // AT+CIPSTART command
		if(!esp.esp8266_tcp_connect(ipAddr, port)) {
			u0_dbg_put("TCP CONNECTION ERROR!\r\n");
			return false;
		}

		delay_ms(DELAY);

		temperature_celsius = TS.getCelsius(); // Store the current temperature sensor value
//		timeStamp = rtc_gettime(); // Get the latest time

		LD.setNumber(temperature_celsius); // Displaying temperature on-board LED Display

		/* Storing the ID, Temp and Time */
//		snprintf(wifiBufferData, sizeof(wifiBufferData), "{\"id\":\"1\",\"temperature\":\"%.2f\",\"timestamp\":\"%02lu:%02lu:%02lu\"}\r\n"
//				, temperature, timeStamp.hour, timeStamp.min, timeStamp.sec);

		/* Storing the ID and Temp */
		snprintf(wifiBufferData, sizeof(wifiBufferData), "{\"id\":\"1\",\"temperature\":\"%.2f\"}\r\n", temperature_celsius);

		/* Storing the POST url along with the data value */
		snprintf(wifiBufferTx, sizeof(wifiBufferTx), "POST /inteliidispenserSvc/api/sensor/ HTTP/1.1\r\n"
				"Host: 52.22.106.58:8090\r\nContent-Length:%d\r\n\r\n%s\r\n\r\n", strlen(wifiBufferData), wifiBufferData);

		u0_dbg_put("\r\nSending data\r\n"); // AT+CIPSEND command
		if (!esp.esp8266_post_data(wifiBufferTx, strlen(wifiBufferTx))) {
			u0_dbg_put("Continue sending data\r\n");
		} else {
			puts("Dispensing starts...\r\n");
			xSemaphoreGive(gDispenserSemaphore); // Giving semaphore signal to Dispenser Task
			vTaskDelay(4000);
			puts("Dispensing completed!\r\n");
		}


			/* SD CARD */
#if(0)
			if(true == sdCardPresent) {
				puts("\r\nLogging data on SD Card... \r\n");
				Storage::append("1:SensorData.txt", bufferData, sizeof(bufferData), 0);
			}

			if(counter == 0) {
				counter = 5; // Reset counter to 5
				puts("\r\nReading from SD Card... \r\n");
				Storage::read("1:SensorData.txt", bufferTx, sizeof(bufferTx), 0); // Read and store the data from the SD card
				printf("%s\r\n", bufferTx);
				memset(bufferData, 0, sizeof(bufferData)); // Clearing the data
				memset(bufferTx, 0 , sizeof(bufferTx));
				Storage::write("1:SensorData.txt", bufferTx, sizeof(bufferTx), 0); // Erasing the file
			}
#endif

		return true;

	} // end of bool run(void *p)

}; // end of class

/*************************************** DISPENSER TASK ***************************************/

class dispTask: public scheduler_task
{
	uint32_t dispTaskStackSize = 2000; // Dispenser task stack size


public:
	dispTask(int priority): scheduler_task("Dispenser Task", dispTaskStackSize, priority) {
		/* Nothing to init */


	}

	bool init(void)
	{
		/* Nothing to init */

		return true;
	}

	bool run(void *p)
	{
		if (xSemaphoreTake(gDispenserSemaphore, portMAX_DELAY)) {
//			LE.on(3);
//			delay_ms(4000);
//			LE.off(3);

			dispense_start();
		}

		return true;
	}
}; // end of class

/*************************************** MAIN FUNCTION ***************************************/
int main(void)
{
	/* Checking if SD Card is mounted or not */
#if(0)
	FileSystemObject& sdcard = Storage::getSDDrive();
	unsigned int totalKb = 0;
	unsigned int availKb = 0;
	const char sdCardMount = sdcard.mount();
	bool mounted = (0 == sdCardMount);

	if (mounted && FR_OK == sdcard.getDriveInfo(&totalKb, &availKb)) {
			const unsigned int maxBytesForKbRange = (32 * 1024);
			const char *size = (totalKb < maxBytesForKbRange) ? "KB" : "MB";
			unsigned int div = (totalKb < maxBytesForKbRange) ? 1 : 1024;
			printf("\r\nSD Card: OK ---> Capacity %-5d%s, Available: %-5u%s\r\n", totalKb/div, size, availKb/div, size);
			sdCardPresent = true;
	} else
		printf("\r\nSD Card Error: %i, Mounted: %s\r\n", sdCardMount, mounted ? "Yes" : "No");
#endif

#if 1
	/* Scheduler Tasks */
	scheduler_add_task(new bleTask(PRIORITY_HIGH));
	scheduler_add_task(new dispTask(PRIORITY_MEDIUM));
	scheduler_add_task(new espTask(PRIORITY_MEDIUM));

	scheduler_start(); //< This shouldn't return!
#endif


    return -1;
}
