#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>

int main(void)
{
	unsigned char c = 32;
	int fds[2];

	pipe(fds);
	pid_t child = fork();
	
	switch (child) {
	case -1: // Fork error
		perror("fork failed");
		return 1;
	case 0: // Child: pumping out characters
		close(fds[0]);
		while (c < 128) {
			write(fds[1], &c, 1);
			c++;
		}
		close(fds[1]);
		break;
	default: // Parent: reading and printing characters
		close(fds[1]);
		while (read(fds[0], &c, 1)) { 
			write(1, &c, 1);
			//read(0, &c, 1);
		}
		close(fds[0]);
		write(1, "\n", 1);
		break;
	}
	return 0;
}
