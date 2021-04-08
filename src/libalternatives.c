//#define  _POSIX_C_SOURCE 202000L
#define __USE_MISC 1
#define _GNU_SOURCE 1

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libalternatives.h"
#include "parser.h"

#if !defined(ETC_PATH)
#error "ETC_PATH is undefined"
#endif

#if !defined(CONFIG_FILENAME)
#error "CONFIG_FILENAME is undefined"
#endif

#define SYSTEM_OVERRIDE_PATH ETC_PATH "/" CONFIG_FILENAME

#if !defined(CONFIG_DIR)
#error "CONFIG_DIR is undefind"
#endif

const char system_override_path[] = "";
const char user_override_path[] = "";

// new_priority = priority from the filesystem
// old_priority = priority from old calls, or 0
// data = additional data
// return => 1 to set new priority, 0 to retain old
typedef int(*PriorityMatchFunction)(int new_priority, int old_priority, int data);

static int PriorityMatch_highest(int a, int b, __attribute__((unused)) int unused)
{
	return (a > b ? 1 : 0);
}

/*
static int PriorityMatch_highestButSmallerThanSupplied(int a, int b, int max)
{
	return (a < max && a > b ? 1 : 0);
}
*/

static int PriorityMatch_getExact(int a, __attribute__((unused)) int b, int prio)
{
	return (a == prio ? 1 : 0);
}

static int findAltConfig(const char *binary_name, PriorityMatchFunction priority_match_func, int *prio)
{
	int configdirfd = open(CONFIG_DIR, O_DIRECTORY, O_RDONLY);
	if (configdirfd < 0)
		return -1;

	int binaryconfigdirfd = openat(configdirfd, binary_name, O_DIRECTORY, O_RDONLY);
	if (binaryconfigdirfd < 0) {
		int saved_error = errno;
		close(configdirfd);
		errno = saved_error;
		return -1;
	}

	close(configdirfd);

	DIR *dir = fdopendir(dup(binaryconfigdirfd));
	if (dir == NULL) {
		int saved_error = errno;
		close(binaryconfigdirfd);
		errno = saved_error;
		return -1;
	}

	struct dirent *dirptr;
	int prio_data = *prio;
	*prio = 0;
	const char *filename = NULL;

	errno = 0;
	while ((dirptr = readdir(dir)) != NULL) {
		if (dirptr->d_name[0] == '.')
			continue;

		switch (dirptr->d_type) {
			case DT_REG:
				break;
			case DT_UNKNOWN: {
				struct stat info;

				if (fstatat(binaryconfigdirfd, dirptr->d_name, &info, AT_SYMLINK_NOFOLLOW) < 0) {
					int saved_error = errno;
					closedir(dir);
					errno = saved_error;
					return -1;
				}

				if (S_ISREG(info.st_mode))
					break;
			}
			// fallthrough
			default:
				continue;
		}

		int new_prio = atoi(dirptr->d_name);
		if (priority_match_func(new_prio, *prio, prio_data) == 1) {
			*prio = new_prio;
			free((void*)filename);
			filename = strdup(dirptr->d_name);
		}
	}
	if (errno != 0) {
		int saved_error = errno;
		closedir(dir);
		errno = saved_error;
		free((void*)filename);
		return -1;
	}
	closedir(dir);

	if (filename == NULL) {
		errno = ENOENT;
		return -1;
	}

	int fd = openat(binaryconfigdirfd, filename, 0, O_RDONLY);
	if (fd < 0) {
		int saved_error = errno;
		free((void*)filename);
		close(binaryconfigdirfd);
		errno = saved_error;
		return -1;
	}
	free((void*)filename);
	close (binaryconfigdirfd);

	return fd;
}

int loadAlternativeForBinary(const char *binary_name, PriorityMatchFunction matcher, int *prio, struct AlternativeLink **alternatives)
{
	int fd = findAltConfig(binary_name, matcher, prio);
	if (fd < 0) {
		*alternatives = NULL;
		return fd;
	}

	struct OptionsParserState *state = initOptionsParser();
	char buffer[1024];
	int ret = 0;
	struct stat stat_data;

	if (fstat(fd, &stat_data) < 0) {
		*alternatives = NULL;
		return -1;
	}
	if (stat_data.st_size > 10240) {
		fprintf(stderr, "options file with priority %d is unusually large. Truncating to 10kB", *prio);
		stat_data.st_size = 10240;
	}
	while (ret == 0 && stat_data.st_size > 0) {
		ssize_t s = read(fd, buffer, 1024);
		if (s > 0) {
			ret = parseOptionsData(buffer, s, state);
			stat_data.st_size -= s;
		}
		else if (s == 0) {
			fprintf(stderr, "options file with priority %d changed during reading?", *prio);
			break;
		}
		else {
			switch (errno) {
				case EINTR:
					continue;
				default:
					return -1;
			}
		}
	}

	close(fd);
	*alternatives = doneOptionsParser(*prio, state);
	return ret;
}

int loadDefaultAlternativesForBinary(const char *binary_name, struct AlternativeLink **alternatives)
{
	int prio = 0;
	return loadAlternativeForBinary(binary_name, PriorityMatch_highest, &prio, alternatives);
}

int loadSpecificAlternativeForBinary(const char *binary_name, int prio, struct AlternativeLink **alternatives)
{
	return loadAlternativeForBinary(binary_name, PriorityMatch_getExact, &prio, alternatives);
}

int loadConfigOverride(const char *binary_name, const char *config_path)
{
	int fd = open(config_path, O_RDONLY | O_NOFOLLOW);
	if (fd < 0)
		return -1;

	struct stat stat_data;
	if (fstat(fd, &stat_data) < 0) {
		switch (errno) {
			case ENOENT:
				return 0;
			default:
				return -1;
		}
	}

	const ssize_t max_config_size = 1 << 10;
	if (stat_data.st_size >= max_config_size) {
		fprintf(stderr, "ignoring libalternatives config file: %s. Too large.\n", config_path);
		close(fd);
		return 0;
	}

	char data[max_config_size];
	ssize_t pos = 0;
	while (stat_data.st_size != 0) {
		ssize_t rs = read(fd, data + pos, stat_data.st_size);
		if (rs == 0 && stat_data.st_size > 0) {
			fprintf(stderr, "libalternatives config seems to have changed: %s. Trying to parse anyway\n", config_path);
			break;
		}
		else if (rs < 0) {
			switch (errno) {
				case EINTR:
					continue;
				default:
					return -1;
			}
		}
		else {
			stat_data.st_size -= rs;
		}
	}
	data[pos] = '\x00';

	struct ConfigParserState *state = initConfigParser(binary_name);
	int prio = parseConfigData(data, state);
	doneConfigParser(state);
	return prio;
}

void freeAlternatives(struct AlternativeLink **links)
{
	for (struct AlternativeLink *ptr=*links; ptr != NULL && ptr->type != ALTLINK_EOL; ptr++)
		free((void*)ptr->target);

	free(*links);
	*links = NULL;
}

#ifdef __GNUC__
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define unlikely(x)     (x)
#endif

int libalternatives_debug = 0;
#define IS_DEBUG unlikely(libalternatives_debug)

static void checkEnvDebug()
{
	const char *debug = secure_getenv("LIBALTERNATIVES_DEBUG");
	libalternatives_debug = ( debug != NULL && debug[0] == '1' && debug[1] == '\x00' );
}

static const char *concat_str_safe(const char *str1, int len1, const char *str2, int len2)
{
	char *str = malloc(len1 + len2);
	strncpy(str, str1, len1);
	strncpy(str+len1, str2, len2);
	return str;
}

static int loadAlternatives(const char *binary_name, struct AlternativeLink **alts)
{
	int priority = 0;

	// try to load user override
	const char *config_home = secure_getenv("XDG_CONFIG_HOME");
	const char *config_path = NULL;
	if (config_home != NULL) {
		const char config_filename[] = "/" CONFIG_FILENAME;
		config_path = concat_str_safe(config_home, strlen(config_home), config_filename, sizeof(config_filename));
	}
	else {
		config_home = secure_getenv("HOME");
		const char config_filename[] = "/.config/" CONFIG_FILENAME;
		if (config_home != NULL)
			config_path = concat_str_safe(config_home, strlen(config_home), config_filename, sizeof(config_filename));
	}

	if (config_path != NULL) {
		if (IS_DEBUG)
			fprintf(stderr, "Trying to load user override for %s from: %s\n", binary_name, config_path);
		priority = loadConfigOverride(binary_name, config_path);
		if (IS_DEBUG)
			fprintf(stderr, "user override priority: %d\n", priority);
	}

	// if not loaded, try system override
	if (priority <= 0) {
		priority = loadConfigOverride(binary_name, SYSTEM_OVERRIDE_PATH);
		if (IS_DEBUG)
			fprintf(stderr, "system override priority: %d\n", priority);
	}

	int ret = 0;
	if (priority > 0)
		ret = loadSpecificAlternativeForBinary(binary_name, priority, alts);
	else
		ret = loadDefaultAlternativesForBinary(binary_name, alts);

	if (IS_DEBUG)
		fprintf(stderr, "loaded alternative for the priority: %d\n", ret);

	return ret;
}

int execDefault(char *argv[])
{
	argv[0]=basename(argv[0]);

	struct AlternativeLink *alts;
	checkEnvDebug();
	loadAlternatives(argv[0], &alts);

	if (alts) {
		while (alts->type != ALTLINK_EOL) {
			if (alts->type == ALTLINK_BINARY) {
				argv[0] = (char*)alts->target;
				execv(alts->target, argv);
				perror("Failed to execute target.");
				break;
			}
		}
	}

	if (IS_DEBUG)
		fprintf(stderr, "execDefault() failed with target %s\n", (alts ? alts->target : NULL));
	return -1;
}

char* defaultManpage(const char *binary_name)
{
	struct AlternativeLink *alts;
	checkEnvDebug();
	loadAlternatives(binary_name, &alts);
}
