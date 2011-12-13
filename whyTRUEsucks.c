/*
 * This small program demonstrates why you should not use TRUE/FALSE
 * in your C programs, for assigning and, especially, checking boolean
 * variables.
 *
 * The main reason is that C has no such thing as boolean variables.
 * Instead, in certain cases C creates a boolean context for values,
 * and checks whether or not they are zero.
 * Such contexts are
 * - in the parentheses after if () and while ()
 * - the 2nd expression in for (x; y; z)
 * - the operands of || and && operators
 *
 * With using TRUE and FALSE you simply create an illusion and write
 * dangerous code.
 *
 * It's no coincidence that K&R do not mention TRUE or FALSE at all.
 */
#include "stdio.h"
#define FALSE 0
#define TRUE (!FALSE)
int main(void)
{
	struct foo_t {
		int          bar: 1;
		unsigned int baz: 1;
		int          spam;
		unsigned int eggs;
	} foo = {
		TRUE,
		TRUE,
		TRUE,
		TRUE,
	};
	printf("\nAssign TRUE (!FALSE) to bar, baz, spam and eggs.\n");
	printf("\nAssign TRUE to a signed bitfield, then compare to TRUE. Fail.\n");
	printf("bar   %2d, baz   %2d\n", foo.bar == TRUE, foo.baz == TRUE);
	printf("\nCheck any variable against our definition of TRUE (~FALSE),\n");
	printf("after it was assigned TRUE (!FALSE) somewhere else. Epic Fail!\n");
	#undef TRUE
	#define TRUE (~FALSE)
	printf("spam  %2d, eggs  %2d\n", foo.spam == TRUE, foo.eggs == TRUE);
	printf("\nThe right way is the K&R way: if (baz) and if (!baz) always works.\n");
	if (foo.bar) printf("bar is true.\n");
	if (foo.baz) printf("baz is true.\n");
	if (foo.spam) printf("spam is true.\n");
	if (foo.eggs) printf("eggs is true.\n");
	return 0;
}
