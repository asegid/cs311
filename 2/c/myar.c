/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  01/27/2013 07:44:06 PM
 *    Filename:  myar.c
 *
 * Description:  Implementation of UNIX archive "ar" utility
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <ar.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PERM_SIZE sizeof("rwxrwxrwx")
#define MAX_STR_SIZE 255
#define TIME_SIZE 26
#define EPOCH_SIZE 10

// use lstat instead of stat!
// almost all code for this project is in textbook
// 71 while numRead read read section is helpful

// must be run before get_next_header
bool check_file(int fd)
{
	char buf[SARMAG];

	read(fd, buf, SARMAG);
	assert(strncmp(buf, ARMAG, sizeof(buf)) == 0);

	return (true);
}

// strip trailing whitespace, if backslash then turn into valid filename
char *format_string(char *str, int len, bool backslash)
{
	int idx = len - 1;
	static char ret[MAX_STR_SIZE];

	// strip trailing whitespace (and backspaces if enabled)
	while (isspace(str[idx]) || (backslash && (str[idx] == '/'))) {
		--idx;
	}
	strncpy(ret, str, idx + 1);
	// null terminate after valid, buffer could have other crap in it
	ret[idx + 1] = '\0';

	return (ret);
}

// assumes file offset only edited by self and check_file()
// pass in header to get, output offset of contents
off_t get_next_header(int fd, struct ar_hdr * out)
{
	char buf[sizeof(struct ar_hdr)];
	ssize_t was_read;
	off_t content_offset;

	/* Read and make sure we got everything */
	was_read = read(fd, buf, sizeof(struct ar_hdr));
	if (was_read == (sizeof(struct ar_hdr))) {

		/* Try filling the struct with a cast */
		memcpy(out, buf, sizeof(*out));

		/* Record content offset in case we care */
		content_offset = lseek(fd, 0, SEEK_CUR);

		/* Advance past content */
		lseek(fd, atoi(out->ar_size) + 1, SEEK_CUR);

	} else {
		content_offset = (off_t) (-1);
	}
	return (content_offset);
}

struct stat *get_stats(struct ar_hdr *hdr)
{
	struct stat *hdr_stat = malloc(sizeof(struct stat));
	assert(lstat(hdr->ar_name, hdr_stat) == 0);
	return (hdr_stat);
}

bool add_file(int fd, struct ar_hdr * hdr, char *contents)
{

	return (true);
}

bool delete_file(int fd, struct ar_hdr * hdr)
{

	return (true);
}

bool extract_file(int fd, struct ar_hdr * hdr)
{

	return (true);
}

char *format_time(char *epoch)
{
	time_t systime;
	static char formatted[TIME_SIZE];

	systime = atoll(epoch);
	strftime(formatted, TIME_SIZE, "%b %d %R %Y", localtime(&systime));

	return (formatted);
}

char *format_permissions(char *octal)
{
// layout assumption (setuid)(setgid)(sticky bit)(U)(G)(O)
	static char ret[PERM_SIZE];

	mode_t perm = strtol(octal, NULL, 8);
	snprintf(ret, PERM_SIZE, "%c%c%c%c%c%c%c%c%c",
		 ((perm & S_IRUSR) ? 'r' : '-'),
		 ((perm & S_IWUSR) ? 'w' : '-'),
		 ((perm & S_IXUSR) ? 'x' : '-'),
		 ((perm & S_IRGRP) ? 'r' : '-'),
		 ((perm & S_IWGRP) ? 'w' : '-'),
		 ((perm & S_IXGRP) ? 'x' : '-'),
		 ((perm & S_IROTH) ? 'r' : '-'),
		 ((perm & S_IWOTH) ? 'w' : '-'),
		 ((perm & S_IXOTH) ? 'x' : '-'));

	return (ret);
}

bool print_archive(int fd, bool verbose)
{
	struct ar_hdr *tmp = malloc(sizeof(struct ar_hdr));

	if (!check_file(fd)) {
		return (false);
	}

	while (get_next_header(fd, tmp) != -1) {
		if (verbose) {
			// permissions
			printf("%s ", format_permissions(tmp->ar_mode));

			// uid
			printf("%s/", format_string(tmp->ar_uid,
						    sizeof(tmp->ar_uid),
						    false));

			//gid
			printf("%s     ", format_string(tmp->ar_gid,
							sizeof(tmp->
							       ar_gid),
							false));

			// file size in bytes
			printf("%s ", format_string(tmp->ar_size,
						    sizeof(tmp->ar_size),
						    false));

			// time
			printf("%s ", format_time(tmp->ar_date));
		}
		//printf ("-%s-", (char *)tmp);
		printf("%s\n", format_string(tmp->ar_name,
					     sizeof(tmp->ar_name), true));
		memset(tmp, 0, sizeof(tmp));
	}
	free(tmp);

	return (true);
}

int main(int argc, char **argv)
{
	char *archive;
	int fd;
	char key;

	/* Syntax "myar key afile name ..." where afile=archive, key=opt */
	assert(argc >= 3);
	assert(strlen(argv[1]) == 2);

	assert(argv[1][0] == '-');

	/* operation key */
	key = argv[1][1];

	/* archive should be second argument */
	archive = argv[2];

	/* open archive */
	fd = open(archive, O_RDWR);

	switch (key) {
	case 'q':		// quickly append named files to archive

		break;
	case 'x':		// extract named files

		break;
	case 't':		// print a concise table of contents of archive
		print_archive(fd, false);
		break;
	case 'v':		// print a verbose table of contents of archive
		print_archive(fd, true);
		break;
	case 'd':		// delete named files from archive

		break;
	case 'A':		// quickly append all "regular" files in the current dir

		break;
	case 'w':		// for a timeout, add all modified files to the archive

		break;
	default:		// unsupported operation

		break;
	}

	return (EXIT_SUCCESS);
}
