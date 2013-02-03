#include <dirent.h>
#define BLOCK_SIZE 8		// bytes
bool check_isarchive(int fd)
	if (strncmp(buf, ARMAG, sizeof(buf)) != 0) {
		return (false);
	}
 * assumes file offset only edited by self and check_isarchive()
		/* advance past content */
		if ((lseek(fd, atoi(out->ar_size), SEEK_CUR) % 2) == 1) {
			/* ensure an even byte boundary */
			lseek(fd, 1, SEEK_CUR);
		}
	int fd;
	ssize_t ret;

	fd = open (fname, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (fd != -1) {
	ret = write(fd, ARMAG, SARMAG);
		if (ret != SARMAG) {
			fd = -1;
		}
	}
	snprintf(out->ar_mode, sizeof(out->ar_mode), "%llo",
			(long long unsigned int)(buf.st_mode));
	} else {
		return (false);
	/* copy contents */
	/* catch errors */
		lseek(fd, cur, SEEK_SET);
	    file_start + (off_t) (sizeof(struct ar_hdr) + atoll(tmp->ar_size));
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
			printf("%s ", fix_str(tmp->ar_size, sizeof(tmp->ar_size),
bool append_all(int fd, char *self)
	DIR *cur_dir;
	struct dirent *entry;

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
bool interpret_and_call(int fd, char key, int cnt, char **args)
	bool ret = false;

	/* quickly append named files to archive */
	case 'q':
		if (cnt >= 3) {
			ret = true;
			for (int i = 3; i < cnt; ++i) {
			ret &= append_file(fd, args[i]);
			}
	/* extract named files from archive */
	case 'x':
		if (cnt >= 3) {
			ret = true;
			for (int i = 3; i < cnt; ++i) {
				ret &= extract_file(fd, args[i]);
			}
	/* print a concise table of contents of archive */
	case 't':
		ret = print_archive(fd, false);
	/* print a verbose table of contents of archive */
	case 'v':
		ret = print_archive(fd, true);
	/* delete named files from archive */
	case 'd':
		if (cnt >= 3) {
			ret = true;
			for (int i = 3; i < cnt; ++i) {
				ret &= delete_file(fd, args[i]);
			}
	/* quickly append all "regular" files in the current dir */
	case 'A':
		if (cnt == 3) {
			ret = append_all(fd, args[2]);
		}
	/* for a timeout, add all modified files to the archive */
	case 'w':
		if (cnt == 4) {
			ret = timeout_add(fd, (time_t) atoi(args[3]));
		}
	/* unsupported operation */
	default:
		ret = false;
	return (ret);
	if (fd == -1) {
		if (key == 'q') {
			fd = create_archive(archive);
			printf("%s: Creating archive\n", archive);
		}
	}
	if (!check_isarchive(fd)) {
		/* puke otherwise... */
		printf("%s: Malformed or nonexistent archive\n", archive);
		return (EXIT_FAILURE);
	if (interpret_and_call(fd, key, argc, argv) == false) {
		printf("Error occurred executing: %c", key);
	}
		printf("Error %d on archive close, data loss? \n", errno);