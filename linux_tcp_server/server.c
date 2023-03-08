#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* for strncpy */
#include <unistd.h>	/* for close */
#include <errno.h>
#include <arpa/inet.h>	/* for inet_ntoa */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <pthread.h>
#include <poll.h>

#define IF_DEV_ETH0	"eth0"
#define IF_DEV_WLAN1	"wlan1"

#define MAXLINE	1024
#define PORT	54321	/* */

char tx_tcp_msg[MAXLINE];



/*  */
int main(int argc, char** argv)
{
	int fd;
	struct sockaddr_in server_addr;	/* */
	struct sockaddr_in client_addr;
	static int counter;
	struct ifreq ifr;
	int out_len;

	socklen_t client_addr_len = sizeof(client_addr);
	char addr_str[32] = {0};
	int client;

	char recv_buf[128];
	int send_len, recv_len;

	/*
	 * 1. create TCP socket
	 *	AF_INET		: ipv4
	 *	SOCK_STREAM	: TCP
	 *	0		: default?
	 */
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("error: socket: %d\n", errno);
		exit(1);
	}

	/*
	 * 2.1 bind socket
	 */
	server_addr.sin_family = AF_INET;			/* ip v4 */
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);	/* */
	server_addr.sin_port = htons(PORT);

	if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("error: bind: %d\n", errno);
		exit(1);
	}

	/* 2.2 You can bind to a specific interface by setting SO_BINDTODEVICE socket option. */
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), IF_DEV_WLAN1);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* setsockopt() 위에 설명 참조.*/
	//if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&ifr, sizeof(ifr)) < 0) {
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&ifr, sizeof(ifr)) < 0) {
		printf("error: can't bind to a specific interface: %d\n", errno);
		exit(1);
	}

#if 1
	ioctl(fd, SIOCGIFADDR, &ifr);
	/* inet_ntoa(): convert integer ip address to string */
	printf("if: %s, ip: %s\n", ifr.ifr_name, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
#endif

	/*
	 * 3. listen
	 * the listen() function turns the file descriptor of bind into a listening socket
	 */
	if (listen(fd, 5) < 0) {
		printf("error: listen: %d\n", errno);
		exit(1);
	}


	printf("%s: TCP server waits for a connection on port %d...\n", __func__, PORT);

	static int seqNo = 1;
	while (1) {
		client = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);
		if (client < 0) {
			printf("error: accept: %d\n", errno);
			continue;
		}

		inet_ntop(client_addr.sin_family, &client_addr.sin_addr, addr_str, sizeof(addr_str));
		printf("Connection #%d from %s\n", counter++, addr_str);

		struct pollfd fds[1];
		fds[0].fd = client;
		fds[0].events = POLLIN | POLLOUT;

		while (1) {
			int n = poll(fds, 1, 1000);
			for (int i = 0; i < n; i++) {
				if (fds[i].revents & POLLOUT) {
					sprintf((char*)tx_tcp_msg, "message@client %d\n", seqNo++);
					send_len = send(fds[i].fd, tx_tcp_msg, strlen(tx_tcp_msg), 0);
					if (send_len == -1) {
						perror(strerror(errno));
					} else {
						if (send_len == 0) {
							perror(strerror(errno));
							goto error;
						}
					}
					fflush(stdout);
				}

				if (fds[i].revents & POLLIN) {
					memset(recv_buf, 0, sizeof(recv_buf));
					recv_len = recv(fds[i].fd, recv_buf, sizeof(tx_tcp_msg), 0);	/* only recv write size */
					if (recv_len == -1) {
						perror(strerror(errno));
					} else {
						if (recv_len == 0) {
							strerror(errno);
							goto error;
						}
						printf("%s\n", recv_buf);
					}
					fflush(stdin);
				}
			}
		}
	}

error:
	close(client);
	close(fd);
	printf("Connection from %s closed\n", addr_str);

	return 0;
}
