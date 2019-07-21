#include "bigint.h"

static void bigint_secure_memset(void* data, size_t len)
{
	volatile unsigned char* p = data;

	while (len--)
		*p++ = 0;
}


void bigint_init(bigint* n)
{
	if (n == NULL) return;

	memset(n, 0, sizeof(bigint));
	n->sign = 1;
}

void bigint_free(bigint* n)
{
	if (n == NULL) return;

	bigint_secure_memset(n->limbs, n->len);

#if defined(BIGINT_ALLOC)
	free(n->limbs);
#endif

	memset(n, 0, sizeof(bigint));
	n->sign = 1;
}


int bigint_alloc(bigint* n, size_t limbs)
{
	if (n == NULL || limbs == 0)
		return BIGINT_ERR_INVALID_ARGS;

	if (limbs > BIGINT_MAX_LIMBS)
		return BIGINT_ERR_MAX_LIMBS_REACHED;

#if defined(BIGINT_ALLOC)

	if (limbs == n->alloc)
		return BIGINT_SUCCESS;

	if (limbs < n->len)
		n->len = limbs;

	bigint_limb* new_limbs = (bigint_limb*)malloc(limbs * BIL);

	if (new_limbs == NULL)
		return BIGINT_ERR_ALLOC;

	memset(new_limbs, 0, limbs * BIL);

	if (n->len > 0 && n->limbs != NULL)
	{
		memcpy(new_limbs, n->limbs, n->len * BIL);
		bigint_secure_memset(n->limbs, n->len);
		free(n->limbs);
	}

	n->limbs = new_limbs;
	n->alloc = limbs;

#endif

	return BIGINT_SUCCESS;
}

int bigint_set_limb(bigint* n, bigint_slimb limb)
{
	if (n == NULL) 
		return BIGINT_ERR_INVALID_ARGS;

	int r;

	if ((r = bigint_alloc(n, 1)) < 0)
		return r;

	bigint_secure_memset(n, n->len);
	n->len = 1;
	n->limbs[0] = (limb < 0) ? -limb : limb;
	n->sign = (limb < 0) ? -1 : 1;
	return BIGINT_SUCCESS;
}


size_t bigint_bitlen(bigint* n)
{
	if (n == NULL) 
		return 0;

	size_t leading_zeroes = 0;

	for (size_t i = 1; i < BiIL; i++)
	{
		if ((n->limbs[n->len - 1] >> (BiIL - i)) & 1)
			break;

		leading_zeroes++;
	}

	return ((n->len - 1) * BiIL) + (BiIL - leading_zeroes);
}

size_t bigint_len(bigint* n)
{
	if (n == NULL) 
		return 0;

	return (bigint_bitlen(n) + 7) >> 3;
}


int bigint_copy(bigint* a, bigint* b)
{
	if (a == NULL || b == NULL) 
		return BIGINT_ERR_INVALID_ARGS;

	int r;

	a->sign = b->sign;
	a->len = b->len;

	if (a->alloc < b->alloc && (r = bigint_alloc(a, b->alloc)) < 0)
		return r;

	bigint_secure_memset(a->limbs, a->len);

	for (size_t i = 0; i < b->len; i++)
		a->limbs[i] = b->limbs[i];

	return BIGINT_SUCCESS;
}

int bigint_cond_copy(int bit, bigint* a, bigint* b)
{
	if (a == NULL || b == NULL || (bit != 0 && bit != 1))
		return BIGINT_ERR_INVALID_ARGS;

	int r;

	a->sign = a->sign * (1 - bit) + b->sign * bit;
	a->len = a->len * (1 - bit) + b->len * bit;

	/* In practice this allocation is not constant time and
	   reveals when an allocation takes place (as opposed to always
	   allocating), despite that the information revealed is benign
	   (namely it leaks that a->alloc is less than b->alloc), however
	   to squeeze in extra performance we allow that to happen.
	*/

	if (a->alloc < b->alloc && (r = bigint_alloc(a, b->alloc)) < 0)
		return r;

	bigint_secure_memset(a->limbs, bit * a->len);

	for (size_t i = 0; i < b->len; i++)
		a->limbs[i] = a->limbs[i] * (1 - bit) + b->limbs[i] * bit;

	return BIGINT_SUCCESS;
}


void bigint_swap(bigint* a, bigint* b)
{
	if (a == NULL || b == NULL) return;

	bigint temp;
	bigint_init(&temp);

	bigint_copy(&temp, a);
	bigint_copy(a, b);
	bigint_copy(b, &temp);

	bigint_free(&temp);
}

int bigint_cond_swap(int bit, bigint* a, bigint* b)
{
	if (a == NULL || b == NULL || (bit != 0 && bit != 1)) 
		return BIGINT_ERR_INVALID_ARGS; 

	int r = BIGINT_SUCCESS;
	bigint temp;
	bigint_init(&temp);

	if ((r = bigint_cond_copy(bit, &temp, a)) < 0)
		return r;

	if ((r = bigint_cond_copy(bit, a, b)) < 0)
		return r;

	if((r = bigint_cond_copy(bit, b, &temp)) < 0)
		return r;

	bigint_free(&temp);
	return r;
}


int bigint_cmp(bigint* a, bigint* b, int sign)
{
	if (a == NULL || b == NULL || (sign != 0 && sign != 1))
		return BIGINT_ERR_INVALID_ARGS;

	int r = BIGINT_GREATERTHAN;

	if (sign && (a->sign == 1 && b->sign == -1))
		return BIGINT_GREATERTHAN;

	if (sign && (a->sign == -1 && b->sign == 1))
		return BIGINT_LESSTHAN;

	if (a->len > b->len)
		return BIGINT_GREATERTHAN;

	if (a->len < b->len)
		return BIGINT_LESSTHAN;

	if (sign && (a->sign == -1 && b->sign == -1))
		r = BIGINT_LESSTHAN;

	for (size_t i = 0; i < a->len; i++)
	{
		if (a->limbs[(a->len - 1) - i] > b->limbs[(a->len - 1) - i]) return r;

		if (a->limbs[(a->len - 1) - i] < b->limbs[(a->len - 1) - i]) return -r;
	}

	return BIGINT_EQUAL;
}

int bigint_cmp_limb(bigint* a, bigint_slimb b)
{
	if (a == NULL)
		return BIGINT_ERR_INVALID_ARGS;

	int r = BIGINT_GREATERTHAN;

	if (a->len > 1)
		return BIGINT_GREATERTHAN;

	bigint_limb t = (b < 0) ? -b : b;

	if (a->sign == 1 && b < 0) return BIGINT_GREATERTHAN;

	if (a->sign == -1 && b >= 0) return BIGINT_LESSTHAN;

	if (a->sign == -1 && b < 0)
		r = BIGINT_LESSTHAN;

	if (a->limbs[0] > t) return r;

	if (a->limbs[0] < t) return -r;

	return BIGINT_EQUAL;
}


int bigint_set_bit(bigint* n, size_t pos, int bit)
{
	if (n == NULL || (bit != 0 && bit != 1)) 
		return BIGINT_ERR_INVALID_ARGS;

	int r;
	size_t nlimb = pos / BiIL;
	size_t noff = pos % BiIL;

	if ((n->alloc * BiIL) < pos && (r = bigint_alloc(n, nlimb + 1) < 0))
		return r;

	if((n->len * BiIL) < pos)
	  n->len = nlimb + 1;

	n->limbs[nlimb] &= ~((bigint_limb)1 << noff);
	n->limbs[nlimb] |= ((bigint_limb)1 << noff);
	return BIGINT_SUCCESS;
}

int bigint_get_bit(bigint* n, size_t pos)
{
	if (n == NULL || (n->len * BiIL) < pos)
		return BIGINT_ERR_INVALID_ARGS;

	return n->limbs[pos / BiIL] >> pos % BiIL;
}


int bigint_lshift(bigint* n, size_t bits)
{
	if (n == NULL || bits == 0)
		return BIGINT_ERR_INVALID_ARGS;

	int r;
	size_t i;
	size_t nlimb = bits / BiIL;
	size_t noff = bits % BiIL;
	size_t nlimbs = n->len + nlimb;
	size_t req_bits = bigint_bitlen(n) + bits;
	bigint_limb c0 = 0, c1;

	if (bits > BIGINT_MAX_LIMBS * BiIL)
		return BIGINT_ERR_MAX_LIMBS_REACHED;

	if ((n->len * BiIL) < req_bits && (r = bigint_alloc(n, BiTOL(req_bits))) < 0)
		return r;

	if (nlimb > 0)
	{
		for (i = nlimbs; i > nlimb; i--)
			n->limbs[i - 1] = n->limbs[(i - 1) - nlimb];

		for (; i > 0; i--)
			n->limbs[i - 1] = 0;
	}

	n->len = nlimbs;

	if (noff > 0)
	{
		for (i = nlimb; i < n->len; i++)
		{
			c1 = n->limbs[i] >> (BiIL - noff);
			n->limbs[i] <<= noff;
			n->limbs[i] |= c0;
			c0 = c1;
		}
	}

	return BIGINT_SUCCESS;
}

int bigint_rshift(bigint* n, size_t bits)
{
	if (n == NULL || bits == 0)
		return BIGINT_ERR_INVALID_ARGS;
	
	size_t i;
	size_t nlimb = bits / BiIL;
	size_t noff = bits % BiIL;
	size_t c0 = 0, c1;

	if (nlimb > 0)
	{
		for (i = 0; i < n->len - nlimb; i++)
			n->limbs[i] = n->limbs[i + nlimb];

		for (; i < n->len; i++)
			n->limbs[i] = 0;
	}

	n->len -= nlimb;

	if (noff > 0)
	{
		for (i = n->len; i > 0; i--)
		{
			c1 = n->limbs[i - 1] << (BiIL - noff);
			n->limbs[i - 1] >>= noff;
			n->limbs[i - 1] |= c0;
			c0 = c1;
		}
	}

	return BIGINT_SUCCESS;
}


int bigint_import_bytes(bigint* n, uint8_t* buf, size_t buflen, int format)
{
	if (n == NULL || buf == NULL || buflen == 0)
		return BIGINT_ERR_INVALID_ARGS;

	int r;
	size_t shift;

	if (n->alloc < BTOL(buflen) && (r = bigint_alloc(n, BTOL(buflen))) < 0)
		return r;

	for (size_t i = 0; i < buflen; i++)
	{
		shift = (format) ? ((BiIL - 8) - i * 8) : i * 8;
		n->limbs[i / BIL] |= (bigint_limb)buf[i] << shift;
	}

	n->len = BTOL(buflen);
	return BIGINT_SUCCESS;
}

int bigint_export_bytes(bigint* n, uint8_t* buf, size_t buflen, int format)
{
	if (n == NULL || buf == NULL || buflen == 0)
		return BIGINT_ERR_INVALID_ARGS;

	if (bigint_len(n) > buflen)
		return BIGINT_ERR_NOT_ENOUGH_BUFLEN;

	size_t shift;

	for (size_t i = 0; i < buflen; i++)
	{
		shift = (format) ? ((BiIL - 8) - i * 8) : i * 8;
		buf[i] = (uint8_t)(n->limbs[i / BIL] >> shift) & 0xFF;
	}

	return BIGINT_SUCCESS;
}


static int bigint_uadd(bigint* c, bigint* a, bigint* b)
{
	int r;
	size_t max, min;
	size_t carry, i;
	bigint* maxlimbs, *minlimbs;

	if (a->len > b->len)
	{
		max = a->len;
		min = b->len;

		maxlimbs = a;
		minlimbs = b;
	}
	else
	{
		max = b->len;
		min = a->len;

		maxlimbs = b;
		minlimbs = a;
	}

	if ((r = bigint_alloc(c, max + 1)) < 0)
		return r;

	bigint_limb* al = maxlimbs->limbs;
	bigint_limb* bl = minlimbs->limbs;
	bigint_limb* cl = c->limbs;
	c->len = min;

	for (carry = 0, i = 0; i < min; i++)
	{
		bigint_limb result;
		result = al[i] + bl[i] + carry;
		carry = (carry && result == al[i]) || (result < al[i]);
		cl[i] = result;
	}

	for(; i < max; i++)
	{ 
		cl[i] = al[i] + carry;
		carry = (cl[i] < carry);
		c->len++;
	}

	if (carry)
	{
		cl[max] = carry;
		c->len++;
	}

	return BIGINT_SUCCESS;
}

static int bigint_usub(bigint* c, bigint* a, bigint* b)
{
	int r;
	size_t max, min;
	size_t borrow, i;
	bigint* maxlimbs, *minlimbs;
	
	if (a->len > b->len)
	{
		max = a->len;
		min = b->len;

		maxlimbs = a;
		minlimbs = b;
	}
	else
	{
		max = b->len;
		min = a->len;

		maxlimbs = b;
		minlimbs = a;
	}

	if ((r = bigint_alloc(c, max)) < 0)
		return r;

	bigint_limb* al = maxlimbs->limbs;
	bigint_limb* bl = minlimbs->limbs;
	bigint_limb* cl = c->limbs;
	c->len = max;

	for (borrow = 0, i = 0; i < min; i++)
	{
		bigint_limb result;
		result = al[i] - bl[i] - borrow;
		borrow = (borrow && (result == al[i])) || (result > al[i]);
		cl[i] = result;
	}

	for (; i < max; i++) 
	{
		cl[i] = al[i] - borrow;
		borrow = (cl[i] > al[i]);

		if(cl[i++] == 0)
		  c->len--;
	}

	return BIGINT_SUCCESS;
}

int bigint_add(bigint* c, bigint* a, bigint* b)
{
	if (c == NULL || a == NULL || b == NULL)
		return BIGINT_ERR_INVALID_ARGS;

	if (a->sign != b->sign)
	{
		if (bigint_cmp(a, b, 0) > 0)
			c->sign = a->sign;
		else
			c->sign = b->sign;

		return bigint_usub(c, a, b);
	}

	c->sign = a->sign;
	return bigint_uadd(c, a, b);
}

int bigint_sub(bigint* c, bigint* a, bigint* b)
{
	if (c == NULL || a == NULL || b == NULL)
		return BIGINT_ERR_INVALID_ARGS;

	if (a->sign == b->sign)
	{
		if (bigint_cmp(a, b, 0) >= 0)
			c->sign = a->sign;
		else
			c->sign = -(a->sign);

		return bigint_usub(c, a, b);
	}

	c->sign = a->sign;
	return bigint_uadd(c, a, b);
}


static void bigint_comba_mul(bigint* c, bigint* a, bigint* b)
{

}

static int bigint_longhand_mul(bigint* c, bigint* a, bigint* b)
{
	int r;
	size_t i, j, res, len;
	bigint temp;

	bigint_init(&temp);
	res = a->len + b->len;

	if ((r = bigint_alloc(&temp, res)) < 0)
		return r;

	/*if ((c->alloc * BiIL) < BIGINT_COMBA_THRESHOLD)
	{
		bigint_comba_mul(&temp, a, b);
		goto DONE;
	}*/

	for (i = 0; i < a->len; i++)
	{
		bigint_limb carry;
		bigint_limb* al = a->limbs;
		bigint_limb* bl = b->limbs;
		bigint_limb* cl = temp.limbs;

		for (carry = 0, j = 0; j < b->len; j++)
		{
			bigint_double result;
			result = cl[j + i] + ((bigint_double)al[i] * bl[j]) + carry;
			cl[j + i] = (bigint_limb)result;
			carry = result >> BiIL;
		}

		temp.limbs[j + i] = carry;
	}
//DONE:

	for (len = 0; len < temp.alloc; len++)
		if (temp.limbs[(temp.alloc - 1) - len] != 0)
			break;

	temp.len = temp.alloc - len;

	if ((r = bigint_copy(c, &temp)) < 0)
		return r;

	c->len = temp.len;
	bigint_free(&temp);
	return r;
}

static int bigint_karatsuba_mul(bigint* c, bigint* a, bigint* b)
{
	int r;
	size_t B, i;
	bigint x0, x1, y0, y1, x0y0, x1y1, t0;

	bigint_init(&x0); bigint_init(&y0); bigint_init(&x1);
	bigint_init(&y1); bigint_init(&x0y0); bigint_init(&x1y1);
	bigint_init(&t0);

	B = MIN(a->len, b->len) / 2;

	/* Allocate appropriate memory */
	if ((r = bigint_alloc(&x0, B)) < 0)
		return r;

	if ((r = bigint_alloc(&x1, a->len - B)) < 0)
		return r;

	if ((r = bigint_alloc(&y0, B)) < 0)
		return r;

	if ((r = bigint_alloc(&y1, b->len - B)) < 0)
		return r;

	/* A = x0 + radix^B * x1. */
	for (i = 0; i < B; i++)
		x0.limbs[i] = a->limbs[i];

	x0.len = B;

	for (i = 0; i < a->len - B; i++)
		x1.limbs[i] = a->limbs[i + B];

	x1.len = a->len - B;
	/* End A */

	/* B = y0 + radix^B * y1. */
	for (i = 0; i < B; i++)
		y0.limbs[i] = b->limbs[i];

	y0.len = B;

	for (i = 0; i < b->len - B; i++)
		y1.limbs[i] = b->limbs[i + B];

	y1.len = b->len - B;
	/* End B */

	if((r = bigint_longhand_mul(&x0y0, &x0, &y0)) < 0)
		return r;

	if ((r = bigint_longhand_mul(&x1y1, &x1, &y1)) < 0)
		return r;

	if ((r = bigint_add(&t0, &x0, &x1)) < 0)
		return r;

	if ((r = bigint_add(&x0, &y0, &y1)) < 0)
		return r;

	if ((r = bigint_longhand_mul(&t0, &t0, &x0)) < 0)
		return r;

	if ((r = bigint_add(&x0, &x0y0, &x1y1)) < 0)
		return r;

	if ((r = bigint_sub(&t0, &t0, &x0)) < 0)
		return r;

	/* reconstruct */
	if ((r = bigint_lshift(&t0, B * BiIL)) < 0)
		return r;

	if ((r = bigint_lshift(&x1y1, 2 * B * BiIL)) < 0)
		return r;

	if ((r = bigint_add(&t0, &t0, &x0y0)) < 0)
		return r;

	if ((r = bigint_add(c, &t0, &x1y1)) < 0)
		return r;

	bigint_free(&x0); bigint_free(&y0); bigint_free(&x1);
	bigint_free(&y1); bigint_free(&x0y0); bigint_free(&x1y1);
	bigint_free(&t0);
	return r;
} 

static int bigint_toomcook_mul(bigint* c, bigint* a, bigint* b)
{
	int r;
	return BIGINT_SUCCESS;
}

int bigint_mul(bigint* c, bigint* a, bigint* b)
{
	if (c == NULL || a == NULL || b == NULL)
		return BIGINT_ERR_INVALID_ARGS;

	int r;
	size_t res;

	/*if ((res * BiIL) >= BIGINT_TOOMCOOK_THRESHOLD)
	    r = bigint_toomcook_mul(c, a, b);
	else if ((res * BiIL) >= BIGINT_KARATSUBA_THRESHOLD)*/
		r = bigint_karatsuba_mul(c, a, b);
	/*else*/
		//r = bigint_longhand_mul(c, a, b);

	c->sign = a->sign * b->sign;
	return r;
}



int bigint_sqr(bigint* c, bigint* a, bigint* b)
{
	return BIGINT_SUCCESS;
}