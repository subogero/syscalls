#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>

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
    unsigned int bytes = 0;
    struct timeval time0, time1;
    gettimeofday(&time0, NULL);
    while (1) {
      lseek(fd, 0, SEEK_SET);
      bytes += read(fd, buf, 256);
      if (bytes >= 1000000000) break;
    }
    gettimeofday(&time1, NULL);
    int dt_us = time1.tv_usec - time0.tv_usec + 1000000 * (time1.tv_sec - time0.tv_sec);
    printf("Data: 1000 MB\nTime: %d us\nRate: %d MB/s\n", dt_us, 1000000000/dt_us);
  }
  // Parent
  else {
    int fd = open("/root/tmp/foo", O_CREAT | O_TRUNC | O_WRONLY);
    while (1) {
      lseek(fd, 0, SEEK_SET);
      write(fd, "--------------------------------------------------------------------\n", 70);
      lseek(fd, 0, SEEK_SET);
      write(fd, "____________________________________________________________________\n", 70);
      lseek(fd, 0, SEEK_SET);
      write(fd, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM\n", 70);
    }
  }
  return 0;
}
