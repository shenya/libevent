#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <event.h>
#include <event2/event.h>

#define PORT 12345
#define BACKLOG 6


struct event_base *base;

//void (*callback)(evutil_socket_t, short, void *)
void on_read(evutil_socket_t sock_fd, short event, void *arg)
{
    printf("read data\n");
	struct event *client_ev = NULL;
	char read_buf[1024] = {0};
	int read_count = 0;

	client_ev = (struct event *)arg;
	if (NULL == client_ev)
	{
        printf("%s: arg is NULL\n", __FUNCTION__);
		return;
	}

	read_count = read(sock_fd, read_buf, 1024);
	if (0 == read_count)
	{
        printf("client close\n");
		event_del(client_ev);
		free(client_ev);
		close(sock_fd);
		return;
	}
	else if (read_count < 0)
	{
        printf("read error\n");
		return;
	}
	else
	{
	    read_buf[read_count] = '\0';
        printf("read data: %s", read_buf);
	}
}

//void (*callback)(evutil_socket_t, short, void *)
void on_accept(evutil_socket_t sock_fd, short event, void *arg)
{
    printf("accept new client\n");
	struct sockaddr_in cli_addr;
	int addr_len = 0;
	int new_fd = 0;

	struct event *client_ev = NULL;

    addr_len = sizeof(cli_addr);
	new_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &addr_len);
	if (new_fd < 0)
	{
	    perror("fail to accept");
		return;
	}

	client_ev = (struct event *)malloc(sizeof(struct event));
	if (NULL == client_ev)
	{
        printf("fail to malloc");
		return;
	}
	event_set(client_ev, new_fd, EV_READ | EV_PERSIST,
			on_read, event_self_cbarg()/*client_ev*/);
	event_base_set(base, client_ev);
	event_add(client_ev, NULL);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
	int sock = -1;
	int flag = 1;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
        perror("error to create socket");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
	{
        perror("error to setsockopt reuse");
		return -1;
	}
	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)))
	{
        perror("error to bind");
		return -1;
	}

	if (listen(sock, BACKLOG))
	{
        perror("error to listen");
		return -1;
	}

	struct event listen_ev;
	
    base = event_base_new();
	if (NULL == base)
	{
        printf("create base failed\n");
		return -1;
	}

	event_set(&listen_ev, sock, EV_READ | EV_PERSIST, on_accept, NULL);
	event_base_set(base, &listen_ev);
	event_add(&listen_ev, NULL);
	event_base_dispatch(base);
    printf("Start to study at 2016-12-17\n");

    return 0;
}
