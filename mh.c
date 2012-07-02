#define DEBUG
#ifdef DEBUG
#include <stdio.h>
#endif

#include <unistd.h>

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
#ifdef DEBUG
	printf("argc = %d\n", argc);
	for (i = 0; i < argc; ++i) {
		printf("argv %d = %s\n", i, argv[i]);
	}
#endif
}

int main(void)
{
	while (read(0, line, 80))
		command();
	return 0;
}
