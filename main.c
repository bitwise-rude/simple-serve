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

	if (!buffer)
	{
		printf("Memory Allocation failed:");
		exit(1);
	}

	int n = read(client_fd, buffer, 1024-1);
	buffer[n] = '\0';
	return buffer;
}

int send_data(int client_fd, char *data,int len)
{
	char len_in_string[10] = "";
	sprintf(len_in_string,"%d",len);
	
	char *response = malloc(strlen(len_in_string)+len+HTTP_RESPONSE_TEMPLATE_LEN+1); 
	
	if (!response)
	{
		printf("Memory Allocation Failed:");
		exit(1);
	}

	response[0] = '\0'; // making this a string type
	
	
	strcat(response,HTTP_RESPONSE_TEMPLATE_HEADER);
	strcat(response,len_in_string);
	strcat(response,HTTP_RESPONSE_TEMPLATE_FOOTER);
	strcat(response,data);
	
	printf("%s",response);

	send(client_fd,response,strlen(len_in_string)+len+HTTP_RESPONSE_TEMPLATE_LEN,0); 

	free(response);
	return 1;
}

int main(){
	int server_fd = create_server(INADDR_ANY,PORT);
	int client_fd = accept(server_fd, NULL, NULL);

	char *req = recv_data(client_fd);

	
	int result = send_data(client_fd,"hello from server",strlen("hello from server"));
		

	free(req);	
	close(client_fd);
	close(server_fd);
}
