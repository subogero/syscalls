/* A command-line multi-user chat program using a client-server model.
 * Uses select() besides the normal socket related system calls.
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

int s;

static int run_server(void);

static int run_client(void);
static void char_term(int on);

static void sig_term(int signal);
static void help(void);

/* Parse command line for server-mode or hostname and port.
 * Create bound (server) or connected (client) socket "s".
 * Then call client or server.
 */
int main(int argc, char *argv[])
{
	/* Get server/host and port from command line */
	char server = 0;
	if (strncmp(argv[1], "-h", 3) == 0) {
		help();
		return 0;
	}
	if (argc < 3) {
		write(2, "Usage: chatcs -s|<host> <port> | -h\n", 39);
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
	if (!server && connect(s, srv->ai_addr, srv->ai_addrlen) < 0) {
		write(2, "could not connect to host\n", 26);
		freeaddrinfo(srv);
		close(s);
		return 4;
	}
	freeaddrinfo(srv);
	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	/* Got the open socket, do my thing */
	return server ? run_server()
	              : run_client();
}

/* Server: listen on socket "s" for new connections.
 * Maintain existing connections in fd_set "all_fds".
 * Wait for incoming text on all connections with select().
 * Copy incoming data with user-specific colouring to all other connections.
 */
static int run_server(void)
{
	/* Start listening for new conections */
	char line[65536];
	char line_in[65536];
	int fd_max = s;
	fd_set all_fds, read_fds, new_fds, color_fds;
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
				FD_SET(fd, &new_fds);
				if (fd > fd_max) fd_max = fd;
			}
		}
		/* Existing connections: read, forward to others */
		int i;
		for (i = 3; i <= fd_max; ++i) {
			if (i == s || !FD_ISSET(i, &read_fds))
				continue;
			int length = read(i, line_in, 65527);
			/* Colors if magic "\a<" in 1st message */
			if (FD_ISSET(i, &new_fds)) {
				FD_CLR(i, &new_fds);
				if (strncmp(line_in, "\a<", 2) == 0)
					FD_SET(i, &color_fds);
			}
			/* Close connection if necessary */
			if (length <= 0) {
				users--;
				write(2, "connection closed\n", 18);
				close(i);
				FD_CLR(i, &all_fds);
				FD_CLR(i, &color_fds);
			}
			/* Colorize line if necessary */
			if (FD_ISSET(i, &color_fds)) {
				sprintf(line, "\033[%dm", 31 + i%6);
				memcpy(line + 5, line_in, length);
				memcpy(line + 5 + length, "\033[0m", 4);
				length += 9;
			}
			/* Copy line to all connections except sender */
			int j;
			for (j = 3; j <= fd_max; ++j) {
				if (j == s || j == i || !FD_ISSET(j, &all_fds))
					continue;
				write(j,
				      FD_ISSET(i, &color_fds) ? line : line_in,
				      length);
			}
		}
	}
}

/* Client: get username from environment or interactively.
 * Disable terminal line-buffering for immediate sending of typed characters.
 * Wait for incoming text on both stdin and socket with select().
 * Print socket data to stdout.
 * Copy data from stdin to socket, add user-name prefix if terminal.
 */
static int run_client(void)
{
	/* Get user name, but only if stdin is a terminal */
	char username[20] = "\a<";
	if (!isatty(0)) {
		username[0] = 0;
	}
	else {
		char *user = getenv("USER");
		if (user) {
			strcat(username, user);
		}
		else {
			write(1, "enter user name: ", 17);
			int len = read(0, username + 2, 16);
			username[len + 1] = 0;
		}
		strcat(username, ">  ");
	}
	/* Prepare multiplexed reading from socket and stdin */
	char line[65536];
	int fd_max = s;
	fd_set all_fds, read_fds;
	FD_SET(s, &all_fds);
	FD_SET(0, &all_fds);
	char_term(1);
	while (1) {
		read_fds = all_fds;
		if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) < 0) {
			write(2, "select error\n", 13);
			char_term(0);
			return 6;
		}
		/* New data from server */
		if (FD_ISSET(s, &read_fds)) {
			int length = read(s, line, 65536);
			/* Close connection if necessary */
			if (length <= 0) {
				write(2, "connection closed\n", 18);
				close(s);
				char_term(0);
				return 5;
			}
			write(1, line, length);
		}
		/* New data from user terminal
		 * Username prefix and Ctrl-D processing 
		 * only if reading from terminal
		 */
		if (FD_ISSET(0, &read_fds)) {
			static char line_closed = 1;
			int ofs_name = 0;
			if (*username && line_closed) {
				strcpy(line, username);
				ofs_name = strlen(username);
				line_closed = 0;
			}
			int length = read(0, line + ofs_name, 65516);
			if (length <= 0 || *username && line[ofs_name] == 4) {
				write(2, "\ngood bye\n", 10);
				close(s);
				char_term(0);
				return 0;
			}
			if (*username) {
				line[length + ofs_name] = 0;
				if (strchr(line, '\n'))
					line_closed = 1;
			}
			write(s, line, length + ofs_name);
		}
	}
}

/* Terminal management:
 * 1 - activate raw character-mode,
 * 0 - restore original settings
 */
static void char_term(int on)
{
	static struct termios orig;
	static char state = 0;
	if (!state && on) {
		struct termios charbased;
		tcgetattr(0, &orig);
		memcpy(&charbased, &orig, sizeof orig);
		charbased.c_lflag &= ~ICANON;
		charbased.c_cc[VMIN] = 1;
		charbased.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &charbased);
		state = 1;
	}
	if (state && !on) {
		tcsetattr(0, TCSANOW, &orig);
		state = 0;
	}
}

/* Signal handler for SIGINT and SIGTERM
 * Close main socket, select() will exit gracefully
 */
static void sig_term(int signal)
{
	close(s);
	write(2, "received SIGTERM or SIGINT\n", 27);
}

/* Print help text */
static void help(void)
{
	write(1, "Usage: chat [-h] -s|<host> <port>\n", 34);
	write(1, "-h     print this help text\n", 28);
	write(1, "-s     start server mode\n", 25);
	write(1, "<host> start client mode, connect to host\n", 42);
	write(1, "<port> to listen or connect\n", 28);
	write(1, "\n", 1);
	write(1, "In client mode your terminal is set to raw mode\n", 48);
	write(1, "thus every byte you type is immediately sent.\n", 46);
	write(1, "There is no line editing, not even Backspace.\n", 46);
	write(1, "\n", 1);
	write(1, "Quit client with Ctrl-D.\n", 25);
}
