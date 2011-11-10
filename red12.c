#include <unistd.h>
#include <fcntl.h>
#include <string.h>
int main(int argc, char *argv[])
{
	/* Error for wrong arg list */
	if (argc < 3) {
		write(2, "Usage: red12 <file> <cmd> [<args>]\n", 35);
		return 1;
	}

	/* Possible filenames: &1 &2 file */
	int r1, r2;
	if      (strcmp(argv[1], "&1") == 0) { r1 = 0; r2 = 1; }
	else if (strcmp(argv[1], "&2") == 0) { r1 = 1; r2 = 0; }
	else                                 { r1 = 1; r2 = 1; }

	/* Attempt to create output file, name in argv[1] 
	 * Redirect standard input to <file>
	 */
	if (r1) {
		int fd = r2 
		       ? creat(argv[1], 0644) 
		       : 2;
		if (fd < 0) {
			write(2, "Could not open ", 15);
			write(2, argv[1], strlen(argv[1]));
			write(2, "\n", 1);
			return 2;
		}
		close(1);
		dup(fd);
		if (r2) {
			close(fd);
		}
	}

	/* Redirect standard error to <file> too */
	if (r2) {
		close(2);
		dup(1);
	}

	/* Execute program, name in argv[2], args from argv[3],
	 * exec*() only returns in case of error,
	 * so no conditions for error message needed.
	 */
	execvp(argv[2], argv + 2);
	write(2, "Could not execute ", 18);
	write(2, argv[2], strlen(argv[2]));
	write(2, "\n", 1);
	return 3;
}
