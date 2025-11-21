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

void send_data(int client_fd, char *data,int len)
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

	send(client_fd,response,strlen(response),0); 

	free(response);
}


void send_stream(int client_fd, char *data,int len)
{
	// sending the header first	
	int sent = send(client_fd,HTTP_VIDEO_STREAM,strlen(HTTP_VIDEO_STREAM),0);
	printf("starting: \n%s\nsend%d\n",HTTP_VIDEO_STREAM,sent	);

	int read = 0;
	char len_in_string[20] = "";

	while (read<len){
		size_t chunk_size = (len - read > CHUNK_SIZE)? CHUNK_SIZE:len-read;

		sprintf(len_in_string,"%zx\r\n",chunk_size); // %x for hex
		send(client_fd, len_in_string, strlen(len_in_string),0);
		send(client_fd, data+read, chunk_size, 0);
		send(client_fd, "\r\n",2,0);
		
		read += chunk_size;
	}	

	// final
	send(client_fd,"0\r\n\r\n",5,0);
}

char *text_from_file(char *fileName){
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
	fclose(fp);
	return data;
}

void raw_from_file(char *fileName, Binary *raw){
	FILE *fp = fopen(fileName,"rb");

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
	
	raw->data = data;
	raw->len = size;

	fclose(fp);
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

	if (argc == 3){
		if (strcmp(argv[1],"-filename")==0){
			char *data_to_send = text_from_file(argv[2]);
		
			printf("About to Serve the page.. Please wait\n");
			send_data(client_fd,data_to_send,strlen(data_to_send));
			free(data_to_send);		
		}

		
		if (strcmp(argv[1],"-video")==0){
			Binary raw;
			raw_from_file(argv[2],&raw);
			send_stream(client_fd,raw.data,raw.len);
			free(raw.data);
		}

	}

	else{

		char *data_to_send = malloc(strlen(argv[1])+1);
		strcpy(data_to_send,argv[1]);

		printf("About to Serve the page.. Please wait\n");
		send_data(client_fd,data_to_send,strlen(data_to_send));
		free(data_to_send);	
	}

	free(req);	
	close(client_fd);
	close(server_fd);
}
