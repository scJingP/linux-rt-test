all:
	$(CC) can/socket_can.c test.c -lpthread -o test
	$(CC) can/socket_can.c testd.c -lpthread -o testd
	cp test testd ~/NFS -af
clean:
	rm test
