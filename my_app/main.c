#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <event.h>
#include <event2/event.h>

#define PORT 12345
#define BACKLOG 6

#define MAX_DATA_LEN 1024

struct socket_data
{
    struct event *read_ev;
    struct event *write_ev;
    char *data_buf;
    int count;
};

struct event_base *base;

void release_socket_data(struct socket_data *sock_data)
{
    if (!sock_data)
    {
        return;
    }

    if (sock_data->read_ev)
    {
        free(sock_data->read_ev);
    }

    if (sock_data->write_ev)
    {
        free(sock_data->write_ev);
    }

    if (sock_data->data_buf)
    {
        free(sock_data->data_buf);
    }

    free(sock_data);
}

//void (*callback)(evutil_socket_t, short, void *)
void on_write(evutil_socket_t sock_fd, short event, void *arg)
{
    int ret = 0;

    char *buf = (char *)arg;
    if (!buf)
    {
	    printf("%s: buf is NULL\n", __FUNCTION__);
        return;
    }

    ret = write(sock_fd, buf, strlen(buf));
    if (ret < 0)
    {
        printf("%s: send failed\n", __FUNCTION__);
    }

    free(buf);
}

//void (*callback)(evutil_socket_t, short, void *)
void on_read(evutil_socket_t sock_fd, short event, void *arg)
{
    printf("read data\n");
    struct event *client_ev = NULL;
    char read_buf[1024] = {0};
    int read_count = 0;
    struct socket_data *ev_data = NULL;

    ev_data = (struct socket_data*)arg;
    if (NULL == ev_data)
    {
        printf("arg is NULL\n");
        return;
    }
    client_ev = ev_data->read_ev;
    if (NULL == client_ev)
    {
        printf("%s: arg is NULL\n", __FUNCTION__);
        return;
	}

    ev_data->data_buf = malloc(MAX_DATA_LEN);
    if (NULL == ev_data->data_buf)
    {
	    printf("malloc data buf failed\n");
        goto on_error;
    }

    read_count = read(sock_fd, ev_data->data_buf, MAX_DATA_LEN);
    if (0 == read_count)
    {
        printf("client close\n");
        event_del(client_ev);
        close(sock_fd);
        release_socket_data(ev_data);
        return;
    }
    else if (read_count < 0)
    {
        printf("read error\n");
        event_del(client_ev);
        close(sock_fd);
        release_socket_data(ev_data);
        return;
    }
    else
    {
        ev_data->data_buf[read_count] = '\0';
        ev_data->count = read_count;
        printf("read data: [%s] from client\n", ev_data->data_buf);

        event_set(ev_data->write_ev, sock_fd, EV_WRITE, on_write,
        ev_data->data_buf);
        event_base_set(base, ev_data->write_ev);
        event_add(ev_data->write_ev, NULL);
    }

on_error:
    return;
}

//void (*bufferevent_data_cb)(struct bufferevent *bev, void *ctx);
void buf_ev_read_cb(struct bufferevent *bev, void *arg)
{
    struct evbuffer *input = NULL;
    struct evbuffer *output = NULL;
    char buf[1024] = {0};
    int ret = 0;
    size_t input_len = 0;
    size_t output_len = 0;

    printf("%s: hello world\n", __FUNCTION__);

    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);
    if (NULL == input || NULL == output)
    {
        printf("%s: invalid I/O buffer\n", __FUNCTION__);
        return;
    }
    input_len = evbuffer_get_length(input);
    printf("input_len: %d\n", input_len);
    output_len = evbuffer_get_length(output);
    printf("output_len: %d\n", output_len);
    ret = evbuffer_remove(input,buf, 1024);
    if (ret < 0)
    {
        return;
    }
    printf("Read buf: %s\n", buf);

    evbuffer_add(output, buf, strlen(buf));

    input_len = evbuffer_get_length(input);
    printf("after r/w: input_len: %d\n", input_len);
    output_len = evbuffer_get_length(output);
    printf("after r/w: output_len: %d\n", output_len);
}

//void (*bufferevent_event_cb)(struct bufferevent *bev, short what, void *ctx);
void buf_ev_error_cb(struct bufferevent *bev, short what, void *ctx)
{
    if (what & BEV_EVENT_EOF)
    {
        printf("connection closed\n");
    }
    else if (what & BEV_EVENT_ERROR)
    {
        printf("unknown error\n");
    }
    else if (what & BEV_EVENT_TIMEOUT)
    {
        printf("Timed out\n");
    }

    printf("%s: error cb: event[%d]\n", __FUNCTION__, what);

    bufferevent_free(bev);
}


#if 1
//void (*callback)(evutil_socket_t, short, void *)
void on_accept(evutil_socket_t sock_fd, short event, void *arg)
{
    printf("accept new client\n");
    struct sockaddr_in cli_addr;
    int addr_len = 0;
    int new_fd = 0;
    int ret = -1;

    addr_len = sizeof(cli_addr);
    new_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &addr_len);
    if (new_fd < 0)
    {
        perror("fail to accept");
        return;
    }

    struct bufferevent *client_bufevent = NULL;

    client_bufevent = bufferevent_socket_new(base, new_fd, BEV_OPT_CLOSE_ON_FREE);
    if (NULL == client_bufevent)
    {
        printf("bufferevent socket new failed\n");
        return;
    }

    bufferevent_setcb(client_bufevent, buf_ev_read_cb, NULL, buf_ev_error_cb, NULL);
    ret = bufferevent_enable(client_bufevent, EV_READ | EV_PERSIST);
    if (ret < 0)
    {
        printf("bufferent enable failed\n");
        goto on_error;
    }

    return;
on_error:
    bufferevent_free(client_bufevent);
}

#else
//void (*callback)(evutil_socket_t, short, void *)
void on_accept(evutil_socket_t sock_fd, short event, void *arg)
{
    printf("accept new client\n");
    struct sockaddr_in cli_addr;
    int addr_len = 0;
    int new_fd = 0;

    struct socket_data *ev_data = NULL;

    addr_len = sizeof(cli_addr);
    new_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &addr_len);
    if (new_fd < 0)
    {
        perror("fail to accept");
        return;
    }

    ev_data = (struct socket_data *)malloc(sizeof(struct socket_data));
    if (NULL == ev_data)
    {
        printf("Fail to malloc ev_data\n");
        return;
    }
    memset(ev_data, 0, sizeof(struct socket_data));

    ev_data->read_ev = (struct event *)malloc(sizeof(struct event));
    ev_data->write_ev = (struct event *)malloc(sizeof(struct event));
    if (NULL == ev_data->read_ev || NULL == ev_data->write_ev)
    {
        printf("fail to malloc read/write event\n");
        goto on_error;
    }
    memset(ev_data->read_ev, 0, sizeof(struct event));
    memset(ev_data->write_ev, 0, sizeof(struct event));

    event_set(ev_data->read_ev, new_fd, EV_READ | EV_PERSIST,
            on_read, ev_data/*event_self_cbarg()/*client_ev*/);
    event_base_set(base, ev_data->read_ev);
    event_add(ev_data->read_ev, NULL);

    return;
on_error:
    //release data
    release_socket_data(ev_data);
}
#endif

//void (*callback)(evutil_socket_t, short, void *)
void on_signal_int_cb(evutil_socket_t sock_fd, short event, void *arg)
{
    printf("Signal int is received\n");
    exit(1);
}

//void (*bufferevent_data_cb)(struct bufferevent *bev, void *ctx);
void buf_ev_listen_cb(struct bufferevent *bev, void *arg)
{
    printf("%s: hello world\n", __FUNCTION__);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    int sock = -1;
    int flag = 1;
    const char **basenames = NULL;
    int i = 0;
    char varbuf[128];
    int ret = -1;

    /* get version */
    printf("libevent version: %s\n", event_get_version());
    printf("libevent version number: %x\n", event_get_version_number());

    basenames = event_get_supported_methods();
    for (i = 0; basenames[i]; ++i) {
        printf("method: %s\n", basenames[i]);
    }

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
    struct event *signal_ev;

    base = event_base_new();
    if (NULL == base)
    {
        printf("create base failed\n");
        return -1;
    }

    printf("Base method: %s\n", event_base_get_method(base));
#if 1
    event_set(&listen_ev, sock, EV_READ | EV_PERSIST, on_accept, NULL);
    event_base_set(base, &listen_ev);
    event_add(&listen_ev, NULL);
#else
    struct bufferevent *listen_bufevent = NULL;

    listen_bufevent = bufferevent_socket_new(base, sock, BEV_OPT_CLOSE_ON_FREE);
    if (NULL == listen_bufevent)
    {
        printf("bufferevent socket new failed\n");
        return -1;
    }

    bufferevent_setcb(listen_bufevent, buf_ev_listen_cb, NULL, NULL, NULL);
    ret = bufferevent_enable(listen_bufevent, EV_READ | EV_PERSIST);
    if (ret < 0)
    {
        printf("bufferent enable failed\n");
        return -1;
    }
#endif

    signal_ev = evsignal_new(base, SIGINT, on_signal_int_cb, NULL);
    if (NULL == signal_ev)
    {
        printf("evsignal new SIGINT failed\n");
        return -1;
    }
    event_add(signal_ev, NULL);

    event_base_dispatch(base);
    printf("Start to study at 2016-12-17\n");

    return 0;
}
