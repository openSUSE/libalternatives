#include <stdlib.h>
#include <CUnit/CUnit.h>
#include "../src/parser.h"

static int noop_function()
{
	return 0;
}

struct ConfigParserState *state;

static void freeResults()
{

}

static void resultsWithoutParsing()
{
	CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
	doneConfigParser(state);
}

static void parsingEmptyData()
{
	CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
	CU_ASSERT_EQUAL(parseConfigData("", state), 0);
	doneConfigParser(state);
}

static void parsingGarbageData()
{
	const char data[] = "nothing to see here";

	CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
	CU_ASSERT_EQUAL(parseConfigData(data, state), 0);
	doneConfigParser(state);
}

static void parsingSimpleEntry()
{
	const char simple_entry[] = "editor=10";

	CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
	CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),10);

	doneConfigParser(state);	
}



void addConfigParserTests()
{
	CU_pSuite tests = CU_add_suite_with_setup_and_teardown("ConfigParser",
	                      noop_function, noop_function,
	                      (void(*)(void))noop_function, freeResults);

	CU_ADD_TEST(tests, resultsWithoutParsing);
	CU_ADD_TEST(tests, parsingEmptyData);
	CU_ADD_TEST(tests, parsingGarbageData);
	CU_ADD_TEST(tests, parsingSimpleEntry);
	/*							
	CU_ADD_TEST(tests, parsingSimpleWithManpage);
	CU_ADD_TEST(tests, parsingSimpleWithManpageOneBytePerCall);
	CU_ADD_TEST(tests, duplicateBinariesUseLatestEntryOnly);
	CU_ADD_TEST(tests, truncateWhitespaceFromEntries);
	CU_ADD_TEST(tests, noDelimeterAfterValidToken);
	CU_ADD_TEST(tests, invalidToken);
	CU_ADD_TEST(tests, noResumeFromError);
	*/
}
