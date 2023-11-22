#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <getopt.h>
#include <time.h>
#include <ctype.h>

#include "headers/bignum.h"
#include "headers/div.h"
#include "headers/mul.h"
#include "headers/sqrt2.h"
#include "tests/test_sqrt2.c"

/* Global variable for numeral_system */
uint32_t base = 10;

/* 
  Function to parse command-line arguments using getopt_long. Catches invalid arguments.
  Call --help or -h for full description of available command-line arguments.
*/
bool parse_options(config_t * config, int argc, char * argv[]) {
  int ch;
  static struct option longopts[] = {
    { "help", optional_argument, NULL, 'h' },
    { NULL,          0,          NULL,  0  },
  };

  bool flag_set = false; // flag to indicate priority of -d over -h;

  while ((ch = getopt_long(argc, argv, "h::d:V:B:t:", longopts, NULL)) != -1) {
    switch (ch) {
      case 'V': 
      {
        int version;
        if (!parse_integer(&version, optarg)) {
          fprintf(stderr, "Something went wrong while parsing argument for option V.\nPlease use -h or --help for valid function call examples\n");
          return false;
        }

        if ((version < VERSION_0) || (version > VERSION_2)) {
          fprintf(stderr, "Invalid version: %d\n" "Valid versions: 0, 1, 2\n", version);
          return false;
        }

        config->version = version;
        break;
      }

      case 'B':
      {
        if (!parse_integer(&config->iteration, optarg)) {
          fprintf(stderr, "Something went wrong while parsing argument for option B.\nPlease use -h or --help for valid function call examples\n");
          return false;
        }
    
        if (config->iteration < 1) {
          fprintf(stderr, "Ivalid iteration for benachmarking: %d.\n" "Must be at least greater than 0\n", config->iteration);
          return false;
        }

        config->show_time = true;
        break;
      }

      case 'h':
        if ((optarg != NULL) || ((optind < argc) && (argv[optind][0] != '-'))) {
          int tmp;
          if (!parse_integer(&tmp, optarg ? optarg : argv[optind])) {
            fprintf(stderr, "Something went wrong while parsing argument for option h.\nPlease use -h or --help for valid function call examples\n");
            return false;
          }
          if (!flag_set) {
            config->numeral_system = HEXADECIMAL;
            config->digits = tmp;
            flag_set = true;
          } 
        } else {
          config->show_help = true;
        }
        break;
      
      case 'd':
        if (!parse_integer(&config->digits, optarg)) {
          fprintf(stderr, "Something went wrong while parsing argument for option d.\nPlease use -h or --help for valid function call examples\n");
          return false;
        }
        config->numeral_system = DECIMAL;
        flag_set = true;
        break;

      case 't':
        if (!parse_integer(&config->test_numeral_system, optarg)) {
          fprintf(stderr, "Something went wrong while parsing argument for option t.\nPlease use -h or --help for valid function call examples\n");
          return false;
        }
        if ((config->test_numeral_system != 10) && (config->test_numeral_system != 16)) {
          fprintf(stderr, "Ivalid value for option t. Must be 10 for DECIMAL or 16 for HEXADECIMAl!\n");
          return false;
        }
        config->test = true;
        break;
      
      case '?':
        fprintf(stderr, "Use --help or -h for available command-line arguments\n");
        return false;
    }
  }
  return true;
}

int main(int argc, char *argv[]) {
  /* Definition of default values */ 
  config_t config = {
    .version = VERSION_0,
    .iteration = 1,  
    .digits = 10,
    .numeral_system = DECIMAL,
    .show_help = false,
    .show_time = false,
    .test = false,
    .test_numeral_system = 10,
  };

  /* Parcing command-line arguments */ 
  if (!parse_options(&config, argc, argv))
    return EXIT_FAILURE;

  /* In case option for help was set, help-message will be shown and program ends afterwards */ 
  if (config.show_help) {
    printf("\n\nOptions:\n\n");
    printf("-V <Zahl>\tSpecifies which implementation to use. -V 0 should use the main implementation.  \n\t\tIf this option is not set, the main implementation should also be executed. (Default: 0)\n\n");
    printf("-B <Zahl>\tIf set, the runtime of the specified implementation is measured and displayed.\n \t\tThe optional argument of this flag specifies the number of repetitions of the function call. (Default: 1)\n\n");
    printf("-d <Zahl>\tOutput of n decimal digits after the decimal point. Has priority over -h. (Default: 10)\n\n");
    printf("-h <Zahl>\tOutput of n hexadecimal digits after the decimal point. (Default: 10)\n\n");
    printf("-h \t\tA description of all the options of the program and usage examples.\n\n");
    printf("--help\t\tA description of all the options of the program and usage examples.\n\n");
    printf("-t <Zahl>\tRun automatic tests. The optional argument of this flag specifies the numeral system the test should be executed for.\n\t\tMust be 10 for DECIMAL or 16 for HEXADECIMAl.\n\t\tCan be paired with other options in order to test correctness or measure and dispaly runtime of certain implementation. See usage examples.\n\n");

    printf("Usage examples:\n\n");
    printf("Example Input:  -V 1\nExample output: 1.4142135623\nExplanation: \tThe option for the version 1 is selected. \n\t\tBy default, the precision of ten decimal places is selected. \n\t\tOne function call is made.\n\n");
    printf("Example Input:  -B 2\nExample output: 1.4142135623\n\t\tVersion_0 done in 0.000339 seconds\nExplanation: \tTwo function calls were selected here. \n\t\tPerformance measurement required. \n\t\tIn this case, version 0 of the code and ten decimal places presision are at default.\n\n");
    printf("Example Input:  -B 2 -V 1 -h 30\nExample output: 1.6A09E667F3BCC908B2FB1366EA957D\n\t\tVersion_1 done in 0.000930 seconds\nExplanation: \tPerformance measurement with two function calls required. \n\t\tThe option of version 1 and 30 hexadecimal decimal places was chosen.\n\n");
    printf("Example Input:  -B 2 -V 1 -d 3\nExample output: 1.414\n\t\tVersion_1 done in 0.000124 seconds\nExplanation: \tPerformance measurement with two function calls required. \n\t\tThe option of version 1 with 3 decimal places was chosen.\n\n");
    printf("Example Input:  -V 0 -d 30\nExample output: 1.414213562373095048801688724209\nExplanation: \tThe option of version 0 of the code and 30 hexadecimal decimal places has been selected. \n\t\tOne function call is made.\n\n");
    printf("Example Input:  -t 10 \nExample output: Too large to dispaly here\nExplanation: \tTest the correctness of calculations of Hauptimplemnetierung in DECIMAL format.\n\n");
    printf("Example Input:  -t 16 -V 2 \nExample output: Too large to dispaly here\nExplanation: \tTest the correctness of calculations of VERSION_2 in HEXADECIMAL format.\n\n");
    printf("Example Input:  -t 10 -V 1 -B 10\nExample output: Too large to dispaly here\nExplanation: \tExecute time measurments for VERSION_1 in DECIMAL format on 10 iteration minimum.\n\n");
  }

  /* In case option for testing was set, tests are going to be executed and program ends afterwards */ 
  if (config.test) { 
    config.numeral_system = (config.test_numeral_system == 10) ? DECIMAL : HEXADECIMAL;
    if (!test(config)) {
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  } 
  else {
    struct bignum sqrt2_value;
    memset(&sqrt2_value, 0, sizeof(sqrt2_value));
    
    /* Benchmarking */
    struct timespec start;
    struct timespec end;
    int i;
    char * version; // for the output of the results
    struct bignum (* current_sqrt) (size_t, numeral_system_t) = sqrt2;

    switch (config.version) {
      case VERSION_0:
        version = "Hauptimplementierung";
        current_sqrt = sqrt2;
        break;
      
      case VERSION_1:
        version = "VERSION_1";
        current_sqrt = sqrt2_V1;
        break;

      case VERSION_2:
        version = "VERSION_2";
        current_sqrt = sqrt2_V2;
        break; 
    }
    struct bignum sqrt2_values[config.iteration];
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < config.iteration; ++i)
     sqrt2_values[i] = current_sqrt(config.digits, config.numeral_system);
    clock_gettime(CLOCK_MONOTONIC, &end);

    sqrt2_value = sqrt2_values[0];
    for (i = 1; i < config.iteration; ++i) {
      free_bignum(&sqrt2_values[i]);
    }

    double time = ((end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec)) / config.iteration);

    char * result = build_string(sqrt2_value, config.digits);

    /* Adjusting output of the result */
    if (config.digits == 0) {
      result[1] = 0;
    } else {
      result[config.digits + 2] = 0;
    }

    printf("%s\n", result);
    free(result);
    free_bignum(&sqrt2_value);

    /* In case option for time measurment was set, above measured time will be shown */ 
    char * numeral_system_string = (config.numeral_system == DECIMAL) ? "decimal" : "hexadecimal";
    if (config.show_time == true) {
      printf("%s calculated %d %s digits in %f seconds\n", version, config.digits, numeral_system_string, time);
    }
    return EXIT_SUCCESS;
  }
}

