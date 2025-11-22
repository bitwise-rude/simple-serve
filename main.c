// main.c - Improved version with fixes

#include "main.h"
#include <errno.h>
#include <limits.h>

int create_server(int address, int port){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_fd == -1){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = address;
    
    // SO_REUSEADDR to avoid TIME_WAIT issues
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        perror("Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1){  // Increased backlog
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

char *recv_data(int client_fd)
{
    char *buffer = malloc(10024);

    if (!buffer)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    ssize_t n = read(client_fd, buffer, 10023);  // Leave room for null terminator
    
    if (n < 0) {
        perror("Read failed");
        free(buffer);
        return NULL;
    }
    
    if (n == 0) {
        printf("Client closed connection\n");
        free(buffer);
        return NULL;
    }
    
    buffer[n] = '\0';
    printf("Received %zd bytes\n", n);
    return buffer;
}

void send_data(int client_fd, char *data, int len)
{
    char len_in_string[32];
    snprintf(len_in_string, sizeof(len_in_string), "%d", len);
    
    size_t response_len = strlen(HTTP_RESPONSE_TEMPLATE_HEADER) + 
                          strlen(len_in_string) + 
                          strlen(HTTP_RESPONSE_TEMPLATE_FOOTER) + 
                          len;
    
    char *response = malloc(response_len + 1);
    
    if (!response)
    {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    response[0] = '\0';
    
    strcat(response, HTTP_RESPONSE_TEMPLATE_HEADER);
    strcat(response, len_in_string);
    strcat(response, HTTP_RESPONSE_TEMPLATE_FOOTER);
    strncat(response, data, len);
    
    ssize_t sent = send(client_fd, response, strlen(response), 0);
    
    if (sent < 0) {
        perror("Send failed");
    } else {
        printf("Sent %zd bytes\n", sent);
    }

    free(response);
}


bool parse_range(const char *request, size_t *start, size_t *end, size_t file_size) {
    const char *range_header = strstr(request, "Range: bytes=");
    if (!range_header) {
        return False;
    }
    
    range_header += 13; 
    
    char *dash = strchr(range_header, '-');
    if (!dash) {
        return False;
    }
    
    *start = atoi(range_header);
    
    if (*(dash + 1) != '\r' && *(dash + 1) != '\n' && *(dash + 1) != '\0') {
        *end = atoi(dash + 1);
    } else {
        *end = file_size - 1;
    }
    
    return True;
}

void send_stream(int client_fd, char *data, int len)
{
    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: video/mp4\r\n"
             "Content-Length: %d\r\n"
             "Accept-Ranges: bytes\r\n"
             "Connection: close\r\n"
             "\r\n", len);
    
    ssize_t sent = send(client_fd, header, strlen(header), 0);
    
    if (sent < 0) {
        perror("Failed to send stream header");
        return;
    }
    
    printf("Sent stream header: %zd bytes\n", sent);

    size_t total_sent = 0;
    while (total_sent < (size_t)len) {
        size_t chunk_size = (len - total_sent > CHUNK_SIZE) ? CHUNK_SIZE : len - total_sent;
        
        ssize_t n = send(client_fd, data + total_sent, chunk_size, 0);
        
        if (n < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                printf("Client closed connection after %zu bytes\n", total_sent);
            } else {
                perror("Failed to send chunk data");
            }
            return;
        }
        
        if (n == 0) {
            printf("Connection closed by client after %zu bytes\n", total_sent);
            return;
        }
        
        total_sent += n;
        
    }

    printf("Stream completed: %zu bytes sent\n", total_sent);
}

void send_stream_with_range(int client_fd, char *data, int len, const char *request)
{
    size_t start = 0;
    size_t end = len - 1;
    bool has_range = parse_range(request, &start, &end, len);
    
    if (start >= (size_t)len) {
        start = 0;
        end = len - 1;
        has_range = False;
    }
    
    if (end >= (size_t)len) {
        end = len - 1;
    }
    
    size_t content_length = end - start + 1;
    
    char header[512];
    if (has_range) {
        snprintf(header, sizeof(header),
                 "HTTP/1.1 206 Partial Content\r\n"
                 "Content-Type: video/mp4\r\n"
                 "Content-Length: %zu\r\n"
                 "Content-Range: bytes %zu-%zu/%d\r\n"
                 "Accept-Ranges: bytes\r\n"
                 "Connection: keep-alive\r\n"
                 "\r\n", content_length, start, end, len);
    } else {
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: video/mp4\r\n"
                 "Content-Length: %zu\r\n"
                 "Accept-Ranges: bytes\r\n"
                 "Connection: close\r\n"
                 "\r\n", content_length);
    }
    
    ssize_t sent = send(client_fd, header, strlen(header), 0);
    
    if (sent < 0) {
        perror("Failed to send header");
        return;
    }
    
    printf("Sending range %zu-%zu (%zu bytes)\n", start, end, content_length);

    size_t total_sent = 0;
    size_t position = start;
    
    while (total_sent < content_length) {
        size_t remaining = content_length - total_sent;
        size_t chunk_size = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        
        ssize_t n = send(client_fd, data + position, chunk_size, 0);
        
        if (n < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                printf("Client closed connection after %zu bytes\n", total_sent);
            } else {
                perror("Failed to send data");
            }
            return;
        }
        
        if (n == 0) {
            printf("Connection closed by client\n");
            return;
        }
        
        total_sent += n;
        position += n;
    }

    printf("Range request completed: %zu bytes sent\n", total_sent);
}

bool is_safe_path(const char *path) {
    char resolved[PATH_MAX];
    
    if (realpath(path, resolved) == NULL) {
        return False;
    }
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return False;
    }
    
    return strncmp(resolved, cwd, strlen(cwd)) == 0;
}

char *text_from_file(char *fileName){
    if (!is_safe_path(fileName)) {
        fprintf(stderr, "Access denied: invalid path\n");
        return NULL;
    }

    FILE *fp = fopen(fileName, "r");

    if (fp == NULL){
        perror("Failed to open file");
        return NULL;
    }
    
    size_t capacity = 4096;
    size_t size = 0;

    char *data = malloc(capacity);
    
    if (!data){
        perror("Memory allocation failed");
        fclose(fp);
        return NULL;
    }

    size_t n;

    while ((n = fread(data + size, 1, 1024, fp)) > 0) {
        size += n;
        if (size + 1024 > capacity) {
            capacity *= 2;
            char *new_data = realloc(data, capacity);

            if (!new_data) {
                perror("Memory reallocation failed");
                free(data);
                fclose(fp);
                return NULL;
            }
            data = new_data;
        }
    }
    
    data[size] = '\0';
    fclose(fp);
    return data;
}

void raw_from_file(char *fileName, Binary *raw){
    if (!is_safe_path(fileName)) {
        fprintf(stderr, "Access denied: invalid path\n");
        raw->data = NULL;
        raw->len = 0;
        return;
    }

    FILE *fp = fopen(fileName, "rb");

    if (fp == NULL){
        perror("Failed to open file");
        raw->data = NULL;
        raw->len = 0;
        return;
    }
    
    size_t capacity = 4096;
    size_t size = 0;

    char *data = malloc(capacity);
    
    if (!data){
        perror("Memory allocation failed");
        fclose(fp);
        raw->data = NULL;
        raw->len = 0;
        return;
    }

    size_t n;

    while ((n = fread(data + size, 1, 1024, fp)) > 0) {
        size += n;
        if (size + 1024 > capacity) {
            capacity *= 2;
            char *new_data = realloc(data, capacity);

            if (!new_data) {
                perror("Memory reallocation failed");
                free(data);
                fclose(fp);
                raw->data = NULL;
                raw->len = 0;
                return;
            }
            data = new_data;
        }
    }
    
    raw->data = data;
    raw->len = size;

    fclose(fp);
}

int main(int argc, char *argv[]){
    if (argc < 2){
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s <text>              - Serve text directly\n", argv[0]);
        fprintf(stderr, "  %s -filename <file>    - Serve HTML file\n", argv[0]);
        fprintf(stderr, "  %s -video <file>       - Stream video file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Starting server on port %d...\n", PORT);
    int server_fd = create_server(INADDR_ANY, PORT);
    
    printf("Server listening. Press Ctrl+C to stop.\n");
    
    
    while (1) {
        printf("\nWaiting for connection...\n");
        int client_fd = accept(server_fd, NULL, NULL);
        
        if (client_fd < 0) {
            perror("Accept failed");
            continue;  // next connection
        }
        
        printf("Client connected!\n");

        char *req = recv_data(client_fd);
        
        if (!req) {
            close(client_fd);
            continue;
        }
        
        printf("Request:\n%.200s...\n", req);  // Print first 200 chars

        if (argc == 3) {
            if (strcmp(argv[1], "-filename") == 0) {
                char *data_to_send = text_from_file(argv[2]);
                
                if (data_to_send) {
                    printf("Serving file: %s\n", argv[2]);
                    send_data(client_fd, data_to_send, strlen(data_to_send));
                    free(data_to_send);
                } else {
                    char *error_msg = "HTTP/1.1 404 Not Found\r\n\r\nFile not found";
                    send(client_fd, error_msg, strlen(error_msg), 0);
                }
            }
            else if (strcmp(argv[1], "-video") == 0) {
                Binary raw;
                raw_from_file(argv[2], &raw);
                
                if (raw.data) {
                    printf("Streaming video: %s (%d bytes)\n", argv[2], raw.len);
                    send_stream_with_range(client_fd, raw.data, raw.len, req);
                    free(raw.data);
                } else {
                    char *error_msg = "HTTP/1.1 404 Not Found\r\n\r\nFile not found";
                    send(client_fd, error_msg, strlen(error_msg), 0);
                }
            }
        }
        else {
            printf("Serving text directly\n");
            send_data(client_fd, argv[1], strlen(argv[1]));
        }

        free(req);
        close(client_fd);
        printf("Connection closed\n");
    }

    close(server_fd);
    
    printf("Server shutting down\n");
    return 0;
}
