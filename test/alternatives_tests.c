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

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <CUnit/CUnit.h>

#include "../src/libalternatives.h"

extern int alternatives_app_main(int argc, char *argv[]);

static int saved_io[3];
static char stdout_buffer[10240];
static char stderr_buffer[10240];
static int start_error_count;

static int wrapCall(int argc, char *argv[])
{
	int stdout_fd = open("test.stdout", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
	int stderr_fd = open("test.stderr", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);

	for (int i=0; i<3; i++)
		saved_io[i] = dup(i);
	fflush(stdout);
	fflush(stderr);
	close(0);
	close(1);
	close(2);

	dup2(stdout_fd, STDOUT_FILENO);
	dup2(stderr_fd, STDERR_FILENO);

	int ret = alternatives_app_main(argc, argv);

	fflush(stdout);
	fflush(stderr);

	for (int i=0; i<3; i++)
		dup2(saved_io[i], i);

	lseek(stdout_fd, 0, SEEK_SET);
	lseek(stderr_fd, 0, SEEK_SET);
	int pos = read(stdout_fd, stdout_buffer, 10240-1);
	stdout_buffer[pos>0?pos:0] = '\x00';
	pos = read(stderr_fd, stderr_buffer, 10240-1);
	stderr_buffer[pos>0?pos:0] = '\x00';

	return ret;
}

extern void setConfigPath(const char *config_path);
static int setupTests()
{
	const char fn[] = "/libalternatives_local.conf";
	const char *wd = get_current_dir_name();
	char *full_path = malloc(strlen(wd) + sizeof(fn)); // includes trailing NULL
	strcpy(full_path, wd);
	strcat(full_path, fn);

	setConfigPath(full_path);
	free((void*)wd);

	unlink(userConfigFile());
	unlink(systemConfigFile());

	return 0;
}

static int removeIOFiles()
{
	if (CU_get_number_of_failures() == 0) {
		unlink(userConfigFile());
		unlink(systemConfigFile());
	}

	unlink("test.stdout");
	unlink("test.stderr");

	return 0;
}

static void storeErrorCount()
{
	start_error_count = CU_get_number_of_failures();
}

static void printOutputOnErrorIncrease()
{
	int new_err_count = CU_get_number_of_failures();

	if (start_error_count < new_err_count) {
		puts("************** TEST OUTPUT ***************");
		printf("stdout: '%s'\n", stdout_buffer);
		printf("stderr: '%s'\n", stderr_buffer);
	}
}

#define WRAP_CALL(a) wrapCall(sizeof(a)/sizeof(char*), a)

void helpScreenWithoutParameters()
{
	char *args[] = {"app"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), -1);
	CU_ASSERT_PTR_NOT_NULL(strstr(stdout_buffer, "this help screen"));
}

void helpScreen()
{
	char *args[] = {"app", "-h"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), 0);
	CU_ASSERT_PTR_NOT_NULL(strstr(stdout_buffer, "this help screen"));
}

void unknownParamsHelpScreen()
{
	char *args[] = {"app", "-8", "nothing to see here"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), -1);
	CU_ASSERT_PTR_NOT_NULL(strstr(stdout_buffer, "this help screen"));
}

void moreThanOneCommand()
{
	char *args[] = {"app", "-h", "-a"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), -1);
	CU_ASSERT_PTR_NOT_NULL(strstr(stdout_buffer, "this help screen"));
}

void listAllAvailablePrograms()
{
	char *args[] = {"app", "-l"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), 0);
	CU_ASSERT_EQUAL(strcmp(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30*  Target: /usr/bin/node30\n\
---\n\
Binary: no_size_alternatives\n\
Alternatives: 0\n\
---\n\
Binary: one_alternative\n\
Alternatives: 1\n\
  Priority: 90*  Target: /usr/bin/ls\n\
---\n\
Binary: test\n\
Alternatives: 2\n\
  Priority: 10   Target: /usr/bin/false\n\
  Priority: 20*  Target: /usr/bin/true\n\
"), 0);
}

void listSpecificProgram()
{
	char *args[] = {"app", "-l", "multiple_alts"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), 0);
	CU_ASSERT_EQUAL(strcmp(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30*  Target: /usr/bin/node30\n\
"), 0);
}

void adjustPriorityForSpecificProgram()
{
	const char binary_name[] = "multiple_alts";
	char *args_read[] = {"app", "-l", (char*)binary_name};
	char *args_set[] = {"app", "-u", "-n", (char*)binary_name, "-p", "20"};
	char *args_set_system[] = {"app", "-s", "-n", (char*)binary_name, "-p", "10"};
	char *args_reset[] = {"app", "-u", "-n", (char*)binary_name};
	char *args_reset_system[] = {"app", "-s", "-n", (char*)binary_name};
	int src;

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_EQUAL(strcmp(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30*  Target: /usr/bin/node30\n\
"), 0);

	CU_ASSERT_EQUAL(loadDefaultConfigOverride(binary_name, &src), 0);

	CU_ASSERT_EQUAL(WRAP_CALL(args_set), 0);
	CU_ASSERT_EQUAL(stdout_buffer[0], '\0');

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_EQUAL(strcmp(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20~  Target: /usr/bin/node20\n\
  Priority: 30   Target: /usr/bin/node30\n\
"), 0);

	CU_ASSERT_EQUAL(loadDefaultConfigOverride(binary_name, &src), 20);
	CU_ASSERT_EQUAL(src, 2);

	CU_ASSERT_EQUAL(WRAP_CALL(args_set_system), 0);
	CU_ASSERT_EQUAL(stdout_buffer[0], '\0');

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_EQUAL(strcmp(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20~  Target: /usr/bin/node20\n\
  Priority: 30   Target: /usr/bin/node30\n\
"), 0);

	CU_ASSERT_EQUAL(loadDefaultConfigOverride(binary_name, &src), 20);
	CU_ASSERT_EQUAL(src, 2);

	CU_ASSERT_EQUAL(WRAP_CALL(args_reset), 0);
	CU_ASSERT_EQUAL(stdout_buffer[0], '\0');

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_EQUAL(strcmp(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10!  Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30   Target: /usr/bin/node30\n\
"), 0);

	CU_ASSERT_EQUAL(loadDefaultConfigOverride(binary_name, &src), 10);
	CU_ASSERT_EQUAL(src, 1);

	CU_ASSERT_EQUAL(WRAP_CALL(args_reset_system), 0);
	CU_ASSERT_EQUAL(stdout_buffer[0], '\0');

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_EQUAL(strcmp(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30*  Target: /usr/bin/node30\n\
"), 0);

	CU_ASSERT_EQUAL(loadDefaultConfigOverride(binary_name, &src), 0);
}

void addAlternativesAppTests()
{
	CU_pSuite suite = CU_add_suite_with_setup_and_teardown("Alternative App Tests", setupTests, removeIOFiles, storeErrorCount, printOutputOnErrorIncrease);
	CU_ADD_TEST(suite, helpScreen);
	CU_ADD_TEST(suite, unknownParamsHelpScreen);
	CU_ADD_TEST(suite, moreThanOneCommand);
	CU_ADD_TEST(suite, helpScreenWithoutParameters);
	CU_ADD_TEST(suite, listAllAvailablePrograms);
	CU_ADD_TEST(suite, listSpecificProgram);
	CU_ADD_TEST(suite, adjustPriorityForSpecificProgram);
}
