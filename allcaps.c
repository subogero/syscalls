/* allcaps is a network server listening on a port specified on command line.
 * It forks a new allcaps process for each incoming connection.
 * Connection established, it echoes incoming packets in all-caps format.
 *
 * Starting server: allcaps <port> & disown
 * Client:          nc <host> <port>
 *                  telnet <host> <port>
 *
 * Further info: http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

static void capitalize(int s);

int main(int argc, char *argv[])
{
	/* Check command line arguments */
	if (argc < 2) {
		write(2, "port invalid\n", 13);
		return 1;
	}
	if (argc >= 3) {
		int logfd = open(argv[2], O_CREAT | O_WRONLY | O_APPEND, 0644);
		if (logfd < 0) {
			write(2, "unable to open logfile\n", 23);
		}
		else {
			close(2);
			dup(logfd);
			close(logfd);
		}
	}

	/* Prepare and start listening on socket */
	struct addrinfo hints, *srv;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;     /* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP socket */
	hints.ai_flags = AI_PASSIVE;     /* Get IP address */
	if (getaddrinfo(NULL, argv[1], &hints, &srv)) {
		write(2, "addrinfo error\n", 15);
		return 2;
	}
	int s = socket(srv->ai_family, srv->ai_socktype, srv->ai_protocol);
	if (s < 0) {
		write(2, "socket creation failed\n", 23);
		freeaddrinfo(srv);
		return 3;
	}
	if (bind(s, srv->ai_addr, srv->ai_addrlen) < 0) {
		write(2, "could not bind to host\n", 23);
		freeaddrinfo(srv);
		return 4;
	}
	freeaddrinfo(srv);
	if (listen(s, 5) < 0) {
		write(2, "listen error\n", 13);
		return 5;
	}

	/* Wait for new connections forever */
	write(2, "server waiting for connections\n", 31);
	while (1) {
		struct sockaddr_storage client_addr;
		unsigned int addr_size = sizeof client_addr;
		int s_new = accept(s, (struct sockaddr *)&client_addr, &addr_size);
		if (s_new < 0) {
			write(2, "accept error\n", 13);
			continue;
		}
		/* Handle each new connection in a child server process */
		pid_t child = fork();
		if (child < 0) {
			write(2, "fork error\n", 11);
			continue;
		}
		/* Parent reaps zombie children for closed connections 
		 * only upon a new connection. I know, I know... */
		else if (child) {
			write(2, "forked child for new connection\n", 32);
			close(s_new);
			int child_status;
			waitpid(-1, &child_status, WNOHANG);
			/* The parent never returns */
		}
		/* Child does the actual communication */
		else {
			write(2, "new connection\n", 15);
			close(s);
			capitalize(s_new);
			close(s_new);
			return 0;
		}
	}
	return 0;
}

static void capitalize(int s)
{
	char line[80];
	while (1) {
		int received = read(s, line, 80);
		if (received == 0) {
			write(2, "connection closed\n", 18);
			break;
		}
		else if (received < 0) {
			write(2, "read error\n", 11);
			continue;
		}
		int i = 0;
		while (1) {
			if (!line[i] || i >= 80) break;
			if (line[i] >= 'a' && line[i] <= 'z') line[i] -= 0x20;
			i++;
		}
		write(s, line, received);
	}
}
