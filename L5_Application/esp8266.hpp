/*
 * esp8266.hpp
 *
 *  Created on: Jun 2, 2016
 *      Author: Prakash Kurup
 */

#ifndef L5_APPLICATION_ESP8266_HPP_
#define L5_APPLICATION_ESP8266_HPP_

#include "singleton_template.hpp"
#include <stdint.h>

#define BUFFER_SIZE 512

class esp8266 : public SingletonTemplate<esp8266>
{
public:
    /**
     * Initializes ESP8266 ESP-01 Module with UART 3
     * @param baudrate  Baudrate to set
     * @param rxsize	Receiver queue size
     * @param txsize	Transmitter queue size
     * @return 			true if success
     */
	bool esp8266_init(unsigned int baudrate, int rxsize, int txsize);

    /**
     * Reset the Wifi Module before use
     * @return 			true if success
     */
	bool esp8266_reset();

    /**
     * Enable or Disable echo (Doesn't send back received command)
     * @return 			true if success
     */
	bool esp8266_echo();

    /**
     * Setting the Wifi Module as Client or Server or both
     * @param mode  1 = client, 2 = server, 3 = both
     * @return 		true if success
     */
	bool esp8266_mode(int mode);

    /**
     * Connecting to a WiFi network
     * @param ssid  SSID of WiFi network
     * @param pwd	Password of WiFi network
     * @return 		true if success
     */
	bool esp8266_wifi_connect(const char* ssid, const char* pwd);

    /**
     * TCP connection to server
     * @param ipAddr	IP Address of the server
     * @param port		Port number of the server
     * @return 			true if success
     */
	bool esp8266_tcp_connect(const char* ipAddr, unsigned int port);

    /**
     * Closes the TCP connection
     * @return 			true if success
     */
	bool esp8266_tcp_close();

    /**
     * Checks the status of the TCP connection
     * Response --> 2: Got IP
     * Response --> 3: Connected
     * Repsonse --> 4: Disconnected
     * @return 			true if success
     */
	bool esp8266_connect_status();

    /**
     * POST request to server endpoint to send data
     * @param url		URL endpoint string format
     * @param urlSize	String length of the URL end point
     * @return 			true if success
     */
	bool esp8266_post_data(const char* url, unsigned int urlSize);

	void esp8266_view_version();

private:
	esp8266() {
		// Private constructor for the Singleton class
	}
	friend class SingletonTemplate<esp8266>; // Friend class used for Singleton Template
	char bufferRx[BUFFER_SIZE] = {0};
};



#endif /* L5_APPLICATION_ESP8266_HPP_ */
