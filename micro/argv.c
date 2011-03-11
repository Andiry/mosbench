#include <stddef.h>
#include <stdlib.h>

#define _ARGV_C
#include "argv.h"
#include "bench.h"

static inline uint8_t *args_ptr(struct args *args, off_t off)
{
	return &(((uint8_t *)args)[off]);
}

static void set_u64_value(struct args *args, const char *value, uint8_t *ptr)
{
	uint64_t *val_ptr;
	val_ptr = (uint64_t *) ptr;
	*val_ptr = strtoll(value, NULL, 10);
}

static int get_u64_value(struct args *args, char *value, const uint8_t *ptr)
{
	uint64_t *val_ptr;
	int r;
	val_ptr = (uint64_t *) ptr;
	r = sprintf(value, "%lu", *val_ptr);
	if (r < 0)
		edie("get_u64_value sprintf");
	return r;
}

static void set_str_value(struct args *args, const char *value, uint8_t *ptr)
{
	char **val_ptr;
	val_ptr = (char **) ptr;
	*val_ptr = strdup(value);
}

static int get_str_value(struct args *args, char *value, const uint8_t *ptr)
{
	char **val_ptr;
	int r;
	val_ptr = (char **) ptr;
	r = sprintf(value, "%s", *val_ptr);
	if (r < 0)
		edie("get_str_value sprintf");
	return r;
}

static int get_arg_table_index(const char *name)
{
	int  i;

	for (i = 0; arg_table[i].name; i++)
		if (!strcmp(name, arg_table[i].name))
			return i;

	die("get_arg_table_index failed on %s", name);
}

static size_t print_arg(const char *name, struct args *args,
			char *buffer, size_t size)
{
	size_t cc;
	int r, i;

	cc = 0;
	r = snprintf(buffer, size, "%s = ", name);
	if (r < 0)
		edie("snprintf");
	cc += r;
	i = get_arg_table_index(name);
	cc += arg_table[i].get_value(args, &buffer[cc], 
				     args_ptr(args, arg_table[i].offset));
	return cc;
}

static int is_valid_arg(const char *name, const char **valid_args)
{
	int i;

	for (i = 0; valid_args[i]; i++)
		if (!strcmp(name, valid_args[i]))
			return i;
	
	return -1;
}

static void set_arg(const char *name, const char *value, struct args *args)
{
	int i;

	i = get_arg_table_index(name);
	arg_table[i].set_value(args, value, args_ptr(args, arg_table[i].offset));
}

void argv_sprint(struct args *args, char *buffer, size_t size)
{
	size_t cc;
	int i, r;

	if (!args->valid_args[0])
		return;

	cc = 0;
	cc += print_arg(args->valid_args[0], args, &buffer[cc], size - cc);
	for (i = 1; args->valid_args[i]; i++) {
		r = snprintf(&buffer[cc], size - cc, "  ");
		if (r < 0)
			edie("argv_sprint snprintf");
		cc += r;
		cc += print_arg(args->valid_args[i], args,
				&buffer[cc], size - cc);
	}
}

void argv_print(struct args *args) {
	char buffer[255];

	argv_sprint(args, buffer, sizeof(buffer));
	printf("%s\n", buffer);
}

void __noret__ argv_usage(struct args *args)
{
	int i, k;
	
	printf("usage: %s", args->av0);
	for (i = 0; args->valid_args[i]; i++) {
		k = get_arg_table_index(args->valid_args[i]);
		printf(" -%s %s", args->valid_args[i], arg_table[k].desc);
	}
	printf("\n");
	exit(EXIT_FAILURE);
}

unsigned int argv_parse(int ac, char **av, struct args *args, const char **valid_args)
{
	char seen_arg[255];
	int i;
	int n;

	args->valid_args = valid_args;
	args->av0 = strdup(av[0]);

	if ((ac % 2) == 0)
		argv_usage(args);

	for (i = 0; valid_args[i]; i++)
		seen_arg[i] = 0;

	n = 0;
	for (i = 1; i < ac; i += 2) {
		const char *name;
		const char *value;
		int k;
		
		name = &av[i][1];
		value = av[i + 1];

		k = is_valid_arg(name, valid_args);
		if (k < 0)
			argv_usage(args);
		
		if (!seen_arg[k])
			n++;
		seen_arg[k] = 1;
		set_arg(name, value, args);
	}

	return n;
}