#ifndef BIGNUM_H
#define BIGNUM_H

#include <inttypes.h>
#include <stdbool.h>

/* 
  The format for storing numbers of any size: the integer part and the fixed point in the exponent position.
  This format allows to store effectively both integers and rational numbers. 
  An example of storing integers: 25 = 25.0 -> 25 is integer part, point position = exponent = 2.
  An example of storing rational numbers: 0.00051 -> 51 is integer part, point position = exponent = -3. Leading and tailing 
  zeroes are not stored.
  The format allows to represent any number as value = 0.mantissa * base ^ exponent, which simplifies the implementation of 
  arithmetical operations. The mantissa is stored using Little Endian. Negative numbers are represented with negative flag set.
*/
struct bignum {
  uint8_t * mantissa; // array to store digits of value, Little Endian.
  void * deallocate; // pointer for memory deallocation.
  uint32_t mantissa_size; // number of digits in bignum.
  int32_t exponent; // point position within the number.
  uint32_t base; // numerical system base of bignum.
  bool negative; // flag for representation of negative numbers.
};

/* Implemenations can be found in corresponding c-file */
bool parse_integer(int * result_int, char * string); // Parser from string to an integer with error handling.
struct bignum bignum_uint64(uint64_t value, uint64_t base); // Parcer from uint64_t to a bignum for easier initialization. 
char * build_string(struct bignum x, int digits); // Builds string representation of bignum with 'digits' decimal places for output.
void normalize(struct bignum * result);  // Delets all leading and trailing zeroes for correct calculations.
void free_bignum(struct bignum * x); // Deallocates memory used by bignum.

#endif
