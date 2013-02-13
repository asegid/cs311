/*
 * 
 *      Author:  Jordan Bayles (baylesj), baylesj@engr.orst.edu
 *     Created:  02/11/2013 12:16:40 AM
 *    Filename:  uniqify.c
 *
 * Description:  Implementation of functionality similar to ``uniq''
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct pipes {
	int in[2];
	int out[2];
};

int find_next(int num, char **words)
{
	return (-1);
}

void open_pipe(int fildes[2])
{
	if (pipe(fildes) != 0) {
		perror("pipe");
		//exit(-1);
	}
}

void close_fd(int fd)
{
	if (close(fd) == -1) {
		perror("close");
	}
}

int run_parser(int num, struct pipes *p)
{
	return (-1);
}

int run_sorts(int num, struct pipes *p)
{
	/* Assumes pipes are already set up */
	execlp("sort", "sort", (char *)NULL);
	return (-1);
}

int run_suppressor(int num, struct pipes *p)
{
	return (-1);
}

int spawn_process(int num, struct pipes *p, int (*torun)(int, struct pipes *))
{
	pid_t child_pid;

	for (int i = 0; i < num; ++i) {
		if (p != NULL) {
			open_pipe(p[i].in);
			open_pipe(p[i].out);
		}

		switch (child_pid = fork()) {
		case -1:
			/* err */
			perror("fork");
			break;
		case 0:
			if (p != NULL) {
				close_fd(p[i].out[0]);
				close_fd(p[i].in[1]);
			
			}
			
			(*torun)(num, p);
			break;
		default:
			if (p != NULL) {
				close_fd(p[i].out[1]);
				close_fd(p[i].in[0]);
			}
		}
	}

	return (-1);
}

int wait_children(int num)
{
	return (-1);
}

int main(int argc, char **argv)
{
	/* Check input and assign appropriately */
	int sort_num;
	//pid_t *procs;
	//struct pipes *p;

	/* Check and assign input */
	if (argc != 2) {
		printf("%d: Incorrect argument count\n", argc);
		exit(-1);
	}
	sort_num = atoi(argv[1]);
	if (sort_num <= 0) {
		printf("%d: Invalid number of sort processes", sort_num);
	}

	/* Fill memory */
	//procs = malloc (sizeof(pid_t) * sort_num);
	//p = malloc (sizeof(struct pipes) * sort_num);
	pid_t procs[sort_num];
	struct pipes p[sort_num];

	/** Run the parser */
	//spawn_process(1, NULL, &run_parser);
	run_parser(sort_num, p);

	/** Run the sorters */
	spawn_process(sort_num, p, &run_sorts);

	/** Run the suppressor */
	spawn_process(1, NULL, &run_suppressor);

	/* Free memory */
	//free(procs);
	//free(p);

	/* return exit status */
	return (-1);
}
