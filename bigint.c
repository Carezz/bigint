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
		memcpy(new_limbs, n->limbs, n->len);
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
	bigint_limb c0 = 0, c1;

	if (bits > BIGINT_MAX_LIMBS * BiIL)
		return BIGINT_ERR_MAX_LIMBS_REACHED;

	if ((n->alloc * BiIL) < bits && (r = bigint_alloc(n, nlimbs)) < 0)
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
	size_t max = a->len, min = b->len;
	size_t carry = 0, i;

	int max_sign = a->sign;
	bigint_limb* maxlimbs = a->limbs;

	if (a->len < b->len)
	{
		max_sign = b->sign;
		maxlimbs = b->limbs;
		max = b->len;
		min = a->len;
	}

	if ((r = bigint_alloc(c, max + 1)) < 0)
		return r;

	bigint_limb* al = a->limbs;
	bigint_limb* bl = b->limbs;
	bigint_limb* cl = c->limbs;

	for (i = 0; i < min; i++)
	{
		cl[i] = al[i] + bl[i] + carry;
		carry = (cl[i] < bl[i]);
	}

	c->len = min;

	while (carry)
	{ 
		cl[i] = maxlimbs[i] + carry;
		carry = (cl[i++] < carry);
		c->len++;
	}

	c->sign = max_sign;
	return BIGINT_SUCCESS;
}

static int bigint_usub(bigint* c, bigint* a, bigint* b)
{
	int r;
	size_t max = a->len, min = b->len;
	size_t borrow = 0, i;

	int max_sign = a->sign;
	bigint_limb* maxlimbs = a->limbs;
	bigint_limb* minlimbs = b->limbs;
	
	if (a->len < b->len)
	{
		max_sign = b->sign;
		maxlimbs = b->limbs;
		minlimbs = a->limbs;
		max = b->len;
		min = a->len;
	}

	if ((r = bigint_alloc(c, max)) < 0)
		return r;

	bigint_limb* al = a->limbs;
	bigint_limb* bl = b->limbs;
	bigint_limb* cl = c->limbs;

	for (i = 0; i < min; i++)
	{
		cl[i] = maxlimbs[i] - minlimbs[i] - borrow;
		borrow = (cl[i] > minlimbs[i]);
	}

	c->len = min;

	while (borrow)
	{
		cl[i] = maxlimbs[i] - borrow;
		borrow = (cl[i++] > maxlimbs[i]);
		c->len++;
	}

	c->sign = max_sign;
	return BIGINT_SUCCESS;
}

int bigint_add(bigint* c, bigint* a, bigint* b)
{
	if (c == NULL || a == NULL || b == NULL)
		return BIGINT_ERR_INVALID_ARGS;

	if (a->sign != b->sign)
		return bigint_usub(c, a, b);

	return bigint_uadd(c, a, b);
}

int bigint_sub(bigint* c, bigint* a, bigint* b)
{
	if (c == NULL || a == NULL || b == NULL)
		return BIGINT_ERR_INVALID_ARGS;

	int r;

	if (a->sign == b->sign)
	{
		if ((r = bigint_usub(c, a, b)) < 0)
			return r;

		if (a->len >= b->len)
			c->sign = a->sign;
		else
			c->sign = -(a->sign);

		return BIGINT_SUCCESS;
	}

	return bigint_uadd(c, a, b);
}