#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define SERVER_PORT 9001
#define MAX_BUFF_SIZE 4096
#define MAX_BACK_LOG 1024

/*
 * wait for child process to change state
 */
static void sigchld_handler(int sig)
{
	int status = 0;
	do {
		if (waitpid(-1, &status, WNOHANG) == -1) {
			perror("Fail to wait child process");
			exit(-1);
		}
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
}

int handle_request(int client_fd)
{
	char buff[MAX_BUFF_SIZE];
	int size = 0;
	while ((size = read(client_fd, buff, MAX_BUFF_SIZE)) != 0) {
		if (size == -1) {
			if (errno == EINTR) {
				continue;
			}

			perror("Fail to recv from client");
			return errno;
		}
	}

	printf("recv msg from client_%d: %s\n", client_fd, buff);
	return 0;
}

int main()
{
	int server_fd = 0;
	int conn_fd = 0;
	struct sockaddr_in servaddr;
	struct sockaddr_in clientaddr;
	socklen_t addr_len;
	char buff[MAX_BUFF_SIZE];
	int n = 0;

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERVER_PORT);
	if (inet_pton(AF_INET, "0.0.0.0", &servaddr.sin_addr) <= 0) {
		perror("inet_pton error");
		exit(-1);
	}

	/* make port reusable */
	int on = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) <
	    0) {
		perror("setsockopt");
		exit(-1);
	}

	if (bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) ==
	    -1) {
		perror("bind error");
		exit(-1);
	}

	if (listen(server_fd, MAX_BACK_LOG) == -1) {
		perror("listen error");
		exit(-1);
	}

	// register SIGCHLD handler
	struct sigaction chld_action;
	chld_action.sa_handler = sigchld_handler;
	chld_action.sa_flags = SA_NODEFER;
	if (sigaction(SIGCHLD, &chld_action, NULL) == -1) {
		perror("Oh! Can not catch SIGCHLD signal. :) ");
		exit(-1);
	}

	printf("server running...listen port %d\n", SERVER_PORT);

	while (1) {
		if ((conn_fd = accept(server_fd, (struct sockaddr *)&clientaddr,
				      &addr_len)) == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("accept error");
			continue;
		}

		pid_t pid = 0;
		if ((pid = fork()) != -1) {
			perror("fork error");
			exit(-1);
		}

		if (pid == 0) {
			int handle_res = 0;
			handle_res = handle_request(conn_fd);

			if (close(conn_fd) == -1) {
				perror("Fail to close client socket.");
				exit(-1);
			}

			return handle_res;
		}

		/*
		* Child process and parent process share thesocket fd.
		* Parent process should close the fd, leave the child process
		* to handle request.
		*/
		if (close(conn_fd) == -1) {
			perror("Fail to close client socket.");
			exit(-1);
		}
	}

	if (close(server_fd) == -1) {
		perror("Fail to close socket");
		exit(-1);
	}
	return 0;
}
