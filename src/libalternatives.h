#include <unistd.h>

enum AlternativeLinkType
{
	ALTLINK_BINARY = 0,
	ALTLINK_MANPAGE = 1,

	ALTLINK_EOL = -1
};

struct AlternativeLink
{
	int priority;
	int type;
	const char *target;
};


int loadDefaultAlternativesForBinary(const char *binary_name, struct AlternativeLink **alternatives);

// int listAllAlternativesForBinary(const char *binary_name, int **alts);
// int loadAlternativeForBinary(int priority, const char *binary_name, struct AlternativeLink **alternatives);


// convenience
// freeAlternatives(struct AlternativeLink **);

// convenience
// int execDefault(char *argv[], int argc); // binary in argv[0]
// char* defaultManpage(const char *binary_name);
