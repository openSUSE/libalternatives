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
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <CUnit/CUnit.h>

#include "../src/libalternatives.h"

extern int alternative_app_main(int argc, char *argv[]);

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

	int ret = alternative_app_main(argc, argv);

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
	char *full_path = (char*)malloc(strlen(wd) + sizeof(fn)); // includes trailing NULL
	strcpy(full_path, wd);
	strcat(full_path, fn);

	setConfigPath(full_path);
	free((void*)wd);
	free(full_path);

	unlink(libalts_get_user_config_path());
	unlink(libalts_get_system_config_path());

	return 0;
}

static int cleanupTests()
{
	if (CU_get_number_of_failures() == 0) {
		unlink(libalts_get_user_config_path());
		unlink(libalts_get_system_config_path());
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
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
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
");
}

void listSpecificProgram()
{
	char *args[] = {"app", "-l", "multiple_alts"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30*  Target: /usr/bin/node30\n\
");
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
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30*  Target: /usr/bin/node30\n\
");

	CU_ASSERT_EQUAL(libalts_read_configured_priority(binary_name, &src), 0);

	CU_ASSERT_EQUAL(WRAP_CALL(args_set), 0);
	CU_ASSERT_EQUAL(stdout_buffer[0], '\0');

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20~  Target: /usr/bin/node20\n\
  Priority: 30   Target: /usr/bin/node30\n\
");

	CU_ASSERT_EQUAL(libalts_read_configured_priority(binary_name, &src), 20);
	CU_ASSERT_EQUAL(src, 2);

	CU_ASSERT_EQUAL(WRAP_CALL(args_set_system), 0);
	CU_ASSERT_EQUAL(stdout_buffer[0], '\0');

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20~  Target: /usr/bin/node20\n\
  Priority: 30   Target: /usr/bin/node30\n\
");

	CU_ASSERT_EQUAL(libalts_read_configured_priority(binary_name, &src), 20);
	CU_ASSERT_EQUAL(src, 2);

	CU_ASSERT_EQUAL(WRAP_CALL(args_reset), 0);
	CU_ASSERT_EQUAL(stdout_buffer[0], '\0');

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10!  Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30   Target: /usr/bin/node30\n\
");

	CU_ASSERT_EQUAL(libalts_read_configured_priority(binary_name, &src), 10);
	CU_ASSERT_EQUAL(src, 1);

	CU_ASSERT_EQUAL(WRAP_CALL(args_reset_system), 0);
	CU_ASSERT_EQUAL(stdout_buffer[0], '\0');

	CU_ASSERT_EQUAL(WRAP_CALL(args_read), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: multiple_alts\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
  Priority: 20   Target: /usr/bin/node20\n\
  Priority: 30*  Target: /usr/bin/node30\n\
");

	CU_ASSERT_EQUAL(libalts_read_configured_priority(binary_name, &src), 0);
}

extern void setConfigDirectory(const char *);
static int setupGroupTests()
{
	setConfigDirectory(CONFIG_DIR "/../test_groups");
	return setupTests();
}

static int restoreGroupTestsAndRemoveIOFiles()
{
	setConfigDirectory(CONFIG_DIR);
	return cleanupTests();
}

static void listSpecificProgramInAGroup()
{
	char *args[] = {"app", "-l", "node"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: node\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
                 Group: node, npm\n\
  Priority: 20   Target: /usr/bin/node20\n\
                 Group: node, npm\n\
  Priority: 30*  Target: /usr/bin/node30\n\
                 Group: node, npm\n\
");
}

static void showErrorsForInconsistentGroups()
{
	char *args[] = {"app", "-l", "node_bad"};

	CU_ASSERT_EQUAL(WRAP_CALL(args), 1);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: node_bad\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/node10\n\
                 Group: node_bad, npm\n\
  Priority: 20   Target: /usr/bin/node20\n\
                 Group: npm\n\
  WARNING: binary not part of the Group\n\
  WARNING: shadows more complete Group with lower priority\n\
  Priority: 30*  Target: /usr/bin/node30\n\
  WARNING: shadows more complete Group with lower priority\n\
");
}

static void setPrioritiesAffectEntireGroup()
{
	char *args_set[] = {"app", "-u", "-n", "node", "-p", "10"};
	char *args_reset[] = {"app", "-u", "-n", "node"};
	char *args_set_system[] = {"app", "-s", "-n", "node", "-p", "10"};
	char *args_reset_system[] = {"app", "-s", "-n", "node"};
	char *args_status[] = {"app", "-l", "npm"};

	CU_ASSERT_EQUAL(WRAP_CALL(args_status), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: npm\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/npm10\n\
                 Group: node, npm\n\
  Priority: 20   Target: /usr/bin/npm20\n\
                 Group: node, npm\n\
  Priority: 30*  Target: /usr/bin/npm30\n\
                 Group: node, npm\n\
");
	char **manpages = libalts_get_default_manpages("npm");
	CU_ASSERT_STRING_EQUAL(manpages[0], "npm30.1");
	CU_ASSERT_EQUAL(manpages[1], NULL);
	free(manpages[0]);
	free(manpages);

	CU_ASSERT_EQUAL(WRAP_CALL(args_set), 0);
	CU_ASSERT_EQUAL(WRAP_CALL(args_status), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: npm\n\
Alternatives: 3\n\
  Priority: 10~  Target: /usr/bin/npm10\n\
                 Group: node, npm\n\
  Priority: 20   Target: /usr/bin/npm20\n\
                 Group: node, npm\n\
  Priority: 30   Target: /usr/bin/npm30\n\
                 Group: node, npm\n\
");
	manpages = libalts_get_default_manpages("npm");
	CU_ASSERT_STRING_EQUAL(manpages[0], "npm10.1");
	CU_ASSERT_EQUAL(manpages[1], NULL);
	free(manpages[0]);
	free(manpages);

	CU_ASSERT_EQUAL(WRAP_CALL(args_set_system), 0);
	CU_ASSERT_EQUAL(WRAP_CALL(args_status), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: npm\n\
Alternatives: 3\n\
  Priority: 10~  Target: /usr/bin/npm10\n\
                 Group: node, npm\n\
  Priority: 20   Target: /usr/bin/npm20\n\
                 Group: node, npm\n\
  Priority: 30   Target: /usr/bin/npm30\n\
                 Group: node, npm\n\
");

	CU_ASSERT_EQUAL(WRAP_CALL(args_reset), 0);
	CU_ASSERT_EQUAL(WRAP_CALL(args_status), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: npm\n\
Alternatives: 3\n\
  Priority: 10!  Target: /usr/bin/npm10\n\
                 Group: node, npm\n\
  Priority: 20   Target: /usr/bin/npm20\n\
                 Group: node, npm\n\
  Priority: 30   Target: /usr/bin/npm30\n\
                 Group: node, npm\n\
");
	manpages = libalts_get_default_manpages("npm");
	CU_ASSERT_STRING_EQUAL(manpages[0], "npm10.1");
	CU_ASSERT_EQUAL(manpages[1], NULL);
	free(manpages[0]);
	free(manpages);

	CU_ASSERT_EQUAL(WRAP_CALL(args_reset_system), 0);
	CU_ASSERT_EQUAL(WRAP_CALL(args_status), 0);
	CU_ASSERT_STRING_EQUAL(stdout_buffer,
"Binary: npm\n\
Alternatives: 3\n\
  Priority: 10   Target: /usr/bin/npm10\n\
                 Group: node, npm\n\
  Priority: 20   Target: /usr/bin/npm20\n\
                 Group: node, npm\n\
  Priority: 30*  Target: /usr/bin/npm30\n\
                 Group: node, npm\n\
");
	manpages = libalts_get_default_manpages("npm");
	CU_ASSERT_STRING_EQUAL(manpages[0], "npm30.1");
	CU_ASSERT_EQUAL(manpages[1], NULL);
	free(manpages[0]);
	free(manpages);
}


static int setupExecTests()
{
	setConfigDirectory(CONFIG_DIR "/../test_exec");
	//setenv("LIBALTERNATIVES_DEBUG", "1", 1);
	return setupTests();
}

static int cleanupExecTests()
{
	setConfigDirectory(CONFIG_DIR);
	//unsetenv("LIBALTERNATIVES_DEBUG");
	return cleanupTests();
}

static void failedExecOfUnknown()
{
	char *command_not_found[] = { "/usr/some/something_not_there", "-param", NULL };
	CU_ASSERT_EQUAL(libalts_exec_default(command_not_found), -1);
	CU_ASSERT_EQUAL(errno, ENOENT);
}

static void validExecCommand()
{
	char *command_false[] = { "/usr/path/test42", NULL };
	pid_t child_pid = fork();
	int status = 1000;

	switch (child_pid) {
		case -1:
			CU_ASSERT_FATAL(-1);
			return;
		case 0:
			libalts_exec_default(command_false);
			exit(100);
		default:
			CU_ASSERT_EQUAL_FATAL(wait(&status), child_pid);
			CU_ASSERT(WIFEXITED(status));
			CU_ASSERT_EQUAL(WEXITSTATUS(status), 1);
	}
}

void addAlternativesAppTests()
{
	CU_pSuite suite = CU_add_suite_with_setup_and_teardown("Alternative App Tests", setupTests, cleanupTests, storeErrorCount, printOutputOnErrorIncrease);
	CU_ADD_TEST(suite, helpScreen);
	CU_ADD_TEST(suite, unknownParamsHelpScreen);
	CU_ADD_TEST(suite, moreThanOneCommand);
	CU_ADD_TEST(suite, helpScreenWithoutParameters);
	CU_ADD_TEST(suite, listAllAvailablePrograms);
	CU_ADD_TEST(suite, listSpecificProgram);
	CU_ADD_TEST(suite, adjustPriorityForSpecificProgram);

	suite = CU_add_suite_with_setup_and_teardown("Alternative App with Groups Tests", setupGroupTests, restoreGroupTestsAndRemoveIOFiles, storeErrorCount, printOutputOnErrorIncrease);
	CU_ADD_TEST(suite, listSpecificProgramInAGroup);
	CU_ADD_TEST(suite, showErrorsForInconsistentGroups);
	CU_ADD_TEST(suite, setPrioritiesAffectEntireGroup);

	suite = CU_add_suite_with_setup_and_teardown("Default Exec Tests", setupExecTests, cleanupExecTests, storeErrorCount, printOutputOnErrorIncrease);
	CU_ADD_TEST(suite, failedExecOfUnknown);
	CU_ADD_TEST(suite, validExecCommand);
}
