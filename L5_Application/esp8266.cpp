/*
 * esp8266.cpp
 *
 *  Created on: Jun 2, 2016
 *      Author: Prakash Kurup
 */

#include "esp8266.hpp"
#include "uart3.hpp"
#include "utilities.h"
#include "LPC17xx.h"
#include "printf_lib.h"
#include "lpc_sys.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

Uart3& wifi = Uart3::getInstance(); // UART 3 Initialization for WiFi ESP8266

#define DELAY_PRINT 2000 // 2 sec delay for receiving the next response

bool esp8266::esp8266_init(unsigned int baudrate, int rxsize, int txsize)
{
	wifi.init(baudrate, rxsize, txsize);

	return true;
}

bool esp8266::esp8266_reset()
{
	wifi.putline("AT+RST"); // putline is used to add '\r\n' at the end of AT command
//	delay_ms(5000);
    do {
    	wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
        if (!strcmp(bufferRx,"ready")) { // Successful case - response 'ready'
        	u0_dbg_printf("STATUS: %s\r\n", bufferRx);
        	return true;
        } else {
        	u0_dbg_printf("--- %s\r\n", bufferRx);
        	delay_ms(DELAY_PRINT);
        }
    } while(1);

    return false;
}

bool esp8266::esp8266_echo()
{
	wifi.putline("ATE0");
    do {
    	wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
        if (!(strcmp(bufferRx,"OK"))) { // Successful case - response 'OK'
        	u0_dbg_printf("STATUS: %s\r\n", bufferRx);
            return true;
        } else {
        	u0_dbg_printf("--- %s\r\n", bufferRx);
        	delay_ms(DELAY_PRINT);
        }
    } while(1);

    return false;
}

bool esp8266::esp8266_mode(int mode)
{
	char modeBuf[256] = {0};
	snprintf(modeBuf, sizeof(modeBuf), "AT+CWMODE=%d", mode);
	wifi.putline(modeBuf);
    do {
    	wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
        if (!strcmp(bufferRx,"OK")) { // Successful case - response 'OK'
        	u0_dbg_printf("STATUS: %s\r\n", bufferRx);
            return true;
        } else {
        	u0_dbg_printf("--- %s\r\n", bufferRx);
        	delay_ms(DELAY_PRINT);
        }
    } while(1);

    return false;
}

bool esp8266::esp8266_wifi_connect(const char* ssid, const char* pwd)
{
	char wifiBuf[256] = {0};
	snprintf(wifiBuf, sizeof(wifiBuf), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
	wifi.putline(wifiBuf);
    do {
    	wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
        if (!strcmp(bufferRx,"OK")) { // Successful case - response 'OK'
        	u0_dbg_printf("STATUS: %s\r\n", bufferRx);
            return true;
        } else {
        	u0_dbg_printf("--- %s\r\n", bufferRx);
        	delay_ms(DELAY_PRINT);
        }
    } while(1);

    return false;
}

bool esp8266::esp8266_tcp_connect(const char* ipAddr, unsigned int port)
{
	char tcpBuf[256] = {0};
	snprintf(tcpBuf, sizeof(tcpBuf), "AT+CIPSTART=\"TCP\",\"%s\",%d", ipAddr, port);
	wifi.putline(tcpBuf);
    do {
    	wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
        if (!strcmp(bufferRx,"CONNECT")) { // Successful case - response 'CONNECT'
        	u0_dbg_printf("STATUS: %s\r\n", bufferRx);
            return true;
        } else if (bufferRx[0] == 'b') { // if 'busy p...' the module needs to reset
        	u0_dbg_put("TCP connection not good... restart the module!\r\n");
        	sys_reboot(); // Reboot the system
        } else {
        	u0_dbg_printf("--- %s\r\n", bufferRx);
        	delay_ms(DELAY_PRINT);
        }
    } while(1);

    return false;
}

bool esp8266::esp8266_tcp_close()
{
	wifi.putline("AT+CIPCLOSE");
    do {
    	wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
        if (!(strcmp(bufferRx,"CLOSED"))) { // Successful case - response 'CLOSED'
        	u0_dbg_printf("STATUS: %s\r\n", bufferRx);
            return true;
        } else {
        	u0_dbg_printf("--- %s\r\n", bufferRx);
        	delay_ms(DELAY_PRINT);
        }
    } while(1);

    return false;
}

bool esp8266::esp8266_connect_status()
{
	wifi.putline("AT+CIPSTATUS");
    do {
    	wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
        if (bufferRx[0] == '+') { // Successful case
        	u0_dbg_printf("STATUS: %s\r\n", bufferRx);
            return true;
        } else {
        	u0_dbg_printf("--- %s\r\n", bufferRx);
        	delay_ms(DELAY_PRINT);
        }
    } while(1);

    return false;
}

bool esp8266::esp8266_post_data(const char* url, unsigned int urlSize)
{
	char dataBuf[512];
	char token[] = "HAN";
	char *pReply;

	snprintf(dataBuf, sizeof(dataBuf),"AT+CIPSEND=%d", urlSize);
	wifi.putline(dataBuf);
	delay_ms(DELAY_PRINT);
	wifi.put(url);
	delay_ms(DELAY_PRINT);

	do {
		wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
		u0_dbg_printf("--- %s\r\n", bufferRx);
		delay_ms(DELAY_PRINT);

		if (bufferRx[0] == '+') {
			pReply = strstr(bufferRx, token);
			if (!strcmp(pReply, "HANUIDISPRETCODE1CLOSED")) { // Dispenser ON
				return true;
			} else if (!strcmp(pReply, "HANUIDISPRETCODE0CLOSED")){ // Dispenser OFF
				return false;
			}
		}

	} while(1);

    return false;
}

void esp8266::esp8266_view_version()
{
	wifi.putline("AT+GMR");
    do {
    	wifi.gets(bufferRx, sizeof(bufferRx), portMAX_DELAY);
		u0_dbg_printf("--- %s\r\n", bufferRx);
		delay_ms(DELAY_PRINT);
    } while(1);

}





