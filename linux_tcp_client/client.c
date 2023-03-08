/*
   단순 server에게 메시지만을 보내고 받는 코드
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define MAXLINE	1024
#define PORT	54321

//#define SERVER_IP	"127.0.0.1"	/* for local test */
#define SERVER_IP      "192.168.0.85"	/* usb wifi */

#define IF_DEV_ETH0	"eth0"
#define IF_DEV_WLAN0	"wlan0"
#define IF_DEV_WLAN1	"wlan1"

char tx_tcp_msg[MAXLINE];

/* tcp client thread */
void* client_thread(void *arg)
{
	struct sockaddr_in server_addr;
	int server_sock;
	struct ifreq ifr;
	int cli_len;
	char recv_buf[MAXLINE] = {0};
	int recv_len;

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("error :");
		return 0;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(PORT);

	cli_len = sizeof(server_addr);

#if 1
	/* You can bind to a specific interface by setting SO_BINDTODEVICE socket option. */
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), IF_DEV_WLAN0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	//if (setsockopt(server_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&ifr, sizeof(ifr)) < 0) {
		printf("error: can't bind to a specific interface: %d\n", errno);
		exit(1);
	}

	ioctl(server_sock, SIOCGIFADDR, &ifr);
	/* inet_ntoa(): convert integer ip address to string */
	printf("if: %s, ip: %s\n", ifr.ifr_name, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
#endif


	printf("%s: start connect\n", __func__);
	/* connect is a blocking call by default, but you can make it non blocking by passing to socket the SOCK_NONBLOCK flag. */
	if (connect(server_sock, (struct sockaddr *)&server_addr, cli_len) == -1) {
		printf("%s: connect error\n", __func__);
		return 0;
	}
	printf("%s: end connect\n", __func__);

	struct pollfd fds[1];
	fds[0].fd = server_sock;
	fds[0].events = POLLIN | POLLOUT;

	static int seqNo = 1;
	while (1) {
		int n = poll(fds, 1, 1000);
		for (int i = 0; i < n; i++) {
			if (fds[i].revents & POLLIN) {
				// 서버에서 온 echo 메시지
				memset(recv_buf, 0, MAXLINE);
				recv_len = recv(server_sock, recv_buf, sizeof(tx_tcp_msg), 0);	/* only recv write size */
				if (recv_len == -1) {
					perror(strerror(errno));
				} else {
					printf("%s\n", recv_buf);
				}
				fflush(stdin);
			}

			if (fds[i].revents & POLLOUT) {
				sprintf((char*)tx_tcp_msg, "message@server %d\n", seqNo++);
				int ret = send(server_sock, tx_tcp_msg, strlen(tx_tcp_msg), 0);
				if (ret == -1) {
					perror(strerror(errno));
				} else {
					if (ret == 0) {
						perror(strerror(errno));
						exit(-1);
					}
				}
				fflush(stdout);
			}
		}
	}
}


int main(void)
{
	pthread_t	th;

	/* start tcp client pthread */
	if (pthread_create(&th, NULL, client_thread, "tcp client thread") < 0) {
		perror("Could not create tcp client thread");
		return -1;
	}

	/* wait pthread stop */
	pthread_join(th, NULL);

	exit(0);
}
