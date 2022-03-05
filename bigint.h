/*
 * Copyright (c) 2022 Krzysztof Karczewski
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef _BIGINT_H
#define _BIGINT_H

#include <stdint.h>
#include <stdio.h>

#define BIGINT_MAJOR 1
#define BIGINT_MINOR 0
#define BIGINT_PATCHLEVEL 0

#define ERROR_MESSAGES_SIZE 8
#define bigint_strerror(A) (A >= 0 && A < ERROR_MESSAGES_SIZE ? error_messages[A] : "unknown error")

struct bigint_node
{
	uint32_t value;
	struct bigint_node *prev;
	struct bigint_node *next;
};

struct bigint_data_structure
{
	struct bigint_node *first;
	struct bigint_node *last;
	size_t length;
	uint8_t sign;
};

enum bigint_error_code
{
	ALL_GOOD_IN_THE_HOOD,
	BIGINT_INCORRECT_STRING,
	BIGINT_MEMORY_ALLOCATION_ERROR,
	BIGINT_INCORRECT_FUNCTION_ARGUMENT,
	BIGINT_TOO_LARGE_BIGINT_TO_CONVERT,
	BIGINT_DIVISION_BY_ZERO,
	BIGINT_LENGTH_INDIVISIBLE_BY_FOUR,
	BIGINT_ERROR_IN_DATA_STRUCTURE
};

enum bigint_base
{
	BIN,
	DEC,
	HEX,
	OTHER
};

typedef struct bigint_node bigint_node;
typedef struct bigint_data_structure *bigint;
typedef enum bigint_error_code bigint_error_code;
typedef enum bigint_base bigint_base;

extern bigint_error_code bigint_errno;
extern char *error_messages[ERROR_MESSAGES_SIZE];

int bigint_info();
bigint bigint_create(char *number, size_t length);
int bigint_release(int count, ...);
size_t bigint_size(bigint number);
int bigint_print(FILE *stream, bigint_base base, bigint number);
int bigint_add(int count, bigint sum, ...);
int bigint_increment(bigint number);
int bigint_subtract(bigint difference, bigint minuend, bigint subtrahend);
int bigint_decrement(bigint number);
int bigint_multiply(int count, bigint product, ...);
int bigint_divide(bigint dividend, bigint divisor, bigint quotient, bigint remainder);
int bigint_compare(bigint number1, bigint number2);
bigint bigint_convert_to_bigint(void *integer, size_t length);
int bigint_convert_to_int(bigint number, uintmax_t *integer);
int bigint_change_sign(bigint number);
int bigint_absolute_value(bigint number);
int bigint_get_sign(bigint number);
int bigint_not(bigint number);
int bigint_shift_left(bigint number, size_t count);
int bigint_shift_right(bigint number, size_t count);
bigint bigint_copy(bigint number);

#endif //_BIGINT_H
