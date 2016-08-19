/*
 * dispenser.cpp
 *
 *  Created on: Jun 15, 2016
 *      Author: Prakash Kurup
 */
#include "lpc_pwm.hpp"
#include "dispenser.hpp"
#include "utilities.h"

enum Positions
{
	LEFT = 3,
	NEUTRAL = 7,
	RIGHT = 12,
} position;

void dispense_start()
{
	PWM servo(PWM::pwm3, 50); // --> P2.1 pin

	servo.set(LEFT);
	delay_ms(2000);
	servo.set(NEUTRAL);
	delay_ms(2000);
}



