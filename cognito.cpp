#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <pthread.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "pinout.hpp"
#include <fstream>
#include <ctime>

unsigned short ANDROID_SERVER_PORT = 5555;
unsigned short NODE_JS_LISTENING_PORT = 7777;
unsigned int LOCALHOST = 16777343;
char LOCAL_ADDRESS[]="127.0.0.1";
char ANY_ADDRESS[]="0.0.0.0";
const unsigned int BUFFER_LEN = 512;
bool allowMessagesToBeReceived = true;
unsigned int keepAliveTimes[4];
bool lightStatus[3];
pthread_mutex_t katimes;
unsigned int TOLERANCE = 3;
unsigned int MD_STANDOFF_TIME = 1;
unsigned int PB_STANDOFF_TIME = 1;
unsigned int LS_STANDOFF_TIME = 1;
unsigned int BLINK_STANDOFF = 2;
unsigned int doNotHandleKeepAlivesUntil = 0;

std::string id;
std::string A="A";
std::string B="B";
std::string C="C";
std::string D="D";

// When the pin is pulled low, the network is on. When the pin is high, it's off.
enum networkState
{
	NETWORK_ON = 0,
	NETWORK_OFF = 1
};

enum lightSettings
{
	OFF = 0,
	ON = 1
};

enum light
{
	EARLY = 0,
	MIDDLE = 1,
	LATE = 2
};

networkState valueChanged = NETWORK_OFF;
bool endThreads = false;

/*
 * ledA is the first leg of the LED
 * ledB is the second leg of the LED
 * colorA is the color
 * colorB is the color (differential)
 */
void blink(int ledA, int ledB, int colorA, int colorB)
{
	digitalWrite(ledA, colorA);				
	digitalWrite(ledB, colorB);
	usleep(200000);
	digitalWrite(ledA, colorB);			
	usleep(175000);
	digitalWrite(ledA, colorA);
	usleep(200000);
	digitalWrite(ledA, colorB);			
	usleep(175000);
	digitalWrite(ledA, colorA);
	usleep(200000);
	digitalWrite(ledA, colorB);			
	usleep(175000);
	digitalWrite(ledA, colorA);
}

void blinkAll(int colorA, int colorB)
{
	digitalWrite(EARLY_LED_A,  colorA);				
	digitalWrite(EARLY_LED_B,  colorB);
	digitalWrite(MIDDLE_LED_A, colorA);				
	digitalWrite(MIDDLE_LED_B, colorB);
	digitalWrite(LATE_LED_A,   colorA);				
	digitalWrite(LATE_LED_B,   colorB);
	usleep(200000);
	digitalWrite(EARLY_LED_A,  colorB);				
	digitalWrite(MIDDLE_LED_A, colorB);				
	digitalWrite(LATE_LED_A,   colorB);				
	usleep(175000);
	digitalWrite(EARLY_LED_A,  colorA);				
	digitalWrite(MIDDLE_LED_A, colorA);				
	digitalWrite(LATE_LED_A,   colorA);				
	usleep(200000);
	digitalWrite(EARLY_LED_A,  colorB);				
	digitalWrite(MIDDLE_LED_A, colorB);				
	digitalWrite(LATE_LED_A,   colorB);				
	usleep(175000);
	digitalWrite(EARLY_LED_A,  colorA);				
	digitalWrite(MIDDLE_LED_A, colorA);				
	digitalWrite(LATE_LED_A,   colorA);				
	usleep(200000);
	digitalWrite(EARLY_LED_A,  colorB);				
	digitalWrite(MIDDLE_LED_A, colorB);				
	digitalWrite(LATE_LED_A,   colorB);				
}

					
void blinkInService(int colorA, int colorB)
{
	if(lightStatus[EARLY] || lightStatus[MIDDLE] || lightStatus[LATE])
	{
		if(lightStatus[EARLY])
		{
			digitalWrite(EARLY_LED_A,  colorA);				
			digitalWrite(EARLY_LED_B,  colorB);
		}
		if(lightStatus[MIDDLE])
		{
			digitalWrite(MIDDLE_LED_A, colorA);				
			digitalWrite(MIDDLE_LED_B, colorB);
		}
		if(lightStatus[LATE]) 
		{
			digitalWrite(LATE_LED_A,   colorA);				
			digitalWrite(LATE_LED_B,   colorB);
		}
		usleep(200000);
		if(lightStatus[EARLY]) digitalWrite(EARLY_LED_A,  colorB);				
		if(lightStatus[MIDDLE]) digitalWrite(MIDDLE_LED_A, colorB);				
		if(lightStatus[LATE]) digitalWrite(LATE_LED_A,   colorB);				
		usleep(175000);
		if(lightStatus[EARLY]) digitalWrite(EARLY_LED_A,  colorA);				
		if(lightStatus[MIDDLE]) digitalWrite(MIDDLE_LED_A, colorA);				
		if(lightStatus[LATE]) digitalWrite(LATE_LED_A,   colorA);				
		usleep(200000);
		if(lightStatus[EARLY]) digitalWrite(EARLY_LED_A,  colorB);				
		if(lightStatus[MIDDLE]) digitalWrite(MIDDLE_LED_A, colorB);				
		if(lightStatus[LATE]) digitalWrite(LATE_LED_A,   colorB);				
		usleep(175000);
		if(lightStatus[EARLY]) digitalWrite(EARLY_LED_A,  colorA);				
		if(lightStatus[MIDDLE]) digitalWrite(MIDDLE_LED_A, colorA);				
		if(lightStatus[LATE]) digitalWrite(LATE_LED_A,   colorA);				
		usleep(200000);
		if(lightStatus[EARLY]) digitalWrite(EARLY_LED_A,  colorB);				
		if(lightStatus[MIDDLE]) digitalWrite(MIDDLE_LED_A, colorB);				
		if(lightStatus[LATE]) digitalWrite(LATE_LED_A,   colorB);				
	}
}

int setup_socket(int *sockfd)
{
	if ((*sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		return (-1);
	}

	int enable=1;
	setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

	return *sockfd;
}

struct sockaddr_in *setup_sockaddr_in(struct sockaddr_in *addr, uint16_t *port, char *addr_string)
{
	memset((char *)addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	if (port != NULL) 
	{
		addr->sin_port = htons(*port);
	}
	if (addr_string == NULL)
	{
		addr->sin_addr.s_addr = htonl(INADDR_ANY);
	} 
	else
	{
		if (inet_aton(addr_string, &addr->sin_addr) == 0) 
		{
			fprintf(stderr, "inet_aton() failed\n");
			return (NULL);
		}
	}
	return (addr);
}

int bind_socket(int sockfd, struct sockaddr_in *addr)
{
	if (bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr)) == -1) 
	{
		fprintf(stderr, "bind() failed\n");
		return (-1);
	}
	return (0);
}

int recv_data_raw(int sockfd, char buffer[], int *recv_len, unsigned int buffer_length, struct sockaddr_in *addr, socklen_t slen)
{
	memset(buffer, 0, buffer_length);
	if ((*recv_len = recvfrom(sockfd, buffer, buffer_length, 0, (struct sockaddr *)addr, &slen)) < 0)
	{
		fprintf(stderr, "recvfrom() failed\n");
		return (-1);
	}
	return *recv_len;
}

void* listenForUdp(void* initialValue)
{
	unsigned int* p = (unsigned int *)initialValue;
	unsigned short port = (unsigned short)*p;
	printf("Listening on %d\n", port);
	int sockfd = 0;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	const socklen_t slen = sizeof(struct sockaddr_in);
	char recv_buff[BUFFER_LEN] = { 0 };
	int recv_len = 0;
	int sending_sockfd = 0;
	struct sockaddr_in sending_client_addr;
	const socklen_t sending_slen = sizeof(struct sockaddr_in);
	char msg[] = "A";

	/// Setup socket
	if ((setup_socket(&sending_sockfd) < 0) || setup_sockaddr_in(&sending_client_addr, &NODE_JS_LISTENING_PORT, ANY_ADDRESS) == NULL)
	{
		printf("UDP listener can't talk to node.js\n");
	}
		
	/// Setup socket
	if (setup_socket(&sockfd) < 0) 
	{
		return NULL;
	}

	/// Setup server addr struct
	if (setup_sockaddr_in(&server_addr, &port, NULL) == NULL)
	{
		return NULL;
	}

	/// Bind ANDROID_SERVER_PORT to address structure on sockfd
	if (bind_socket(sockfd, &server_addr)) 
	{
		return NULL;
	}

	while(!endThreads)
	{
		recv_data_raw(sockfd, recv_buff, &recv_len, BUFFER_LEN, &client_addr, slen);
		if(allowMessagesToBeReceived)
		{
			/* KDM Debug
			printf("received %d bytes: ", recv_len);
			for(int i=0; i < recv_len; ++i)
			{
				printf("0x%02X ", recv_buff[i]);
			}
			printf("\n");
			*/
			if (recv_len > 1)
			{
				// register a keep-alive
				pthread_mutex_lock(&katimes);
				keepAliveTimes[recv_buff[0] - 'A'] = std::time(0);
				pthread_mutex_unlock(&katimes);
				
				if (recv_buff[1] == '2')
				{ // Opcode 2, slew camera to node
					doNotHandleKeepAlivesUntil = std::time(0) + BLINK_STANDOFF;
					msg[0] = recv_buff[0];
					sendto(sending_sockfd, msg, 1, 0, (struct sockaddr *)&sending_client_addr, sending_slen);
					system((std::string("echo -n \"") + recv_buff[0] + "2\" | sudo alfred -s 64").c_str());
					// blink all lights in service in send mode
					if(client_addr.sin_addr.s_addr == LOCALHOST)
					{  // if it's from localhost
						printf("recieved from %c\n",recv_buff[0]);
						if(recv_buff[0] == 'A')
						{
							if(id == B)blink(EARLY_LED_A, EARLY_LED_B, GREEN_A, GREEN_B);
							else if(id == C)blink(MIDDLE_LED_A, MIDDLE_LED_B, GREEN_A, GREEN_B);
							else if(id == D)blink(LATE_LED_A, LATE_LED_B, GREEN_A, GREEN_B);
						}
						else if(recv_buff[0] == 'B')
						{
							if(id == C)blink(EARLY_LED_A, EARLY_LED_B, GREEN_A, GREEN_B);
							else if(id == D)blink(MIDDLE_LED_A, MIDDLE_LED_B, GREEN_A, GREEN_B);
							else if(id == A)blink(LATE_LED_A, LATE_LED_B, GREEN_A, GREEN_B);
						}
						else if(recv_buff[0] == 'C')
						{
							if(id == D)blink(EARLY_LED_A, EARLY_LED_B, GREEN_A, GREEN_B);
							else if(id == A)blink(MIDDLE_LED_A, MIDDLE_LED_B, GREEN_A, GREEN_B);
							else if(id == B)blink(LATE_LED_A, LATE_LED_B, GREEN_A, GREEN_B);
						}
						else if(recv_buff[0] == 'D')
						{
							if(id == C)blink(EARLY_LED_A, EARLY_LED_B, GREEN_A, GREEN_B);
							else if(id == B)blink(MIDDLE_LED_A, MIDDLE_LED_B, GREEN_A, GREEN_B);
							else if(id == A)blink(LATE_LED_A, LATE_LED_B, GREEN_A, GREEN_B);
						}
					}
					else
					{ // if it's from android
						blinkInService(GREEN_A, GREEN_B);
					}
				}
			}
			else 
			{
				printf("Bad UDP message received\n");
			}
		}
	}
}

void setnetworkAndLights(networkState setting)
{
	if (setting == NETWORK_ON)
	{
		allowMessagesToBeReceived = true;
		unsigned int currentTime = std::time(0);
		pthread_mutex_lock(&katimes);
		if(doNotHandleKeepAlivesUntil < currentTime)
		{
			// Handle setting lights based on keep-alives
			if(id == A)
			{ // Motion detector
				if ((currentTime - keepAliveTimes[1]) < TOLERANCE)
				{
					digitalWrite(LATE_LED_A, GREEN_A);				
					digitalWrite(LATE_LED_B, GREEN_B);
					lightStatus[LATE]=ON;
				}
				else
				{
					digitalWrite(LATE_LED_A, RED_A);
					digitalWrite(LATE_LED_B, RED_B);
					lightStatus[LATE]=OFF;
				}
				if ((currentTime - keepAliveTimes[2]) < TOLERANCE)
				{
					digitalWrite(MIDDLE_LED_A, GREEN_A);				
					digitalWrite(MIDDLE_LED_B, GREEN_B);
					lightStatus[MIDDLE]=ON;
				}
				else
				{
					digitalWrite(MIDDLE_LED_A, RED_A);
					digitalWrite(MIDDLE_LED_B, RED_B);
					lightStatus[MIDDLE]=OFF;
				}
				if ((currentTime - keepAliveTimes[3]) < TOLERANCE)
				{
					digitalWrite(EARLY_LED_A, GREEN_A);				
					digitalWrite(EARLY_LED_B, GREEN_B);
					lightStatus[EARLY]=ON;				
				}
				else
				{
					digitalWrite(EARLY_LED_A, RED_A);
					digitalWrite(EARLY_LED_B, RED_B);
					lightStatus[EARLY]=OFF;
				}		
			}
			else if(id == B)
			{ // UDP listener for Android phone
				if ((currentTime - keepAliveTimes[0]) < TOLERANCE)
				{
					digitalWrite(EARLY_LED_A, GREEN_A);				
					digitalWrite(EARLY_LED_B, GREEN_B);
					lightStatus[EARLY]=ON;
				}
				else
				{
					digitalWrite(EARLY_LED_A, RED_A);
					digitalWrite(EARLY_LED_B, RED_B);
					lightStatus[EARLY]=OFF;
				}
				if ((currentTime - keepAliveTimes[2]) < TOLERANCE)
				{
					digitalWrite(LATE_LED_A, GREEN_A);				
					digitalWrite(LATE_LED_B, GREEN_B);				
					lightStatus[LATE]=ON;
				}
				else
				{
					digitalWrite(LATE_LED_A, RED_A);
					digitalWrite(LATE_LED_B, RED_B);
					lightStatus[LATE]=OFF;
				}
				if ((currentTime - keepAliveTimes[3]) < TOLERANCE)
				{
					digitalWrite(MIDDLE_LED_A, GREEN_A);				
					digitalWrite(MIDDLE_LED_B, GREEN_B);				
					lightStatus[MIDDLE]=ON;
				}
				else
				{
					digitalWrite(MIDDLE_LED_A, RED_A);
					digitalWrite(MIDDLE_LED_B, RED_B);
					lightStatus[MIDDLE]=OFF;
				}		
			}
			else if(id == C)
			{ // Button press
				if ((currentTime - keepAliveTimes[0]) < TOLERANCE)
				{
					digitalWrite(MIDDLE_LED_A, GREEN_A);				
					digitalWrite(MIDDLE_LED_B, GREEN_B);
					lightStatus[MIDDLE]=ON;
				}
				else
				{
					digitalWrite(MIDDLE_LED_A, RED_A);
					digitalWrite(MIDDLE_LED_B, RED_B);
					lightStatus[MIDDLE]=OFF;
				}
				if ((currentTime - keepAliveTimes[1]) < TOLERANCE)
				{
					digitalWrite(EARLY_LED_A, GREEN_A);				
					digitalWrite(EARLY_LED_B, GREEN_B);				
					lightStatus[EARLY]=ON;
				}
				else
				{
					digitalWrite(EARLY_LED_A, RED_A);
					digitalWrite(EARLY_LED_B, RED_B);
					lightStatus[EARLY]=OFF;
				}
				if ((currentTime - keepAliveTimes[3]) < TOLERANCE)
				{
					digitalWrite(LATE_LED_A, GREEN_A);				
					digitalWrite(LATE_LED_B, GREEN_B);				
					lightStatus[LATE]=ON;
				}
				else
				{
					digitalWrite(LATE_LED_A, RED_A);
					digitalWrite(LATE_LED_B, RED_B);
					lightStatus[LATE]=OFF;
				}		
			}
			else if(id == D)
			{ // Light sensor
				if ((currentTime - keepAliveTimes[0]) < TOLERANCE)
				{
					digitalWrite(LATE_LED_A, GREEN_A);				
					digitalWrite(LATE_LED_B, GREEN_B);
					lightStatus[LATE]=ON;
				}
				else
				{
					digitalWrite(LATE_LED_A, RED_A);
					digitalWrite(LATE_LED_B, RED_B);
					lightStatus[LATE]=OFF;
				}
				if ((currentTime - keepAliveTimes[1]) < TOLERANCE)
				{
					digitalWrite(MIDDLE_LED_A, GREEN_A);				
					digitalWrite(MIDDLE_LED_B, GREEN_B);				
					lightStatus[MIDDLE]=ON;
				}
				else
				{
					digitalWrite(MIDDLE_LED_A, RED_A);
					digitalWrite(MIDDLE_LED_B, RED_B);
					lightStatus[MIDDLE]=OFF;
				}
				if ((currentTime - keepAliveTimes[2]) < TOLERANCE)
				{
					digitalWrite(EARLY_LED_A, GREEN_A);				
					digitalWrite(EARLY_LED_B, GREEN_B);				
					lightStatus[EARLY]=ON;
				}
				else
				{
					digitalWrite(EARLY_LED_A, RED_A);
					digitalWrite(EARLY_LED_B, RED_B);
					lightStatus[EARLY]=OFF;
				}		
			}
		}
		pthread_mutex_unlock(&katimes);
	}
	else
	{
		allowMessagesToBeReceived = false;
		lightStatus[EARLY] = OFF;
		lightStatus[MIDDLE] = OFF;
		lightStatus[LATE] = OFF;
		digitalWrite(EARLY_LED_A, RED_A);
		digitalWrite(EARLY_LED_B, RED_B);
		digitalWrite(MIDDLE_LED_A, RED_A);
		digitalWrite(MIDDLE_LED_B, RED_B);
		digitalWrite(LATE_LED_A, RED_A);
		digitalWrite(LATE_LED_B, RED_B);
	}
}

void* monitorPolling(void* initialValue)
{
	networkState* pOldValue = (networkState*)initialValue;
	networkState oldValue = *pOldValue;
	networkState readValue = *pOldValue;
	unsigned int currentTime = std::time(0); 
	unsigned int mdLastTriggered = 0;
	unsigned int pbLastTriggered = 0;
	unsigned int lsLastTriggered = 0;
	int sending_sockfd = 0;
	struct sockaddr_in sending_client_addr;
	const socklen_t sending_slen = sizeof(struct sockaddr_in);
	char msg[] = "A";

	/// Setup socket
	if ((setup_socket(&sending_sockfd) < 0) || setup_sockaddr_in(&sending_client_addr, &NODE_JS_LISTENING_PORT, LOCAL_ADDRESS) == NULL)
	{
		printf("Monitor polling can't talk to node.js\n");
	}
		
	while(!endThreads)
	{
		// bulk control the lights from the communication cutoff rocker switch
		if(oldValue != readValue)
		{ // if the rocker switch has changed, 
			oldValue = readValue;
			printf("Turning comms %s\n", readValue?"Off":"On");
		}
		setnetworkAndLights(readValue);

		currentTime = std::time(0);			
		if(id == A)
		{ // Motion detector
			if((currentTime - mdLastTriggered) > MD_STANDOFF_TIME)
			{
				if(digitalRead(MOTION_SENSOR_DATA))
				{
					mdLastTriggered = currentTime;
					// Slew Camera
					msg[0] = 'A';
					sendto(sending_sockfd, msg, 1, 0, (struct sockaddr *)&sending_client_addr, sending_slen);
					
					// Inform rest of network
					system("echo -n \"A2\" | sudo alfred -s 64");
					doNotHandleKeepAlivesUntil = std::time(0) + BLINK_STANDOFF;
					blinkInService(GREEN_A, GREEN_B);
				}
			}	
		}
		// skip id == B, as it has it's own UDP listening thread
		else if(id == C)
		{ // Button press
			if((currentTime - pbLastTriggered) > PB_STANDOFF_TIME)
			{
				if(!digitalRead(PUSHBUTTON_A))
				{
					pbLastTriggered = currentTime;
					// Slew Camera
					msg[0] = 'C';
					sendto(sending_sockfd, msg, 1, 0, (struct sockaddr *)&sending_client_addr, sending_slen);
					
					// Inform rest of network
					system("echo -n \"C2\" | sudo alfred -s 64");
					doNotHandleKeepAlivesUntil = std::time(0) + BLINK_STANDOFF;
					blinkInService(GREEN_A, GREEN_B);
				}	
			}
		}
		else if(id == D)
		{ // Light sensor
			if((currentTime - lsLastTriggered) > LS_STANDOFF_TIME)
			{
				if(digitalRead(LIGHT_SENSOR_B))
				{
					lsLastTriggered = currentTime;
					// Slew Camera
					msg[0] = 'D';
					sendto(sending_sockfd, msg, 1, 0, (struct sockaddr *)&sending_client_addr, sending_slen);
					
					// Inform rest of network
					system("echo -n \"D2\" | sudo alfred -s 64");
					doNotHandleKeepAlivesUntil = std::time(0) + BLINK_STANDOFF;
					blinkInService(GREEN_A, GREEN_B);
				}	
			}
		}			
		usleep(1000);
		readValue = (networkState)digitalRead(NETWORK_A);
	}
}


int main()
{
	pthread_mutex_init(&katimes, NULL);
	pthread_mutex_lock(&katimes);
	keepAliveTimes[0]= 0;
	keepAliveTimes[1]= 0;
	keepAliveTimes[2]= 0;
	keepAliveTimes[3]= 0;
	pthread_mutex_unlock(&katimes);
	std::ifstream file("id.txt");
	std::getline(file, id);
	wiringPiSetup();
	pinMode(EARLY_LED_A, OUTPUT);
	pinMode(EARLY_LED_B, OUTPUT);
	pinMode(MIDDLE_LED_A, OUTPUT);
	pinMode(MIDDLE_LED_B, OUTPUT);
	pinMode(LATE_LED_A, OUTPUT);
	pinMode(LATE_LED_B, OUTPUT);
	pinMode(PUSHBUTTON_A, INPUT);
	pullUpDnControl(PUSHBUTTON_A, PUD_UP);
	
	blinkAll(RED_A, RED_B);
	blinkAll(GREEN_A, GREEN_B);
	/*
	blink(EARLY_LED_A, EARLY_LED_B, GREEN_A, GREEN_B);
	blink(MIDDLE_LED_A, MIDDLE_LED_B, GREEN_A, GREEN_B);
	blink(LATE_LED_A, LATE_LED_B, GREEN_A, GREEN_B);

	blink(EARLY_LED_A, EARLY_LED_B, RED_A, RED_B);
	blink(MIDDLE_LED_A, MIDDLE_LED_B, RED_A, RED_B);
	blink(LATE_LED_A, LATE_LED_B, RED_A, RED_B);
	*/
		
	networkState initialValue = (networkState)digitalRead(NETWORK_A);
	
	setnetworkAndLights(initialValue);
	pullUpDnControl(NETWORK_A, PUD_UP);
	pullUpDnControl(MOTION_SENSOR_DATA, PUD_DOWN);

	pthread_t pollingMonitor;
	pthread_create(&pollingMonitor, NULL, monitorPolling, &initialValue);
	printf("Started polling monitor\n");

	pthread_t udpMonitor;

// KDM put this back when done integrating!!!	if(id == B)
	{ // UDP listener for Android phone
		printf("Running profile B\n");
		
		unsigned int port = ANDROID_SERVER_PORT;
		pthread_create(&udpMonitor, NULL, listenForUdp, &port);
		printf("Started UDP monitor\n");
	}
	
	pthread_join(pollingMonitor, NULL);
	
	if(id == B)
	{
		pthread_join(udpMonitor, NULL);
	}
	printf("All done\n");

	return 0;	
}