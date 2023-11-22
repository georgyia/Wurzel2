#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <getopt.h>

#include "headers/bignum.h"
#include "headers/div.h"
#include "headers/mul.h"
#include "headers/sqrt2.h"

/* Structure to store P(n1,n2), Q(n1, n2), T(n1, n2). All values are Integer. */
typedef struct pq_series_result_t {
  struct bignum p, q, t;
} pq_series_result_t;

/* 
  Binary Splitting: 
  sum_pq calculates three values: P(from, to), Q(from, to), T(from, to) in numerical base 'base'.
  sum_pq uses recurrent formula for calculation: 
    P(from, to) = P(from, mid) * P(mid, to), 
    Q(from, to) = Q(from, mid) * Q(mid, to),
    T(from, to) = T(from, mid) * Q(mid, to) + P(from, mid) * T(mid, to).
*/
pq_series_result_t sum_pq(uint32_t base, size_t from, size_t to) {
  pq_series_result_t pq_series_result;
  memset(&pq_series_result, 0, sizeof(pq_series_result));
  
  if (to - from == 1) { // Edge case: Interval consists of one element
    pq_series_result.p = bignum_uint64((from << 1) - 1, base); // p(from) = 2 * from - 1
    pq_series_result.q = bignum_uint64(from << 2, base); // q(from) = 4 * from
    pq_series_result.t = bignum_uint64((from << 1) - 1, base); // t(from) = 1 * p(from)
  } 
  else { // Main case: Description above
    size_t mid = (from + to) >> 1;
    pq_series_result_t low = sum_pq(base, from, mid);
    pq_series_result_t high = sum_pq(base, mid, to);

    pq_series_result.p = mul_bignum_karazuba(low.p, high.p);
    pq_series_result.q = mul_bignum_karazuba(low.q, high.q);
    struct bignum lt_hq = mul_bignum_karazuba(low.t, high.q);
    struct bignum lp_ht = mul_bignum_karazuba(low.p, high.t);

    pq_series_result.t = add_bignum(lt_hq, lp_ht);
    
    /* Memory deallocation of temporary results */
    free_bignum(&lp_ht);
    free_bignum(&lt_hq);
    free_bignum(&high.t);
    free_bignum(&high.q);
    free_bignum(&high.p);
    free_bignum(&low.t);
    free_bignum(&low.q);
    free_bignum(&low.p);
  }
  return pq_series_result;
}

/* 
  Hauptimplementierung:
  Calulates root of 2 using Binary-Splitting with at least s positions in numerical base 'base' after the point.
*/
struct bignum sqrt2(size_t s, numeral_system_t base) {
   s = (s < 5) ? 5 : s;
  uint32_t numeral_system_base = 0;
  size_t binary_digits = s; // Default value.

  switch (base) {
    case HEXADECIMAL:
      numeral_system_base = 0x10;
      binary_digits = (s << 2) + 1; // 1 Hexadecimal position requires 4 binary positions.
      break;
      
    case DECIMAL:
      numeral_system_base = 10;
      binary_digits = (s * 10 + 2) / 3 + 1; // 1 Decimal position requires 10/3 binary positions. 10 bits is enough to store 3 decimal positions.
      break;
  }
  
  /* Calculation of the formula given in the Aufgabenstellung */
  pq_series_result_t pq_series_result = sum_pq(numeral_system_base, 1, binary_digits);
  struct bignum fractional = div_bignum(pq_series_result.t, pq_series_result.q, s + 1);
  struct bignum one = bignum_uint64(1, numeral_system_base);
  struct bignum sqrt2_value = add_bignum(one, fractional);

  /* Memory deallocation of temporary results */
  free_bignum(&one);
  free_bignum(&fractional);
  free_bignum(&pq_series_result.p);
  free_bignum(&pq_series_result.q);
  free_bignum(&pq_series_result.t);

  return sqrt2_value;
}

/* Constant value 2 */
static struct bignum two = {
  .mantissa_size = 1,
  .deallocate = NULL,
  .mantissa = (uint8_t[]) { 2 },
  .negative = false,
  .exponent = 1,
};

/* Constant value 1/2 to the given base */
static struct bignum half_base = {
  .mantissa_size = 1,
  .deallocate = NULL,
  .mantissa = (uint8_t[]) { 0 },
  .negative = false,
  .exponent = 0,
};

struct bignum sqrt2_V2(size_t s, numeral_system_t base) {
  s = (s < 5) ? 5 : s;
  size_t bignum_digits = s + 1;
  uint32_t numeral_system_base = (base == DECIMAL) ? 10 : 0x10;
  two.base = numeral_system_base;
  half_base.base = numeral_system_base;
  half_base.mantissa[0] = numeral_system_base >> 1;

  /* 
    Target function: f(x) = x ^ 2 - 2.
    next = prev - f(prev) / f'(prev) = prev - ((prev^2 - 2) / (2 * prev)) = prev + 1/2(2/prev - prev)
  */

  size_t expected_accuracy = 2;

  /* Initial guess: 1 */
  struct bignum sqrt2_value = bignum_uint64(1, numeral_system_base);

  while(true) {
    struct bignum two_div_sqrt2_value = div_bignum(two, sqrt2_value, expected_accuracy);
    struct bignum double_residual = sub_bignum(two_div_sqrt2_value, sqrt2_value);
    struct bignum residual =  mul_bignum_karazuba(half_base, double_residual);
    struct bignum next_approximation = add_bignum(sqrt2_value, residual);

    bool residual_is_zero = (1 == residual.mantissa_size) && (0 == residual.mantissa[0]);

    /* Memory deallocation of temporary results */
    free_bignum(&two_div_sqrt2_value);
    free_bignum(&double_residual);
    free_bignum(&residual);
    free_bignum(&sqrt2_value);

    sqrt2_value = next_approximation;

    /* Checking whether all arithmetic calculations went well */
    if (NULL == sqrt2_value.mantissa)
      return sqrt2_value;

    if (residual_is_zero || -(residual.exponent) > (int32_t)bignum_digits + 4)
      break;

   /* expected_accuracy is chosen in a way, that 2 / sqrt2_value results with accurate number of decimal digits */
    expected_accuracy = 4 - (residual.exponent << 1);

    //fprintf (stderr, "exp = %d expecetd = %zd\n", residual.exponent, expected_accuracy);
  }
  truncate_mantissa(&sqrt2_value, bignum_digits);
  return sqrt2_value;
}

/* 
  Global variable to indicate usage of vectorised multiplication in Karazuba for VERSION_2.
  Otherwise alternative realisation with, for example, change of signature required larger amount of code and considered less concised.
*/
bool vectorised;

/* 
  Alternativ implementation (VERSION_2) with usage of vectorized multiplication. 
*/
struct bignum sqrt2_V1(size_t s, numeral_system_t base)  {
  vectorised = true;
  return sqrt2(s, base);
}
