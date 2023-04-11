
# The Bigint Library

## TABLE OF CONTENTS

1) GENERAL INFO
2) REQUIREMENTS
3) USAGE
4) GETTING LIBRARY VERSION AND EXTRA INFO
5) ERROR HANDLING
6) FUNCTIONS
7) ACKNOWLEDGEMENTS
8) ABOUT AUTHOR

## GENERAL INFO

The Bigint Library is an [open source](https://opensource.org/osd) C library that allows users to make calculations involving signed integers of any size. The only limitation is amount of memory available on the specific platform. The code is made available on terms of [*The Mozilla Public License Version 2.0*](https://www.mozilla.org/en-US/MPL/2.0/) described precisely in the LICENSE file. The library is designed to be user-friendly and portable. It consists of two files thus no installation is needed.

## REQUIREMENTS
The only requirement is C compiler. The library has been tested on several operating systems including Windows, Linux and MacOS. During trials various compilers were used on both 32-bit and 64-bit platforms. However, if you have any problems with the library on your computer, please contact me so I can fix the bug and make the project more portable.

## USAGE

Make sure you have copy of both bigint.c and bigint.h files in your current working directory. Include bigint.h header in your main.c file and then compile both main.c and bigint.c. If you want to, you can follow one of the tutorials available on the Internet and keep compiled library somewhere in your system and just add it to your project during link time.

## GETTING LIBRARY VERSION AND EXTRA INFO

BIGINT_MAJOR, BIGINT_MINOR and BIGINT_PATCHLEVEL are macros that expand to integers representing version of the Bigint Library. BIGINT_MAJOR is incremented each time incompatible interface changes are made. BIGINT_MINOR, likewise, is increased when some new functionality is added and BIGINT_PATCHLEVEL represents number of the latest patch.
If you want to generate some extra information about library use bigint_info() function. Current Bigint Library version is 1.0.0

#### **`main.c`**
```c
#include "bigint.h"
int main(int argc, char *argv[]) {
    bigint_info();
    return 0;
}
```

```console
krzysiek@krzysiek:~/Documents$ ls
bigint.c  bigint.h  main.c
krzysiek@krzysiek:~/Documents$ gcc bigint.c main.c -o main && ./main
This is Bigint Library version 1.0.0 running on Linux
Copyright (c) 2022 Krzysztof Karczewski
Compiled by gcc 9.3.0 on Feb 14 2022 19:00:00
krzysiek@krzysiek:~/Documents$
```

## ERROR HANDLING

If anything goes wrong in any function, it returns -1 or NULL depending on its type. Otherwise 0 or bigint type variable is returned. The library defines the special global enum variable bigint_errno which stores error code of last error that occured. You can check its meaning on the list below or use bigint_strerror() macro which expands to string with problem's description, as follows:

```c
puts(bigint_strerror(bigint_errno));
```

### bigint_errno value with meaning
| value | enum variable                      | description                                                               |
| ----- | ---------------------------------- | ------------------------------------------------------------------------- |
| 0     | ALL_GOOD_IN_THE_HOOD               | everything is all right                                                   |
| 1     | BIGINT_INCORRECT_STRING            | bigint_create() function was given an incorrect string                    |
| 2     | BIGINT_MEMORY_ALLOCATION_ERROR     | failed to allocate memory on the heap                                     |
| 3     | BIGINT_INCORRECT_FUNCTION_ARGUMENT | an incorrect argument was given to a function                             |
| 4     | BIGINT_TOO_LARGE_BIGINT_TO_CONVERT | bigint variable is too large to be converted to integer                   |
| 5     | BIGINT_DIVISION_BY_ZERO            | division by zero                                                          |
| 6     | BIGINT_LENGTH_INDIVISIBLE_BY_FOUR  | cannot convert to bigint integer with number of bytes indivisible by four |
| 7     | BIGINT_ERROR_IN_DATA_STRUCTURE     | unexpected value in bigint data structure                                 |

## FUNCTIONS

### Initializing variables

```c
bigint bigint_create(char *number, size_t length)
```

Use this function to initialize bigint variable. Two arguments are required: pointer referencing a char array which contains a number and an exact length of that array. Due to problems that may occur when using null-terminated strings in c, strlen() function is not used in the code. However, if you don't worry about security issues, you can define a special macro to simplify usage of bigint_create()

#### **`main.c`**
```c
#include <string.h>
#include "bigint.h"
#define my_create(A) bigint_create(A, strlen(A))
int main(int argc, char *argv[]) {
    bigint num = my_create("12345");
    bigint_release(1, num);
    return 0;
}
```

You can choose from three numeral systems: decimal, hexadecimal and binary. In the second case add "0x" characters at the beginning of the string. For example 

```c
bigint_create("0x499602d2", 10)
```
is a proper call of the function.

In the last case use "0b" characters just like in the next example:

```c
bigint_create("0b1001101", 10)
```

Minus at the beginning changes the sign of the number.

### Releasing memory

```c
int bigint_release(int count, ...)
```

Use this variadic function to free memory allocated on heap. After using three bigint variables you should add the following line at the end of your program:
```c
bigint_release(3, var1, var2, var3)
```

### Getting size

```c
size_t bigint_size(bigint number)
```

Use this function to check how many bytes are allocated for a certain bigint variable.

### Printing variables

```c
int bigint_print(FILE *stream, bigint_base base, bigint number)
```

Use this function to print *number* to the *stream*. Use enum type *base* to set the base of the numeral system. You can choose from BIN, DEC and HEX.

#### **`main.c`**
```c
#include "bigint.h"
int main(int argc, char *argv[]) {
    bigint var1 = bigint_create("123456789", 9);
    bigint var2 = bigint_create("0b1100110011", 12);
    bigint var3 = bigint_create("0xfffaaa555222", 14);
    bigint_print(stdout, DEC, var1); puts("");
    bigint_print(stdout, BIN, var2); puts("");
    bigint_print(stdout, HEX, var3); puts("");
    bigint_release(3, var1, var2, var3);
    return 0;
}
```

```console
krzysiek@krzysiek:~/Documents$ ls
bigint.c  bigint.h  main.c
krzysiek@krzysiek:~/Documents$ gcc bigint.c main.c -o main && ./main
123456789
0b1100110011
0xfffaaa555222
krzysiek@krzysiek:~/Documents$
```

If the only stream you are going to use is stdout and you want all variables to be printed in decimal system you can define another macro to shorten code.

```c
#define my_print(A) bigint_print(stdout, DEC, A)
```

Thereby the following lines are equivalent:

```c
my_print(var1)
bigint_print(stdout, DEC, var1)
```

### Addition

```c
int bigint_add(int count, bigint sum, ...)
```

This function adds multiple summands and saves the result in *sum*. As it is variadic, remember to pass *count* of summands to the function. Note that sum must be already created before function call.
Example usage:

```c
bigint_add(4, sum, var1, var2, var3, var4)
```

In mathematical notation it is equivalent to: 

sum = var1 + var2 + var3 + var4

### Subtraction

```c
int bigint_subtract(bigint difference, bigint minuend, bigint subtrahend)
```

This function subtracts two numbers and saves the result in *difference*. It is equivalent to:

difference = minuend - subtrahend

### Incrementation and decrementation

There are also fast functions for incrementation and decrementation.

```c
int bigint_increment(number)
```

which is equivalent to number += 1

```c
int bigint_decrement(number)
```

which is equivalent to number -= 1

### Multiplication

```c
int bigint_multiply(int count, bigint product, ...)
```

This functions multiplies numerous factors and saves the result in *product*. As it is variadic, remember to pass *count* of factors to the function. Note that product must be already created before function call.
Example usage:

```c
bigint_multiply(2, product, var1, var2)
```

### Division

```c
int bigint_divide(bigint dividend, bigint divisor, bigint quotient, bigint remainder)
```

This function divides *dividend* by *divisor* and saves result to *quotient*. If there is any remainder, it is saved to *remainder* variable. If you want to perform integer division and you do not care about *remainder*, pass NULL instead of it. If all you need is remainder pass NULL instead of *quotient*.

Getting both quotient and remainder:

```c
bigint_divide(var1, var2, var3, var4)
```

Getting only quotient:

```c
bigint_divide(var1, var2, var3, NULL)
```

Getting only remainder:

```c
bigint_divide(var1, var2, NULL, var4)
```

Note that variables you pass to function must be already initialized.

### Comparison

```c
int bigint_compare(bigint number1, bigint number2)
```

Use this function to compare two numbers. It returns:
- 1 if number1 > number2
- -1 if number2 > number1
- 0 if number1 == number2

### Managing sign

```c
int bigint bigint_change_sign(bigint number)
```

This function changes sign of the *number*. In is equivalent to multiplying the number by -1

```c
int bigint_absolute_value(bigint number)
```

Use this function to change *number* sign to positive.

```c
int bigint_get_sign(bigint number)
```

This function returns sign of the *number*: 1 in case it is negative, 0 in case it is positive or equals 0.

### Copying

```c
bigint bigint_copy(bigint number)
```

As bigint is a pointer type, the special function is required for copying. If you want to create a copy of *b* variable and store it in *a* variable, write:

```c
bigint a = bigint_copy(b)
```

### Bitwise negation

```c
int bigint_not(bigint number)
```

Use this function to perform logical negation on each bit of the *number*. Note that number is stored in 32-bit segments. For example:

#### **`main.c`**
```c
#include <string.h>
#include "bigint.h"
#define my_create(A) bigint_create(A, strlen(A))
#define my_print(A) bigint_print(stdout, HEX, A);puts("")
int main(int argc, char *argv[]) {
	bigint var1 = my_create("0xffff");
	bigint var2 = my_create("0xffffffffffff");
	bigint_not(var1);
	bigint_not(var2);
	my_print(var1);
	my_print(var2);
	bigint_release(2, var1, var2);
	return 0;
}
```

```console
krzysiek@krzysiek:~/Documents$ ls
bigint.c  bigint.h  main.c
krzysiek@krzysiek:~/Documents$ gcc bigint.c main.c -o main && ./main
0xffff0000
0xffff000000000000
krzysiek@krzysiek:~/Documents$
```

### Bitwise shift

```c
int bigint_shift_left(bigint number, size_t count)
```

Left arithmetic shift is performed on *number*. It is equivalent to multiplying by 2 raised to the power of *count*.

```c
int bigint_shift_right(bigint number, size_t count)
```

Right arithmetic shift is performed on *number*. It is equivalent to taking the floor of division by 2 raised to the power of *count*.

### Conversion to bigint

```c
bigint bigint_convert_to_bigint(void *integer, size_t length)
```

This function converts unsigned integer of size *length* pointed to by *integer*. Type conversion of pointer passed to function is a good habit. Note that number of bytes of *integer* must be a multiple of 4.

#### **`main.c`**
```c
#include "bigint.h"
int main(int argc, char *argv[]) {
    long unsigned int var_int = 123456789;
    bigint var_bint = bigint_convert_to_bigint((void*)&var_int, sizeof(var_int));
    bigint_print(stdout, DEC, var_bint);
    bigint_release(1, var_bint);
    return 0;
}
```

```console
krzysiek@krzysiek:~/Documents$ ls
bigint.c  bigint.h  main.c
krzysiek@krzysiek:~/Documents$ gcc bigint.c main.c -o main && ./main
123456789
krzysiek@krzysiek:~/Documents$
```

### Conversion to integer

```c
int bigint_convert_to_int(bigint number, uintmax_t *integer)
```

This function converts a bigint number to integer pointed to by *integer*. It omitts sign and if number is too big to be converted to integer, it returns -1 and information about overflow is saved in bigint_errno. The uintmax_t type is the largest unsigned integer that system can handle without this library. It can be found in stdint.h header. If you want to print it, get interested in inttypes.h header.

## ACKNOWLEDGEMENTS

The author thanks Aleksander Bąba, Augustyn Majtyka, Andrzej Mazur, Jerzy Karczewski and Kamila Prabucka for help in this project.

## ABOUT AUTHOR

My name is Krzysztof Karczewski. I am not a professional programmer and I have created this project in my free time. If you want to contact me in the matter of the library or any other, send me an email please. You can find the address on the [homepage](https://kakrzysiek.github.io/bigint/docs/index.html) of the Bigint Library.
