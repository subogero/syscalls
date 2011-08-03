#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>

int main(void)
{
  pid_t child = fork();
  if (child < 0) {
    perror("fork failed");
    return 1;
  }
  // Child
  else if (child == 0) {
    int fd = open("/root/tmp/foo", O_RDONLY | O_CREAT);
    char buf[256];
    while (1) {
      lseek(fd, 0, SEEK_SET);
      read(fd, buf, 256);
      printf(buf);
    }
  }
  // Parent
  else {
    int fd = open("/root/tmp/foo", O_CREAT | O_TRUNC | O_WRONLY);
    while (1) {
      lseek(fd, 0, SEEK_SET);
      write(fd, "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n", 245);
      lseek(fd, 0, SEEK_SET);
      write(fd, "___________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________\n", 245);
      lseek(fd, 0, SEEK_SET);
      write(fd, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\n", 245);
    }
  }
  return 0;
}
