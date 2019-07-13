#include<stdio.h>

#include "bigint.h"

int main()
{
	bigint a, b, c;
	bigint_init(&a);
	bigint_init(&b);
	bigint_init(&c);

	uint8_t buf[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	bigint_import_bytes(&a, buf, 8, BIGINT_BIGENDIAN);

	bigint_set_limb(&b, 1);
	int r = bigint_add(&c, &a, &b);

	uint8_t buf2[12] = {0};
	//bigint_export_bytes(&c, buf2, 12, BIGINT_BIGENDIAN);

	for (size_t i = 0; i < c.len; i++)
		printf("%X ", c.limbs[i]);

	int ch = getchar();
	bigint_free(&a);
	bigint_free(&b);
	bigint_free(&c);
	return 0;
}