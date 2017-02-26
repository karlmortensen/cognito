
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>

/*
 * gcc -I. -o server server.c socket.c
 */

char SERVER_IP[] = "192.168.42.217";
unsigned short SERVER_PORT = 5555;
const unsigned int BUFFER_LEN = 512;

int setup_socket(int *sockfd)
{
        if ((*sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
                return (-1);
        }

        return *sockfd;
}

struct sockaddr_in *setup_sockaddr_in(struct sockaddr_in *addr, uint16_t *port, char *addr_string)
{

        memset((char *)addr, 0, sizeof(struct sockaddr_in));

        addr->sin_family = AF_INET;
        if (port != NULL) {
                addr->sin_port = htons(*port);
        }
        if (addr_string == NULL) {
                addr->sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
                if (inet_aton(addr_string, &addr->sin_addr) == 0) {
                        fprintf(stderr, "inet_aton() failed\n");
                        return (NULL);
                }
        }

        return (addr);
}

int bind_socket(int sockfd, struct sockaddr_in *addr)
{

        if (bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr)) == -1) {
                fprintf(stderr, "bind() failed\n");
                return (-1);
        }
        return (0);
}

int send_data_raw(int sockfd, char buffer[], unsigned int buffer_length, struct sockaddr_in *addr,
                  const socklen_t slen)
{
        if (sendto(sockfd, buffer, buffer_length, 0, (struct sockaddr *)addr, slen) < 0) {
                fprintf(stderr, "sendto() failed\n");
                return (-1);
        }
        return (0);
}

int recv_data_raw(int sockfd, char buffer[], int *recv_len, unsigned int buffer_length,
                  struct sockaddr_in *addr, socklen_t slen)
{
        memset(buffer, 0, buffer_length);

        if ((*recv_len = recvfrom
             (sockfd, buffer, buffer_length, 0, (struct sockaddr *)addr, &slen)) < 0) {
                fprintf(stderr, "recvfrom() failed\n");
                return (-1);
        }
        return *recv_len;
}

void listenForUdp()
{
	int sockfd = 0;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	const socklen_t slen = sizeof(struct sockaddr_in);

	char recv_buff[BUFFER_LEN] = { 0 };
	int recv_len = 0;
	char send_buff[BUFFER_LEN] = { 0 };

	printf("SERVER\n");

	/// Setup socket
	if (setup_socket(&sockfd) < 0) 
	{
		return;
	}
	/// Setup server addr struct
	if (setup_sockaddr_in(&server_addr, &SERVER_PORT, SERVER_IP) == NULL)
	{
		return;
	}
	/// Bind SERVER_PORT to address structure on sockfd
	if (bind_socket(sockfd, &server_addr)) 
	{
		return;
	}

	while (true)
	{
		recv_data_raw(sockfd, recv_buff, &recv_len, BUFFER_LEN, &client_addr, slen);
		printf("Received packet from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		printf("Data: %s\n", recv_buff);
		//send_data_raw(sockfd, recv_buff, recv_len, &client_addr, slen);
		system((std::string("node slewCameraTo.js ") + recv_buff).c_str());
	}
	return;
}

int main(int argc, char **argv)
{
	listenForUdp();
}
