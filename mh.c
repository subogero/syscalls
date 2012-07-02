#define DEBUG
#ifdef DEBUG
#include <stdio.h>
#endif

char line[81] = "cat foo rt.c >log 2>&1\n";
int main(void)
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
	return 0;
}
