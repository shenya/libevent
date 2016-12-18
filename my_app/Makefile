
LD_FLAG = -levent
CFLAGS = -g

app: main.c client.c
	gcc -o app main.c $(LD_FLAG) $(CFLAGS)
	gcc -o client client.c $(CFLAGS)

clean:
	rm app client
