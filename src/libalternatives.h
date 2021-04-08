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


// loads highest priority alternative
int loadDefaultAlternativesForBinary(const char *binary_name, struct AlternativeLink **alternatives);

int loadSpecificAlternativeForBinary(const char *binary_name, int prio, struct AlternativeLink **alternatives);

// int listAllAlternativesForBinary(const char *binary_name, int **alts);
// int loadAlternativeForBinary(int priority, const char *binary_name, struct AlternativeLink **alternatives);

// returns config override (priority) from a given config file
// 0 otherwise, or -1 on error
int loadConfigOverride(const char *binary_name, const char *config_path);

// convenience
void freeAlternatives(struct AlternativeLink **);

// convenience
int execDefault(char *argv[], int argc); // binary in argv[0]
char* defaultManpage(const char *binary_name);
