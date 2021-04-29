/*  libalternatives - update-alternatives alternative
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
#include <CUnit/Basic.h>

#include "../src/libalternatives.h"

static int suite_init()
{
	return 0;
}

static int suite_cleanup()
{
	return 0;
}

static void free_null()
{
	struct AlternativeLink *data = NULL;

	freeAlternatives(&data);
	CU_ASSERT_PTR_NULL(data);
}

static void invalid_binary()
{
	int ret;
	struct AlternativeLink *data = NULL;

	ret = loadHighestAlternativesForBinary("not_real_binary_binary", &data);
	CU_ASSERT_EQUAL(ret, -1);
	CU_ASSERT_EQUAL(errno, ENOENT);
	CU_ASSERT_PTR_NULL(data);

	ret = loadHighestAlternativesForBinary("no_size_alternatives", &data);
	CU_ASSERT_EQUAL(ret, -1);
	CU_ASSERT_PTR_NULL(data);
}

static void single_alternative_binary()
{
	size_t ret;
	struct AlternativeLink *data;

	ret = loadHighestAlternativesForBinary("one_alternative", &data);
	CU_ASSERT_EQUAL(ret, 0);
	CU_ASSERT_PTR_NOT_NULL(data);

	CU_ASSERT_EQUAL(data->priority, 90);
	CU_ASSERT_STRING_EQUAL(data->target, "/usr/bin/ls");
	CU_ASSERT_EQUAL(data->type, ALTLINK_BINARY);

	CU_ASSERT_EQUAL(data[1].type, ALTLINK_EOL);

	freeAlternatives(&data);
	CU_ASSERT_PTR_NULL(data);
}

static void multiple_alternative_binary()
{
	size_t ret;
	struct AlternativeLink *data;

	ret = loadHighestAlternativesForBinary("multiple_alts", &data);
	CU_ASSERT_EQUAL(ret, 0);
	CU_ASSERT_PTR_NOT_NULL(data);

	CU_ASSERT_EQUAL(data->priority, 30);
	CU_ASSERT_STRING_EQUAL(data->target, "/usr/bin/node30");
	CU_ASSERT_EQUAL(data->type, ALTLINK_BINARY);

	CU_ASSERT_EQUAL(data[1].type, ALTLINK_EOL);

	freeAlternatives(&data);
	CU_ASSERT_PTR_NULL(data);
}


extern void addOptionsParserTests();
extern void addConfigParserTests();
extern void addAlternativesAppTests();

int main()
{
	CU_initialize_registry();

	CU_pSuite suite = CU_add_suite("libalternatives test suite", suite_init, suite_cleanup);
	CU_ADD_TEST(suite, free_null);
	CU_ADD_TEST(suite, invalid_binary);
	CU_ADD_TEST(suite, single_alternative_binary);
	CU_ADD_TEST(suite, multiple_alternative_binary);

	addOptionsParserTests();
	addConfigParserTests();
	addAlternativesAppTests();

	CU_basic_run_tests();
	CU_cleanup_registry();
}
