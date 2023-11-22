#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "headers/mul.h"
#include "headers/div.h"

/* Makes the length of x.mantissa less or equal than mantissa_size */
void truncate_mantissa(struct bignum * x, size_t mantissa_size) {
  if (x->mantissa_size > mantissa_size) {
    memmove(x->mantissa, &x->mantissa[x->mantissa_size - mantissa_size], mantissa_size * sizeof(x->mantissa[0]));
    x->mantissa_size = mantissa_size;
  }
}

/* Constant value 1 */
static struct bignum one = {
  .mantissa = (uint8_t[]){ 1 },
  .mantissa_size = 1,
  .exponent = 1,
  .negative = false,
};

/*
  Division a/b with n digit precision:
  Calculates a/b, as a * (1/b).
  Reciprocal (1/b) is calculated using Newton–Raphson method.
*/
struct bignum div_bignum(struct bignum a, struct bignum b, size_t n) {
  struct bignum reciprocal;
  memset(&reciprocal, 0, sizeof(reciprocal));
  
  /* Checking that a and b are valid bignums */
  if ((NULL == a.mantissa) || (0 == a.mantissa_size) || (NULL == b.mantissa) || (0 == b.mantissa_size))
    return reciprocal;

  if (a.base != b.base)
    return reciprocal;

  /* Precautions */
  normalize(&a);
  normalize(&b);

  if ((b.mantissa_size == 1) && (b.mantissa[0] == 0)) {
    fprintf(stderr, "Division by 0!\n");
    return reciprocal;
  }

  /* Initial guess = 1 */ 
  reciprocal = bignum_uint64(1, b.base);

  one.base = b.base;
  a.exponent -= b.exponent;
  b.exponent = 0;

  /*
    Calculates the next reciprocal:
    Target function: f(x) = 1/x - b.
    next_reciprocal = reciprocal * (2 - b * reciprocal) = reciprocal + (1 - b * reciprocal) * reciprocal.
  */
 
  while (true) {
    struct bignum b_mul_reciprocal = mul_bignum_karazuba(b, reciprocal);
    struct bignum one_sub_b_mul_reciprocal = sub_bignum(one, b_mul_reciprocal);
    struct bignum residual = mul_bignum_karazuba(reciprocal, one_sub_b_mul_reciprocal); // residual shows error of current approximation
    struct bignum next_reciprocal = add_bignum(reciprocal, residual);
    
    /* Memory deallocation of temporary results */
    free_bignum(&b_mul_reciprocal);
    free_bignum(&one_sub_b_mul_reciprocal);
    free_bignum(&residual);
    free_bignum(&reciprocal);

    if (NULL == next_reciprocal.mantissa)
      return next_reciprocal;
    reciprocal = next_reciprocal;

    /* 
      Error of the current step is around residual.exponent.
      Let's ignore reciprocal positions with indices greater than -2 * residual.exponent + 4.
      These positions don't affect correctness of answer, but speeds the calculations up.
    */
    int truncate_length = 4 - (residual.exponent << 1);
    
    /* Truncating insignificant positions */
    truncate_mantissa(&reciprocal, truncate_length);

    if (-residual.exponent > (int32_t)n) // Approxomation is close enough to the required precision -> stop.
      break;
  }
  struct bignum result = mul_bignum_karazuba(a, reciprocal);
  truncate_mantissa(&result, n);
  free_bignum(&reciprocal);
  reciprocal.negative = a.negative ^ b.negative;
  return result;
}
