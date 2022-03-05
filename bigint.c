/*
 * Copyright (c) 2022 Krzysztof Karczewski
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "bigint.h"

#if defined(__has_include)
#if __has_include(<inttypes.h>)
#include <inttypes.h>
#define PRINTING_MACROS
#define PRINTING_FORMAT_SPECIFIER PRIu32
#define PRINTING_TYPE uint32_t
#endif
#endif

#ifndef PRINTING_MACROS
#define PRINTING_MACROS
#define PRINTING_FORMAT_SPECIFIER "lu"
#define PRINTING_TYPE unsigned long
#endif

#define SUCCESS 0
#define FAILURE -1

#define NEGATIVE 1
#define POSITIVE 0

#define TRUE 1
#define FALSE 0

#define ODD 1
#define EVEN 0

#define BIGINT_LITTLE_ENDIAN -1
#define BIGINT_BIG_ENDIAN 1

// Check perating system
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define BIGINT_OS "Windows"
#elif defined(__APPLE__)
#define BIGINT_OS "Mac OS"
#elif defined(__linux__)
#define BIGINT_OS "Linux"
#elif defined(__unix__)
#define BIGINT_OS "Unix"
#else
#define BIGINT_OS "unknown OS"
#endif

// Check compiler
#if defined(__clang__)
#define BIGINT_COMPILER "clang"
#define BIGINT_COMPILER_VERSION " %d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__
#elif defined(__GNUC__)
#define BIGINT_COMPILER "gcc"
#define BIGINT_COMPILER_VERSION " %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
#elif defined(_MSC_VER)
#define BIGINT_COMPILER "MSVC"
#define BIGINT_COMPILER_VERSION " %d.%d", _MSC_VER / 100, _MSC_VER % 100
#else
#define BIGINT_COMPILER "unknown c compiler"
#define BIGINT_COMPILER_VERSION ""
#endif

// Check compile time
#ifdef __DATE__
#define BIGINT_DATE __DATE__
#ifdef __TIME__
#define BIGINT_TIME __TIME__
#else
#define BIGINT_TIME ""
#endif
#else
#define BIGINT_DATE "unknown date"
#define BIGINT_TIME ""
#endif

// Macro that checks performance of malloc function; use only in functions that return pointer
#define check_memory_ptr(A)                            \
	if ((A) == NULL)                                   \
	{                                                  \
		bigint_errno = BIGINT_MEMORY_ALLOCATION_ERROR; \
		return NULL;                                   \
	}

// Macro that checks performance of malloc function; use only in functions that return integer
#define check_memory_int(A)                            \
	if ((A) == NULL)                                   \
	{                                                  \
		bigint_errno = BIGINT_MEMORY_ALLOCATION_ERROR; \
		return FAILURE;                                \
	}

// Macro that sets all node structure members to 0
#define initialise_node(A)        \
	(A)->next = (A)->prev = NULL; \
	(A)->value = 0

// Macro that sets all bigint structure members to 0
#define initialise_bigint(A)       \
	(A)->first = (A)->last = NULL; \
	(A)->length = 0;               \
	(A)->sign = 0

static int bigint_release_basic(bigint number);
static int bigint_release_segments(bigint number);
static int add_segments_beginning(bigint number, size_t count);
static uint32_t *copy_to_chain(bigint number);
static size_t bit_len(uint32_t number);
static int check_endian();
static int print_bits(uint32_t number);
static uint8_t get_n_bit(uint32_t *chain, size_t length, size_t number);
static size_t chain_length(uint32_t *chain, size_t max_count);
static int set_n_bit_to_1(uint32_t *chain, size_t length, size_t number);
static int bigint_add_basic(bigint sum, bigint summand1, bigint summand2);
static int bigint_increment_basic(bigint number);
static int bigint_subtract_basic(bigint difference, bigint minuend, bigint subtrahend);
static int bigint_decrement_basic(bigint number);
static int bigint_multiply_basic(bigint product, bigint element1, bigint element2);
static int bigint_compare_absolute(bigint number1, bigint number2);
static bigint bigint_create_empty_segments(size_t count);
static int add_segments(bigint number, size_t count);
static int reset(bigint number);
static uint8_t check_sign(char **number, size_t *length);
static bigint_base check_base(char **number, size_t *length);
static int check_syntax(char **number, size_t length, bigint_base base);
static uint32_t save_binary_segment(char *number);
static int save_binary(bigint return_number, char *number, size_t length);
static char *convert_to_binary(char *number, size_t length);
static int get_parity(char *number, size_t length);
static void divide_one_digit(char *digit);
static void divide_digits(char *number, size_t length);
static int is_zero(char *number, size_t length);
static int save_decimal(bigint return_number, char *number, size_t length);
static int print_bits_first_element(uint32_t num);
static uint64_t divide(uint32_t *divident, uint32_t *quotient, size_t count);
static int print_decimal(bigint number);
static int bigint_shift_left_basic(bigint number);
static int bigint_shift_right_basic(bigint number);
static int bigint_add_sign(bigint sum, bigint summand1, bigint summand2);
static int leave_one_segment(bigint number);
static uint32_t *chain_alignment(bigint number, size_t length);
static int greater_or_eq(uint32_t *array1, uint32_t *array2, size_t length);

bigint_error_code bigint_errno = ALL_GOOD_IN_THE_HOOD;

char *error_messages[] = {
	"everything is all right",
	"bigint_create() function was given an incorrect string",
	"failed to allocate memory on the heap",
	"an incorrect argument was given to a function",
	"bigint variable is too large to be converted to integer",
	"division by zero",
	"cannot convert to bigint integer with number of bytes indivisible by four",
	"unexpected value in bigint data structure"};

int bigint_info()
{
	printf("This is Bigint Library version %d.%d.%d ", BIGINT_MAJOR, BIGINT_MINOR, BIGINT_PATCHLEVEL);
	printf("running on %s\n", BIGINT_OS);
	printf("Copyright (c) 2022 Krzysztof Karczewski\n");
	printf("Compiled by " BIGINT_COMPILER);
	printf(BIGINT_COMPILER_VERSION);
	printf(" on %s %s\n", BIGINT_DATE, BIGINT_TIME);
	return SUCCESS;
}

int check_endian()
{
	uint32_t i = 1;
	uint8_t *c = (uint8_t *)&i;
	if (*c)
	{
		return BIGINT_LITTLE_ENDIAN;
	}
	else
	{
		return BIGINT_BIG_ENDIAN;
	}
}

uint8_t check_sign(char **number, size_t *length)
{
	// Wrong argument
	if (number == NULL || *number == NULL || length == NULL)
	{
		return 0;
	}

	// Get the sign
	switch ((*number)[0])
	{
	case '-':
		(*number)++;
		(*length)--;
		return NEGATIVE;
	case '+':
		(*number)++;
		(*length)--;
		return POSITIVE;
	default:
		return POSITIVE;
	}
}

bigint_base check_base(char **number, size_t *length)
{
	// Wrong arguments
	if (number == NULL || *number == NULL || length == NULL || *length <= 0)
	{
		return OTHER;
	}

	// Get the base and increase the pointer if necessary
	if ((*number)[0] == '0' && *length != 1)
	{
		switch ((*number)[1])
		{
		case 'x':
			*number += 2;
			*length -= 2;
			return HEX;
		case 'b':
			*number += 2;
			*length -= 2;
			return BIN;
		default:
			return OTHER;
		}
	}
	else
	{
		return DEC;
	}
}

int check_syntax(char **number, size_t length, bigint_base base)
{
	// Wrong arguments
	if (number == NULL || *number == NULL || length <= 0)
	{
		return FAILURE;
	}

	// Check number's first digit and base
	if (base == OTHER || ((*number)[0] == '0' && length != 1))
	{
		return FAILURE;
	}

	// Choose proper charset according to the base
	char *charsets[] = {"01", "0123456789", "0123456789ABCDEF"};
	char *charset = charsets[base];

	// Check each digit in the number
	size_t i = 0;
	for (i = 0; i < length; i++)
	{
		if (strchr(charset, toupper((*number)[i])) == NULL)
		{
			return FAILURE;
		}
	}

	return SUCCESS;
}

uint32_t save_binary_segment(char *number)
{
	// Wrong argument passed to function
	if (number == NULL)
	{
		return 0;
	}

	// Save last 32 digits of the number
	uint32_t return_number = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
	{
		return_number |= (number[31 - i] - 48) << i;
	}
	return return_number;
}

int save_binary(bigint return_number, char *number, size_t length)
{
	// Wrong arguments passed to function
	if (return_number == NULL || return_number->first == NULL || number == NULL || length == 0)
	{
		return FAILURE;
	}

	// Determine number of empty bits at the beginning of the number
	int empty = (length % 32 == 0 ? 0 : 32 - length % 32);

	// Create new strign and copy the old one
	char *new_number = (char *)malloc(sizeof(char) * (length + empty));
	check_memory_int(new_number);
	memset(new_number, 0, sizeof(char) * (length + empty));
	int j = 0;
	for (j = 0; j < empty; j++)
	{
		new_number[j] = '0';
	}
	strncpy(new_number + empty, number, length);

	// Determine number of nodes
	return_number->length = (size_t)(length % 32 == 0 ? length / 32 : length / 32 + 1);

	// Fill nodes with values
	bigint_node *current = return_number->first, *previous = NULL;
	current->prev = NULL;
	size_t i = 0;
	for (i = 0; i < return_number->length; i++)
	{
		current->value = save_binary_segment(new_number + length + empty - 32 * (i + 1));
		current->next = (bigint_node *)malloc(sizeof(bigint_node));
		check_memory_int(current->next);
		initialise_node(current->next);
		if (i != 0)
		{
			current->prev = previous;
		}
		previous = current;
		current = current->next;
	}
	previous->next = NULL;
	return_number->last = previous;

	// Free allocated memory
	free(new_number);
	free(current);

	return SUCCESS;
}

char *convert_to_binary(char *number, size_t length)
{
	// Wrong arguments passed to function
	if (number == NULL || length == 0)
	{
		return NULL;
	}

	// Each string matches one hexadecimal digit
	char *binary_numbers[] = {"0000", "0001", "0010", "0011",
							  "0100", "0101", "0110", "0111",
							  "1000", "1001", "1010", "1011",
							  "1100", "1101", "1110", "1111"};
	char *hex_digits = "0123456789ABCDEF##########abcdef";

	// Allocate memory for string the function is going to retrun
	char *return_number = (char *)malloc(length * 4 * sizeof(char) + 1);
	check_memory_ptr(return_number);
	memset(return_number, 0, length * 4 * sizeof(char) + 1);

	// Convert each hexadecimal digit into four decimal digits
	size_t i = 0;
	for (i = 0; i < length; i++)
	{
		sprintf(return_number + 4 * i, "%s", binary_numbers[(strchr(hex_digits, number[i]) - hex_digits) % 16]);
	}

	return return_number;
}

int get_parity(char *number, size_t length)
{
	if (number[length - 1] % 2 == 1)
	{
		number[length - 1] -= 1;
		return ODD;
	}
	else
	{
		return EVEN;
	}
}

void divide_one_digit(char *digit)
{
	if (digit[0] % 2 == 1)
	{
		digit[0] -= 1;
		digit[1] += 5;
	}
	digit[0] = (digit[0] - 48) / 2 + 48;
}

void divide_digits(char *number, size_t length)
{
	size_t i = length - 1;
	do
	{
		divide_one_digit(number + i);
		i--;
	} while (i != 0);
	divide_one_digit(number + i);
}

int is_zero(char *number, size_t length)
{
	size_t i = 0;
	for (i = 0; i < length; i++)
	{
		if (number[i] != '0')
		{
			return FALSE;
		}
	}
	return TRUE;
}

int save_decimal(bigint return_number, char *number, size_t length)
{
	// Wrong arguments passed to function
	if (return_number == NULL || number == NULL || length == 0)
	{
		bigint_errno = BIGINT_INCORRECT_STRING;
		return FAILURE;
	}

	// Create copy of 'number' string
	char *duplicate = malloc(length * sizeof(char));
	check_memory_int(duplicate);
	strncpy(duplicate, number, length);

	// Variables used in the loop
	size_t count = 0, nodes = 1;
	uint32_t segment = 0;
	bigint_node *current = return_number->first, *previous = NULL;
	check_memory_int(current);
	current->prev = NULL;

	// One digit number
	if (length == 1)
	{
		return_number->first->value = (uint32_t)(number[0] - 48);
		return_number->last = return_number->first;
		return_number->length = 1;
		free(duplicate);
		return SUCCESS;
	}

	// Divide by two and get the remainder until zero
	while (!is_zero(duplicate, length))
	{

		// Fill in next digit in one node
		segment |= (uint32_t)get_parity(duplicate, length) << count;
		divide_digits(duplicate, length);
		count++;

		// Go to another node
		if (count == 32)
		{
			nodes++;
			current->value = segment;
			count = 0;
			segment = 0;
			current->next = (bigint_node *)malloc(sizeof(bigint_node));
			check_memory_int(current->next);
			initialise_node(current->next);
			if (nodes != 2)
			{
				current->prev = previous;
			}
			previous = current;
			current = current->next;
		}
	}

	current->value = segment;
	current->prev = previous;
	return_number->length = nodes;
	return_number->last = current;
	free(duplicate);
	return SUCCESS;
}

bigint bigint_create(char *number, size_t length)
{

	// Wrong arguments passed to function
	if (number == NULL || length == 0)
	{
		bigint_errno = BIGINT_INCORRECT_STRING;
		return NULL;
	}

	// Check all the parameters
	uint8_t sign = check_sign(&number, &length);
	bigint_base base = check_base(&number, &length);
	int correct = check_syntax(&number, length, base);

	// Tests' failure
	if (correct == FAILURE)
	{
		bigint_errno = BIGINT_INCORRECT_STRING;
		return NULL;
	}

	// Allocate memory for bigint
	bigint return_number = (bigint)malloc(sizeof(struct bigint_data_structure));
	check_memory_ptr(return_number);
	initialise_bigint(return_number);

	return_number->sign = sign;
	return_number->first = (bigint_node *)malloc(sizeof(bigint_node));
	check_memory_ptr(return_number->first);
	initialise_node(return_number->first);

	// Create bigint
	if (base == BIN)
	{
		correct = save_binary(return_number, number, length);
	}
	else if (base == HEX)
	{
		char *binary_number = convert_to_binary(number, length);
		correct = save_binary(return_number, binary_number, length * 4);
		free(binary_number);
	}
	else
	{
		correct = save_decimal(return_number, number, length);
	}

	// Check save functions performance
	if (correct == FAILURE)
	{
		return NULL;
	}

	// Zero cannot be negative
	if (return_number->length == 1 && return_number->first->value == 0)
	{
		return_number->sign = 0;
	}

	return return_number;
}

bigint bigint_convert_to_bigint(void *integer, size_t length)
{
	// Wrong arguments
	if (integer == NULL || length == 0)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return NULL;
	}

	// length should be the multiple of 4
	if (length % 4 != 0)
	{
		bigint_errno = BIGINT_LENGTH_INDIVISIBLE_BY_FOUR;
		return NULL;
	}

	// Check endianness
	int endianness = check_endian();
	uint32_t *integer32 = (uint32_t *)integer;

	size_t segments = length / 4;
	size_t i = 0;
	bigint return_number = bigint_create_empty_segments(segments);

	// Fill nodes with values
	bigint_node *current = return_number->first;
	for (i = 0; i < segments; i++)
	{
		if (endianness == -1)
		{
			// Little endian
			current->value = (integer32[i]);
		}
		else
		{
			// Big endian
			current->value = (integer32[segments - i - 1]);
		}
		current = current->next;
	}

	// Delete leading nodes with 0 value
	bigint_node *temp = NULL;
	current = return_number->last;
	// i = 0 in case number equals zero
	for (i = 1; i < segments; i++)
	{
		if (current->value == 0)
		{
			return_number->length -= 1;
			return_number->last = current->prev;
			current->prev->next = NULL;
			temp = current;
			free(current);
			current = temp->prev;
		}
		else
		{
			break;
		}
	}
	return return_number;
}

int bigint_convert_to_int(bigint number, uintmax_t *integer)
{

	// Wrong arguments passed to function
	if (number == NULL || integer == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	// Clear target integer
	*integer = 0;

	// Check if bigint is small enough
	uintmax_t max_value_i = UINTMAX_MAX;
	bigint max_value_b = bigint_convert_to_bigint(&max_value_i, sizeof(uintmax_t));
	if (bigint_compare_absolute(max_value_b, number) == -1)
	{
		bigint_errno = BIGINT_TOO_LARGE_BIGINT_TO_CONVERT;
		bigint_release_basic(max_value_b);
		return FAILURE;
	}
	bigint_release_basic(max_value_b);

	// Conversion
	int endianness = check_endian();
	uint32_t *integer32 = (uint32_t *)integer;
	size_t segments = number->length;
	size_t i = 0;
	bigint_node *current = number->first;
	for (i = 0; i < segments; i++)
	{
		if (endianness == -1)
		{
			// Little endian
			integer32[i] = current->value;
		}
		else
		{
			// Big endian
			integer32[segments - i - 1] = current->value;
		}
		current = current->next;
	}

	return SUCCESS;
}

bigint bigint_create_empty_segments(size_t count)
{
	// Wrong argument
	if (count == 0)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return NULL;
	}

	bigint return_number = (bigint)malloc(sizeof(struct bigint_data_structure));
	check_memory_ptr(return_number);
	initialise_bigint(return_number);

	return_number->first = (bigint_node *)malloc(sizeof(bigint_node));
	check_memory_ptr(return_number->first);
	initialise_node(return_number->first);
	return_number->length = count;
	bigint_node *current = return_number->first;
	bigint_node *previous = NULL;
	size_t i = 0;

	for (i = 1; i < count; i++)
	{
		current->next = (bigint_node *)malloc(sizeof(bigint_node));
		check_memory_ptr(current->next);
		initialise_node(current->next);
		if (i != 1)
		{
			current->prev = previous;
		}
		previous = current;
		current = current->next;
	}
	return_number->last = current;
	current->prev = previous;
	return_number->last->next = NULL;
	return return_number;
}

int print_bits(uint32_t num)
{
	int i = 0;
	for (i = 31; i >= 0; i--)
	{
		if ((num >> i) & 1)
		{
			printf("1");
		}
		else
		{
			printf("0");
		}
	}
	return SUCCESS;
}

// Omit leading zeros
int print_bits_first_element(uint32_t num)
{
	int i = 0;
	for (i = (int)bit_len(num) - 1; i >= 0; i--)
	{
		if ((num >> i) & 1)
		{
			printf("1");
		}
		else
		{
			printf("0");
		}
	}
	return SUCCESS;
}

// Save values of nodes in linked list to array
uint32_t *copy_to_chain(bigint number)
{

	uint32_t *return_chain = (uint32_t *)malloc(number->length * sizeof(uint32_t));
	check_memory_ptr(return_chain);
	memset(return_chain, 0, number->length * sizeof(uint32_t));

	size_t i = 0;
	bigint_node *current = number->last;
	for (i = 0; i < number->length; i++)
	{
		return_chain[i] = current->value;
		current = current->prev;
	}

	return return_chain;
}

// Number of digits in binary system
size_t bit_len(uint32_t number)
{

	if (number == 0)
	{
		return 0;
	}

	size_t i = 0;
	for (i = 31; i != 0; i--)
	{
		if ((number >> i) & 1)
		{
			return i + 1;
		}
	}

	return 1;
}

// Gets bit no 'number' from 'chain' array
uint8_t get_n_bit(uint32_t *chain, size_t length, size_t number)
{

	if (chain == NULL || (number / 32) + 1 > length)
	{
		return 0;
	}

	uint32_t current = chain[length - 1 - (number / 32)];
	if (current & (1 << (number % 32)))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

// Sets bit no 'number' in 'chain' array to one
int set_n_bit_to_1(uint32_t *chain, size_t length, size_t number)
{

	if (chain == NULL || (number / 32) + 1 > length)
	{
		return FAILURE;
	}

	uint32_t *current = chain + (length - 1 - (number / 32));
	*(current) |= (1 << (number % 32));

	return SUCCESS;
}

// Divide two numbers
uint64_t divide(uint32_t *divident, uint32_t *quotient, size_t count)
{

	if (divident == NULL || quotient == NULL || count == 0)
	{
		return 0;
	}

	uint32_t d = 1000000000;
	uint64_t r = 0;
	size_t length = count * 32 - 32 + bit_len(divident[0]);

	int i = 0;
	for (i = length - 1; i >= 0; i--)
	{
		r <<= 1;
		r |= get_n_bit(divident, count, i);
		if (r >= d)
		{
			r -= d;
			set_n_bit_to_1(quotient, count, i);
		}
	}

	return r;
}

// Find non-zero array element
size_t chain_length(uint32_t *chain, size_t max_count)
{

	if (chain == NULL || max_count == 0)
	{
		return 0;
	}

	size_t i = 0;
	for (i = 0; i < max_count; i++)
	{
		if (chain[i] != 0)
		{
			return max_count - i;
		}
	}

	return 0;
}

int print_decimal(bigint number)
{

	if (number == NULL || number->first == NULL)
	{
		return FAILURE;
	}

	if (number->length == 1)
	{
		// print_bits(current->value);
		printf("%" PRINTING_FORMAT_SPECIFIER, (PRINTING_TYPE)number->first->value);
		return SUCCESS;
	}

	uint32_t *chain = copy_to_chain(number);
	if (chain == NULL)
	{
		return FAILURE;
	}

	size_t count = number->length;

	// Amount of memory needed in decimal system
	size_t decimal_digits = count * 10 + 1;
	size_t decimal_segments = (decimal_digits % 9 == 0 ? decimal_digits / 9 : decimal_digits / 9 + 1);
	uint32_t *result = (uint32_t *)malloc(decimal_segments * sizeof(uint32_t));
	check_memory_int(result);
	memset(result, 0, decimal_segments * sizeof(uint32_t));

	uint32_t *q1 = (uint32_t *)malloc(sizeof(uint32_t) * count);
	uint32_t *q2 = (uint32_t *)malloc(sizeof(uint32_t) * count);
	memset(q1, 0, count * sizeof(uint32_t));
	memset(q2, 0, count * sizeof(uint32_t));
	memcpy(q1, chain, count * sizeof(uint32_t));

	uint64_t r = 0;
	uint32_t *arr[] = {q1, q2};

	size_t i = 0;
	for (i = 0; i < decimal_segments; i++)
	{
		memset(arr[(i + 1) % 2], 0, count * (sizeof(uint32_t)));
		size_t shift = count - chain_length(arr[i % 2], count);
		r = divide(arr[i % 2] + shift, arr[(i + 1) % 2] + shift, chain_length(arr[i % 2], count));
		result[decimal_segments - i - 1] = r;
	}

	size_t j = 0;
	size_t k = 0;
	for (k = 0; k < decimal_segments; k++)
	{
		if (result[k] == 0 && decimal_segments != 1)
		{
			j++;
		}
		else
		{
			break;
		}
	}
	k = 0;
	for (; j < decimal_segments; j++)
	{
		if (k == 0)
		{
			printf("%" PRINTING_FORMAT_SPECIFIER, (PRINTING_TYPE)result[j]);
			k++;
			continue;
		}
		printf("%09" PRINTING_FORMAT_SPECIFIER, (PRINTING_TYPE)result[j]);
		k++;
	}

	free(result);
	free(chain);
	free(q1);
	free(q2);

	return SUCCESS;
}

int bigint_print(FILE *stream, bigint_base base, bigint number)
{

	// Wrong arguments passed to function
	if (stream == NULL || (base != BIN && base != DEC && base != HEX) || number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	if (number->sign == NEGATIVE)
	{
		printf("-");
	}

	if (base == BIN)
	{
		printf("0b");
	}
	else if (base == HEX)
	{
		printf("0x");
	}

	if (base == DEC)
	{
		return (print_decimal(number));
	}

	bigint_node *current = number->last;

	// Don't print zeros at the beginning
	if (base == BIN)
	{
		print_bits_first_element(current->value);
	}
	else
	{
		printf("%x", current->value);
	}
	current = current->prev;

	size_t i = number->length;
	for (; i > 1; i--)
	{
		if (base == BIN)
		{
			print_bits(current->value);
		}
		else
		{
			printf("%08x", current->value);
		}
		current = current->prev;
	}

	return SUCCESS;
}

int bigint_release_basic(bigint number)
{

	// Wrong arguments passed to function
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	bigint_node *current = number->first;
	bigint_node *previous = NULL;
	size_t i = 0;
	for (i = 0; i < number->length; i++)
	{
		previous = current;
		current = current->next;
		free(previous);
	}

	free(number);

	// current should be null
	if (current != NULL)
	{
		bigint_errno = BIGINT_ERROR_IN_DATA_STRUCTURE;
		return FAILURE;
	}

	return SUCCESS;
}

int bigint_release_segments(bigint number)
{

	// Wrong arguments passed to function
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	bigint_node *current = number->first;
	bigint_node *previous = NULL;
	size_t i = 0;
	for (i = 0; i < number->length; i++)
	{
		previous = current;
		current = current->next;
		free(previous);
	}

	// current should be null
	if (current != NULL)
	{
		bigint_errno = BIGINT_ERROR_IN_DATA_STRUCTURE;
		return FAILURE;
	}

	return SUCCESS;
}

int bigint_release(int count, ...)
{
	// Wrong argument
	if (count < 0)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	else if (count == 0)
	{
		return SUCCESS;
	}

	va_list ap;
	va_start(ap, count);
	int status = SUCCESS;
	int i = 0;
	for (i = 0; i < count; i++)
	{
		status = bigint_release_basic(va_arg(ap, bigint));
		if (status == FAILURE)
		{
			return status;
		}
	}
	va_end(ap);

	return SUCCESS;
}

size_t bigint_size(bigint number)
{
	// Wrong arguments passed to function
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	return sizeof(struct bigint_data_structure) + (number->length * sizeof(bigint_node));
}

int bigint_get_sign(bigint number)
{

	// Wrong argument passed to function
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	return number->sign;
}

int bigint_absolute_value(bigint number)
{

	// Wrong argument passed to funcion
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	number->sign = 0;

	return SUCCESS;
}

int bigint_change_sign(bigint number)
{

	// Wrong argument passed to function
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	number->sign = (number->sign == 1 ? 0 : 1);

	return SUCCESS;
}

int add_segments_beginning(bigint number, size_t count)
{
	// Wrong arguments
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	if (count == 0)
	{
		return SUCCESS;
	}
	number->length += count;

	bigint_node *current = (bigint_node *)malloc(sizeof(bigint_node));
	bigint_node *temp = current;
	check_memory_int(current);
	initialise_node(current);

	size_t i = 0;
	for (i = 0; i < count - 1; i++)
	{
		current->next = (bigint_node *)malloc(sizeof(bigint_node));
		check_memory_int(current->next);
		initialise_node(current->next);
		current->next->prev = current;
		current = current->next;
	}
	number->first->prev = current;
	current->next = number->first;
	number->first = temp;
	return SUCCESS;
}

int bigint_shift_left_basic(bigint number)
{

	// Wrong argument passed to function
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	// In case new segment needs to be added
	if (number->last->value & 0x80000000)
	{
		number->last->next = (bigint_node *)malloc(sizeof(bigint_node));
		check_memory_int(number->last->next);
		initialise_node(number->last->next);
		number->last->next->prev = number->last;
		number->last = number->last->next;
		number->length += 1;
		number->last->value = 0;
	}

	uint32_t remaining1 = 0;
	uint32_t remaining2 = 0;
	size_t i = 0;
	bigint_node *current = number->first;
	for (i = 0; i < number->length; i++)
	{
		remaining1 = (current->value & 0x80000000) >> 31;
		current->value = (current->value << 1) | remaining2;
		remaining2 = remaining1;
		current = current->next;
	}

	return SUCCESS;
}

int bigint_shift_left(bigint number, size_t count)
{
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	if (count == 0)
	{
		return SUCCESS;
	}
	size_t segments = count / 32;
	size_t bits = count % 32;
	size_t i = 0;
	int result = SUCCESS;
	if (add_segments_beginning(number, segments) == FAILURE)
	{
		return FAILURE;
	}
	for (i = 0; i < bits; i++)
	{
		result = bigint_shift_left_basic(number);
		if (result == FAILURE)
		{
			return FAILURE;
		}
	}
	return SUCCESS;
}

int bigint_shift_right_basic(bigint number)
{
	// Wrong arguments
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	uint32_t remaining1 = 0;
	uint32_t remaining2 = 0;
	size_t i = 0;
	bigint_node *current = number->last;
	for (i = 0; i < number->length; i++)
	{
		remaining1 = (current->value & 1) << 31;
		current->value = (current->value >> 1) | remaining2;
		remaining2 = remaining1;
		current = current->prev;
	}

	// Delete last segment
	bigint_node *temp_ptr = number->last;
	if (temp_ptr->value == 0 && number->length != 1)
	{
		number->last->prev->next = NULL;
		number->last = number->last->prev;
		free(temp_ptr);
		number->length -= 1;
	}

	return SUCCESS;
}

int bigint_not(bigint number)
{
	// Wrong arguments
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	bigint_node *current = number->first;
	size_t i = 0;
	for (i = 0; i < number->length; i++)
	{
		current->value = ~(current->value);
		current = current->next;
	}
	return SUCCESS;
}

int bigint_shift_right(bigint number, size_t count)
{
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	if (count == 0)
	{
		return SUCCESS;
	}
	size_t i = 0;
	int result = SUCCESS;
	for (i = 0; i < count; i++)
	{
		result = bigint_shift_right_basic(number);
		if (result == FAILURE)
		{
			return FAILURE;
		}
	}
	return SUCCESS;
}

int bigint_compare_absolute(bigint number1, bigint number2)
{

	// Wrong arguments passed to function
	if (number1 == NULL || number2 == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	if (number1->length != number2->length)
	{
		return (number1->length > number2->length ? 1 : -1);
	}

	size_t i = 0;
	bigint_node *current1 = number1->last;
	bigint_node *current2 = number2->last;
	for (i = 0; i < number1->length; i++)
	{
		if (current1->value != current2->value)
		{
			return (current1->value > current2->value ? 1 : -1);
		}
		current1 = current1->prev;
		current2 = current2->prev;
	}
	return 0;
}

int bigint_compare(bigint number1, bigint number2)
{

	// Wrong arguments passed to function
	if (number1 == NULL || number2 == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return 0;
	}

	switch (2 * number1->sign + number2->sign)
	{
	case 0:
		// Both numbers positive
		return bigint_compare_absolute(number1, number2);
	case 1:
		// number1 positive, number2 negative
		return 1;
	case 2:
		// number1 negativem number2 positive
		return -1;
	case 3:
		// Both numbers negative
		return bigint_compare_absolute(number2, number1);
	default:
		return 0;
	}
}

bigint bigint_copy(bigint number)
{
	// Wrong argument passed to function
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return NULL;
	}
	bigint return_number = bigint_create_empty_segments(number->length);
	return_number->length = number->length;
	return_number->sign = number->sign;
	bigint_node *current1 = return_number->first;
	bigint_node *current2 = number->first;
	size_t i = 0;
	for (i = 0; i < number->length; i++)
	{
		current1->value = current2->value;
		current1 = current1->next;
		current2 = current2->next;
	}
	return return_number;
}

int bigint_add_basic(bigint sum, bigint _summand1, bigint _summand2)
{
	// Wrong arguments passed to function
	if (sum == NULL || _summand1 == NULL || _summand2 == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	// Just in case sum and summand 1/2 is the same number
	bigint summand1 = bigint_copy(_summand1);
	bigint summand2 = bigint_copy(_summand2);

	bigint longer = (summand1->length > summand2->length ? summand1 : summand2);
	bigint shorter = (summand1->length > summand2->length ? summand2 : summand1);

	// Empty sum
	reset(sum);
	if (sum->length < longer->length + 1)
	{
		add_segments(sum, longer->length + 1 - sum->length);
	}

	bigint_node empty_node = {0, NULL, NULL};
	empty_node.next = &empty_node;
	uint64_t integer_sum = 0;
	uint32_t temp = 0;
	size_t i = 0;
	bigint_node *current_sum = sum->first;
	bigint_node *current_longer = longer->first;
	bigint_node *current_shorter = shorter->first;
	uint32_t *int_ptr = (uint32_t *)&integer_sum;
	int endianness = check_endian();

	for (i = 0; i <= longer->length; i++)
	{
		integer_sum = (uint64_t)(current_longer->value) + (uint64_t)(current_shorter->value) + (uint64_t)temp;
		if (endianness == -1)
		{
			// Little endian
			current_sum->value = int_ptr[0];
			temp = int_ptr[1];
		}
		else
		{
			// Big endian
			current_sum->value = int_ptr[1];
			temp = int_ptr[0];
		}

		current_sum = current_sum->next;
		current_shorter = (current_shorter->next == NULL ? &empty_node : current_shorter->next);
		current_longer = (current_longer->next == NULL ? &empty_node : current_longer->next);
	}

	bigint_node *temp_ptr = NULL;
	while (sum->last->value == 0 && sum->length != 1)
	{
		temp_ptr = sum->last;
		sum->last->prev->next = NULL;
		sum->last = sum->last->prev;
		if (temp_ptr)
		{
			free(temp_ptr);
			temp_ptr = NULL;
		}
		sum->length -= 1;
	}

	bigint_release_basic(summand1);
	bigint_release_basic(summand2);

	return SUCCESS;
}

int bigint_add_sign(bigint sum, bigint summand1, bigint summand2)
{
	// Wrong arguments
	if (sum == NULL || summand1 == NULL || summand2 == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	int result = 0;
	int comparison = 0;
	switch (2 * summand1->sign + summand2->sign)
	{
	case 0:
		// Both numbers positive
		return bigint_add_basic(sum, summand1, summand2);
	case 1:
	case 2:
		// different sign
		comparison = bigint_compare_absolute(summand1, summand2);
		bigint bigger = (comparison == 1 ? summand1 : summand2);
		bigint smaller = (comparison == 1 ? summand2 : summand1);
		result = bigint_subtract_basic(sum, bigger, smaller);
		sum->sign = (comparison == 0 ? 0 : bigger->sign);
		return result;
	case 3:
		// Both numbers negative
		result = bigint_add_basic(sum, summand1, summand2);
		sum->sign = 1;
		return result;
	default:
		return FAILURE;
	}
}

int bigint_add(int args, bigint sum, ...)
{

	va_list ap;
	va_start(ap, sum);

	// Wrong arguments
	if (args <= 0 || sum == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	// In case sum is also one of summands
	bigint temp_sum = bigint_create("0", 1);

	int status = SUCCESS;
	int i = 0;
	for (i = 0; i < args; i++)
	{
		status = bigint_add_sign(temp_sum, temp_sum, va_arg(ap, bigint));
		if (status == FAILURE)
		{
			return status;
		}
	}
	va_end(ap);

	bigint_release_segments(sum);
	sum->length = temp_sum->length;
	sum->first = temp_sum->first;
	sum->last = temp_sum->last;
	sum->sign = temp_sum->sign;
	free(temp_sum);

	return SUCCESS;
}

int bigint_increment_basic(bigint number)
{
	// Wrong arguments
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	// In case we need to add new segment
	add_segments(number, 1);
	size_t i = 0;
	bigint_node *current = number->first;
	for (i = 0; i < number->length; i++)
	{
		if (~(current->value) == 0)
		{
			current->value += 1;
		}
		else
		{
			current->value += 1;
			break;
		}
		current = current->next;
	}
	if (number->last->value == 0 && number->length != 1)
	{
		bigint_node *temp = number->last;
		number->last->prev->next = NULL;
		number->last = number->last->prev;
		free(temp);
		number->length -= 1;
	}
	return SUCCESS;
}

int bigint_increment(bigint number)
{
	// Wrong arguments
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	if (number->sign == 0)
	{
		// positive
		bigint_increment_basic(number);
	}
	else
	{
		// negative
		bigint_decrement_basic(number);
		// change sign if necessary
		if (number->length == 1 && number->first->value == 0)
		{
			number->sign = 0;
		}
	}
	return SUCCESS;
}

int bigint_subtract_basic(bigint difference, bigint minuend, bigint subtrahend)
{
	// Main idea:
	// x - y = x + ~y + 1

	// Make sure minuend and subtrahend have the same length
	bigint adjusted = bigint_create_empty_segments(minuend->length);
	size_t i = 0;
	bigint_node *current1 = subtrahend->first;
	bigint_node *current2 = adjusted->first;
	for (i = 0; i < subtrahend->length; i++)
	{
		current2->value = ~(current1->value);
		current1 = current1->next;
		current2 = current2->next;
	}
	for (i = subtrahend->length; i < minuend->length; i++)
	{
		current2->value = ~(uint32_t)0;
		current2 = current2->next;
	}

	bigint_add_basic(difference, minuend, adjusted);
	bigint_increment_basic(difference);

	// Delete last segment
	difference->length -= 1;
	difference->last->prev->next = NULL;
	bigint_node *temp = difference->last;
	difference->last = temp->prev;
	free(temp);

	// Delete segments filled with zeros
	bigint_node *temp_ptr = NULL;
	while (difference->last->value == 0 && difference->length != 1)
	{
		temp_ptr = difference->last;
		difference->last->prev->next = NULL;
		difference->last = difference->last->prev;
		free(temp_ptr);
		difference->length -= 1;
	}

	return SUCCESS;
}

int bigint_subtract(bigint difference, bigint minuend, bigint subtrahend)
{
	// Wrong arguments
	if (difference == NULL || minuend == NULL || subtrahend == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	bigint bigger, smaller;
	int result = 0;
	int comparison = 0;
	switch (2 * minuend->sign + subtrahend->sign)
	{
	case 0:
		// Both numbers positive
		comparison = bigint_compare_absolute(minuend, subtrahend);
		bigger = (comparison == 1 ? minuend : subtrahend);
		smaller = (comparison == 1 ? subtrahend : minuend);
		result = bigint_subtract_basic(difference, bigger, smaller);
		difference->sign = (comparison == 0 ? 0 : (subtrahend == bigger ? 1 : 0));
		return result;
	case 1:
		// minuend positive, subtrahend negative
		result = bigint_add_basic(difference, minuend, subtrahend);
		difference->sign = 0;
		return result;
	case 2:
		// minuend negative, subtrahend positive
		result = bigint_add_basic(difference, minuend, subtrahend);
		difference->sign = 1;
		return result;
	case 3:
		// Both numbers negative
		comparison = bigint_compare_absolute(minuend, subtrahend);
		bigger = (comparison == 1 ? minuend : subtrahend);
		smaller = (comparison == 1 ? subtrahend : minuend);
		result = bigint_subtract_basic(difference, bigger, smaller);
		difference->sign = (comparison != 1 ? 0 : 1);
		return result;
	}

	return SUCCESS;
}

int bigint_decrement_basic(bigint number)
{
	// Wrong arguments
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	bigint ones = bigint_create_empty_segments(number->length);
	bigint_not(ones);
	bigint_add_basic(number, number, ones);

	// Delete last segment
	bigint_node *temp_ptr = number->last;
	number->last->prev->next = NULL;
	number->last = number->last->prev;
	free(temp_ptr);
	number->length -= 1;
	bigint_release_basic(ones);
	return SUCCESS;
}

int bigint_decrement(bigint number)
{
	// Wrong arguments
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	// if zero passed to function
	if (number->length == 1 && number->first->value == 0)
	{
		number->first->value += 1;
		number->sign = 1;
		return SUCCESS;
	}
	if (number->sign == 0)
	{
		// positive
		bigint_decrement_basic(number);
	}
	else
	{
		// negative
		bigint_increment_basic(number);
	}
	return SUCCESS;
}

int reset(bigint number)
{
	bigint_node *current = number->first;
	while (current != NULL)
	{
		current->value = 0;
		current = current->next;
	}
	return SUCCESS;
}

int add_segments(bigint number, size_t count)
{
	if (count == 0)
	{
		return SUCCESS;
	}
	number->length += count;
	size_t i = 0;
	bigint_node *current = number->last;
	for (i = 0; i < count; i++)
	{
		current->next = (bigint_node *)malloc(sizeof(bigint_node));
		check_memory_int(current->next);
		initialise_node(current->next);
		current->next->prev = current;
		current = current->next;
	}
	number->last = current;
	return SUCCESS;
}

int bigint_multiply_basic(bigint product, bigint _element1, bigint _element2)
{

	// Wrong argument passed to function
	if (product == NULL || _element1 == NULL || _element2 == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	// In case product and element 1/2 is the same number
	bigint element1 = bigint_copy(_element1);
	bigint element2 = bigint_copy(_element2);

	bigint longer = (bigint_compare_absolute(element1, element2) == 1 ? element1 : element2);
	bigint shorter = (bigint_compare_absolute(element1, element2) == 1 ? element2 : element1);

	reset(product);
	bigint_release_segments(product);
	product->length = 1;
	product->first = (bigint_node *)malloc(sizeof(bigint_node));
	check_memory_int(product->first);
	initialise_node(product->first);
	product->last = product->first;

	bigint temp = NULL;
	size_t counter = 0;
	size_t i = 0;
	size_t j = 0;
	bigint_node *current = shorter->first;
	for (i = 0; i < shorter->length; i++)
	{
		for (j = 0; j < 32; j++)
		{
			if ((current->value & (1 << j)) != 0)
			{
				temp = bigint_copy(longer);
				bigint_shift_left(temp, counter);
				bigint_add_basic(product, product, temp);
				bigint_release_basic(temp);
			}
			counter += 1;
		}
		current = current->next;
	}

	bigint_release_basic(element1);
	bigint_release_basic(element2);

	return SUCCESS;
}

int bigint_multiply(int count, bigint product, ...)
{

	va_list ap;
	va_start(ap, product);

	// Wrong arguments
	if (count <= 0 || product == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	// In case product appears as factor
	bigint temp_product = bigint_create("1", 1);

	int status = SUCCESS;
	int i = 0;
	uint8_t sign = 0;
	bigint current = NULL;
	for (i = 0; i < count; i++)
	{
		current = va_arg(ap, bigint);
		status = bigint_multiply_basic(temp_product, temp_product, current);
		if (status == FAILURE)
		{
			return status;
		}
		sign += current->sign;
	}
	va_end(ap);

	bigint_release_segments(product);
	product->length = temp_product->length;
	product->first = temp_product->first;
	product->last = temp_product->last;
	free(temp_product);
	product->sign = sign % 2;

	return SUCCESS;
}

int leave_one_segment(bigint number)
{
	// Wrong argument
	if (number == NULL)
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}
	if (number->length == 1)
	{
		return SUCCESS;
	}
	bigint_node *current = number->first->next;
	bigint_node *previous = NULL;
	size_t i = 0;
	for (i = 1; i < number->length; i++)
	{
		previous = current;
		current = current->next;
		free(previous);
	}
	number->last = number->first;
	number->length = 1;
	number->first->value = 0;
	return SUCCESS;
}

uint32_t *chain_alignment(bigint number, size_t length)
{
	if (number->length > length)
	{
		return NULL;
	}
	if (number == NULL)
	{
		return NULL;
	}
	uint32_t *chain = (uint32_t *)malloc(length * sizeof(uint32_t));
	memset(chain, 0, length * sizeof(uint32_t));
	size_t shift = length - number->length;
	bigint_node *current = number->last;
	for (; shift < length; shift++)
	{
		chain[shift] = current->value;
		current = current->prev;
	}
	return chain;
}

int array_shift_left(uint32_t *array, size_t length)
{
	size_t i = length - 1;
	uint32_t remaining1 = 0;
	uint32_t remaining2 = 0;
	for (; i != 0; i--)
	{
		remaining1 = (array[i] & 0x80000000) >> 31;
		array[i] = (array[i] << 1) | remaining2;
		remaining2 = remaining1;
	}
	array[i] = (array[i] << 1) | remaining2;
	return SUCCESS;
}

// Length must be the same
int array_subtract(uint32_t *array1, uint32_t *array2, size_t length)
{
	int i = 0;
	uint64_t sum = 0;
	uint32_t temp = 0;
	uint32_t *sum_ptr = (uint32_t *)&sum;
	int endianness = check_endian();
	for (i = length - 1; i >= 0; i--)
	{
		sum = (uint64_t)array1[i] + (uint64_t)(~array2[i]) + (uint64_t)temp;
		if ((size_t)i == length - 1)
		{
			sum += 1;
		}
		if (endianness == -1)
		{
			// Little endian
			array1[i] = sum_ptr[0];
			temp = sum_ptr[1];
		}
		else
		{
			// Big endian
			array1[i] = sum_ptr[1];
			temp = sum_ptr[0];
		}
	}
	return SUCCESS;
}

int greater_or_eq(uint32_t *array1, uint32_t *array2, size_t length)
{
	size_t i = 0;
	for (i = 0; i < length; i++)
	{
		if (array1[i] > array2[i])
		{
			return 1;
		}
		else if (array1[i] < array2[i])
		{
			return 0;
		}
	}
	return 1;
}

int bigint_divide(bigint dividend, bigint divisor, bigint quotient, bigint remainder)
{

	// Wrong arguments passed to function
	if (dividend == NULL || divisor == NULL || (quotient == NULL && remainder == NULL))
	{
		bigint_errno = BIGINT_INCORRECT_FUNCTION_ARGUMENT;
		return FAILURE;
	}

	// Division by zero
	if (divisor->length == 1 && divisor->first->value == 0)
	{
		bigint_errno = BIGINT_DIVISION_BY_ZERO;
		return FAILURE;
	}

	// In case dividend and quotient are the same;
	// Later on we change quotient's length
	size_t initial_len = dividend->length;

	int comparison = bigint_compare_absolute(dividend, divisor);

	// Divisor is greater than dividend or divisor is equal to dividend
	if (comparison == -1 || comparison == 0)
	{
		if (quotient != NULL)
		{
			bigint_release_segments(quotient);
			bigint_node *node = (bigint_node *)malloc(sizeof(bigint_node));
			check_memory_int(node);
			initialise_node(node);
			if (comparison == 0)
			{
				node->value = 1;
				quotient->sign = (dividend->sign == divisor->sign ? 0 : 1);
			}
			else
			{
				node->value = 0;
				quotient->sign = 0;
			}
			quotient->length = 1;
			quotient->first = quotient->last = node;
		}
		if (remainder != NULL)
		{
			bigint_release_segments(remainder);
			if (comparison == 0)
			{
				bigint_node *node = (bigint_node *)malloc(sizeof(bigint_node));
				check_memory_int(node);
				initialise_node(node);
				node->value = 0;
				remainder->sign = 0;
				remainder->length = 1;
				remainder->first = remainder->last = node;
			}
			else
			{
				bigint temp = bigint_copy(dividend);
				remainder->length = temp->length;
				remainder->first = temp->first;
				remainder->last = temp->last;
				remainder->sign = temp->sign;
				free(temp);
			}
		}
		return SUCCESS;
	}

	size_t length = (initial_len) * sizeof(uint32_t);
	uint32_t *remainder_int = (uint32_t *)malloc(length);
	check_memory_int(remainder_int);
	uint32_t *quotient_int = (uint32_t *)malloc(length);
	check_memory_int(remainder_int);
	memset(remainder_int, 0, length);
	memset(quotient_int, 0, length);

	uint32_t *dividend_int = copy_to_chain(dividend);
	uint32_t *divisor_int = chain_alignment(divisor, initial_len);
	int i = (initial_len - 1) * sizeof(uint32_t) * 8 + bit_len(dividend_int[0]) - 1;
	uint32_t bit = 0;

	for (; i >= 0; i--)
	{
		array_shift_left(remainder_int, initial_len);
		bit = get_n_bit(dividend_int, initial_len, i);
		if (bit)
		{
			remainder_int[initial_len - 1] |= 1;
		}
		if (greater_or_eq(remainder_int, divisor_int, initial_len))
		{
			array_subtract(remainder_int, divisor_int, initial_len);
			set_n_bit_to_1(quotient_int, initial_len, i);
		}
	}

	// Save result to quotient bigint structure
	if (quotient != NULL)
	{
		size_t real_len = chain_length(quotient_int, initial_len);
		real_len = (real_len != 0 ? real_len : 1);
		leave_one_segment(quotient);
		add_segments(quotient, real_len - 1);
		size_t j = 0;
		bigint_node *current = quotient->last;
		for (j = 0; j < real_len; j++)
		{
			current->value = quotient_int[initial_len - real_len + j];
			current = current->prev;
		}
		// Negative number division
		if (dividend->sign + divisor->sign == 1)
		{
			quotient->sign = 1;
		}
	}

	// save result to remainder bigint structure
	if (remainder != NULL)
	{
		size_t real_len = chain_length(remainder_int, initial_len);
		real_len = (real_len != 0 ? real_len : 1);
		leave_one_segment(remainder);
		add_segments(remainder, real_len - 1);
		size_t j = 0;
		bigint_node *current = remainder->last;
		for (j = 0; j < real_len; j++)
		{
			current->value = remainder_int[initial_len - real_len + j];
			current = current->prev;
		}
		// Negative number division
		remainder->sign = dividend->sign;
	}

	// free memory
	free(dividend_int);
	free(divisor_int);
	free(remainder_int);
	free(quotient_int);
	return SUCCESS;
}
