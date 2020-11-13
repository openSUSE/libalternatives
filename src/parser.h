struct AlternativeLink;
struct ParserState;

struct ParserState* initParser();
int parseConfigData(const char *buffer, size_t len, struct ParserState *state);
struct AlternativeLink* doneParser(int priority, struct ParserState *state);
