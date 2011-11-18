/* A command-line multi-user chat program using a client-server model.
 * Uses select() besides the normal socket related system calls.
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

int s;

static int run_server(void);
enum userop {
	userop_CLR,
	userop_SET,
	userop_GET,
	userop_CATLIST,
	userop_NOF
};
static char *usernames(int fd, const char *line, enum userop op);

static int run_client(void);
static void char_term(int on);

static void help(void);

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
		static time_t t_last_post;
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
				sprintf(line, "%d user(s) chatting: ", users);
				usernames(fd, line, userop_CATLIST);
				strcat(line, "\n");
				write(fd, line, strlen(line));
				sprintf(line, "Time since last post [s]: %ld\n",
				              time(NULL) - t_last_post);
				write(fd, line, strlen(line));
			}
		}
		/* Existing connections: read, forward to others */
		int i;
		for (i = 3; i <= fd_max; ++i) {
			if (i == s || !FD_ISSET(i, &read_fds))
				continue;
			sprintf(line, "\033[%dm", 31 + i%6);
			int length = 5 + read(i, line + 5, 256);
			/* Close connection if necessary */
			if (length <= 5) {
				users--;
				write(2, "connection closed\n", 18);
				close(i);
				FD_CLR(i, &all_fds);
				char *name = usernames(i, NULL, userop_GET);
				strcpy(line + 5, name ? name : "");
				if (name)
					strcat(line, "  --- GOOD BYE ---  \n");
				length = strlen(line);
				usernames(i, NULL, userop_CLR);
			}
			else {
				t_last_post = time(NULL);
				line[length] = 0;
				usernames(i, line + 5, userop_SET);
			}
			sprintf(line + length, "\033[0m");
			length += 4;
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

static char *usernames(int fd, const char *line, enum userop op)
{
	int i;
	static char *names[64] = { [0] = NULL, }; /* C99, wipes whole array */
	if (fd >= 64) return NULL;
	switch (op) {
	case userop_SET:
		if (names[fd]) return NULL;
		if (line[0] != '<') return NULL;
		char *end_name = strchr(line, '>');
		if (!end_name) return NULL;
		int size = end_name - line + 1;
		names[fd] = malloc(size + 1);
		if (!names[fd]) return NULL;
		memcpy(names[fd], line, size);
		names[fd][size] = 0;
	case userop_GET:
		return names[fd];
	case userop_CLR:
		free(names[fd]);
		names[fd] = NULL;
		return NULL;
	case userop_CATLIST:
		for (i = 0; i < 64; ++i) {
			if (names[i])
				strcat(line, names[i]);
		}
	default:
		return NULL;
	}
}

static int run_client(void)
{
	/* Get user name */
	char username[20] = "<";
	char *user = getenv("USER");
	if (user) {
		strcat(username, user);
	}
	else {
		write(1, "enter user name: ", 17);
		int len = read(0, username + 1, 17);
		username[len] = 0;
	}
	strcat(username, ">  ");
	/* Prepare multiplexed reading from socket and stdin */
	char line[256];
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
			int length = read(s, line, 256);
			/* Close connection if necessary */
			if (length <= 0) {
				write(2, "connection closed\n", 18);
				close(s);
				char_term(0);
				return 5;
			}
			write(2, line, length);
		}
		/* New data from user terminal */
		if (FD_ISSET(0, &read_fds)) {
			static char line_closed = 1;
			int ofs_name = 0;
			if (line_closed) {
				strcpy(line, username);
				ofs_name = strlen(username);
				line_closed = 0;
			}
			int length = read(0, line + ofs_name, 256);
			if (length <= 0 || line[ofs_name] == 4) {
				write(1, "\ngood bye\n", 10);
				close(s);
				char_term(0);
				return 0;
			}
			line[length + ofs_name] = 0;
			if (strchr(line, '\n'))
				line_closed = 1;
			write(s, line, length + ofs_name);
		}
	}
}

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
