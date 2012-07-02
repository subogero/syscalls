#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

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
	int redir = 0;
	for (i = 0; i < argc; ++i) {
		int what = argv[i][1] != '<' && argv[i][1] != '>'
		         ? -1
		         : argv[i][0] - '0';
		if (what < 0 || what > 2)
			continue;
		if (!redir)
			redir = i;
		int where = argv[i][2] == '&'
		          ? argv[i][3] - '0'
		          : what == 0
		          ? open(argv[i] + 2, O_RDONLY)
		          : creat(argv[i] + 2, 0644);
		close(what);
		dup(where);
		if (where > 2)
			close(where);
	}
	if (redir)
		argv[redir] = 0;
	if (execvp(argv[0], argv)) {
		write(2, "Ilyen nincs!\n", 13);
		_exit(-1);
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
