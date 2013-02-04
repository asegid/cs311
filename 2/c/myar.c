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
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BLOCK_SIZE 1024		// bytes
#define PERM_SIZE sizeof("rwxrwxrwx")
#define MAX_STR_SIZE 255
#define TIME_SIZE 26
#define EPOCH_SIZE 10


/* must be run before get_next_header */
bool check_isarchive(int fd)
{
	char buf[SARMAG];

	assert((fd >= 0));

	if (lseek(fd, 0, SEEK_SET) != 0) return (false);

	if (read(fd, buf, SARMAG) != SARMAG) return (false);

	/* Finally, can check to ensure strings are the same */
	if (strncmp(buf, ARMAG, sizeof(buf)) != 0) return (false);

	return (true);
}

/*
 * assumes file offset only edited by self and check_isarchive()
 * pass in header to get, output header absolute offset
 */
off_t get_next_header(int fd, struct ar_hdr * out)
{
	char buf[sizeof(struct ar_hdr)];
	off_t hdr_offset;
	off_t arsize;

	assert((fd >= 0) && (out != NULL));

	/* Record header offset */
	if ((hdr_offset = lseek(fd, 0, SEEK_CUR)) == -1) return (hdr_offset);

	/* Read and make sure we got everything */
	if ((size_t)(read(fd, buf, sizeof(buf))) != sizeof(buf)) return (-1);

	/* Try filling the struct with a cast */
	memcpy(out, buf, sizeof(*out));

	/* advance past content */
	arsize = atoll(out->ar_size);
	if ((arsize % 2) == 1) ++arsize;
	if(lseek(fd, arsize, SEEK_CUR) == -1) return (-1);

	return (hdr_offset);
}

/* strip trailing whitespace, if backslash then turn into valid fname */
char *fix_str(char *str, int len, bool backslash)
{
	size_t idx;
	static char ret[MAX_STR_SIZE];

	assert((str != NULL) && (len > 0));

	/* strip trailing whitespace (and backspaces if enabled) */
	idx = (size_t) (len - 1);
	while (isspace(str[idx]) || (backslash && (str[idx] == '/'))) --idx;
	strncpy(ret, str, idx + 1);
	/* null terminate after valid, buffer could have other crap in it */
	ret[idx + 1] = '\0';

	return (ret);
}

int create_archive(char *fname)
{
	int fd;
	int flags = (O_RDWR | O_CREAT | O_EXCL);

	assert(fname != NULL);

	if ((fd = open (fname, flags, 0666)) == -1) return (fd);
	/* If error writing to archive, try unlinking it and exit with error */
	if (write(fd, ARMAG, SARMAG) != SARMAG) {
			fd = -1;
			unlink (fname);
		}

	return (fd);
}

off_t find_header(int fd, char *fname, struct ar_hdr * out)
{
	off_t hdr_offset;

	assert((fd >= 0) && (fname != NULL) && (out != NULL));

	do {
		if ((hdr_offset = get_next_header(fd, out)) == -1) return (-1);
	} while (strcmp(fix_str(out->ar_name, (int) sizeof(out->ar_name), true),
			fname) != 0);

	return (hdr_offset);
}

void replace_chars(char *str, int len, char from, char to)
{
	assert((str != NULL) && (len > 0));

	for (int i = 0; i < len; ++i) {
		if (str[i] == from) str[i] = to;
	}
}

bool create_header(char *fname, struct ar_hdr * out)
{
	struct stat buf;

	assert((fname != NULL) && (out != NULL));

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
	snprintf(out->ar_mode, sizeof(out->ar_mode), "%llo",
			(long long unsigned int)(buf.st_mode));
	snprintf(out->ar_size, sizeof(out->ar_size), "%llu",
			(long long unsigned int)buf.st_size);
	memcpy(out->ar_fmag, ARFMAG, sizeof(out->ar_fmag));

	/* strip newlines created by snprintf */
	replace_chars((char *)out, sizeof(struct ar_hdr), '\0', ' ');

	return (true);
}

bool append_file(int fd, char *fname)
{
	struct ar_hdr *hdr = malloc (sizeof (struct ar_hdr));
	char buf[BLOCK_SIZE];
	int file_fd;
	ssize_t num_read;

	assert((fd >= 0) && (fname != NULL));

	/* seek to end of archive */
	/* each archive file members begins on an even byte boundary */
	if ((lseek(fd, 0, SEEK_END) % 2) == 1) write(fd, "\n", 1);

	/* generate header, add to archive */
	if (create_header(fname, hdr)){
		write(fd, (char *) hdr, sizeof(struct ar_hdr));
	} else {
		return (false);
	}

	/* copy contents */
	file_fd = open(fname, O_RDONLY);
	while ((num_read = read(file_fd, buf, BLOCK_SIZE)) > 0) {
		if (write(fd, buf, num_read) != num_read) return (false);
	}

	/* catch errors */
	errno = 0;
	if (close(file_fd) == -1) {
		printf("Error %d on closing %s. Check contents", errno, fname);
		return (false);
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

	assert((size > 0) && (fd >= 0));

	/* set file offset to end of file to delete */
	lseek(fd, end + 1, SEEK_SET);
	while ((num_read = read(fd, buf, BLOCK_SIZE)) > 0) {
		cur = lseek(fd, 0, SEEK_CUR);

		/* jump back to over write previous block */
		lseek(fd, cur - num_read - size - 1, SEEK_SET);

		/* if failure to write, archive may be broken */
		if (write(fd, buf, num_read) != num_read) return (false);

		/* jump forward to read again */
		lseek(fd, cur, SEEK_SET);
	}

	/* truncate archive */
	new_end = lseek(fd, 0, SEEK_END) - size - 1;
	ftruncate(fd, new_end);

	return (true);
}

bool delete_file(int fd, char *fname)
{
	off_t file_end;
	off_t file_start;
	struct ar_hdr *tmp;

	assert((fd >= 0) && (fname != NULL));

	tmp = malloc(sizeof(struct ar_hdr));
	file_start = find_header(fd, fname, tmp);
	if (file_start < 0) {
		return (false);
	}
	file_end =
	    file_start + (off_t) (sizeof(struct ar_hdr) + atoll(tmp->ar_size));
	free (tmp);

	return (delete_bytes(fd, file_start, file_end));
}

bool extract_file(int fd, char *fname)
{
	char buf[BLOCK_SIZE];
	off_t content_start;
	off_t cur;
	off_t file_start;
	off_t file_end;
	int new_fd;
	ssize_t num_read;
	ssize_t read_size;
	mode_t perm;
	struct ar_hdr *tmp;

	assert((fd >= 0) && (fname != NULL));

	/* get file header from archive */
	tmp = malloc(sizeof(struct ar_hdr));
	file_start = find_header(fd, fname, tmp);
	content_start = (file_start + sizeof(struct ar_hdr));
	file_end = content_start + (off_t) (atoll(tmp->ar_size));

	/* create new file with proper name and mode */
	perm = (mode_t) strtol(tmp->ar_mode, NULL, 8);
	new_fd = open(fix_str(tmp->ar_name, sizeof(tmp->ar_name), true),
	              ( O_WRONLY | O_CREAT | O_EXCL ), perm);
	if (new_fd == -1) {
		return (false);
	}

	/* copy relevant lines over */
	cur = lseek(fd, content_start, SEEK_SET);
	read_size = BLOCK_SIZE;
	if ((cur + read_size) > file_end) {
		read_size = file_end - cur;
	}
	while ((num_read = read(fd, buf, read_size)) > 0) {
		write(new_fd, buf, num_read);
		cur += num_read;
		if ((cur + read_size) > file_end) {
			read_size = file_end - cur;
		}
	}

	/* delete lines from archive */
	delete_bytes(fd, file_start, file_end);
	/* close new file */
	close(new_fd);

	return (true);
}

char *fix_perm(char *octal)
{
	static char ret[PERM_SIZE];

	assert((octal != NULL));

	mode_t perm = (mode_t) strtol(octal, NULL, 8);
	snprintf(ret, PERM_SIZE, "%c%c%c%c%c%c%c%c%c",
	         ((perm & S_IRUSR) ? 'r' : '-'),
	         ((perm & S_IWUSR) ? 'w' : '-'),
	         ((perm & S_IXUSR) ? ((perm & S_ISUID) ? 's' : 'x') :
	                             ((perm & S_ISUID) ? 'S' : '-')),
	         ((perm & S_IRGRP) ? 'r' : '-'),
	         ((perm & S_IWGRP) ? 'w' : '-'),
	         ((perm & S_IXGRP) ? ((perm & S_ISGID) ? 's' : 'x') :
	                             ((perm & S_ISGID) ? 'S' : '-')),
	         ((perm & S_IROTH) ? 'r' : '-'),
	         ((perm & S_IWOTH) ? 'w' : '-'),
	         ((perm & S_IXOTH) ? ((perm & S_ISVTX) ? 't' : 'x') :
	                             ((perm & S_ISVTX) ? 'S' : '-')));

	return (ret);
}

char *fix_time(char *epoch)
{
	static char formatted[TIME_SIZE];
	time_t systime;

	assert((epoch != NULL));

	systime = atoll(epoch);
	strftime(formatted, TIME_SIZE, "%b %d %R %Y", localtime(&systime));

	return (formatted);
}

bool print_archive(int fd, bool verbose)
{
	struct ar_hdr *tmp = malloc(sizeof(struct ar_hdr));

	assert((fd >= 0));

	while (get_next_header(fd, tmp) != -1) {
		if (verbose) {
			/* print additional file information */
			printf("%s ", fix_perm(tmp->ar_mode));
			printf("%s/",
			       fix_str(tmp->ar_uid, sizeof(tmp->ar_uid),
				       false));
			printf("%s ",
			       fix_str(tmp->ar_gid, sizeof(tmp->ar_gid),
				       false));
			printf("%6s ", fix_str(tmp->ar_size, sizeof(tmp->ar_size),
				       false));
			printf("%s ", fix_time(tmp->ar_date));
		}
		//printf ("-%s-", (char *)tmp);
		/* always print file names */
		printf("%s\n",
		       fix_str(tmp->ar_name, sizeof(tmp->ar_name), true));
	}
	free(tmp);

	return (true);
}

bool timeout_add(int fd, time_t timeout)
{
	assert((fd >= 0));

	return (false);
}

bool append_all(int fd, char *self)
{
	DIR *cur_dir;
	struct dirent *entry;

	assert((fd >= 0) && (self != NULL));

	errno = 0;
	cur_dir = opendir(".");

	if (cur_dir == NULL) {
		return (false);
	}
	while ((entry = readdir(cur_dir)) != NULL) {
		if ((entry->d_type == DT_REG)
		  &&(strcmp(entry->d_name, self) != 0)) {
			append_file(fd, entry->d_name);
		}
	}

	return (true);
}

bool interpret_and_call(int fd, char key, int cnt, char **args)
{
	bool ret = false;

	assert((fd >= 0) && (cnt > 0) && (args != NULL));

	switch (key) {
	/* quickly append named files to archive */
	case 'q':
		if (cnt >= 3) {
			ret = true;
			for (int i = 3; i < cnt; ++i) {
			ret &= append_file(fd, args[i]);
			}
		}
		break;
	/* extract named files from archive */
	case 'x':
		if (cnt >= 3) {
			ret = true;
			for (int i = 3; i < cnt; ++i) {
				ret &= extract_file(fd, args[i]);
			}
		}
		break;
	/* print a concise table of contents of archive */
	case 't':
		ret = print_archive(fd, false);
		break;
	/* print a verbose table of contents of archive */
	case 'v':
		ret = print_archive(fd, true);
		break;
	/* delete named files from archive */
	case 'd':
		if (cnt >= 3) {
			ret = true;
			for (int i = 3; i < cnt; ++i) {
				ret &= delete_file(fd, args[i]);
			}
		}
		break;
	/* quickly append all "regular" files in the current dir */
	case 'A':
		if (cnt == 3) {
			ret = append_all(fd, args[2]);
		}
		break;
	/* for a timeout, add all modified files to the archive */
	case 'w':
		//bonus
		if (cnt == 4) {
			ret = timeout_add(fd, (time_t) atoi(args[3]));
		}
		break;
	/* unsupported operation */
	default:
		printf("invalid option -- '%c'\n", key);
		ret = false;
		break;
	}

	return (ret);
}

bool check_args (int argc, char **argv) {

	assert((argc > 0) && (argv != NULL));

	/* Syntax "myar key afile name ..." where afile=archive, key=opt */
	if (argc < 3) {
		printf("%d: Missing at least one argument\n", argc);
		return (false);
	}
	if (strlen(argv[1]) != 2) {
		printf("%s: Incorrectly sized key/option\n", argv[1]);
		return (false);
	}
	if (argv[1][0] != '-') {
		printf("%s: Invalid key (missing '-'?)\n", argv[1]);
		return (false);
	}
	for (int i = 3; i < argc; ++i) {
		if (strlen(argv[i]) > 15) {
			printf("%s: Filename is too long.", argv[i]);
			return (false);
		}
	}

	return (true);
}

int main(int argc, char **argv)
{
	char *archive;
	int fd;
	char key;

	/* check input argument validity */
	if (!check_args (argc, argv)) {
		return (EXIT_FAILURE);
	}
	/* operation key */
	key = argv[1][1];
	/* archive should be second argument */
	archive = argv[2];
	/* open archive */
	fd = open(archive, O_RDWR);
	if (fd == -1) {
		if (key == 'q') {
			fd = create_archive(archive);
			printf("%s: Creating archive\n", archive);
		} else {
			printf("%s: Error opening archive\n", archive);
			return (EXIT_FAILURE);
		}
	}
	/* check archive */
	if (!check_isarchive(fd)) {
		/* puke otherwise... */
		printf("%s: Malformed archive\n", archive);
		return (EXIT_FAILURE);
	}
	/* Acquire lock */
	/* handle function calls */
	if (interpret_and_call(fd, key, argc, argv) == false) {
		printf("-%c: Error occurred during execution\n", key);
	}
	/* Unlock file */
	errno = 0;
	if (close(fd) == -1) {
		printf("%d: Error on archive close, data loss?\n", errno);
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
