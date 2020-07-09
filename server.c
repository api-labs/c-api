#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define PORT 8000

struct thread_args {
	int socket;
};

void* handle_request(void *args) {
	struct thread_args *socket_args = args;
	int socket = socket_args->socket;
	long valread;

	char buffer[30000] = {0};
	valread = read(socket, buffer, 30000);
	char *save_ptr_1, *save_ptr_2, *save_ptr_3;
	char *request_line = strtok_r(buffer, "\n", &save_ptr_1);
	char *request_method = strtok_r(request_line, " ", &save_ptr_2);
	if (strncmp(request_method, "GET", 3) != 0)
	{
		perror("Method not implemented");
		exit(EXIT_FAILURE);
	}
	char *request_target = strtok_r(NULL, " ", &save_ptr_2);
	if (request_target[0] != '/')
	{
		perror("Doesn't seem to be a valid URL");
		exit(EXIT_FAILURE);
	}

	if (strlen(request_target) == 1) 
	{
		char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 0\n\n";
		write(socket, response, strlen(response));
		close(socket);
		free(socket_args);

		return NULL;
	}

	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	unsigned long time_recv = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

	char *request_query = strtok_r(request_target, "?", &save_ptr_3);
	request_query = strtok_r(NULL, "&", &save_ptr_3);
	char *message;
	char *time_sent;
	char *id;
	while (request_query != NULL)
	{
		char *save_ptr_4 = NULL;
		char *key = strtok_r(request_query, "=", &save_ptr_4);
		char *val = strtok_r(NULL, "\0", &save_ptr_4);

		if (strncmp(key, "id", 2) == 0)
		{
			id = val;
		}
		else if (strncmp(key, "payload", 7) == 0)
		{
			message = val;
		}
		else if (strncmp(key, "time_sent", 9) == 0)
		{
			time_sent = val;
		}
		else if (strncmp(key, "sleep_intv", 10) == 0)
		{
			sleep(atoi(val) / 1000.0);
		}

		request_query = strtok_r(NULL, "&", &save_ptr_3);
	}

	int length = snprintf(NULL, 0, "{\"id\": \"%s\", \"time_sent\": %s, \"message\": \"%s\", \"time_recv\": %lu}", id, time_sent, message, time_recv);
	char *payload = malloc(length + 1);
	snprintf(payload, length + 1, "{\"id\": \"%s\", \"time_sent\": %s, \"message\": \"%s\", \"time_recv\": %lu}", id, time_sent, message, time_recv);

	length = snprintf(NULL, 0, "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: %lu\n\n%s", strlen(payload), payload);
	char *response = malloc(length + 1);
	snprintf(response, length + 1, "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: %lu\n\n%s", strlen(payload), payload);
	write(socket, response, strlen(response));
	close(socket);
	free(response);
	free(payload);
	free(socket_args);

	return NULL;
}

int main(int agrc, char const *argv[])
{
	int server_fd, new_socket;
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("In socket");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	memset(address.sin_zero, '\0', sizeof address.sin_zero);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("In bind");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 10) < 0)
	{
		perror("In listen");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
		{
			perror("In accept");
			exit(EXIT_FAILURE);
		}
		struct thread_args *args;
		args = malloc(sizeof(struct thread_args));
		args->socket = new_socket;

		pthread_t thread_id;
		pthread_create(&thread_id, NULL, handle_request, args);
	}

	return 0;
}