#include <stdlib.h>
#include <memory.h>

#include "libalternatives.h"
#include "parser.h"

typedef int(*ParserFunction)(const char *data, size_t len, struct ParserState *state);

struct ParserState
{
	u_int32_t parser_func_param, parser_func_param2;
	ParserFunction parser_func;

	struct AlternativeLink *parsed_data;
	int parsed_data_size;
};

static int isWhitespace(const char c)
{
	return c == ' ' || c == '\t';
}

static int isDelimeter(const char c)
{
	return c == '=';
}

static int parser_alreadyErrorNoResumePossible(const char *data, size_t len, struct ParserState *state)
{
	(void)data;
	(void)len;
	(void)state;

	return -1;
}

static int parser_searchToken(const char *data, size_t len, struct ParserState *state);
static int parser_parseValue(const char *data, size_t len, struct ParserState *state)
{
	struct AlternativeLink *link = state->parsed_data + state->parser_func_param;
	u_int16_t value_string_pos = state->parser_func_param2 & 0xFFFF;
	u_int16_t value_string_size = state->parser_func_param2 >> 16;

	while (len > 0) {
		char c = *data;
		char *out = (char*)link->target;

		if (value_string_pos >= value_string_size) {
			value_string_size += 0x100;
			link->target = realloc((void*)link->target, value_string_size);
			out = (char*)link->target;
		}

		switch (c) {
			case '\n':
			case '\r':
				out[value_string_pos] = '\x00';
				while (value_string_pos > 0 && isWhitespace(out[value_string_pos-1])) {
					out[--value_string_pos] = '\x00';
				}

				state->parser_func = parser_searchToken;
				return state->parser_func(data+1, len-1, state);
			default:
				out[value_string_pos] = c;
				break;
		}

		data++;
		len--;
		value_string_pos++;
	}

	state->parser_func_param2 = value_string_pos | (((u_int32_t)value_string_size) << 16);
	return 0;
}

static void allocateBuffer(struct AlternativeLink **link, int *size_ptr)
{
	int size = *size_ptr;
	if (*link == NULL || size < 8)
		size += 8;

	*link = realloc(*link, sizeof(struct AlternativeLink) * size);

	for (int i=*size_ptr; i<size; ++i) {
		(*link+i)->type = ALTLINK_EOL;
		(*link+i)->target = 0;
	}
	*size_ptr = size;
}

static int findFirstParsedDataLocation(struct ParserState *state, int type)
{
	for (int i=0; i<state->parsed_data_size - 1; ++i) {
		struct AlternativeLink *data = state->parsed_data + i;
		if (data->type == type)
			return i;

		if (data->type == ALTLINK_EOL) {
			data->type = type;
			return i;
		}
	}

	allocateBuffer(&state->parsed_data, &state->parsed_data_size);
	return findFirstParsedDataLocation(state, type);
}

#define AFTER_OFFSET 1000
enum WhiteSpaceSkipType
{
	BINARY_FUNCTION_BEFORE_EQUAL=1,
	BINARY_FUNCTION_AFTER_EQUAL=BINARY_FUNCTION_BEFORE_EQUAL+AFTER_OFFSET,
	MAN_FUNCTION_BEFORE_EQUAL=2,
	MAN_FUNCTION_AFTER_EQUAL=MAN_FUNCTION_BEFORE_EQUAL+AFTER_OFFSET,
};

static int parser_skipOptionalWhiteSpace(const char *data, size_t len, struct ParserState *state)
{
	while (len > 0 && isWhitespace(*data)) {
		len--;
		data++;
	}

	if (len == 0)
		return 0;

	switch (state->parser_func_param) {
		case BINARY_FUNCTION_BEFORE_EQUAL:
		case MAN_FUNCTION_BEFORE_EQUAL:
			if (!isDelimeter(*data)) {
				state->parser_func = parser_alreadyErrorNoResumePossible;
				break;
			}

			data++;
			len--;

			state->parser_func_param = state->parser_func_param + AFTER_OFFSET;
			break;

		case BINARY_FUNCTION_AFTER_EQUAL:
			state->parser_func = parser_parseValue;

			state->parser_func_param = findFirstParsedDataLocation(state, ALTLINK_BINARY);
			state->parser_func_param2 = 0;
			break;
		case MAN_FUNCTION_AFTER_EQUAL:
			state->parser_func = parser_parseValue;

			state->parser_func_param = findFirstParsedDataLocation(state, ALTLINK_MANPAGE);
			state->parser_func_param2 = 0;
			break;
	}

	return state->parser_func(data, len, state);
}

static int assertTokenMatch(const char *data, size_t len, struct ParserState *state, const char *match, int match_len, int whiteSpace_param)
{
	int pos = state->parser_func_param;
	int end_pos = match_len - pos + 1 <= (int)len ? match_len - pos + 1 : (int)len + pos;

	while (pos < end_pos && *data == match[pos]) {
		data++;
		pos++;
		len--;
	}

	if (pos != end_pos) {
		state->parser_func = parser_alreadyErrorNoResumePossible;
		return -1;
	}

	if (len == 0 && pos < match_len) {
		state->parser_func_param = pos;
		return 0;
	}
	state->parser_func = parser_skipOptionalWhiteSpace;
	state->parser_func_param = whiteSpace_param;
	return state->parser_func(data, len, state);
}

static int parser_assertBinaryToken(const char *data, size_t len, struct ParserState *state)
{
	const char match[] = "binary";
	return assertTokenMatch(data, len, state, match, sizeof(match)-1, BINARY_FUNCTION_BEFORE_EQUAL);
}

static int parser_assertManpageToken(const char *data, size_t len, struct ParserState *state)
{
	const char match[] = "man";

	return assertTokenMatch(data, len, state, match, sizeof(match)-1, MAN_FUNCTION_BEFORE_EQUAL);
}

static int parser_searchToken(const char *data, size_t len, struct ParserState *state)
{
	while (len > 0 && (isWhitespace(*data) || *data == '\n' || *data == '\r')) {
		data++;
		len--;
	}

	if (len == 0)
		return 0;

	switch (data[0]) {
		case 'b':
			state->parser_func_param = 1;
			state->parser_func = parser_assertBinaryToken;
			break;
		case 'm':
			state->parser_func_param = 1;
			state->parser_func = parser_assertManpageToken;
			break;
		default:
			state->parser_func = parser_alreadyErrorNoResumePossible;
	};

	return state->parser_func(data+1, len-1, state);
}

struct ParserState* initParser()
{
	struct ParserState *state = malloc(sizeof(struct ParserState));

	state->parser_func = parser_searchToken;
	state->parser_func_param = 0;
	state->parsed_data = NULL;
	state->parsed_data_size = 0;

	return state;
}

int parseConfigData(const char *buffer, size_t len, struct ParserState *state)
{
	return state->parser_func(buffer, len, state);
}

struct AlternativeLink* doneParser(int priority, struct ParserState *state)
{
	int errors = 0;
	while (state->parser_func != parser_searchToken && (errors = state->parser_func("\n", 1, state)) == 0);

	if (errors != 0) {
		free(state->parsed_data);
		free(state);
		return NULL;
	}

	struct AlternativeLink *parsed_data = state->parsed_data;
	for (struct AlternativeLink *ptr = parsed_data; ptr && ptr->type != ALTLINK_EOL; ptr++)
		ptr->priority = priority;

	free(state);
	return parsed_data;
}
