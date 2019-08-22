all:
	$(CC) socket_server.c -lpthread -o socket_server
	arm-linux-gcc socket_client.c -lpthread -o socket_client
	arm-linux-gcc socket_client_while_send.c -lpthread -o socket_client_while_send
	cp socket_server socket_client socket_client_while_send ~/NFS -af
clean:
	rm socket_server socket_client socket_client_while_send