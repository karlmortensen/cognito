#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>

// When the pin is pulled low, the wifi is on. When the pin is high, it's off.
static const unsigned int WIFI_PIN = 29;
enum wifiState
{
	WIFI_ON = 0,
	WIFI_OFF = 1
};

wifiState valueChanged = WIFI_OFF;

void setWifi(wifiState setting)
{
	if (setting == WIFI_ON)
	{
		system("sudo ip link set wlan0 up");
	}
	else
	{
		system("sudo ip link set wlan0 down");
	}
}
/*
void myInterrupt(void)
{
	valueChanged = (wifiState)!valueChanged;
}

void withInterrupts(wifiState oldValue)
{ // turns out there is some sort of timing issue with the interrupts... using poling instead
	valueChanged = oldValue;
	wiringPiISR(WIFI_PIN, INT_EDGE_BOTH, &myInterrupt);
	while(1)
	{
		if(valueChanged != oldValue)
		{
			delay(100);
			printf("Setting wifi to %s\n", digitalRead(WIFI_PIN)?"Off":"On");
			oldValue = valueChanged;
		}
		delay(500);
	}
}
*/
void withoutInterrupts(wifiState initialValue)
{
	wifiState oldValue = initialValue;
	wifiState readValue = initialValue;
	while(1)
	{
		if(oldValue != readValue)
		{
			oldValue = readValue;
			// printf("Setting wifi to %s\n", readValue?"Off":"On");
			setWifi(readValue);
		}
		readValue = (wifiState)digitalRead(WIFI_PIN);
		delay(250);
	}
}

int main()
{
	wiringPiSetup();
	wifiState initialValue = (wifiState)digitalRead(WIFI_PIN);
	// printf("Wifi started up %s\n", (initialValue==WIFI_OFF)? "Off":"On");
	setWifi(initialValue);
	pullUpDnControl(WIFI_PIN, PUD_UP);

	// withInterrupts((bool)initialValue);
	withoutInterrupts(initialValue);
	
	return 0;	
}
