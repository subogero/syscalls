/* A command-line multi-user chat program using a client-server model.
 * Uses select() besides the normal socket related system calls.
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

int s;
static int run_server(void);
static int run_client(void);

int main(int argc, char *argv[])
{
	/* Get server/host and port from command line */
	char server = 0;
	if (argc < 3) {
		write(2, "Usage: chatcs -s|<host> <port>\n", 34);
		return 1;
	}
	if (strncmp(argv[1], "-s", 3) == 0) {
		server = 1;
	}
	/* Prepare socket */
	struct addrinfo hints, *srv;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;     /* IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP socket */
	if (server) hints.ai_flags = AI_PASSIVE;     /* Get IP address */
	if (getaddrinfo(server ? NULL : argv[1], argv[2], &hints, &srv)) {
		write(2, "addrinfo error\n", 15);
		return 2;
	}
	s = socket(srv->ai_family, srv->ai_socktype, srv->ai_protocol);
	if (s < 0) {
		write(2, "socket creation failed\n", 23);
		freeaddrinfo(srv);
		return 3;
	}
	if (server && bind(s, srv->ai_addr, srv->ai_addrlen) < 0) {
		write(2, "could not bind to host\n", 23);
		freeaddrinfo(srv);
		close(s);
		return 4;
	}
	freeaddrinfo(srv);
	/* Got the open socket, do my thing */
	return server ? run_server()
	              : run_client();
}

static int run_server(void)
{
	/* Start listening for new conections */
	char line[256];
	int fd_max = s;
	fd_set all_fds, read_fds;
	FD_SET(s, &all_fds);
	if (listen(s, 5) < 0) {
		write(2, "listen error\n", 13);
		return 5;
	}
	/* Main loop */
	while (1) {
		static int users = 0;
		/* Wait for incoming data on listener or connections */
		read_fds = all_fds;
		if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) < 0) {
			write(2, "select error\n", 13);
			return 6;
		}
		/* New connection */
		if (FD_ISSET(s, &read_fds)) {
			struct sockaddr_storage cadr;
			socklen_t clen = sizeof cadr;
			int fd = accept(s, (struct sockaddr *)&cadr, &clen);
			if (fd < 0) {
				write(2, "accept error\n", 13);
				continue;
			}
			else {
				users++;
				write(2, "new connection\n", 15);
				FD_SET(fd, &all_fds);
				if (fd > fd_max) fd_max = fd;
				sprintf(line, "%d user(s) chatting.\n", users);
				write(fd, line, strlen(line));
			}
		}
		/* Existing connections: read, forward to others */
		int i;
		for (i = 3; i <= fd_max; ++i) {
			if (i == s || !FD_ISSET(i, &read_fds))
				continue;
			int length = read(i, line, 256);
			/* Close connection if necessary */
			if (length <= 0) {
				users--;
				write(2, "connection closed\n", 18);
				close(i);
				FD_CLR(i, &all_fds);
			}
			/* Copy line to all connections except sender */
			int j;
			for (j = 3; j <= fd_max; ++j) {
				if (j == s || j == i || !FD_ISSET(j, &all_fds))
					continue;
				write(j, line, length);
			}
		}
	}
}

static int run_client(void)
{
	return 0;
}
