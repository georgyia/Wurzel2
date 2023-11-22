#ifndef SQRT2_H
#define SQRT2_H

#include "bignum.h"
#include "mul.h"
#include "div.h"

/* Enum for Hauptimplementierung, Optimierung and alternative Implementierung respectively  */
typedef enum {
  VERSION_0 = 0,
  VERSION_1 = 1,
  VERSION_2 = 2,
} version_t;

/* Enum for numeral system the root of two will be calculated */
typedef enum {
  DECIMAL,
  HEXADECIMAL,
} numeral_system_t;

/* Data structure to store values of options and arguments given in command-line. Default values are set in main.c */
typedef struct config_t{
  version_t version; // Version of implementation.
  int iteration; // Number of iterations of the function call for Benchmarking, if -B is set.
  int digits; // Number of binary decimal places to be printed.
  numeral_system_t numeral_system; // numeral system the root of two will be calculated
  bool show_help; // Flag for --help or -h.
  bool show_time; // // Flag to show results of time measurments.
  bool test; // Flag to run tests. 
  int test_numeral_system; // numeral system for testing, must be 10 or 16 only. 
} config_t;

/* Implementations can be found in corresponding c-file */
struct bignum sqrt2(size_t s, numeral_system_t base); // Hauptimplementierung. (VERSION_0) 
struct bignum sqrt2_V1(size_t s, numeral_system_t base); // Optimierung using Newton–Raphson. (VERSION_1) 
struct bignum sqrt2_V2(size_t s, numeral_system_t base); // Alternative Implementierung with vectorized multiplication. (VERSION_2) 
#endif
