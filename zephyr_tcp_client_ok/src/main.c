/*
 * Zephyr OS tcp/ipv4 client test program
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(my_tcp_client_demo, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/linker/sections.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_conn_mgr.h>
#include <zephyr/net/socket.h>

#include <stdio.h>

#define USER_STACKSIZE	2048

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
		    NET_EVENT_L4_DISCONNECTED)

#define PEER_PORT	54321

#define RECV_BUF_SIZE	128

struct k_thread user_thread_tcp_w;
K_THREAD_STACK_DEFINE(user_stack_tcp_w, USER_STACKSIZE);

static struct net_mgmt_event_callback mgmt_cb;
static bool connected;





/* 물리연결 체크 */
static void event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("%s: Network connected", __func__);
		connected = true;
		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		if (connected == false) {
			LOG_INF("%s: Waiting network to be connected", __func__);
		} else {
			LOG_INF("%s: Network disconnected", __func__);
			connected = false;
		}
		return;
	}
}

#if 0
static void user_tcp_thread_func(void *p1, void *p2, void *p3)
{
	char tx_tcp_msg[RECV_BUF_SIZE];
	char recv_buf[RECV_BUF_SIZE] = {0};
	int recv_len;
	ssize_t out_len;
	static int seqNo = 1;

	while (1) {
		int n = poll(fds, 1, 1000);
		for (int i = 0; i < n; i++) {
			if (fds[i].revents & POLLOUT) {
				sprintf((char *)tx_tcp_msg, "message@server %d\n", seqNo++);
				out_len = send(fds[i].fd, tx_tcp_msg, strlen(tx_tcp_msg), 0);
				if (out_len == -1) {
					printf("can't send: %d\n", errno);
				}
			}

			if (fds[i].revents & POLLIN) {
				memset(recv_buf, 0, sizeof(recv_buf));
				recv_len = recv(fds[i].fd, recv_buf, sizeof(recv_buf), 0);
				if (recv_len == -1) {
					printf("can't recv: %d\n", errno);
				} else {
					printf("%s\n", recv_buf);
				}
			}
		}
	}
}
#endif



int main(void)
{
	int ret = 0;
	char recv_buf[RECV_BUF_SIZE] = {0};
	int recv_len;
	ssize_t out_len;
	static int seqNo = 1;

	int sock;
	struct sockaddr_in addr4;

	printk("Start TCP client on %s\n", CONFIG_BOARD);

	/* 물리연결 체크 */
	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		net_mgmt_init_event_callback(&mgmt_cb, event_handler, EVENT_MASK);
		net_mgmt_add_event_callback(&mgmt_cb);
		net_conn_mgr_resend_status();
	}

	addr4.sin_family = AF_INET;				/* ipv4 */
	addr4.sin_port = htons(PEER_PORT);			/* 4242 */
	inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,	/* Server ip */
		  &addr4.sin_addr);

	/* create socket */
	sock = socket(addr4.sin_family, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("Failed to create TCP socket: %d", errno);
		return -errno;
	}

	/* connect to the server */
	ret = connect(sock, (struct sockaddr *)&addr4, sizeof(addr4));
	if (ret < 0) {
		LOG_ERR("Cannot connect to TCP remote: %d", errno);
		ret = -errno;
	}

	/*  */
	struct pollfd fds[1];
	fds[0].fd = sock;
	fds[0].events = POLLIN | POLLOUT;

#if 0
	k_tid_t tcp_thread;
	/*  */
	tcp_thread = k_thread_create(&user_thread_tcp_w, user_stack_tcp_w, USER_STACKSIZE,
			user_tcp_thread_func, NULL, NULL, NULL,
			//-1, K_USER, K_MSEC(0));		/* USER mode thread */
			-1, K_INHERIT_PERMS, K_MSEC(0));	/* SVC mode thread */
#endif
	char tx_tcp_msg[RECV_BUF_SIZE];

	while (1) {
		if (!connected) {
			printf("TCP cable disconnected %d, Exit!\n", errno);
			break;
		}

		int n = poll(fds, 1, 1000);
		for (int i = 0; i < n; i++) {
			if (fds[i].revents & POLLIN) {
				memset(recv_buf, 0, sizeof(recv_buf));
				recv_len = recv(fds[i].fd, recv_buf, sizeof(recv_buf), 0);
				if (recv_len == -1) {
					printf("can't recv: %d\n", errno);
				} else {
					printf("%s\n", recv_buf);
				}
			}
			if (fds[i].revents & POLLOUT) {
				sprintf((char *)tx_tcp_msg, "message@server %d\n", seqNo++);
				out_len = send(fds[i].fd, tx_tcp_msg, strlen(tx_tcp_msg), 0);
				if (out_len == -1) {
					printf("can't send: %d\n", errno);
				}
				memset(tx_tcp_msg, 0, sizeof(tx_tcp_msg));	/* FIXME: */
			}
		}
	}

	return 0;
}
