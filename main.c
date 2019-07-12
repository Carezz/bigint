#include<stdio.h>

#include "bigint.h"

int main()
{
	bigint a, b, c;
	bigint_init(&a);
	bigint_init(&b);
	bigint_init(&c);

	/*uint8_t buf[12] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c};
	uint8_t buf2[12] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c};

	int r = bigint_import_bytes(&a, buf, 12, BIGINT_LITTLEENDIAN);
    r = bigint_import_bytes(&b, buf, 12, BIGINT_LITTLEENDIAN);
	r = bigint_add(&c, &a, &b);*/

	bigint_set_limb(&a, 5);
	bigint_set_limb(&b, -3);
	int r = bigint_sub(&c, &a, &b);
	int ch = getchar();
	bigint_free(&a);
	bigint_free(&b);
	bigint_free(&c);
	return 0;
}