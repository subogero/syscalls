#include <unistd.h>
#include <fcntl.h>

int main(void)
{
	char s1[3], s2[3];
	int f1, f2;
	char s_f1, s_f2;
	/* Redirect standard output to duplog
	 * Create/open duplog, close stdout, dup duplog's fd
	 */
	f1 = open("duplog", O_CREAT | O_WRONLY, 0644);
	close(1);
	s_f2 = '0' + dup(f1);
	close(f1);
	write(1, "duplog's fd dupped into ", 24);
	write(1, &s_f2, 1);
	write(1, "\n", 1);
	/* Create file with text */
	f1 = open("foo", O_CREAT | O_WRONLY, 0644);
	s_f1 = '0' + f1;
	write(f1, "foobar", 6);
	write(1, "Write fd    ", 12);
	write(1, &s_f1, 1);
	write(1, "\n", 1);
	close(f1);
	/* Open twice
	 * 2 fd-s, 2 file objects, 1 inode
	 * both fd-s read from position 0 of file
	 */
	f1 = open("foo", O_RDONLY);
	f2 = open("foo", O_RDONLY);
	s_f1 = '0' + f1;
	s_f2 = '0' + f2;
	read(f1, s1, 3);
	read(f2, s2, 3);
	write(1, "Open twice  ", 12);
	write(1, &s_f1, 1);
	write(1, " ", 1);
	write(1, &s_f2, 1);
	write(1, " ", 1);
	write(1, s1, 3);
	write(1, s2, 3);
	write(1, "\n", 1);
	close(f1);
	close(f2);
	/* Dup
	 * 2 fd-s point to the same file-object, 1 inode
	 * two fd-s read from subsequent positions
	 */
	f1 = open("foo", O_RDONLY);
	f2 = dup(f1);
	unlink("foo");
	s_f1 = '0' + f1;
	s_f2 = '0' + f2;
	read(f1, s1, 3);
	read(f2, s2, 3);
	write(1, "Dup instead ", 12);
	write(1, &s_f1, 1);
	write(1, " ", 1);
	write(1, &s_f2, 1);
	write(1, " ", 1);
	write(1, s1, 3);
	write(1, s2, 3);
	write(1, "\n", 1);
	close(f1);
	close(f2);
	return 0;
}
