#ifndef _BITSPEC_BITSPEC_H
#define _BITSPEC_BITSPEC_H

struct rule {
	struct rule 	*next;
	char			name[32];
	unsigned int	startbit;
	unsigned int	endbit;
	unsigned int	notation;
	unsigned int	checkvalue;
	unsigned int	needcheck: 1;
	unsigned int	enable: 1;
};

#endif