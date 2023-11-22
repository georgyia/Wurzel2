#ifndef DIV_H
#define DIV_H

#include "bignum.h"

/* Implemenations can be found in corresponding c-file */
struct bignum div_bignum(struct bignum a, struct bignum b, size_t n);
void truncate_mantissa(struct bignum * x, size_t mantissa_size);

#endif
