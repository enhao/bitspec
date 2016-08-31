#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "inih/ini.h"
#include "bitspec.h"

void usage(void)
{	
	fprintf(stderr, "Usage:\tbispec [SPEC] [HEX]\n");
}

unsigned int str_to_uint(const char *str, const int radix)
{
	long int value;
	
	if ((radix != 2) && (radix != 10) && (radix != 16)) {
		errno = EINVAL;
		return 0;
	} 

	errno = 0;
	value = strtol(str, (char **)NULL, radix);

	if ((value > UINT_MAX) || (value < 0))
		errno = ERANGE;
	
	if (errno)
		return 0;

	return (unsigned int)value;
}


struct rule *rule_new(void)
{
	struct rule *rule;

	rule = calloc(1, sizeof(struct rule));
	if (rule)
		rule->notation = 16;

	return rule;
}

void free_rules(struct rule *rules)
{
	struct rule *rule = rules;
	struct rule *priv = NULL;

	if (!rule)
		return;
	
	while (rule) {
		priv = rule;
		rule = rule->next;

		free(priv);
		priv = NULL;
	}
}

struct rule *get_rule(struct rule *rules, const char *name)
{
	struct rule *rule = rules;
	struct rule *priv = NULL;
	int length, count = 0;

	while (rule) {
		if (!strcmp(name, rule->name))
			return rule;
		
		priv = rule;
		rule = rule->next;
		count++;
	}

	length = strlen(name);
	length = (length > 31) ? 31 : length;

	if (count == 1 && !strlen(priv->name)) {
		strncpy(priv->name, name, length);	
		return priv;
	}

	rule = rule_new();
	if (!rule) {
		printf("failed to allocate memory\n");
		return rule;
	}

	priv->next = rule;	
	strncpy(rule->name, name, length);	
	return rule;
}

void set_bits(struct rule *rule, const char *value)
{
	char *delim;
	
	errno = 0;
	rule->startbit = str_to_uint(value, 10);
	if (errno) {
		printf("%s: failed to parse startbit\n", rule->name);
		return;
	}

	delim = strchr(value, ':');
	if (delim) {
		rule->endbit = str_to_uint(++delim, 10);
		if (errno) {
			printf("%s: failed to parse endbit\n", rule->name);
			return;
		}
	} else {
		rule->endbit = rule->startbit;
	}

	rule->enable = 1;
}

void set_checkvalue(struct rule *rule, const char *value)
{
	errno = 0;
	rule->checkvalue = str_to_uint(value, rule->notation);
	if (errno) {
		printf("%s: failed to parse checkvalue\n", rule->name);
		return;
	}

	rule->needcheck = 1;
}

void set_notation(struct rule *rule, const char *value)
{
	char buf[20] = {'\0',};

	errno = 0;
	rule->notation = str_to_uint(value, 10);
	if (errno) {
		printf("%s: failed to parse notation\n", rule->name);
		return;
	}

	if (!rule->needcheck)
		return;

	sprintf(buf, "%x", rule->checkvalue);
	set_checkvalue(rule, buf);
}

int rule_parser(void *user, const char *section, const char *name, const char *value)
{
    struct rule *rule, *rules = (struct rule *)user;

    rule = get_rule(rules, section);
    if (!rule)
    	return 0;

	if (!strcmp(name, "bits")) {
		set_bits(rule, value);
	} else if (!strcmp(name, "notation")) {
		set_notation(rule, value);
	} else if (!strcmp(name, "checkvalue")) {
		set_checkvalue(rule, value);
	} else {
        return 0;
    }
    
    return 1;
}

void list_rules(struct rule *rules)
{
	struct rule *rule = rules;

	while (rule) {
		printf("[%c] %s\n"
			   "    bits             :  %u:%u\n"
			   "    notation         :  %u\n"
			   "    checkvalue (hex) :  %X\n\n",
			   rule->enable ? 'v' : ' ', rule->name,
			   rule->startbit, rule->endbit,
			   rule->notation, rule->checkvalue);

		rule = rule->next;
	}
}

unsigned int get_bitwidth(const struct rule *rule)
{
	unsigned int max, min;
	
	max = (rule->startbit > rule->endbit) ? rule->startbit : rule->endbit;
	min = (rule->startbit < rule->endbit) ? rule->startbit : rule->endbit;
	return max - min + 1;
}

unsigned int get_mask(const struct rule *rule)
{
	unsigned int width, shift, result = 0;
	
	width = get_bitwidth(rule);
	shift = (rule->startbit > rule->endbit) ? rule->startbit : rule->endbit;

	for (; width; width--, shift--)
		result |= 1 << shift;

	return result;
}

void print_binary(unsigned int width, unsigned int value)
{	
	while (--width > 0)
		printf("%u", (value & (1 << width)) ? 1 : 0);

	printf("%u", (value & (1 << width)) ? 1 : 0);
}

void print_checkvalue(const struct rule *rule)
{
	unsigned int width;

	if (!rule->needcheck) {
		printf("\n");
		return;
	}
	
	width = get_bitwidth(rule);

	if (rule->notation == 2) {
		printf(", checkvalue = ");
		print_binary(width, rule->checkvalue);
		printf("\n");
	} else if (rule->notation == 10)
		printf(", checkvalue = %d\n", rule->checkvalue);
	else
		printf(", checkvalue = %#X\n", rule->checkvalue); 
}

void print_result(const struct rule *rule, unsigned int result)
{
	unsigned int width;
	
	if (!rule->needcheck)
		printf("[  VIEW]");
	else if (result != rule->checkvalue)
		printf("[FAILED]");
	else 
		printf("[    OK]");

	if (rule->startbit != rule->endbit)
		printf("  %s:  bit[%u:%u] = ", rule->name, rule->startbit, 
			   rule->endbit);
	else
		printf("  %s:  bit[%u] = ", rule->name, rule->startbit);

	width = get_bitwidth(rule);
	
	if (rule->notation == 16)
		printf("%#X", result);
	else if (rule->notation == 10) 
		printf("%d", result);
	else
		print_binary(width, result);

	print_checkvalue(rule);
}

int spec_validate(struct rule *rules, const char *value)
{
	struct rule *rule = rules;
	unsigned int mask, input, shift, result;

	errno = 0;
	input = str_to_uint(value, 16);
	if (errno) {
		printf("failed to parse input\n");
		return -1;
	}

	printf("Input: %#X\n", input);
	while (rule) {
		if (rule->enable) {
			mask = get_mask(rule);
			shift = (rule->startbit < rule->endbit) ? 
					rule->startbit : rule->endbit;
			result = (input & mask) >> shift;

			print_result(rule, result);
		}

		rule = rule->next;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct rule *rules = NULL;

	do {
		if (argc != 3) {
			usage();
			break;
		}

		rules = rule_new();
		if (!rules) {
			printf("failed to allocate memory\n");
			break;
		}

		if (ini_parse(argv[1], rule_parser, rules) < 0) {
			printf("failed to parse spec\n");
			break;
		}

		(void)spec_validate(rules, argv[2]);

		free_rules(rules);
		return 0;
	} while(0);

	free_rules(rules);
	return -1;
}
