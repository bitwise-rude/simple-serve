 // main.c

#include "main.h"

int create_server(int address, int port){
	int server_fd = socket(AF_INET, SOCK_STREAM,0);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = address;
	
	bind(server_fd,(struct sockaddr *)&addr, sizeof(addr)); 
	listen(server_fd,1);

	return server_fd;
}

char *recv_data(int client_fd)
{
	char *buffer = malloc(1024);

	int n = read(client_fd, buffer, sizeof(buffer)-1);
	buffer[n] = '\0';
	return buffer;
}

int main(){
	int server_fd = create_server(INADDR_ANY,PORT);
	int client_fd = accept(server_fd, NULL, NULL);

	char *buffer = recv_data(client_fd);
	
	printf("RESPOSE: %s\n",buffer);

	free(buffer);	
	close(client_fd);
	close(server_fd);
}
