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
#include "../src/libalternative.h"
#include "../src/parser.h"

static int noop_function()
{
	return 0;
}

static struct OptionsParserState *state;
static struct AlternativeLink *result = NULL;

static void freeResults()
{
	for (struct AlternativeLink *link=result; link != NULL && link->type != ALTLINK_EOL; link++)
		free((void*)link->target);

	free(result);
	result = NULL;
}

static void resultsWithoutParsing()
{
	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_PTR_NULL(doneOptionsParser(0, state));
}

static void parsingEmptyData()
{
	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData("", 0, state), 0);
	CU_ASSERT_PTR_NULL(doneOptionsParser(0, state));
}

static void parsingGarbageData()
{
	const char data[] = "nothing to see here";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, sizeof(data)-1, state), -1);
	CU_ASSERT_PTR_NULL(doneOptionsParser(0, state));
}

static void parsingSimpleEntry()
{
	const char simple_entry[] = "binary=/usr/bin/vim";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(simple_entry, sizeof(simple_entry)-1, state), 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneOptionsParser(10, state));

	CU_ASSERT_EQUAL(result->type, ALTLINK_BINARY);
	CU_ASSERT_STRING_EQUAL(result->target, "/usr/bin/vim");
	CU_ASSERT_EQUAL(result->priority, 10);
}

static void parsingSimpleWithManpage()
{
	const char simple_entry[] = "binary=/usr/bin/vim\nman=vim.123";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(simple_entry, sizeof(simple_entry)-1, state), 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneOptionsParser(10, state));

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

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	for (unsigned i=0; i<sizeof(simple_entry); ++i) {
		CU_ASSERT_EQUAL(parseOptionsData(simple_entry+i, 1, state), 0);
	}
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneOptionsParser(10, state));

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

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, sizeof(data)-1, state), 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneOptionsParser(10, state));

	CU_ASSERT_EQUAL(result->type, ALTLINK_BINARY);
	CU_ASSERT_STRING_EQUAL(result->target, "/usr/bin/emacs");
	CU_ASSERT_EQUAL(result->priority, 10);
}

static void truncateWhitespaceFromEntries()
{
	const char data[] = "     man    = \t  man.123 \n  \t \n\t\n   binary =   /usr/bin/em \tacs\t\t  \t";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, sizeof(data)-1, state), 0);
	CU_ASSERT_PTR_NOT_NULL_FATAL(result = doneOptionsParser(10, state));

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

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, sizeof(data)-1, state), -1);
	CU_ASSERT_PTR_NULL(doneOptionsParser(10, state));
}

static void invalidToken()
{
	const char data[] = "binaries = /usr/bin/emacs";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, sizeof(data)-1, state), -1);
	CU_ASSERT_PTR_NULL(doneOptionsParser(10, state));
}

static void noResumeFromError()
{
	const char data[] = "bbinary = /usr/bin/ls";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, 2, state), -1);
	CU_ASSERT_EQUAL(parseOptionsData(data+2, sizeof(data)-1-2, state), -1);
	CU_ASSERT_PTR_NULL(doneOptionsParser(10, state));

	const char data2[] = "~binary = /usr/bin/ls";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data2, sizeof(data2)-1, state), -1);
	CU_ASSERT_EQUAL(parseOptionsData(data2+1, sizeof(data2)-1-1, state), -1);
	CU_ASSERT_PTR_NULL(doneOptionsParser(10, state));

	const char data3[] = "binary != /usr/bin/ls";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data3, 8, state), -1);
	CU_ASSERT_EQUAL(parseOptionsData(data3+8, sizeof(data3)-1-8, state), -1);
	CU_ASSERT_PTR_NULL(doneOptionsParser(10, state));
}

static void parsingMultipleManpages()
{
	const char data[] = "binary=/usr/bin/ls\nman=foo.1,  \t bar.5n,, ,   omega.8section   ";
	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, sizeof(data), state), 0);
	CU_ASSERT_PTR_NOT_NULL(result = doneOptionsParser(10, state));

	CU_ASSERT_EQUAL(result[0].type, ALTLINK_BINARY);
	CU_ASSERT_EQUAL(result[1].type, ALTLINK_MANPAGE);
	CU_ASSERT_EQUAL(result[2].type, ALTLINK_MANPAGE);
	CU_ASSERT_EQUAL(result[3].type, ALTLINK_MANPAGE);
	CU_ASSERT_EQUAL(result[4].type, ALTLINK_EOL);

	CU_ASSERT_STRING_EQUAL(result[0].target, "/usr/bin/ls");
	CU_ASSERT_STRING_EQUAL(result[1].target, "foo.1");
	CU_ASSERT_STRING_EQUAL(result[2].target, "bar.5n");
	CU_ASSERT_STRING_EQUAL(result[3].target, "omega.8section");
}

static void parsesSingleGroup()
{
	const char data[] = "binary=/usr/bin/ls\ngroup=foobar";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, sizeof(data), state), 0);
	CU_ASSERT_PTR_NOT_NULL(result = doneOptionsParser(10, state));

	CU_ASSERT_EQUAL(result[0].type, ALTLINK_BINARY);
	CU_ASSERT_EQUAL(result[1].type, ALTLINK_GROUP);
	CU_ASSERT_EQUAL(result[2].type, ALTLINK_EOL);

	CU_ASSERT_STRING_EQUAL(result[1].target, "foobar");
}

static void parsesMultipleGroupsAsLatestGroupList()
{
	const char data[] = "binary=/usr/bin/ls\ngroup=foobar\ngroup=bar,foo";

	CU_ASSERT_PTR_NOT_NULL(state = initOptionsParser());
	CU_ASSERT_EQUAL(parseOptionsData(data, sizeof(data), state), 0);
	CU_ASSERT_PTR_NOT_NULL(result = doneOptionsParser(10, state));

	CU_ASSERT_EQUAL(result[0].type, ALTLINK_BINARY);
	CU_ASSERT_EQUAL(result[1].type, ALTLINK_GROUP);
	CU_ASSERT_EQUAL(result[2].type, ALTLINK_GROUP);
	CU_ASSERT_EQUAL(result[3].type, ALTLINK_EOL);

	CU_ASSERT_STRING_EQUAL(result[1].target, "bar");
	CU_ASSERT_STRING_EQUAL(result[2].target, "foo");
}

void addOptionsParserTests()
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
	CU_ADD_TEST(tests, parsingMultipleManpages);
	CU_ADD_TEST(tests, parsesSingleGroup);
	CU_ADD_TEST(tests, parsesMultipleGroupsAsLatestGroupList);
}
