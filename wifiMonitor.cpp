#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include "pinout.hpp"
#include <pthread.h>

// When the pin is pulled low, the wifi is on. When the pin is high, it's off.
enum wifiState
{
	WIFI_ON = 0,
	WIFI_OFF = 1
};

wifiState valueChanged = WIFI_OFF;
bool endThreads = false;

void setWifiAndLights(wifiState setting)
{
	if (setting == WIFI_ON)
	{
		system("sudo ip link set wlan0 up");
		// KDM note this should be saved for when we get a packet from that link. This is just an example
		digitalWrite(EARLY_LED_A, GREEN_A);
		digitalWrite(EARLY_LED_B, GREEN_B);
		digitalWrite(MIDDLE_LED_A, GREEN_A);
		digitalWrite(MIDDLE_LED_B, GREEN_B);
		digitalWrite(LATE_LED_A, GREEN_A);
		digitalWrite(LATE_LED_B, GREEN_B);

	}
	else
	{
		system("sudo ip link set wlan0 down");
		digitalWrite(EARLY_LED_A, RED_A);
		digitalWrite(EARLY_LED_B, RED_B);
		digitalWrite(MIDDLE_LED_A, RED_A);
		digitalWrite(MIDDLE_LED_B, RED_B);
		digitalWrite(LATE_LED_A, RED_A);
		digitalWrite(LATE_LED_B, RED_B);
	}
}
/*
void myInterrupt(void)
{
	valueChanged = (wifiState)!valueChanged;
}

void withInterrupts(wifiState oldValue)
{ // turns out there is some sort of timing issue with the interrupts... using polling instead
	valueChanged = oldValue;
	wiringPiISR(WIFI_A, INT_EDGE_BOTH, &myInterrupt);
	while(1)
	{
		if(valueChanged != oldValue)
		{
			delay(100);
			printf("Setting wifi to %s\n", digitalRead(WIFI_A)?"Off":"On");
			oldValue = valueChanged;
		}
		delay(500);
	}
}
*/

void* monitorWifiPolling(void* initialValue)
{
	wifiState* pOldValue = (wifiState*)initialValue;
	wifiState oldValue = *pOldValue;
	wifiState readValue = *pOldValue;
	
	int x=0;
	while(!endThreads)
	{
		if(oldValue != readValue)
		{
			oldValue = readValue;
			//printf("Setting wifi to %s\n", readValue?"Off":"On");
			setWifiAndLights(readValue);
		}
		readValue = (wifiState)digitalRead(WIFI_A);
		delay(250);
		printf("reading again\n");
		if(x++>25) // KDM
		{
			endThreads=true;
		}
	}
}

int main()
{
	wiringPiSetup();
	pinMode(EARLY_LED_A, OUTPUT);
	pinMode(EARLY_LED_B, OUTPUT);
	pinMode(MIDDLE_LED_A, OUTPUT);
	pinMode(MIDDLE_LED_B, OUTPUT);
	pinMode(LATE_LED_A, OUTPUT);
	pinMode(LATE_LED_B, OUTPUT);
	wifiState initialValue = (wifiState)digitalRead(WIFI_A);
	// printf("Wifi started up %s\n", (initialValue==WIFI_OFF)? "Off":"On");
	setWifiAndLights(initialValue);
	pullUpDnControl(WIFI_A, PUD_UP);

	// withInterrupts((bool)initialValue);
	pthread_t wifiMonitor;
	pthread_create(&wifiMonitor, NULL, monitorWifiPolling, &initialValue);
	printf("cows!!!\n");
	
	pthread_join(wifiMonitor, NULL);
	printf("YEAH!!!\n");
	return 0;	
}
