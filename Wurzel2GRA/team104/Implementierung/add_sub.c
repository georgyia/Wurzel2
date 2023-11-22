#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "headers/add_sub.h"

/* 
  Helper-funtion to calculate a_shift and b_shift to adjust the positions of digits in bignums for addition or subtraction: 
  Calculation based of example for a_shift and b_shift:
  a: 0.134 * 10 ^ 2 = 13.4; decPlace(1) = 1, decPlace(3) = 0, decPlace(4) = -1;
  b: 0.25 * 10 ^ 0 =  0.25; decPlace(0) = 0, decPlace(2) = -1, decPlace(5) = -2;
  a_shift, b_shift = (0 - 2) - (2 - 3) = -1; -> a_decPlace > b_decPlace;
  a_shift = 1, b_shift = 0;
*/
struct bignum add_sub_prepare(struct bignum * a, struct bignum * b, int * _b_shift) {
  struct bignum result;
  memset(&result, 0, sizeof (result));

  /* Checking that a and b are valid bignums */
  if ((NULL == a->mantissa) || (0 == a->mantissa_size) || (NULL == b->mantissa) || (0 == b->mantissa_size))
    return result;
  
  if (a->base != b->base)
    return result;

  if (a->negative != b->negative)
    return result;

  /* Precautions */
  normalize(a);
  normalize(b);

  int a_shift, b_shift = (b->exponent - b->mantissa_size) - (a->exponent - a->mantissa_size); 

  /* a_decPlace <= b_decPlace */
  if (b_shift >= 0) {
    a_shift = 0;
  } 
  else { /* a_decPlace > b_decPlace */
    a_shift = -b_shift;
    b_shift = 0;
  }
  *_b_shift = b_shift;

  /* result.mantissa_size = max{a->mantissa_size + a_shift, b->mantissa_size + b_shift} */
  result.mantissa_size = a->mantissa_size + a_shift;
  if (b->mantissa_size + b_shift > result.mantissa_size)
    result.mantissa_size = b->mantissa_size + b_shift;

  /* result.exponent = max{a.exponent, b.exponent} */
  result.exponent = a->exponent;
  if (b->exponent > result.exponent)
    result.exponent = b->exponent;
  
  result.deallocate = result.mantissa = calloc(result.mantissa_size + 1, sizeof(result.mantissa[0]));
  if (NULL == result.mantissa) {
    fprintf (stderr, "add_sub_prepare: Memory allocation error!\nCould not allocate %d bytes\n", result.mantissa_size + 1);
    return result;
  }

  result.base = a->base;
  result.negative = a->negative;

  /* result.mantissa[i + a_shift] = a.mantissa[i] */
  memcpy(&result.mantissa[a_shift], a->mantissa, a->mantissa_size * sizeof(result.mantissa[0]));
  return result;
}

/* Addition of two bignums with same sign */
struct bignum add_same_sign(struct bignum a, struct bignum b) {
  int i, b_shift;
  struct bignum result = add_sub_prepare(&a, &b, &b_shift);
  if (NULL == result.mantissa)
    return result;

  int32_t carry = 0;
  for (i = 0; i < (int32_t)b.mantissa_size; ++i) {
    uint64_t digit = carry + result.mantissa[i + b_shift] + b.mantissa[i];
    carry = 0;
    if (digit >= result.base) {
      carry = 1;
      digit -= result.base;
    }
    result.mantissa[i + b_shift] = digit;
  }
  
  /* In case of carry on highest position, fix the result to the current base */
  if (carry) {
    i += b_shift;
    while (result.mantissa[i] == result.base - 1)
      result.mantissa[i++] = 0;
    ++result.mantissa[i];
    
    if (i == (int32_t)result.mantissa_size) {
      ++result.mantissa_size;
      ++result.exponent;
    }
  }
  
  normalize(&result);
  return result;
}

/* Subtraction of two bignums with same sign */
struct bignum sub_same_sign(struct bignum a, struct bignum b) {
  int i, b_shift;
  struct bignum result = add_sub_prepare(&a, &b, &b_shift);
  if (NULL == result.mantissa)
    return result;

  result.mantissa[result.mantissa_size] = 1; // Extra carry to borrow from in case a < b

  int32_t carry = 0;
  for (i = 0; i < (int32_t)b.mantissa_size; ++i) {
    int64_t digit = carry + result.mantissa[i + b_shift] - b.mantissa[i];
    carry = 0;
    if (digit < 0) {
      carry = -1;
      digit += result.base;
    }
    result.mantissa[i + b_shift] = digit;
  }
  
  if (carry) {
    i += b_shift;
    while (result.mantissa[i] == 0)
      result.mantissa[i++] = result.base - 1;
    --result.mantissa[i];
    
    /* Number is negative -> convert from double compliment to normal representation in current base */
    if (i == (int32_t)result.mantissa_size) {
      carry = 0;
      for (i = 0; i < (int32_t)result.mantissa_size; ++i) {
        int32_t digit = carry - result.mantissa[i];
        carry = 0;
        if (digit < 0) {
          carry = -1;
          digit += result.base;
        }
        result.mantissa[i] = digit;
      }
      result.negative = !result.negative;
    }
  }
  
  normalize(&result);
  return (result);
}

struct bignum add_bignum(struct bignum a, struct bignum b) {
  if (a.negative == b.negative)
    return (add_same_sign(a, b)); 
  /* 
    In case signes are different, a + b can be viewed as a - (-b) 
    If b is negative -> a + (-b) = a - b. 
    If a is negative -> (-a) + b = (-a) - (-b) 
  */
  b.negative = !b.negative;
  return (sub_same_sign(a, b));
}

struct bignum sub_bignum(struct bignum a, struct bignum b) {
  if (a.negative == b.negative)
    return (sub_same_sign(a, b));
  /* Same idea as in Addition */
  b.negative = !b.negative;
  return (add_same_sign(a, b));
}
