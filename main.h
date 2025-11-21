// main.h 

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define HTTP_RESPONSE_TEMPLATE_HEADER "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length:"
#define HTTP_RESPONSE_TEMPLATE_FOOTER "\r\n\r\n"
#define HTTP_RESPONSE_TEMPLATE_LEN (strlen(HTTP_RESPONSE_TEMPLATE_HEADER) + strlen(HTTP_RESPONSE_TEMPLATE_FOOTER))

int create_server(int address, int port);
char *recv_data(int client_fd);
int send_data(int client_fd, char *data,int len);
char *html_from_file(char *filename);
