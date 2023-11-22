#ifndef ADD_SUB_H
#define ADD_SUB_H

#include "bignum.h"

/* Implemenations can be found in corresponding c-file */
struct bignum sub_bignum(struct bignum a, struct bignum b);
struct bignum add_bignum(struct bignum a, struct bignum b);

#endif
