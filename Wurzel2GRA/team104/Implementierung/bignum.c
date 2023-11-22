#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "headers/bignum.h"

/* Parser from string to an integer with error handling. Based on material from Arbeitsblatt 6 (Praktikums-Website) */
bool parse_integer(int * result_int, char * string) {
  char * endptr;
  errno = 0;
  long result = strtol(string, &endptr, 0);

  if (endptr == string) {
    fprintf(stderr, "\"%s\" could not be converted to number!\n", string);
    return false;
  } 

  if (errno != 0) {
    fprintf (stderr, "Number \"%s\" overflows long!\n", string);
    return false;
  } 

  while (isspace(*endptr))
    ++endptr;

  if (*endptr != '\0') {
    fprintf(stderr, "Further characters after number: \"%s\"!\n", endptr);
    return false;
  }

  if ((0 > result) || (result > INT32_MAX)) {
    fprintf(stderr, "Following value out of range: %ld\n" "Must be between 0 and INT32_MAX\n", result);
    return false;
  }

  *result_int = result;
  return true;
}

/* Parcer from uint64_t to a bignum for easier initilisation */
struct bignum bignum_uint64(uint64_t value, uint64_t base) {
  struct bignum bignum_value;
  memset(&bignum_value, 0, sizeof(bignum_value));

  uint64_t tmp = value;
  for (bignum_value.mantissa_size = 1; tmp >= base; bignum_value.mantissa_size++)
    tmp /= base;
  
  bignum_value.deallocate = bignum_value.mantissa = calloc(bignum_value.mantissa_size, sizeof(bignum_value.mantissa[0]));
  if (NULL == bignum_value.mantissa) {
    fprintf(stderr, "bignum_uint64: Memory allocation error!\n" "%d bytes could not be allocated\n", bignum_value.mantissa_size);
    return bignum_value;
  }

  int i = 0;
  do {
    bignum_value.mantissa[i++] = value % base;
    value /= base;
  } while (value != 0);
  
  bignum_value.exponent = bignum_value.mantissa_size;
  bignum_value.base = base;
  bignum_value.negative = false;

  return bignum_value;
}

/* Builds string representation of bignum with 'digits' decimal places for the output */
char * build_string(struct bignum x, int digits) {
  int i;
  char * buffer;
  uint32_t curr = 0;
  size_t buffer_size = 1;
  
  if (x.negative)
      ++buffer_size;
  /* 
    Possible examples of bignum: 
    0.01 = 0.1 * 10 ^ -1
    50.5 = 0.505 * 10 ^ 2
    220 = 0.22 * 10 ^ 3 
    Output: mantisa[exp - mantissa]
  */

  /* Case: exp <= 0 -> Output: 0.[-exp]mantissa */
  if (x.exponent <= 0) {
      buffer_size += digits + sizeof("-0.");
      buffer = malloc(buffer_size * sizeof(char));
      if (NULL == buffer) {
        fprintf(stderr, "build_string: Memory allocation error!\n" "%zu chars could not be allocated\n", buffer_size);
        return NULL;
      }

      if (x.negative) 
        buffer[curr++] = '-';
      
      buffer[curr++] = '0';
      buffer[curr++] = '.';

      int digits_to_add = digits;
      if (digits_to_add > (-x.exponent)) 
        digits_to_add = -x.exponent;
          
      for (i = 0; i < digits_to_add; ++i) {
        buffer[curr++] = '0';
      }
      
      digits -= digits_to_add;
      digits_to_add = digits;
      if (digits_to_add > (int)x.mantissa_size) 
        digits_to_add = x.mantissa_size;

      for (i = 0; i < digits_to_add; ++i) {
        uint8_t digit = x.mantissa[x.mantissa_size - 1 - i];
        if (digit < 10) {
          buffer[curr++] = '0' + digit;
        } else {
          buffer[curr++] = 'A' + digit - 10;
        }
      }
      for (i = digits_to_add; i < digits; ++i)
        buffer[curr++] = '0';
  } 
  /* Case: exp < mantissa -> Output: mant.issa, point_position = exp. */
  else if (x.exponent < (int32_t)x.mantissa_size) {
      buffer_size += x.mantissa_size + digits + sizeof("-.");
      buffer = malloc(buffer_size * sizeof(char));
      if (NULL == buffer) {
        fprintf(stderr, "build_string: Memory allocation error!\n" "%zu could not be allocated\n", buffer_size);
        return NULL;
      }

      if (x.negative) 
        buffer[curr++] = '-';
      
      uint32_t i;
      for (i = 0; i < x.mantissa_size; ++i) {
        if (x.exponent == (int32_t)i) 
          buffer[curr++] = '.';
        uint8_t digit = x.mantissa[x.mantissa_size - 1 - i];
        if (digit < 10) {
          buffer[curr++] = '0' + digit;
        } else {
          buffer[curr++] = 'A' + digit - 10;
        }
      }
      for (i = 0; i < (uint32_t)digits; ++i)
        buffer[curr++] = '0';
  } 
  /* Output: mantisa[exp - mantissa] */
  else { 
      buffer_size += x.exponent;

      buffer = malloc(buffer_size * sizeof(char));
      if (NULL == buffer) {
        fprintf(stderr, "build_string: Memory allocation error!\n" "%zu could not be allocated\n", buffer_size);
        return NULL;
      }

      if (x.negative) 
        buffer[curr++] = '-';
      
      for (uint32_t i = 0; i < x.mantissa_size; ++i) {
        uint8_t digit = x.mantissa[x.mantissa_size - 1 - i];
        if (digit < 10) {
          buffer[curr++] = '0' + digit;
        } else {
          buffer[curr++] = 'A' + digit - 10;
        }
      }
      for (int i = 0; i < x.exponent - (int32_t)x.mantissa_size; ++i) {
        buffer[curr++] = '0';
      }
  }
  buffer[curr++] = 0;
  return buffer;
}

/* Deletes all leading and trailing zeroes for correct calculations */
void normalize(struct bignum * x) {
  while (x->mantissa[x->mantissa_size - 1] == 0) {
    if (x->mantissa_size == 1) {
      x->exponent = 0;
      x->negative = false;
      return;
    }
    --x->mantissa_size;
    --x->exponent;
  }
  
  for ( ; x->mantissa[0] == 0; ++x->mantissa)
    --x->mantissa_size;
}

/* Deallocates memory used by bignum */
void free_bignum(struct bignum * x) {
  if (x->deallocate) 
    free(x->deallocate);
  x->deallocate = NULL;
}
