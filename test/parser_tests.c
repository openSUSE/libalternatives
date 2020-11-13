#include <stdlib.h>
#include <CUnit/CUnit.h>
#include "../src/libalternatives.h"
#include "../src/parser.h"

static int noop_function()
{
	return 0;
}

struct ParserState *state;
struct AlternativeLink *result = NULL;

static void freeResults()
{
	for (struct AlternativeLink *link=result; link != NULL && link->type != ALTLINK_EOL; link++)
		free((void*)link->target);

	free(result);
	result = NULL;
}

static void resultsWithoutParsing()
{
	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_PTR_NULL(doneParser(0, state));
}

static void parsingEmptyData()
{
	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData("", 0, state), 0);
	CU_ASSERT_PTR_NULL(doneParser(0, state));
}

static void parsingGarbageData()
{
	const char data[] = "nothing to see here";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(data, sizeof(data)-1, state), -1);
	CU_ASSERT_PTR_NULL(doneParser(0, state));
}

static void parsingSimpleEntry()
{
	const char simple_entry[] = "binary=/usr/bin/vim";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(simple_entry, sizeof(simple_entry)-1, state), 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneParser(10, state));

	CU_ASSERT_EQUAL(result->type, ALTLINK_BINARY);
	CU_ASSERT_STRING_EQUAL(result->target, "/usr/bin/vim");
	CU_ASSERT_EQUAL(result->priority, 10);
}

static void parsingSimpleWithManpage()
{
	const char simple_entry[] = "binary=/usr/bin/vim\nman=vim.123";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(simple_entry, sizeof(simple_entry)-1, state), 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneParser(10, state));

	CU_ASSERT_EQUAL(result->type, ALTLINK_BINARY);
	CU_ASSERT_STRING_EQUAL(result->target, "/usr/bin/vim");
	CU_ASSERT_EQUAL(result->priority, 10);

	CU_ASSERT_EQUAL(result[1].type, ALTLINK_MANPAGE);
	CU_ASSERT_STRING_EQUAL(result[1].target,"vim.123");
	CU_ASSERT_EQUAL(result[1].priority, 10);
}

static void parsingSimpleWithManpageOneBytePerCall()
{
	const char simple_entry[] = "binary=/usr/bin/vim\nman=vim.123";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	for (unsigned i=0; i<sizeof(simple_entry); ++i) {
		CU_ASSERT_EQUAL(parseConfigData(simple_entry+i, 1, state), 0);
	}
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneParser(10, state));

	CU_ASSERT_EQUAL(result->type, ALTLINK_BINARY);
	CU_ASSERT_STRING_EQUAL(result->target, "/usr/bin/vim");
	CU_ASSERT_EQUAL(result->priority, 10);

	CU_ASSERT_EQUAL(result[1].type, ALTLINK_MANPAGE);
	CU_ASSERT_STRING_EQUAL(result[1].target,"vim.123");
	CU_ASSERT_EQUAL(result[1].priority, 10);
}

static void duplicateBinariesUseLatestEntryOnly()
{
	const char data[] = "binary=/usr/bin/vim\nbinary=/usr/bin/emacs";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(data, sizeof(data)-1, state), 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneParser(10, state));

	CU_ASSERT_EQUAL(result->type, ALTLINK_BINARY);
	CU_ASSERT_STRING_EQUAL(result->target, "/usr/bin/emacs");
	CU_ASSERT_EQUAL(result->priority, 10);
}

static void truncateWhitespaceFromEntries()
{
	const char data[] = "     man    = \t  man.123 \n  \t \n\t\n   binary =   /usr/bin/em \tacs\t\t  \t";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(data, sizeof(data)-1, state), 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneParser(10, state));

	CU_ASSERT_EQUAL(result[0].type, ALTLINK_MANPAGE);
	CU_ASSERT_STRING_EQUAL(result[0].target, "man.123");
	CU_ASSERT_EQUAL(result[0].priority, 10);

	CU_ASSERT_EQUAL(result[1].type, ALTLINK_BINARY);
	CU_ASSERT_STRING_EQUAL(result[1].target, "/usr/bin/em \tacs");
	CU_ASSERT_EQUAL(result[1].priority, 10);
}

static void noDelimeterAfterValidToken()
{
	const char data[] = "binary /usr/bin/emacs";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(data, sizeof(data)-1, state), -1);
	CU_ASSERT_PTR_NULL(doneParser(10, state));
}

static void invalidToken()
{
	const char data[] = "binaries = /usr/bin/emacs";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(data, sizeof(data)-1, state), -1);
	CU_ASSERT_PTR_NULL(doneParser(10, state));
}

static void noResumeFromError()
{
	const char data[] = "bbinary = /usr/bin/ls";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(data, 2, state), -1);
	CU_ASSERT_EQUAL(parseConfigData(data+2, sizeof(data)-1-2, state), -1);
	CU_ASSERT_PTR_NULL(doneParser(10, state));

	const char data2[] = "~binary = /usr/bin/ls";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(data2, sizeof(data2)-1, state), -1);
	CU_ASSERT_EQUAL(parseConfigData(data2+1, sizeof(data2)-1-1, state), -1);
	CU_ASSERT_PTR_NULL(doneParser(10, state));

	const char data3[] = "binary != /usr/bin/ls";

	CU_ASSERT_PTR_NOT_NULL(state = initParser());
	CU_ASSERT_EQUAL(parseConfigData(data3, 8, state), -1);
	CU_ASSERT_EQUAL(parseConfigData(data3+8, sizeof(data3)-1-8, state), -1);
	CU_ASSERT_PTR_NULL(doneParser(10, state));
}

void addParserTests()
{
	CU_pSuite tests = CU_add_suite_with_setup_and_teardown("parser",
	                      noop_function, noop_function,
	                      (void(*)(void))noop_function, freeResults);

	CU_ADD_TEST(tests, resultsWithoutParsing);
	CU_ADD_TEST(tests, parsingEmptyData);
	CU_ADD_TEST(tests, parsingGarbageData);
	CU_ADD_TEST(tests, parsingSimpleEntry);
	CU_ADD_TEST(tests, parsingSimpleWithManpage);
	CU_ADD_TEST(tests, parsingSimpleWithManpageOneBytePerCall);
	CU_ADD_TEST(tests, duplicateBinariesUseLatestEntryOnly);
	CU_ADD_TEST(tests, truncateWhitespaceFromEntries);
	CU_ADD_TEST(tests, noDelimeterAfterValidToken);
	CU_ADD_TEST(tests, invalidToken);
	CU_ADD_TEST(tests, noResumeFromError);
}
