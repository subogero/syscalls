#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
  char filename[] = "fifo";
  mknod(filename, S_IFIFO | 0644, 0);

  pid_t child = fork();
  // Parent
  if (child > 0) {
    int fd = open(filename, O_WRONLY);
    int i;
    for (i = 0; i < 10; ++i) {
      char buffer[40] = "parent  write ";
      size_t length = strlen(buffer);
      char i_ascii = '0' + i;
      buffer[length    ] = i_ascii;
      buffer[length + 1] = '\n';
      buffer[length + 2] = 0;
      write(1, buffer, strlen(buffer));
      write(fd, &i_ascii, 1);
    }
    wait(&i);
  }
  // Child
  else {
    int fd = open(filename, O_RDONLY | O_NONBLOCK);
    pid_t grandchild = fork();
    int i;
    for (i = 0; i < 10; ++i) {
      char buffer[40] = { 0, };
      strcat(buffer, grandchild ? "child    " : "grandson ");
      strcat(buffer, "read ");
      size_t length = strlen(buffer);
      int bytes = read(fd, buffer + length, 1);
      buffer[length + bytes]     = '\n';
      buffer[length + bytes + 1] = 0;
      write(1, buffer, strlen(buffer));
      usleep(grandchild ? 23 : 39);
    }
    wait(&i);
  }
  return 0;
}
