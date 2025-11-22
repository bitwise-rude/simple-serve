// main.h 

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8060

#define HTTP_RESPONSE_TEMPLATE_HEADER "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length:"
#define HTTP_RESPONSE_TEMPLATE_FOOTER "\r\n\r\n"
#define HTTP_RESPONSE_TEMPLATE_LEN (strlen(HTTP_RESPONSE_TEMPLATE_HEADER) + strlen(HTTP_RESPONSE_TEMPLATE_FOOTER))

#define HTTP_VIDEO_STREAM "HTTP/1.1 200 OK\r\nContent-Type: video/mp4\r\nTransfer-Encoding: chunked\r\n\r\n"
#define CHUNK_SIZE 1024

typedef int bool;
#define True 1
#define False 0

typedef struct BinaryFile {
	char *data;
	int len;
} Binary;

int create_server(int address, int port);
char *recv_data(int client_fd);
void send_data(int client_fd, char *data,int len);
char *text_from_file(char *filename);
void send_data_stream(int client_fd, char *data, int len);
void raw_from_file(char *filename, Binary *raw);
void send_stream(int client_fd, char *data, int len);

