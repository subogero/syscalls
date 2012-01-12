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
 * - 1st operand of ?: operators
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
	/* Initialize signed/unsigned ints/bitfields to TRUE */
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
	/*
	 * Check signed/unsigned bitfields against TRUE, print results
	 * A signed int 1-bitfield fails, as it's aither 0 or -1, never 1 (TRUE).
	 */
	printf("\nAssign TRUE (!FALSE) to bar, baz, spam and eggs.\n");
	printf("\nCompare all to TRUE. Signed bitfield Fails.\n");
	printf("bar  (signed bitf) is %s.\n", foo.bar  == TRUE ? "true" : "false");
	printf("baz  (unsig. bitf) is %s.\n", foo.baz  == TRUE ? "true" : "false");
	printf("spam (signed int)  is %s.\n", foo.spam == TRUE ? "true" : "false");
	printf("eggs (unsig. int)  is %s.\n", foo.eggs == TRUE ? "true" : "false");
	/*
	 * Redefine TRUE to ~FALSE, simulating inconsistent TRUEs in multiple files
	 * Check signed/unsigned integers against our TRUE, print results.
	 * Even integers fail, as 1 is never equal to ~0 (0xFFFFFFFF)
	 */
	printf("\nChange TRUE to ~FALSE. Epic Fail, except signed bitfield.\n");
	#undef TRUE
	#define TRUE (~FALSE)
	printf("bar  (signed bitf) is %s.\n", foo.bar  == TRUE ? "true" : "false");
	printf("baz  (unsig. bitf) is %s.\n", foo.baz  == TRUE ? "true" : "false");
	printf("spam (signed int)  is %s.\n", foo.spam == TRUE ? "true" : "false");
	printf("eggs (unsig. int)  is %s.\n", foo.eggs == TRUE ? "true" : "false");
	/*
	 * Check all signed/unsigned ints/bitfields in the K&R way
	 */
	printf("\nThe right way is the K&R way: if (baz) and if (!baz) always works.\n");
	printf("bar  (signed bitf) is %s.\n", foo.bar  ? "true" : "false");
	printf("baz  (unsig. bitf) is %s.\n", foo.baz  ? "true" : "false");
	printf("spam (signed int)  is %s.\n", foo.spam ? "true" : "false");
	printf("eggs (unsig. int)  is %s.\n", foo.eggs ? "true" : "false");
	return 0;
}
