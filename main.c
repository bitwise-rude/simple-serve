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

char *html_from_file(char *fileName){
	FILE *fp = fopen(fileName,"r");

	if (fp == NULL){
		printf("Some error opening the file");
		exit(0);
	}
	
	// dynamically reading the file
	size_t capacity = 4096;
	size_t size = 0;

	char *data = malloc(capacity);
	
	if (!data){
		printf("Memory Allocation Failed");
		exit(0);
	}


	size_t n;

	while ((n = fread(data + size, 1, 1024, fp)) > 0 ){
		size +=n;
		if (size + 1024 > capacity){
			capacity *=2;
			data = realloc(data, capacity);

			if (!data)
			{
				printf("Memory Allocation Failed");
				exit(0);
			}
		}		
	}
	data[size] = '\0';
	return data;
}

int main(int argc, char *argv[]){
	// analyze arguments
	// main -file <file>
	// main text
	
	if (argc < 2){
		printf("Incorrect Usage:\n");
		exit(0);
	}

	int server_fd = create_server(INADDR_ANY,PORT);
	int client_fd = accept(server_fd, NULL, NULL);

	char *req = recv_data(client_fd);
	
	char *data_to_send;

	if ((argc == 3)&&(strcmp(argv[1],"-filename")==0)){
		data_to_send = html_from_file(argv[2]);
		}

	else{

		data_to_send = malloc(strlen(argv[1])+1);
		strcpy(data_to_send,argv[1]);

	}

	int result = send_data(client_fd,data_to_send,strlen(data_to_send));
	
	free(data_to_send);	

	free(req);	
	close(client_fd);
	close(server_fd);
}
