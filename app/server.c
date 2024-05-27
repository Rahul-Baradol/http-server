#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

char *get_path(char *request);

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting REUSE_PORT
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	
	int client_id = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");
	
	while (1) {
		char request[1024] = {};
		ssize_t size = recv(client_id, request, 1024, 0);
		if (size <= 0) {
			printf("Invalid request\n");
			continue;
		}

		char *path = get_path(request);

		char *response = NULL;		
		if (strcmp(path, "/") == 0) {
			response = "HTTP/1.1 200 OK\r\n\r\n";
		} else if (strncmp(path, "/echo/", 6) == 0) {
			// /echo/abc
			// 9 - 6 = 3
			char *body = path + 6;
			size_t contentLength = strlen(path) - 6;

			char *format = "HTTP/1.1 200 OK\r\n" 
							"Content-Type: text/plain\r\n" 
							"Content-Length: %zu\r\n\r\n%s";

			response = malloc(512);
			sprintf(response, format, contentLength, body);
			printf("%s\n", response);
		} else {
			response = "HTTP/1.1 404 Not Found\r\n\r\n";
		}

		int responseSize = strlen(response) + 1;
		if (write(client_id, response, responseSize) != responseSize) {
			printf("Unable to write to the socket");
		}
		break;
	}

	close(client_id);
	close(server_fd);

	return 0;
}

char *get_path(char *request) {
	// GET /echo/a HTTP/1.1
	// 11 - 3 = 8
	char *start = strchr(request, ' ');
	char *end = strchr(start + 1, ' ');
	int startPos = start - request;
	int endPos = end - request;
	int length = endPos - startPos - 1;
	char *path = malloc(length);
	memcpy(path, start + 1, length);
	path[length] = '\0';
	return path;
}