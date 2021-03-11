#include <stdlib.h>
#include <CUnit/CUnit.h>
#include "../src/parser.h"

static int noop_function()
{
  return 0;
}

struct ConfigParserState *state;

static void resultsWithoutParsing()
{
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1);
  doneConfigParser(state);
}

static void parsingEmptyData()
{
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData("", state), 0);
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1)
  doneConfigParser(state);
}

static void parsingSimpleEntry()
{
  const char simple_entry[] = "editor=10";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),10);
  CU_ASSERT_EQUAL(getConfigLineNr(state), 1);
  doneConfigParser(state);
}

static void parsingGarbageData1()
{
  const char data[] = "nothing to see here";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(data, state), 0);
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1);
  doneConfigParser(state);
}

static void parsingGarbageData2()
{
  const char simple_entry[] = "editor=10 foo";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),0);
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1);
  doneConfigParser(state);
}

static void parsingGarbageData3()
{
  const char simple_entry[] = "editor foo=10";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),0);
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1);
  doneConfigParser(state);
}

static void parsingFalseEntry()
{
  const char simple_entry[] = "foo=10";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),0);
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1);
  doneConfigParser(state);
}

static void parsingWithOtherEntries()
{
  const char entries[] = "line1 \n line 2 \neditor=5\n line 3";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  CU_ASSERT_EQUAL(getConfigLineNr(state), 3);
  doneConfigParser(state);
}

static void parsingWithWhitespaces1()
{
  const char entries[] = "\n\n  editor = 50 \n \n";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),50);
  CU_ASSERT_EQUAL(getConfigLineNr(state), 3);
  doneConfigParser(state);
}

static void parsingWithWhitespaces2()
{
  const char entries[] = "\n\n \t  editor\t =\t 56 \t \n";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),56);
  CU_ASSERT_EQUAL(getConfigLineNr(state), 3);
  doneConfigParser(state);
}

static void parsingWithComment()
{
  const char entries[] = "editor = 57 #this is a comment \n";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),57);
  CU_ASSERT_EQUAL(getConfigLineNr(state), 1);
  doneConfigParser(state);
}

static void parsingNoneDigitalValue()
{
  const char entries[] = "editor=123foo";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),0);
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1);
  doneConfigParser(state);
}

static void  duplicateUseFirstEntry()
{
  const char entries[] = "\n\n editor=5 \n editor=10 \n";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  CU_ASSERT_EQUAL(getConfigLineNr(state), 3);
  doneConfigParser(state);
}

static void SetGetResetEntry()
{
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1);
  CU_ASSERT_EQUAL(getConfigPriority(state), 0);
  setConfigPriority(2,state);
  CU_ASSERT_EQUAL(getConfigPriority(state), 2);
  setConfigDefaultPriority(state);
  CU_ASSERT_EQUAL(getConfigPriority(state), 0);
  CU_ASSERT_EQUAL(strcmp(getConfigBinaryName(state), "editor"), 0);
  doneConfigParser(state);
}

static void SetGetResetNullEntry()
{
  state = NULL;
  getConfigLineNr(state);
  CU_ASSERT_EQUAL(getConfigPriority(state), -1);
  CU_ASSERT_EQUAL(getConfigLineNr(state), -1);
  getConfigBinaryName(state);
  setConfigPriority(2,state);
  setConfigDefaultPriority(state);
  doneConfigParser(state);
}

void addConfigParserTests()
{
  CU_pSuite tests = CU_add_suite_with_setup_and_teardown("ConfigParser",
							 noop_function,
							 noop_function,
							 (void(*)(void))noop_function,
							 (void(*)(void))noop_function);

  CU_ADD_TEST(tests, resultsWithoutParsing);
  CU_ADD_TEST(tests, parsingEmptyData);
  CU_ADD_TEST(tests, parsingGarbageData1);
  CU_ADD_TEST(tests, parsingGarbageData2);
  CU_ADD_TEST(tests, parsingGarbageData3);
  CU_ADD_TEST(tests, parsingSimpleEntry);
  CU_ADD_TEST(tests, parsingFalseEntry);
  CU_ADD_TEST(tests, parsingWithOtherEntries);
  CU_ADD_TEST(tests, parsingWithWhitespaces1);
  CU_ADD_TEST(tests, parsingWithWhitespaces2);
  CU_ADD_TEST(tests, parsingNoneDigitalValue);
  CU_ADD_TEST(tests, duplicateUseFirstEntry);
  CU_ADD_TEST(tests, parsingWithComment);
  CU_ADD_TEST(tests, SetGetResetEntry);
  CU_ADD_TEST(tests, SetGetResetNullEntry);
}
