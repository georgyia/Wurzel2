# Wurzel2

This project is a C implementation of a program that calculates the square root of 2 with high precision. The program supports different versions of the calculation algorithm, and it can output the result in either decimal or hexadecimal format. The precision of the output (i.e., the number of digits after the decimal point) can be specified by the user. The project also includes a testing framework to verify the correctness of the calculations and to measure the performance of the different versions of the algorithm.

## The project consists of several C source files and headers. Here are the most important of them:

- main.c: The main entry point of the program. It handles command-line arguments and controls the execution of the program.
- bignum.c: Contains the implementation of a "big number" data type that is used for high-precision calculations.
- add_sub.c: Contains functions for adding and subtracting big numbers.
- mul.c: Contains functions for multiplying big numbers. It includes a simple multiplication algorithm and a vectorized multiplication algorithm.
- div.c: Contains functions for dividing big numbers.
- sqrt2.c: Contains different versions of the algorithm for calculating the square root of 2.
- Makefile: A makefile for building the project.
- Ausarbeitung.tex: A LaTeX document that provides a detailed explanation of the project and the implemented algorithms.
- Ausarbeitung.pdf: A PDF version of the LaTeX document.
- Vortrag.pdf: A PDF presentation of the presentation.
  
## How to Run

To compile and run the project, you need a C compiler (like gcc) and a Unix-like environment. Use the following commands:

```
make
./sqrt2
```

You can specify various command-line options to control the behavior of the program. For example, you can use -V to select the version of the algorithm, -d to specify the number of decimal digits in the output, and -h to specify the number of hexadecimal digits. Use -h or --help to display a help message with a description of all available options.

## Framework

The project is structured as a typical C program, with separate source files for different parts of the program and header files for declarations. The code follows the C99 standard. The testing framework is implemented within the program itself: when the -t option is specified, the program runs a series of tests to verify the correctness of the calculations.

## Documentation

The Ausarbeitung.tex file is a LaTeX document that provides a detailed explanation of the project and the mathematical background of the implemented algorithms. It discusses the data structures used in the project, the implementation of basic arithmetic operations, and the methods used for calculating the square root of 2. The document also includes a performance analysis of the different versions of the algorithm. A PDF version of the document (Ausarbeitung.pdf) is also available. The Vortrag.pdf file is a presentation that provides an overview of the project.
