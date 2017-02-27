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

char SERVER_IP[] = "192.168.42.217";
unsigned short SERVER_PORT = 5555;
const unsigned int BUFFER_LEN = 512;

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

void * listenForUdp(void* initialValue)
{
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
	if (setup_sockaddr_in(&server_addr, &SERVER_PORT, SERVER_IP) == NULL)
	{
		return NULL;
	}

	/// Bind SERVER_PORT to address structure on sockfd
	if (bind_socket(sockfd, &server_addr)) 
	{
		return NULL;
	}

	while(!endThreads)
	{
		recv_data_raw(sockfd, recv_buff, &recv_len, BUFFER_LEN, &client_addr, slen);
		system((std::string("node slewCameraTo.js ") + recv_buff).c_str());
		printf("Looping for UDP\n");
	}
}

void setnetworkAndLights(networkState setting)
{
	if (setting == NETWORK_ON)
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

void* monitornetworkPolling(void* initialValue)
{
	networkState* pOldValue = (networkState*)initialValue;
	networkState oldValue = *pOldValue;
	networkState readValue = *pOldValue;
	
	int x=0;
	while(!endThreads)
	{
		if(oldValue != readValue)
		{
			oldValue = readValue;
			//printf("Setting network to %s\n", readValue?"Off":"On");
			setnetworkAndLights(readValue);
		}
		readValue = (networkState)digitalRead(NETWORK_A);
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
	networkState initialValue = (networkState)digitalRead(NETWORK_A);

	setnetworkAndLights(initialValue);
	pullUpDnControl(NETWORK_A, PUD_UP);

	pthread_t networkMonitor;
	pthread_create(&networkMonitor, NULL, monitornetworkPolling, &initialValue);
	printf("Started network monitor\n");
	
	pthread_t udpMonitor;
	pthread_create(&udpMonitor, NULL, listenForUdp, &initialValue); // doesn't use the initialValue
	printf("Started UDP monitor\n");
	
	pthread_join(networkMonitor, NULL);
	pthread_join(udpMonitor, NULL);
	printf("YEAH!!!\n");
	return 0;	
}
