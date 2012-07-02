#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

char line[81];

void command(void)
{
	int argc = 0;
	int i = 0;
	char *argv[10] = {0,0,0,0,0,0,0,0,0,0,};
	while (1) {
		if (line[i] != ' ' && line[i] != '\n') {
			if (argv[argc] == 0)
				argv[argc] = line + i;
		} else {
			if (i > 0 && line[i-1])
				argc++;
			if (line[i] == '\n') {
				line[i] = 0;
				break;
			}
			line[i] = 0;
		}
		i++;
	}
	/* Parent: create child process and wait for it */
	pid_t pid = fork();
	if (pid) {
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
			write(1, "ZSIR>> ", 8);
		else
			write(1, "GYASZ>> ", 8);
		return;
	}
	/* Child: perform redirection and exec command */
	else {
		_exit(0);
	}
}

int main(void)
{
	write(1, "MAGYAR HEJ>> ", 13); 
	while (read(0, line, 80))
		command();
	write(1, "\n", 1);
	return 0;
}
