#include<stdint.h>
#include<string.h>
#include<stdlib.h>

#include "bigint_conf.h"

typedef uint32_t bigint_limb;
typedef int32_t bigint_slimb;
typedef uint64_t bigint_double;

#define BIGINT_SUCCESS 1

/* Errors */
#define BIGINT_ERR_ALLOC -2
#define BIGINT_ERR_INVALID_ARGS -3
#define BIGINT_ERR_MAX_LIMBS_REACHED -4
#define BIGINT_ERR_NOT_ENOUGH_BUFLEN -5

/* Comparisons */
#define BIGINT_EQUAL 0
#define BIGINT_LESSTHAN -1
#define BIGINT_GREATERTHAN 1

/* Formats for export */
#define BIGINT_BIGENDIAN 1
#define BIGINT_LITTLEENDIAN 0 

/* Algorithm-specific (thresholds measured in bits)*/
#define BIGINT_KARATSUBA_THRESHOLD 1024

/* Bytes in a limb */
#define BIL (sizeof(bigint_limb))

/* Bits in a limb */
#define BiIL (BIL << 3)

/* Converts a number of bits to a number of limbs */
#define BiTOL(x) (((x) / biIL) + ((x) % BiIL != 0))

/* Converts a number of bytes to a number of limbs */
#define BTOL(x) (((x) / BIL) + ((x) % BIL != 0))

typedef struct
{
	int sign;
	size_t len;
	size_t alloc;

#if defined(BIGINT_ALLOC)
	bigint_limb* limbs;
#elif
	bigint_limb limbs[BIGINT_MAX_LIMBS];
#endif

}bigint;

/*
   Initialize & free
*/
void bigint_init(bigint* n);
void bigint_free(bigint* n);

/*
  Set a single limb & obtain information
  regarding their lengths (in bits and bytes
  respectively)
*/
int bigint_set_limb(bigint* n, bigint_slimb limb);
size_t bigint_bitlen(bigint* n);
size_t bigint_len(bigint* n);

/*
   Copy & Conditional copy
   Destination - A
   Source - B
*/
int bigint_copy(bigint* a, bigint* b);
int bigint_cond_copy(int bit, bigint* a, bigint* b);

/*
   Swap & Conditional swap
   A & B
*/
void bigint_swap(bigint* a, bigint* b);
int bigint_cond_swap(int bit, bigint* a, bigint* b);

/*
   Compare 2 big ints.
   A & B and A & b
   returns -1 when A < B, 
            1 when A > B,
			0 when A = B.
*/
int bigint_cmp(bigint* a, bigint* b, int sign);
int bigint_cmp_limb(bigint* a, bigint_slimb b);

/* Bit-level operations */

/*
   Get and set a bit at a given position.
*/
int bigint_set_bit(bigint* n, size_t pos, int bit);
int bigint_get_bit(bigint* n, size_t pos);

/*
  Left & right shift.
*/
int bigint_lshift(bigint* n, size_t bits);
int bigint_rshift(bigint* n, size_t bits);

/* Import & Export */

/*
  Import and export from/to bytes to/from bigint.
*/
int bigint_import_bytes(bigint* n, uint8_t* buf, size_t buflen, int format);
int bigint_export_bytes(bigint* n, uint8_t* buf, size_t buflen, int format);

/* Arithmetic */

/*
  Addition & Subtraction
  C = A + B
  C = A - B
*/
int bigint_add(bigint* c, bigint* a, bigint* b);
int bigint_sub(bigint* c, bigint* a, bigint* b);

/*
  Multiplication & Squaring
  C = A * B
  C = A^2
*/
int bigint_mul(bigint* c, bigint* a, bigint* b);
int bigint_sqr(bigint*c, bigint* a, bigint* b);