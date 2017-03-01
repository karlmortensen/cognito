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

char SERVER_IP[] = "192.168.86.89";
//KDM char SERVER_IP[] = "192.168.42.217";
unsigned short ANDROID_SERVER_PORT = 5555;
const unsigned int BUFFER_LEN = 512;
bool allowMessagesToBeReceived = true;
unsigned int keepAliveTimes[4];
pthread_mutex_t katimes;
unsigned int TOLERANCE = 3;
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

networkState valueChanged = NETWORK_OFF;
bool endThreads = false;

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

int send_data_raw(int sockfd, char buffer[], unsigned int buffer_length, struct sockaddr_in *addr, const socklen_t slen)
{
	if (sendto(sockfd, buffer, buffer_length, 0, (struct sockaddr *)addr, slen) < 0) 
	{
		fprintf(stderr, "sendto() failed\n");
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
	char send_buff[BUFFER_LEN] = { 0 };

	/// Setup socket
	if (setup_socket(&sockfd) < 0) 
	{
		return NULL;
	}

	/// Setup server addr struct
	if (setup_sockaddr_in(&server_addr, &port, SERVER_IP) == NULL)
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
			/*
			printf("received %d bytes: ", recv_len);
			for(int i=0; i < recv_len; ++i)
			{
				printf("0x%02X ", recv_buff[i]);
			}
			printf("\n");
			*/
			if (recv_len > 1)
			{
				if (recv_buff[1] != '2') //KDM
				{ // Opcode 2, slew camera to node
					//KDM system((std::string("node slewCameraTo.js ") + recv_buff[0]).c_str());
					printf("would have run: %s\n",(std::string("node slewCameraTo.js ") + recv_buff[0]).c_str());
				}
				else
				{ // Opcode 1 (or anything other than 2), register a keep-alive
					// printf("just got a keepalive for 0x%02X\n", recv_buff[0]);
					pthread_mutex_lock(&katimes);
					keepAliveTimes[recv_buff[0] - 'A'] = std::time(0);
					pthread_mutex_unlock(&katimes);
					printf("keepalive times = \n\t%d\n\t%d\n\t%d\n\t%d\n", keepAliveTimes[0], keepAliveTimes[1], keepAliveTimes[2], keepAliveTimes[3]);
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
		//system("sudo ip link set wlan0 up");
		allowMessagesToBeReceived = true;
		// KDM note this should be saved for when we get a packet from that link. This is just an example
		unsigned int currentTime = std::time(0);
		pthread_mutex_lock(&katimes);

		if(id == A)
		{ // Motion detector
			if ((currentTime - keepAliveTimes[1]) < TOLERANCE)
			{
				digitalWrite(LATE_LED_A, GREEN_A);				
				digitalWrite(LATE_LED_B, GREEN_B);
			}
			else
			{
				digitalWrite(LATE_LED_A, RED_A);
				digitalWrite(LATE_LED_B, RED_B);
			}
			if ((currentTime - keepAliveTimes[2]) < TOLERANCE)
			{
				digitalWrite(MIDDLE_LED_A, GREEN_A);				
				digitalWrite(MIDDLE_LED_B, GREEN_B);				
			}
			else
			{
				digitalWrite(MIDDLE_LED_A, RED_A);
				digitalWrite(MIDDLE_LED_B, RED_B);
			}
			if ((currentTime - keepAliveTimes[3]) < TOLERANCE)
			{
				digitalWrite(EARLY_LED_A, GREEN_A);				
				digitalWrite(EARLY_LED_B, GREEN_B);				
			}
			else
			{
				digitalWrite(EARLY_LED_A, RED_A);
				digitalWrite(EARLY_LED_B, RED_B);
			}		
		}
		else if(id == B)
		{ // UDP listener for Android phone
			if ((currentTime - keepAliveTimes[0]) < TOLERANCE)
			{
				printf("%d - %d = %d\n",currentTime, keepAliveTimes[0], (currentTime - keepAliveTimes[0]));
				digitalWrite(EARLY_LED_A, GREEN_A);				
				digitalWrite(EARLY_LED_B, GREEN_B);
			}
			else
			{
				printf("%d - %d = %d\n",currentTime, keepAliveTimes[0], (currentTime - keepAliveTimes[0]));
				digitalWrite(EARLY_LED_A, RED_A);
				digitalWrite(EARLY_LED_B, RED_B);
			}
			if ((currentTime - keepAliveTimes[2]) < TOLERANCE)
			{
				digitalWrite(LATE_LED_A, GREEN_A);				
				digitalWrite(LATE_LED_B, GREEN_B);				
			}
			else
			{
				digitalWrite(LATE_LED_A, RED_A);
				digitalWrite(LATE_LED_B, RED_B);
			}
			if ((currentTime - keepAliveTimes[3]) < TOLERANCE)
			{
				digitalWrite(MIDDLE_LED_A, GREEN_A);				
				digitalWrite(MIDDLE_LED_B, GREEN_B);				
			}
			else
			{
				digitalWrite(MIDDLE_LED_A, RED_A);
				digitalWrite(MIDDLE_LED_B, RED_B);
			}		
		}
		else if(id == C)
		{ // Button press
			if ((currentTime - keepAliveTimes[0]) < TOLERANCE)
			{
				digitalWrite(MIDDLE_LED_A, GREEN_A);				
				digitalWrite(MIDDLE_LED_B, GREEN_B);
			}
			else
			{
				digitalWrite(MIDDLE_LED_A, RED_A);
				digitalWrite(MIDDLE_LED_B, RED_B);
			}
			if ((currentTime - keepAliveTimes[1]) < TOLERANCE)
			{
				digitalWrite(EARLY_LED_A, GREEN_A);				
				digitalWrite(EARLY_LED_B, GREEN_B);				
			}
			else
			{
				digitalWrite(EARLY_LED_A, RED_A);
				digitalWrite(EARLY_LED_B, RED_B);
			}
			if ((currentTime - keepAliveTimes[3]) < TOLERANCE)
			{
				digitalWrite(LATE_LED_A, GREEN_A);				
				digitalWrite(LATE_LED_B, GREEN_B);				
			}
			else
			{
				digitalWrite(LATE_LED_A, RED_A);
				digitalWrite(LATE_LED_B, RED_B);
			}		
		}
		else if(id == D)
		{ // Light sensor
			if ((currentTime - keepAliveTimes[0]) < TOLERANCE)
			{
				digitalWrite(LATE_LED_A, GREEN_A);				
				digitalWrite(LATE_LED_B, GREEN_B);
			}
			else
			{
				digitalWrite(LATE_LED_A, RED_A);
				digitalWrite(LATE_LED_B, RED_B);
			}
			if ((currentTime - keepAliveTimes[1]) < TOLERANCE)
			{
				digitalWrite(MIDDLE_LED_A, GREEN_A);				
				digitalWrite(MIDDLE_LED_B, GREEN_B);				
			}
			else
			{
				digitalWrite(MIDDLE_LED_A, RED_A);
				digitalWrite(MIDDLE_LED_B, RED_B);
			}
			if ((currentTime - keepAliveTimes[2]) < TOLERANCE)
			{
				digitalWrite(EARLY_LED_A, GREEN_A);				
				digitalWrite(EARLY_LED_B, GREEN_B);				
			}
			else
			{
				digitalWrite(EARLY_LED_A, RED_A);
				digitalWrite(EARLY_LED_B, RED_B);
			}		
		}
		pthread_mutex_unlock(&katimes);
	}
	else
	{
		// system("sudo ip link set wlan0 down");
		allowMessagesToBeReceived = false;
		digitalWrite(EARLY_LED_A, RED_A);
		digitalWrite(EARLY_LED_B, RED_B);
		digitalWrite(MIDDLE_LED_A, RED_A);
		digitalWrite(MIDDLE_LED_B, RED_B);
		digitalWrite(LATE_LED_A, RED_A);
		digitalWrite(LATE_LED_B, RED_B);
	}
}

void* monitornetworkPolling(void* initialValue)
{
	networkState* pOldValue = (networkState*)initialValue;
	networkState oldValue = *pOldValue;
	networkState readValue = *pOldValue;
	
	int x=0;
	while(!endThreads)
	{
		// bulk control the lights
		if(oldValue != readValue)
		{
			oldValue = readValue;
			printf("Setting network to %s\n", readValue?"Off":"On");
			setnetworkAndLights(readValue);
		}
		readValue = (networkState)digitalRead(NETWORK_A);
		usleep(200000);		
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
	networkState initialValue = (networkState)digitalRead(NETWORK_A);

	setnetworkAndLights(initialValue);
	pullUpDnControl(NETWORK_A, PUD_UP);

	pthread_t networkMonitor;
	pthread_create(&networkMonitor, NULL, monitornetworkPolling, &initialValue);
	printf("Started network monitor\n");

	if(id == A)
	{ // Motion detector
		printf("Running profile A\n");
	}
	else if(id == B)
	{ // UDP listener for Android phone
		printf("Running profile B\n");
		pthread_t udpMonitor;
		unsigned int port = ANDROID_SERVER_PORT;
		pthread_create(&udpMonitor, NULL, listenForUdp, &port);
		printf("Started UDP monitor\n");
	}
	else if(id == C)
	{ // Button press
		printf("Running profile C\n");
	}
	else if(id == D)
	{ // Light sensor
		printf("Running profile D\n");
	}
	else
	{
		printf("No personality profile!\n");
		return 0;
	}
	
	pthread_join(networkMonitor, NULL);
	// pthread_join(udpMonitor, NULL);
	printf("YEAH!!!\n");

	return 0;	
}
