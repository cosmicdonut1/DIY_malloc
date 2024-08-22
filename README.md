# Custom Malloc and Free Implementation

## Description

This project includes an implementation of `malloc` and `free` functions that support multithreading using `pthread_mutex`. A test program is also provided to verify the correctness of these functions in a multithreaded environment.

## Compilation and Execution

1. Save the implementation of `malloc` and `free` in the file `custom_malloc.c`.
2. Save the test program in the file `test.c`.

### Compilation

```sh
gcc -o test custom_malloc.c test.c -lpthread

### Execution
./test

### License
This project is distributed under a free license.
