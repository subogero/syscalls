#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
int main(int argc, char *argv[])
{
	if (argc < 2) return 1;
	struct stat st;
	stat(argv[1], &st);
	printf("Device ID %d\n", st.st_dev);
	printf("Inode     %d\n", st.st_ino);
	printf("Mode      %o\n", st.st_mode);
	printf("Links     %d\n", st.st_nlink);
	printf("User ID   %d\n", st.st_uid);
	printf("Group ID  %d\n", st.st_gid);
	printf("Dev MA MI %x\n", st.st_rdev);
	printf("Size      %d\n", st.st_size);
	printf("Blocksize %d\n", st.st_blksize);
	printf("Blocks    %d\n", st.st_blocks);
	printf("Acess     %d\n", st.st_atime);
	printf("Modified  %d\n", st.st_mtime);
	printf("Changed   %d\n", st.st_ctime);

	return 0;
}
