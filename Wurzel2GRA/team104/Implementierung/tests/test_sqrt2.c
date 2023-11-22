#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "../headers/bignum.h"
#include "../headers/mul.h"
#include "../headers/sqrt2.h"

/* Data structure that contains relevant data for testing*/
typedef struct test_data_t {
  int test_count;
  char ** expected;
  int * test_length;
} test_data_t;

/* Correct closing of file */
void cleanup_file(FILE * file) {
  if (file) {
    fclose(file); 
  }
}

/* Correct struct cleaning up in case of errors */
void cleanup_test_data(struct test_data_t * test_data) {
  if (test_data->expected) {
    for (int i = 0; i < test_data->test_count; ++i) 
      if (test_data->expected[i])
        free(test_data->expected[i]);
    free(test_data->expected);
  }

  if (test_data->test_length)
    free(test_data->test_length);

  memset(test_data, 0, sizeof(*test_data));
}

/* Retrieving tests from file with error handling. Based on material from Arbeitsblatt 7 (Praktikums-Website) */
struct test_data_t read_test_data(char * filename) {
  struct test_data_t test_data;
  memset(&test_data, 0, sizeof (test_data));

  FILE * file;

  if (!(file = fopen(filename, "r"))) {
    fprintf(stderr, "Test: Error opening file %s for testing.\nThis test is sharpened for reading sqrt2_dec.txt or sqrt2_hex.txt", filename);
    cleanup_file(file);
    return test_data;
  }

  struct stat statbuf;
  if (fstat(fileno(file), &statbuf)) {
    fprintf(stderr, "Test: Error retrieving file stats!\nSomething wrong with the file %s\n", filename);
    cleanup_file(file);
    return test_data;
  }
  
  if (!S_ISREG(statbuf.st_mode) || statbuf.st_size <= 0) {
    fprintf(stderr, "Test: Error processing %s: Not a regular file or invalid size\n", filename);
    cleanup_file(file);
    return test_data;
  }

  #define BUFFER_SIZE 100
  char buffer[BUFFER_SIZE];

  if (!fgets(buffer, BUFFER_SIZE, file)) {
    fprintf(stderr, "Test: Error reading characters from the file %s\n", filename);
    cleanup_file(file);
    return test_data;
  }

  if (!parse_integer(&test_data.test_count, buffer)) {
    fprintf(stderr, "Test: Error parcing test_count from file %s\n", filename);
    cleanup_file(file);
    cleanup_test_data(&test_data);
    return test_data;
  }

  test_data.expected = calloc(test_data.test_count, sizeof(test_data.expected[0]));
  if (NULL == test_data.expected) {
    fprintf (stderr, "Test: Memory allocation error for test_data.expected.\nCould not allocate %d bytes\n", test_data.test_count);
    cleanup_file(file);
    cleanup_test_data(&test_data);
    return test_data;
  }

  test_data.test_length = calloc(test_data.test_count, sizeof(test_data.test_length[0]));
  if (NULL == test_data.test_length) {
    fprintf (stderr, "Test: Memory allocation error for test_data.test_length.\nCould not allocate %d\n", test_data.test_count);
    cleanup_file(file);
    cleanup_test_data(&test_data);
    return test_data;
  }

  for (int i = 0; i < test_data.test_count; ++i) {
    test_data.expected[i] = calloc(55, sizeof(test_data.expected[0]));
    if (!fgets(buffer, BUFFER_SIZE, file)) {
      fprintf(stderr, "Test: Error reading test_count\n");
      cleanup_file(file);
      cleanup_test_data(&test_data);
      return test_data;
    }

    char * position = strchr(buffer, ':');
    if (NULL == position || position - buffer != 55) {
      fprintf(stderr, "Test: Error reading expected amount of pre-calculated values!\nExpected: 55");
      cleanup_file(file);
      cleanup_test_data(&test_data);
      return test_data;
    }
    
    for (int j = 0; j < 5; ++j) {
      for (int k = 0; k < 10; ++k) {
        test_data.expected[i][10 * j + k] = buffer[11 * j + k];
      }
    }

    if (!parse_integer(&test_data.test_length[i], position + 2)) {
      fprintf(stderr, "Test: Invalid value %d passed to parse_integer\n", test_data.test_length[i]);
      cleanup_file(file);
      cleanup_test_data(&test_data);
      return test_data;
    }
  }
  cleanup_file(file);

  return test_data;
}

/* 
  Tester for the root of 2 function in required numeral system with parameters from 50 to 75000000 (for future optimisations).
  Retrieves pre-calculated values from the appropriate file, compares them with the results of our calculations and measures, if 
  requested, the amount of time it took to calculate certain value. Warning: time measurements will be automatically executed 
  with more iterations, if parameter doesn't satisfy following rules:
  - 1 second between measurements for each implementation.
  - Minimum 3 repetitions of the measurements.
  Moreover measurements are executed 3 times and the median of all averages then considered as the actually determined result.

  It should be noted that when checking the accuracy of the result with a large number of requested places, not every 
  single place is checked. To ensure adequate testing efficiency, it is sufficient to test first 100 digits of the 
  calculations digit by digit and then only check the last 50 digits of each result. 

  If the first 100 are correct, it can be assumed that the values will not deviate from further results "in the middle". 
  This is based on the assumption that the accuracy of the result is constant and does not change in certain ranges. 
  However we test the last 50 digits of each number and stop the tests if the values do not match. 
  This method is valid only under certain conditions and for certain applications, and in other cases a full check of 
  all places may be required.

  How precalculated values are stored is shown in corresponding file.
  Sourse for values: http://www.numberworld.org/digits/Sqrt(2)  and https://catonmat.net/tools/generate-sqrt2-digits

  Example calls: 
    ./main -t 10 -V 1 - run tests for in decimal numeral system for VERSION_1 to check correctness.
    ./main -t 16 -B 3 - run tests for hexadecimal numeral system for Hauptimplementirung(default) to check correctness and measure time. 
  
  Further information regarding testing can be found in Ausarbeitung.
 */
bool test(config_t config) {
  /* Adjusting testing for current numeral_system */
  char * filename = (config.numeral_system == DECIMAL) ? "tests/sqrt2_dec.txt" : "tests/sqrt2_hex.txt";
  char * numeral_system_string = (config.numeral_system == DECIMAL) ? "DECIMAL" : "HEXADECIMAL";
  char * bench_string = (config.show_time) ? "with Benchmarking" : "";
  printf("TESTING %s %s:\n", numeral_system_string, bench_string);
  if (config.show_time) {
    printf("Measurements are executed 3 times and the median of all averages then considered as the actually determined result.\n");
    printf("\n");
  }

  struct test_data_t test_data = read_test_data(filename);
  if (test_data.test_count == 0)
    return false;

  for (int t = 0; t < test_data.test_count; ++t) {
    struct bignum sqrt2_value;
    memset(&sqrt2_value, 0, sizeof(sqrt2_value));

    config.digits = test_data.test_length[t];
    int i;
    double time;

    if (config.show_time) {
      printf("Testing %d digits...\n", test_data.test_length[t]);
    }

    /* In case tests are used for Benchmarking, following code is executed */
    if (config.show_time) { 
      int iteration_number;
      /* Making sure there is at least 1 second between measurements for each implementation */
      if (test_data.test_length[t] <= 1000) {
        iteration_number = (int)5e7 / test_data.test_length[t] / test_data.test_length[t]; // T^2 * x >= 1 --> x >= 1/T^2. 
      } else {
        iteration_number = (config.iteration >= 3) ? config.iteration : 3;
      }

      if (config.iteration < iteration_number) {
        fprintf(stderr, "Number of iterations you gave (%d) for this test is too small to ensure 1 Second and minimum 3 iterations of measurements.\n", config.iteration);
        fprintf(stderr, "Number of iterations is set appropriately to %d in order to satisfy the conditions above.\n", iteration_number);
      }

      #define PROBE_NUMBER (3)
      double times[PROBE_NUMBER];
      struct bignum (* current_sqrt) (size_t, numeral_system_t) = sqrt2;
      struct bignum probe_values[PROBE_NUMBER];

      switch (config.version) {
        case VERSION_0:
          current_sqrt = sqrt2;
          break;
        
        case VERSION_1:
          current_sqrt = sqrt2_V1;
          break;

        case VERSION_2:
          current_sqrt = sqrt2_V2;
          break; 
      }

      struct timespec start;
      struct timespec end;

      for (int k = 0; k < PROBE_NUMBER; ++k) {
        struct bignum sqrt2_values[iteration_number];
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (i = 0; i < iteration_number; ++i)
          sqrt2_values[i] = current_sqrt(config.digits, config.numeral_system);
        clock_gettime(CLOCK_MONOTONIC, &end);

        probe_values[k] = sqrt2_values[0];
        for (i = 1; i < iteration_number; ++i) {
          free_bignum(&sqrt2_values[i]);
        }

        times[k] = ((end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec)) / iteration_number);
        printf("Probe %d, iterations: %d, time: %f\n", k + 1, iteration_number, times[k]);
      }
      sqrt2_value = probe_values[0];
      for (i = 1; i < PROBE_NUMBER; ++i) {
        free_bignum(&probe_values[i]);
      }

      /* Selection of the median with Bubble-Sort*/
      for (int i = 0; i < PROBE_NUMBER - 1; ++i) {
        for (int j = 0; j < PROBE_NUMBER - i - 1; ++j) {
          if (times[j] > times[j + 1]) {
            double tmp = times[j];
            times[j] = times[j + 1];
            times[j + 1] = tmp;
          }
        }
      }
      time = times[1];
    } 
    else {
      switch (config.version) {
        case VERSION_0:
          sqrt2_value = sqrt2(config.digits, config.numeral_system);
          break;
        
        case VERSION_1:
          sqrt2_value = sqrt2_V1(config.digits, config.numeral_system);
          break;

        case VERSION_2:
          sqrt2_value = sqrt2_V2(config.digits, config.numeral_system);
          break;
      }
    }

    char * actual = build_string(sqrt2_value, test_data.test_count);

    /* Testing if mismatch between actual and expected occured */
    for (i = 0; i < 50; ++i) {
      if (test_data.expected[t][i] != actual[test_data.test_length[t] + i + 2 - 50]) {
        fprintf(stderr, "Test mismatch %s for %d digits : \n%s\n%s!\n", numeral_system_string, test_data.test_length[t], test_data.expected[t], &actual[test_data.test_length[t] + 2 - 50]);
        free(actual);
        cleanup_test_data(&test_data);
        return false;
      }
    }
    free(actual);

    /* In this case all went well and results are going to be printed accordingly */
    char * version;
    switch (config.version) {
      case VERSION_0:
        version = ("Hauptimplemetierung");
        break;
      
      case VERSION_1:
        version = ("VERSION_1");
        break;

      case VERSION_2:
        version = ("VERSION_2");
        break;
    }

    if (config.show_time) {
      printf("Test on %d %s digits using %s worked for %f seconds!\n", test_data.test_length[t], numeral_system_string, version, time);
      printf("\n");
    } 
    else {
      printf("Test on %d %s digits using %s worked!\n", test_data.test_length[t], numeral_system_string, version);
    }
    free_bignum(&sqrt2_value);
  }
  cleanup_test_data(&test_data);
  return true;
}

