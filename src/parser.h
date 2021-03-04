#include <stdlib.h>

struct AlternativeLink;
struct OptionsParserState;

// parser.c
// parsing priority options installed on the system
struct OptionsParserState* initOptionsParser();
int parseOptionsData(const char *buffer, size_t len, struct OptionsParserState *state);
struct AlternativeLink* doneOptionsParser(int priority, struct OptionsParserState *state);


// config_parser.c
// parsing user override config files
struct ConfigParserState;
struct ConfigParserState* initConfigParser(const char *binary_name);
int parseConfigData(const char *buffer, size_t len, struct ConfigParserState *state);
int doneConfigParserAndReturnPreferredPriority(struct ConfigParserState *state);
