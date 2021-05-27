/*  libalternative - update-alternatives alternative
 *  Copyright (C) 2021  SUSE LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <CUnit/CUnit.h>
#include "../src/parser.h"

static int noop_function()
{
  return 0;
}

static struct ConfigParserState *state;

static void resultsWithoutParsing()
{
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  doneConfigParser(state);
}

static void resultsWithoutParsingNULLBinary()
{
  CU_ASSERT_PTR_NULL(state = initConfigParser(NULL));
  doneConfigParser(state);
}

static void parsingEmptyData()
{
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData("", state), 0);
  doneConfigParser(state);
}

static void parseEmptyDataAndAddSingleEntry()
{
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData("", state), 0);
  CU_ASSERT_EQUAL(strcmp(setBinaryPriorityAndReturnUpdatedConfig(20, state), "editor=20"), 0)
  doneConfigParser(state);
}

static void parsingSimpleEntry()
{
  const char simple_entry[] = "editor=10";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),10);
  doneConfigParser(state);
}

static void parsingGarbageData1()
{
  const char data[] = "nothing to see here";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(data, state), 0);
  doneConfigParser(state);
}

static void parsingGarbageData2()
{
  const char simple_entry[] = "editor=10 foo";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),0);
  doneConfigParser(state);
}

static void parsingGarbageData3()
{
  const char simple_entry[] = "editor foo=10";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),0);
  doneConfigParser(state);
}

static void parsingFalseEntry()
{
  const char simple_entry[] = "foo=10";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(simple_entry, state),0);
  doneConfigParser(state);
}

static void parsingWithOtherEntries()
{
  const char entries[] = "line1 \n line 2 \neditor=5\n line 3";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  doneConfigParser(state);
}

static void parsingWithWhitespaces1()
{
  const char entries[] = "\n\n  editor = 50 \n \n";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),50);
  doneConfigParser(state);
}

static void parsingWithWhitespaces2()
{
  const char entries[] = "\n\n \t  editor\t =\t 56 \t \n";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),56);
  doneConfigParser(state);
}

static void parsingWithComment()
{
  const char entries[] = "editor = 57 #this is a comment \n";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),57);
  doneConfigParser(state);
}

static void parsingNoneDigitalValue()
{
  const char entries[] = "editor=123foo";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),0);
  doneConfigParser(state);
}

static void  duplicateUseFirstEntry()
{
  const char entries[] = "\n\n editor=5 \n editor=10 \n";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  doneConfigParser(state);
}

static void similarBinaries()
{
  const char editor_with_space[] = "my editor=10\neditor=11\n";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(editor_with_space, state), 11);
}

static void invalidNegativePriority()
{
  const char editor_with_space[] = "editor=-10\neditor=11\n";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(editor_with_space, state), 11);
}

static void setEntryInTheMiddle()
{
  const char entries[] = "line1 \n line 2 \neditor=5\n line 3";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  const char *buffer = setBinaryPriorityAndReturnUpdatedConfig(22,state);
  CU_ASSERT_EQUAL(strcmp(buffer, "line1 \n line 2 \neditor=22\n line 3"), 0);
  CU_ASSERT_EQUAL(getConfigPriority(state), 22);
  doneConfigParser(state);
}

static void setEntryAtTheEnd()
{
  const char entries[] = "line1 \n line 2 \n line 3\neditor=5";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  const char *buffer = setBinaryPriorityAndReturnUpdatedConfig(333,state);
  CU_ASSERT_EQUAL(strcmp(buffer, "line1 \n line 2 \n line 3\neditor=333"), 0);
  CU_ASSERT_EQUAL(getConfigPriority(state), 333);
  doneConfigParser(state);
}

static void setEntryAtTheBeginning()
{
  const char entries[] = "editor=555\n line 2 \n line 3\n";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),555);
  const char *buffer = setBinaryPriorityAndReturnUpdatedConfig(4,state);
  CU_ASSERT_EQUAL(strcmp(buffer, "editor=4\n line 2 \n line 3\n"), 0);
  CU_ASSERT_EQUAL(getConfigPriority(state), 4);
  doneConfigParser(state);
}

static void setNewEntry()
{
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  const char *buffer = setBinaryPriorityAndReturnUpdatedConfig(4,state);
  CU_ASSERT_EQUAL(strcmp(buffer, "editor=4"), 0);
  CU_ASSERT_EQUAL(getConfigPriority(state), 4);
  doneConfigParser(state);
}

static void setEntryRemoveDoubleEntry1()
{
  const char entries[] = "line1 \n line 2 \neditor=5\n line 3\neditor=7";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  const char *buffer = setBinaryPriorityAndReturnUpdatedConfig(22,state);
  CU_ASSERT_EQUAL(strcmp(buffer, "line1 \n line 2 \neditor=22\n line 3"), 0);
  CU_ASSERT_EQUAL(getConfigPriority(state), 22);
  doneConfigParser(state);
}

static void setEntryRemoveDoubleEntry2()
{
  const char entries[] = "line1 \neditor=2\n line 2 \neditor=5\n line 3";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),2);
  const char *buffer = setBinaryPriorityAndReturnUpdatedConfig(22,state);
  CU_ASSERT_EQUAL(strcmp(buffer, "line1 \neditor=22\n line 2 \n line 3"), 0);
  CU_ASSERT_EQUAL(getConfigPriority(state), 22);
  doneConfigParser(state);
}

static void setWithEmptyArguments()
{
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_PTR_NULL(setBinaryPriorityAndReturnUpdatedConfig(0,state));
  CU_ASSERT_PTR_NULL(setBinaryPriorityAndReturnUpdatedConfig(1,NULL));
}

static void resetEntries()
{
  const char entries[] = "editor=5\nline1 \n line 2 \neditor=5\n line 3\neditor=6";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  const char *buffer = resetToDefaultPriorityAndReturnUpdatedConfig(state);
  CU_ASSERT_EQUAL(strcmp(buffer, "line1 \n line 2 \n line 3"), 0);
  CU_ASSERT_EQUAL(getConfigPriority(state), 0);
  doneConfigParser(state);
}

static void setPriorityMultipleTimes()
{
  const char entries[] = "editor=5\nline1 \n line 2 \neditor=5\n line 3\neditor=6";
  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),5);
  const char *b1 = setBinaryPriorityAndReturnUpdatedConfig(10, state);
  CU_ASSERT_EQUAL(strcmp(b1, "editor=10\nline1 \n line 2 \n line 3"), 0);
  const char *buffer = resetToDefaultPriorityAndReturnUpdatedConfig(state);
  CU_ASSERT_EQUAL(strcmp(buffer, "line1 \n line 2 \n line 3"), 0);
  CU_ASSERT_EQUAL(getConfigPriority(state), 0);
//  CU_ASSERT_EQUAL(strcmp(b1, "editor=10\nline1 \n line 2 \n line 3"), 0);
  doneConfigParser(state);
}

static void resetNULLEntry()
{
  CU_ASSERT_PTR_NULL(resetToDefaultPriorityAndReturnUpdatedConfig(NULL));
}

static void resetNonExistentEntryShouldNotAddNewEntry()
{
  const char entries[] = "";

  CU_ASSERT_PTR_NOT_NULL(state = initConfigParser("editor"));
  CU_ASSERT_EQUAL(parseConfigData(entries, state),0);
  const char *buffer = resetToDefaultPriorityAndReturnUpdatedConfig(state);
  CU_ASSERT_STRING_EQUAL(buffer, "");
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
  CU_ADD_TEST(tests, resultsWithoutParsingNULLBinary);
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
  CU_ADD_TEST(tests, similarBinaries);
  CU_ADD_TEST(tests, invalidNegativePriority);
  CU_ADD_TEST(tests, setEntryInTheMiddle);
  CU_ADD_TEST(tests, setEntryAtTheEnd);
  CU_ADD_TEST(tests, setEntryAtTheBeginning);
  CU_ADD_TEST(tests, setNewEntry);
  CU_ADD_TEST(tests, setEntryRemoveDoubleEntry1);
  CU_ADD_TEST(tests, setEntryRemoveDoubleEntry2);
  CU_ADD_TEST(tests, setWithEmptyArguments);
  CU_ADD_TEST(tests, resetEntries);
  CU_ADD_TEST(tests, setPriorityMultipleTimes);
  CU_ADD_TEST(tests, resetNULLEntry);
  CU_ADD_TEST(tests, parseEmptyDataAndAddSingleEntry);
  CU_ADD_TEST(tests, resetNonExistentEntryShouldNotAddNewEntry);
}
