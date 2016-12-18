
LD_FLAG = -levent
CFLAGS = -g

app: server client
server: main.c
	gcc -o server main.c $(LD_FLAG) $(CFLAGS)
client: client.c
	gcc -o client client.c $(CFLAGS)

clean:
	rm app cli
