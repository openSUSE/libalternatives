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

static void invalid_binary()
{
	int ret;
	struct AlternativeLink *data = NULL;

	ret = loadDefaultAlternativesForBinary("not_real_binary_binary", &data);
	CU_ASSERT_EQUAL(ret, -1);
	CU_ASSERT_EQUAL(errno, ENOENT);
	CU_ASSERT_PTR_NULL(data);

	ret = loadDefaultAlternativesForBinary("no_size_alternatives", &data);
	CU_ASSERT_EQUAL(ret, -1);
	CU_ASSERT_PTR_NULL(data);
}

static void single_alternative_binary()
{
	size_t ret;
	struct AlternativeLink *data;

	ret = loadDefaultAlternativesForBinary("one_alternative", &data);
	CU_ASSERT_EQUAL(ret, 0);
	CU_ASSERT_PTR_NOT_NULL(data);

	CU_ASSERT_EQUAL(data->priority, 90);
	CU_ASSERT_STRING_EQUAL(data->target, "/usr/bin/ls");
	CU_ASSERT_EQUAL(data->type, ALTLINK_BINARY);

	CU_ASSERT_EQUAL(data[1].type, ALTLINK_EOL);

	free((void*)data->target);
	free(data);
}

static void multiple_alternative_binary()
{
	size_t ret;
	struct AlternativeLink *data;

	ret = loadDefaultAlternativesForBinary("multiple_alts", &data);
	CU_ASSERT_EQUAL(ret, 0);
	CU_ASSERT_PTR_NOT_NULL(data);

	CU_ASSERT_EQUAL(data->priority, 30);
	CU_ASSERT_STRING_EQUAL(data->target, "/usr/bin/node30");
	CU_ASSERT_EQUAL(data->type, ALTLINK_BINARY);

	CU_ASSERT_EQUAL(data[1].type, ALTLINK_EOL);
	free((void*)data->target);
	free(data);
}


extern void addParserTests();

int main()
{
	CU_initialize_registry();

	CU_pSuite suite = CU_add_suite("libalternatives test suite", suite_init, suite_cleanup);
	CU_ADD_TEST(suite, invalid_binary);
	CU_ADD_TEST(suite, single_alternative_binary);
	CU_ADD_TEST(suite, multiple_alternative_binary);

	addParserTests();
	addConfigParserTests();

	CU_basic_run_tests();
	CU_cleanup_registry();
}
