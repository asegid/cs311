#define BLOCK_SIZE 1024		// bytes
	off_t file_start;
	free (tmp);
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

			/* print additional file information */
			printf("%s ",
			printf("%6s ", fix_str(tmp->ar_size, sizeof(tmp->ar_size),
		/* always print file names */
bool check_args (int argc, char **argv) {

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

	/* check input argument validity */
	if (!check_args (argc, argv)) {
		return (EXIT_FAILURE);
	}
		} else {
			printf("%s: Error opening archive\n", archive);
			return (EXIT_FAILURE);
		printf("%s: Malformed archive\n", archive);
		printf("-%c: Error occurred during execution\n", key);
		printf("%d: Error on archive close, data loss?\n", errno);
