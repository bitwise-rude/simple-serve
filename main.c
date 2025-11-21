 // main.c

#include "main.h"

int main(){
	int server_fd = socket(AF_INET, SOCK_STREAM,0);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
	
	bind(server_fd,(struct sockaddr *)&addr, sizeof(addr)); 
	listen(server_fd,1);
	
	int client_fd = accept(server_fd, NULL, NULL);

	char buffer[1024];
	int n = read(client_fd, buffer, sizeof(buffer)-1);
	buffer[n] = '\0';
	
	printf("RESPOSE: %s\n",buffer);
	
	close(client_fd);
	close(server_fd);
}
