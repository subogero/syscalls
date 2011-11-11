/* Two processes communicate on a Unix Domain socket,
 * created with socketpair().
 * Client reads lines from user, sends them to server to capitalize letters,
 * then prints replies.
 */
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

static void server(void);
static void client(void);
static char line[80];
int sockets[2];

int main(void)
{
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
		write(2, "Unable to create socket pair\n", 29);
		return 1;
	}

	/* Child will be server, parent the client */
	pid_t child = fork();
	switch (child) {
	case -1: write(2, "Unable to fork\n", 15); return 2;
	case  0: server(); break;
	default: client(); break;
	}
	return 0;
}

/* Receive packet on socket 1 from client, capitalize letters, send back */
static void server(void)
{
	close(sockets[0]);
	while (1) {
		int received = read(sockets[1], line, 80);
		if (received == 0) {
			write(2, "Server: connection closed\n", 26);
			break;
		}
		else if (received < 0) {
			write(2, "Server: read error\n", 19);
			break;
		}
		int i = 0;
		while (1) {
			if (!line[i] || i >= 80) break;
			if (line[i] >= 'a' && line[i] <= 'z') line[i] -= 0x20;
			i++;
		}
		write(sockets[1], line, 80);
	}
	close(sockets[1]);
}

/* Read line from user, send to server on socket 0, print reply */
static void client(void)
{
	close(sockets[1]);
	while (1) {
		if (!fgets(line, 80, stdin)) {
			write(1, "Client: Good bye\n", 17);
			break;
		}
		write(sockets[0], line, 80);
		usleep(10000);
		int received = read(sockets[0], line, 80);
		if (received == 0) {
			write(2, "Client: connection closed\n", 26);
			break;
		}
		else if (received < 0) {
			write(2, "Client: read error\n", 19);
			break;
		}
		fputs(line, stdout);
	}
	close(sockets[0]);
}
