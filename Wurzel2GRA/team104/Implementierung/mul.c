#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <emmintrin.h>

#include "headers/mul.h"

/*  
  Simple multiplication, works in O(a.mantissa_size * b.mantissa_size) time.
*/ 
struct bignum mul_bignum_V0(struct bignum a, struct bignum b) {
  struct bignum result;
  memset(&result, 0, sizeof (result));

  /* Checking that a and b are valid bignums */
  if ((NULL == a.mantissa) || (0 == a.mantissa_size) || (NULL == b.mantissa) || (0 == b.mantissa_size))
    return result;
  
  if (a.base != b.base)
    return result;

  /* Precautions */
  normalize(&a);
  normalize(&b);
  
  result.mantissa_size = a.mantissa_size + b.mantissa_size;
  result.exponent = a.exponent + b.exponent; // By Multiplication lenth of decimal parts are summed up
  result.negative = a.negative ^ b.negative;
  result.base = a.base;
  
  result.deallocate = result.mantissa = calloc(result.mantissa_size, sizeof(result.mantissa[0]));
  if (NULL == result.mantissa) {
    fprintf(stderr, "Multiplication: Memory allocation error!\n" "%d bytes could not be allocated\n", result.mantissa_size);
    return result;
  }
 
  uint32_t i, j;
  uint32_t carry = 0;
  for (i = 0; i < result.mantissa_size; ++i) {
    uint32_t l = 0;
    if (b.mantissa_size + l < i + 1) { // l = max{0, i + 1 - |b|}
      l = i + 1 - b.mantissa_size;
    }
    
    uint32_t r = i;
    if (r > a.mantissa_size - 1) { // r =  min{|a| - 1, i}
      r = a.mantissa_size - 1;
    }
    
    for (j = l; j <= r; ++j)
      carry += a.mantissa[j] * (uint32_t)b.mantissa[i - j];
    result.mantissa[i] = carry % result.base;
    carry /= result.base;
  }
  
  normalize(&result);
  return result;
}

/*  
  Vectorized multiplication. Vector multiplication of 8 digits from a.mantissa with 8 digits from b.mantissa at one time. 
*/ 
struct bignum mul_bignum_V1(struct bignum a, struct bignum b) {
  struct bignum result;
  memset(&result, 0, sizeof (result));

  /* Checking that a and b are valid bignums */
  if ((NULL == a.mantissa) || (0 == a.mantissa_size) || (NULL == b.mantissa) || (0 == b.mantissa_size))
    return result;
  
  if (a.base != b.base)
    return result;

  /* Precautions */
  normalize(&a);
  normalize(&b);
  
  result.mantissa_size = a.mantissa_size + b.mantissa_size;
  result.exponent = a.exponent + b.exponent; // By Multiplication lenth of decimal parts are summed up
  result.negative = a.negative ^ b.negative;
  result.base = a.base;
  
  result.deallocate = result.mantissa = calloc(result.mantissa_size, sizeof(result.mantissa[0]));
  if (NULL == result.mantissa) {
    fprintf(stderr, "Multiplication: Memory allocation error!\n" "%d bytes could not be allocated\n", result.mantissa_size);
    return result;
  }

  #define VEC_SIZE (8)

  uint32_t i, j;
  uint32_t carry = 0;
  for (i = 0; i < result.mantissa_size; ++i) {
  	uint32_t l = 0;
  	if (b.mantissa_size + l < i + 1) {
  		l = i + 1 - b.mantissa_size;
    }
  	
  	uint32_t r = i + 1;
  	if (r > a.mantissa_size) {
  		r = a.mantissa_size;
    }
  	
  	for (j = l; j + VEC_SIZE < r; j += VEC_SIZE) {
      int32_t i_minus_j = i - j;
      __m128i vector_a = _mm_set_epi16(a.mantissa[j], a.mantissa[j + 1], a.mantissa[j + 2], a.mantissa[j + 3], a.mantissa[j + 4], a.mantissa[j + 5], a.mantissa[j + 6], a.mantissa[j + 7]);
      __m128i vector_b = _mm_set_epi16(b.mantissa[i_minus_j], b.mantissa[i_minus_j - 1], b.mantissa[i_minus_j - 2], b.mantissa[i_minus_j - 3], b.mantissa[i_minus_j - 4], b.mantissa[i_minus_j - 5], b.mantissa[i_minus_j - 6], b.mantissa[i_minus_j - 7]);
	    __m128i a_mul_b = _mm_mullo_epi16(vector_a, vector_b);
      /* Retrieving right parts from result vector */
	    carry += _mm_extract_epi16(a_mul_b, 0) + _mm_extract_epi16(a_mul_b, 1) + _mm_extract_epi16(a_mul_b, 2) + _mm_extract_epi16(a_mul_b, 3) + _mm_extract_epi16(a_mul_b, 4) + _mm_extract_epi16(a_mul_b, 5) + _mm_extract_epi16(a_mul_b, 6) + _mm_extract_epi16(a_mul_b, 7);
    }

    for( ; j < r; ++j) {
      carry += a.mantissa[j] * b.mantissa[i - j];
    }

    result.mantissa[i] = carry % result.base;
    carry /= result.base;
  }
  normalize(&result);
  return result;
}

extern bool vectorised;

/*
  Karazuba-Multiplikation: works in O((max{a.mantissa_size, b.mantissa_size}) ^ 1.59) time.
  a = (ah + al * base ^ (-half_size)) * base ^ (a.exponent).
  b = (bh + bl * base ^ (-half_size)) * base ^ (b.exponent).
  Exponents will be added in the end -> Integer-Multiplication of mantissas.
  a' * b' = (ah + al * base ^ (-half_size)) * (bh + bl * base ^ (-half_size)) =
  = ah * bh + ((ah + al) * (bh + bl) - ah * bh - al * bl) * base ^ (-half_size) + al * bl * base ^ (-2 * half_size) =
  = ahbh + (ahal * bhbl - ahbh - albl) * base ^ (-half_size) + albl * base ^ (-2 * half_size).
*/
struct bignum mul_bignum_karazuba(struct bignum a, struct bignum b) {
  bool vectorized_mul = vectorised;
  struct bignum result;
  memset(&result, 0, sizeof (result));

   /* Checking that a and b are valid bignums */
  if ((NULL == a.mantissa) || (0 == a.mantissa_size) || (NULL == b.mantissa) || (0 == b.mantissa_size))
    return result;
  
  if (a.base != b.base)
    return result;

  /* Precautions */
  normalize(&a);
  normalize(&b);

  /* Making a.mantissa_size >= b.mantissa_size */
  if (b.mantissa_size > a.mantissa_size) {
    struct bignum swap = a;
    a = b;
    b = swap;
  }

  /* If significant positions of b less than 32 -> simple multiplication works faster than Karazuba */
  if (b.mantissa_size <= 32) {
    if (vectorized_mul) {
      return mul_bignum_V1(a, b); // vectorized multiplikation
    }
    return mul_bignum_V0(a, b); // sequential multiplication 
  }
    
  /* Deducing half of mantissa and starting to slice bignums */
  size_t half_mantissa_size = (a.mantissa_size + 1) >> 1;
  struct bignum al, ah;
  memset(&al, 0, sizeof(al));
  memset(&ah, 0, sizeof(ah));

  /* 
    a = ah + al * base ^ (-half_mantissa_size);
    Because a.exponent is ignored, a should be seen as [0].(a.mantissa);
    For example: a = 0.12345 = [5, 4, 3, 2, 1] Little Endian!, half_mantissa_size = 3
    ah = 0.123 = [3, 2, 1], al = 0.45 = [5, 4] -> a = 0.123 + 0.45 * 10 ^ (-3)
  */

  ah.mantissa = &a.mantissa[a.mantissa_size - half_mantissa_size];
  ah.mantissa_size = half_mantissa_size;
  ah.base = a.base;
  
  al.mantissa = a.mantissa;
  al.mantissa_size = a.mantissa_size - half_mantissa_size;
  al.base = a.base;

  /* 
    Simple case, when b is too short -> bl is 0:
    a * b = (ah + al * base ^ (-half_size)) * bh = ah * bh + al * bh * base ^ (-half_size)
  */
  if (b.mantissa_size <= half_mantissa_size) {
    struct bignum bh;
    memset(&bh, 0, sizeof(bh));
    
    bh.mantissa = b.mantissa;
    bh.mantissa_size = b.mantissa_size;
    bh.base = b.base;
    
    struct bignum ah_mul_bh = mul_bignum_karazuba(ah, bh);
    struct bignum al_mul_bh = mul_bignum_karazuba(al, bh);

    al_mul_bh.exponent -= half_mantissa_size; // equivalent to multiplication by (base ^ (-half_size));
    result = add_bignum(ah_mul_bh, al_mul_bh);

    free_bignum(&ah_mul_bh);
    free_bignum(&al_mul_bh);
  } 
  /* Main case: a' * b' = ahbh + (ahal * bhbl - ahbh - albl) * base ^ (-half_size) + albl * base ^ (-2 * half_size) */
  else { 
    struct bignum bl, bh;
    memset(&bl, 0, sizeof(bl));
    memset(&bh, 0, sizeof(bh));

    /* b = (bh + bl * base ^ (-half_size)) */
    bh.mantissa = &b.mantissa[b.mantissa_size - half_mantissa_size];
    bh.mantissa_size = half_mantissa_size;
    bh.base = b.base;

    bl.mantissa = b.mantissa;
    bl.mantissa_size = b.mantissa_size - half_mantissa_size;
    bl.base = b.base;

    struct bignum ah_mul_bh = mul_bignum_karazuba(ah, bh); 
    struct bignum al_mul_bl = mul_bignum_karazuba(al, bl);
    struct bignum ah_add_al = add_bignum(ah, al);
    struct bignum bh_add_bl = add_bignum(bh, bl);
    struct bignum ahal_mul_bhbl = mul_bignum_karazuba(ah_add_al, bh_add_bl);
    struct bignum ahal_mul_bhbl_sub_ahbh = sub_bignum(ahal_mul_bhbl, ah_mul_bh);
    struct bignum ahal_mul_bhbl_sub_ahbh_sub_albl = sub_bignum(ahal_mul_bhbl_sub_ahbh, al_mul_bl);
    
    ahal_mul_bhbl_sub_ahbh_sub_albl.exponent -= half_mantissa_size; // multiplication by (base ^ (-half_size))
    struct bignum ahbh_add_ahal_mul_bhbl_sub_ahbh_sub_albl = add_bignum(ah_mul_bh, ahal_mul_bhbl_sub_ahbh_sub_albl);
    al_mul_bl.exponent -= half_mantissa_size << 1; // multiplication by (base ^ (-2 * half_size))
    result = add_bignum(ahbh_add_ahal_mul_bhbl_sub_ahbh_sub_albl, al_mul_bl);

    /* Memory deallocation of temporary results */
    free_bignum(&ahbh_add_ahal_mul_bhbl_sub_ahbh_sub_albl);
    free_bignum(&ahal_mul_bhbl_sub_ahbh_sub_albl);
    free_bignum(&ahal_mul_bhbl_sub_ahbh);
    free_bignum(&ahal_mul_bhbl);
    free_bignum(&bh_add_bl);
    free_bignum(&ah_add_al);
    free_bignum(&al_mul_bl);
    free_bignum(&ah_mul_bh);
  }
  
  /* Returning the exponent to the result: In multiplication lenths of decimal parts are summed up */
  result.exponent += a.exponent + b.exponent;
  result.negative = a.negative ^ b.negative;

  return result;
}
