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

#define TMP_ARCHIVE_NAME "tmp_archive"

#define BLOCK_SIZE 1024 // bytes
#define PERM_SIZE sizeof("rwxrwxrwx")
#define MAX_STR_SIZE 255
#define TIME_SIZE 26
#define EPOCH_SIZE 10

// use lstat instead of stat!
// almost all code for this project is in textbook
// 71 while numRead read section is helpful

// must be run before get_next_header
bool check_file(int fd)
{
	char buf[SARMAG];

	read(fd, buf, SARMAG);
	assert(strncmp(buf, ARMAG, sizeof(buf)) == 0);

	return (true);
}

// assumes file offset only edited by self and check_file()
// pass in header to get, output header absolute offset
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

struct stat *get_stats(struct ar_hdr *hdr)
{
	struct stat *hdr_stat = malloc(sizeof(struct stat));
	assert(lstat(hdr->ar_name, hdr_stat) == 0);
	return (hdr_stat);
}

int create_archive(char *name)
{
	int fd = open(fname, O_RDWR);

	ssize_t ret = write(fd, ARMAG, SARMAG);
	assert (ret == SARMAG);

	return (fd);
}

off_t find_header(int fd, struct ar_hdr *out, char *fname)
{
	off_t hdr_offset;

	do {
		hdr_offset = get_next_header (fd, out);
	} while (strcmp (fix_str(out->ar_name, sizeof(out->ar_name), true),
	                 fname) != 0);

	return (hdr_offset);
}

bool add_file(int fd, struct ar_hdr * hdr, char *contents)
{

	return (true);
}

bool delete_file(int fd, char *fname)
{
	char buf[BLOCK_SIZE];
	int tmp_fd;
	int read_size;
	off_t file_start;
	off_t file_end;
	off_t cur_offset;
	ssize_t num_read;
	struct ar_hdr *tmp;

	// Find offset, size of file to delete
	tmp = malloc (sizeof (struct ar_hdr));
	file_start = find_header(fd, fname, tmp);
	file_end = file_offset + (off_t) (tmp->ar_size);
	// create a new archive
	tmp_fd = open(TMP_ARCHIVE_NAME, O_RDWR);
	// set old file offset to beginning
	lseek (fd, 0, SEEK_SET);
	// copy all contents outside invalid offset range
	while(1) {
		read_size = BLOCK_SIZE;
		cur_offset = lseek (fd, 0, SEEK_CUR);
		/* Skip over contents if in them */
		if ((file_start < cur_offset) && (cur_offset < file_end)) {
			lseek (fd, file_end, SEEK_SET);
		}
		/* crop offset if it goes over */
		if ((cur_offset + BLOCK_SIZE) > file_start) {
			read_size = file_start - cur_offset;
		}
		/* break before write if read failed */
		if ((num_read = read(fd, buf, read_size)) <= 0) {
			break;
		}
		/* if failure to write, archive may be broken */
		if (write (tmp_fd, buf, num_read) != num_read) {
			return(false);
		}
	}
	// delete old archive
	// rename archive to new name

	return (true);
}

bool extract_file(int fd, char *fname)
{

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

// strip trailing whitespace, if backslash then turn into valid fname
char *fix_str(char *str, int len, bool backslash)
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

	if (!check_file(fd)) {
		return (false);
	}
	while (get_next_header(fd, tmp) != -1) {
		if (verbose) {
			// permissions
			printf("%s ", fix_perm(tmp->ar_mode));
			// uid
			printf("%s/", fix_str(tmp->ar_uid, sizeof(tmp->ar_uid), false));
			//gid
			printf("%s     ", fix_str(tmp->ar_gid, sizeof(tmp->ar_gid), false));
			// file size in bytes
			printf("%s ", fix_str(tmp->ar_size, sizeof(tmp->ar_size), false));
			// time
			printf("%s ", fix_time(tmp->ar_date));
		}
		//printf ("-%s-", (char *)tmp);
		printf("%s\n", fix_str(tmp->ar_name, sizeof(tmp->ar_name), true));
		//memset(tmp, 0, sizeof(tmp));
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

	/* Acquire lock */

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
		//bonus
		break;
	default:		// unsupported operation
		printf("Unsupported operation flag. Check for typos.\n");
		break;
	}

	/* Unlock file */

	errno = 0;
	if (close(fd) == -1) {
		printf("Error %d on archive close, data loss may have occured.", errno);

	return (EXIT_SUCCESS);
}
