/*
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  01/27/2013 07:44:06 PM
 *    Filename:  myar.c
 *
 * Description:  Implementation of UNIX archive "ar" utility
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
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

#define TMP_ARCHIVE_NAME "tmp_archive"

#define BLOCK_SIZE 1024		// bytes
#define PERM_SIZE sizeof("rwxrwxrwx")
#define MAX_STR_SIZE 255
#define TIME_SIZE 26
#define EPOCH_SIZE 10

/*
 * use lstat instead of stat!
 * almost all code for this project is in textbook
 * 71 while numRead read section is helpful
 */

/* must be run before get_next_header */
bool check_file(int fd)
{
	char buf[SARMAG];

	lseek(fd, 0, SEEK_SET);
	read(fd, buf, SARMAG);
	assert(strncmp(buf, ARMAG, sizeof(buf)) == 0);

	return (true);
}

/*
 * assumes file offset only edited by self and check_file()
 * pass in header to get, output header absolute offset
 */
off_t get_next_header(int fd, struct ar_hdr * out)
{
	char buf[sizeof(struct ar_hdr)];
	ssize_t was_read;
	off_t offset;

	/* Record header offset */
	offset = lseek(fd, 0, SEEK_CUR);
	/* Read and make sure we got everything */
	was_read = read(fd, buf, sizeof(struct ar_hdr));
	if (was_read == (sizeof(struct ar_hdr))) {
		/* Try filling the struct with a cast */
		memcpy(out, buf, sizeof(*out));
		/* Advance past content */
		lseek(fd, atoi(out->ar_size) + 1, SEEK_CUR);
	} else {
		offset = (off_t) (-1);
	}

	return (offset);
}

/* strip trailing whitespace, if backslash then turn into valid fname */
char *fix_str(char *str, int len, bool backslash)
{
	int idx = len - 1;
	static char ret[MAX_STR_SIZE];

	/* strip trailing whitespace (and backspaces if enabled) */
	while (isspace(str[idx]) || (backslash && (str[idx] == '/'))) {
		--idx;
	}
	strncpy(ret, str, idx + 1);
	/* null terminate after valid, buffer could have other crap in it */
	ret[idx + 1] = '\0';

	return (ret);
}

struct stat *get_stats(struct ar_hdr *hdr)
{
	struct stat *hdr_stat = malloc(sizeof(struct stat));
	assert(lstat(hdr->ar_name, hdr_stat) == 0);
	return (hdr_stat);
}

int create_archive(char *fname)
{
	int fd = open(fname, O_RDWR);

	ssize_t ret = write(fd, ARMAG, SARMAG);
	assert(ret == SARMAG);

	return (fd);
}

off_t find_header(int fd, char *fname, struct ar_hdr * out)
{
	off_t hdr_offset;

	do {
		hdr_offset = get_next_header(fd, out);
		if (hdr_offset == -1) {
			return (-1);
		}
	} while (strcmp(fix_str(out->ar_name, sizeof(out->ar_name), true),
			fname) != 0);

	return (hdr_offset);
}

bool create_header(char *fname, struct ar_hdr * out)
{
	struct stat buf;

	if (lstat(fname, &buf) == -1) {
		return (false);
	}
	/* fill ar_hdr struct */
	snprintf(out->ar_name, sizeof(out->ar_name), "%s/", fname);
	snprintf(out->ar_date, sizeof(out->ar_date), "%llu",
			(long long unsigned int)buf.st_mtime);
	snprintf(out->ar_uid, sizeof(out->ar_uid), "%llu",
			(long long unsigned int)buf.st_uid);
	snprintf(out->ar_gid, sizeof(out->ar_gid), "%llu",
			(long long unsigned int)buf.st_gid);
	snprintf(out->ar_mode, sizeof(out->ar_mode), "%llu",
			(long long unsigned int)buf.st_mode);
	snprintf(out->ar_size, sizeof(out->ar_size), "%llu",
			(long long unsigned int)buf.st_size);
	memcpy(out->ar_fmag, ARFMAG, sizeof(out->ar_fmag));
	/* strip newlines created by snprintf */
	for (int i = 0; i < sizeof(struct ar_hdr); ++i) {
		if (((char *)out)[i] == '\0') {
			((char *)out)[i] = ' ';
		}
	}
	return (true);
}

bool append_file(int fd, char *fname)
{
	struct ar_hdr *hdr = malloc (sizeof (struct ar_hdr));
	char buf[BLOCK_SIZE];
	int file_fd;
	ssize_t num_read;

	/* seek to end of archive */
	/* each archive file members begins on an even byte boundary */
	if ((lseek(fd, 0, SEEK_END) % 2) == 1) {
		write(fd, "\n", 1);
	}
	/* generate header, add to archive */
	if (create_header(fname, hdr)) {
		write(fd, (char *) hdr, sizeof(struct ar_hdr));
	}
	// copy contents
	file_fd = open(fname, O_RDONLY);
	while ((num_read = read(file_fd, buf, BLOCK_SIZE)) > 0) {
		if (write(fd, buf, num_read) != num_read) {
			return (false);
		}
	}
	errno = 0;
	if (close(file_fd) == -1) {
		printf("Error %d on closing %s. Check contents", errno,
		       fname);
	}

	return (true);
}

bool delete_bytes(int fd, off_t start, off_t end)
{
	char buf[BLOCK_SIZE];
	off_t cur;
	off_t new_end;
	ssize_t num_read;
	off_t size = end - start;

	if ((size <= 0) || (fd < 0)) {
		return (false);
	}
	/* set file offset to end of file to delete */
	lseek(fd, end + 1, SEEK_SET);
	while ((num_read = read(fd, buf, BLOCK_SIZE)) > 0) {
		cur = lseek(fd, 0, SEEK_CUR);
		/* jump back to over write previous block */
		lseek(fd, cur - num_read - size - 1, SEEK_SET);
		/* if failure to write, archive may be broken */
		if (write(fd, buf, num_read) != num_read) {
			return (false);
		}
		/* jump forward to read again */
		lseek(fd, cur + size - 1, SEEK_SET);
	}
	/* truncate archive */
	new_end = lseek(fd, 0, SEEK_END) - size - 1;
	ftruncate(fd, new_end);

	return (true);
}

bool delete_file(int fd, char *fname)
{
	off_t file_start;
	off_t file_end;
	struct ar_hdr *tmp;

	tmp = malloc(sizeof(struct ar_hdr));
	file_start = find_header(fd, fname, tmp);
	if (file_start < 0) {
		return (false);
	}
	file_end =
	    file_start + (off_t) (sizeof(struct ar_hdr) +
				  atoll(tmp->ar_size));

	return (delete_bytes(fd, file_start, file_end));
}

bool extract_file(int fd, char *fname)
{
	// get file header from archive
	// create new file with proper name
	// change new file mode
	// copy relevant lines over
	// delete lines from archive
	return (true);
}

char *fix_perm(char *octal)
{
	// layout assumption (setuid)(setgid)(sticky bit)(U)(G)(O)
	// don't care about upper three bits representation, not ar
	static char ret[PERM_SIZE];

	mode_t perm = (mode_t) strtol(octal, NULL, 8);
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

char *fix_time(char *epoch)
{
	static char formatted[TIME_SIZE];
	time_t systime;

	systime = atoll(epoch);
	strftime(formatted, TIME_SIZE, "%b %d %R %Y", localtime(&systime));

	return (formatted);
}

bool print_archive(int fd, bool verbose)
{
	struct ar_hdr *tmp = malloc(sizeof(struct ar_hdr));

	while (get_next_header(fd, tmp) != -1) {
		if (verbose) {
			// permissions
			printf("%s ", fix_perm(tmp->ar_mode));
			// uid
			printf("%s/",
			       fix_str(tmp->ar_uid, sizeof(tmp->ar_uid),
				       false));
			//gid
			printf("%s     ",
			       fix_str(tmp->ar_gid, sizeof(tmp->ar_gid),
				       false));
			// file size in bytes
			printf("%s ",
			       fix_str(tmp->ar_size, sizeof(tmp->ar_size),
				       false));
			// time
			printf("%s ", fix_time(tmp->ar_date));
		}
		//printf ("-%s-", (char *)tmp);
		printf("%s\n",
		       fix_str(tmp->ar_name, sizeof(tmp->ar_name), true));
		//memset(tmp, 0, sizeof(tmp));
	}
	free(tmp);

	return (true);
}

bool timeout_add(int fd, time_t timeout)
{

	return (false);
}

bool append_all(int fd)
{
	return (false);
}

bool interpret_and_call(int fd, char key, int argc, char **argv)
{
	switch (key) {
	case 'q':		// quickly append named files to archive
		for (int i = 3; i < argc; ++i) {
			append_file(fd, argv[i]);
		}
		break;
	case 'x':		// extract named files
		for (int i = 3; i < argc; ++i) {
			extract_file(fd, argv[i]);
		}
		break;
	case 't':		// print a concise table of contents of archive
		print_archive(fd, false);
		break;
	case 'v':		// print a verbose table of contents of archive
		print_archive(fd, true);
		break;
	case 'd':		// delete named files from archive
		for (int i = 3; i < argc; ++i) {
			printf("delete: '%d'",
			       (int) delete_file(fd, argv[i]));
		}
		break;
	case 'A':		// quickly append all "regular" files in the current dir
		assert(argc == 2);
		append_all(fd);
		break;
	case 'w':		// for a timeout, add all modified files to the archive
		//bonus
		assert(argc == 4);
		timeout_add(fd, (time_t) atoi(argv[3]));
		break;
	default:		// unsupported operation
		printf("invalid option -- '%c'\n", key);
		break;
	}

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

	// consider archive init function
	/* archive should be second argument */
	archive = argv[2];

	/* open archive */
	fd = open(archive, O_RDWR);

	/* check archive */
	if (!check_file(fd)) {
		// create an archive file with perms `666` only for q
		// puke otherwise...
		printf("%s: Malformed archive", archive);
		return (false);
	}

	/* Acquire lock */

	/* handle function calls */
	interpret_and_call(fd, key, argc, argv);

	/* Unlock file */

	errno = 0;
	if (close(fd) == -1) {
		printf("Error %d on archive close, data loss possible.\n",
		       errno);
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
