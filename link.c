#include <unistd.h>
int main(void)
{
	link("link", "link1");
	symlink("link1", "link2");
	return 0;
}
