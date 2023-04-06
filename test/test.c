/*  libalternatives - update-alternatives alternative
 *  Copyright © 2021  SUSE LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <unistd.h>

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

	libalts_free_alternatives_ptr(&data);
	CU_ASSERT_PTR_NULL(data);
}

static void invalid_binary()
{
	int ret;
	struct AlternativeLink *data = NULL;

	ret = libalts_load_highest_priority_binary_alternatives("not_real_binary_binary", &data);
	CU_ASSERT_EQUAL(ret, -1);
	CU_ASSERT_EQUAL(errno, ENOENT);
	CU_ASSERT_PTR_NULL(data);

	ret = libalts_load_highest_priority_binary_alternatives("no_size_alternatives", &data);
	CU_ASSERT_EQUAL(ret, -1);
	CU_ASSERT_PTR_NULL(data);
}

static void single_alternative_binary()
{
	size_t ret;
	struct AlternativeLink *data;

	ret = libalts_load_highest_priority_binary_alternatives("one_alternative", &data);
	CU_ASSERT_EQUAL(ret, 0);
	CU_ASSERT_PTR_NOT_NULL(data);

	CU_ASSERT_EQUAL(data->priority, 90);
	CU_ASSERT_STRING_EQUAL(data->target, "/usr/bin/ls");
	CU_ASSERT_EQUAL(data->type, ALTLINK_BINARY);

	CU_ASSERT_EQUAL(data[1].type, ALTLINK_EOL);

	libalts_free_alternatives_ptr(&data);
	CU_ASSERT_PTR_NULL(data);
}

static void multiple_alternative_binary()
{
	size_t ret;
	struct AlternativeLink *data;

	ret = libalts_load_highest_priority_binary_alternatives("multiple_alts", &data);
	CU_ASSERT_EQUAL(ret, 0);
	CU_ASSERT_PTR_NOT_NULL(data);

	CU_ASSERT_EQUAL(data->priority, 30);
	CU_ASSERT_STRING_EQUAL(data->target, "/usr/bin/node30");
	CU_ASSERT_EQUAL(data->type, ALTLINK_BINARY);

	CU_ASSERT_EQUAL(data[1].type, ALTLINK_EOL);

	libalts_free_alternatives_ptr(&data);
	CU_ASSERT_PTR_NULL(data);
}


extern void addOptionsParserTests();
extern void addConfigParserTests();
extern void addAlternativesAppTests();

int main()
{
	mkdir(CONFIG_DIR "/no_size_alternatives", 0777);
	chdir(CONFIG_DIR "/../..");
	printf("cwd: %s\n", get_current_dir_name());
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
	int failed_tests = CU_get_number_of_tests_failed();
	CU_cleanup_registry();

	return failed_tests != 0;
}
