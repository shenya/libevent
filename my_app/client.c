#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>


//#include <event.h>

#define SERVER_PORT 12345
#define SERVER_ADDR "127.0.0.1"


int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
	int sock = -1;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
        perror("error to create socket");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)))
	{
        perror("Fail to connect");
		return -1;
	}
	
    char buf[1024] = {0};
    int count = 0;
	while (1)
	{
        count = read(STDIN_FILENO, buf, 1024);
		if (count < 0)
		{
            perror("failed to read");
			return -1;
		}
		else
		{
		    buf[count - 1] = '\0';
            printf("read count[%d] from stdin\n", count);
			count = write(sock, buf, count);
			if (count < 0)
			{
				perror("failed to write");
				return -1;
			}

			count = read(sock, buf, 1024);
			if (count > 0)
			{
                buf[count] = '\0';
				printf("read data: [%s] from server\n", buf);
			}
			else if (count < 0)
			{
                printf("read error\n");
				break;
			}
			else
			{
                printf("server is closed\n");
				break;
			}
		}
	}

	close(sock);
    return 0;
}
