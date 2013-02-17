/*
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Length picked for longest word ever to appear in literature */
#define MAX_STR_LEN 184

struct pipes {
	int in[2];
	int out[2];
};

/*
 * procs must be global to use with a signal handler, must be pointer as there
 * is no guaranteed limit on the max number of processes.
 */
pid_t *procs;

/* Function Prototypes */
void close_fd(int fd);
void close_pipes(int num, struct pipes *p);
int find_next(int num, char words[][MAX_STR_LEN]);
int main(int argc, char **argv);
void open_pipes(int num, struct pipes *p);
void sig_handle(int signo);
int spawn_sorts(int num, struct pipes *p);
int spawn_suppressor(int num, struct pipes *p);
void str_tolower(char *str, int len);
void str_rm_newline(char *str, int len);
void run_parser(int num, struct pipes *p);
void run_sort(int num, struct pipes *p);
void run_suppressor(int num, struct pipes *p);
int wait_children(int num);

int find_next(int num, char words[][MAX_STR_LEN])
{
	int idx = 0;

	for (int i = 0; i < num; ++i) {
		if (strcmp(words[i], words[idx]) < 0)
			idx = i;
	}

	return (idx);
}

void open_pipes(int num, struct pipes *p)
{
	for (int i = 0; i < num; ++i) {
		if ((pipe(p[i].in) != 0) || (pipe(p[i].out) != 0)) {
			perror("pipe");
		}
	}
}

void close_pipes(int num, struct pipes *p)
{
	for (int i = 0; i < num; ++i) {
		close(p[i].in[0]);
		close(p[i].in[1]);
		close(p[i].out[0]);
		close(p[i].out[1]);
	}
}

void close_fd(int fd)
{
	if (close(fd) == -1)
		perror("close");
}

void run_parser(int num, struct pipes *p)
{
	char cur[MAX_STR_LEN];
	int cur_sorter = 0;
	int ret;
	FILE *write_ends[num];

	/* Open pipes for writing */
	for (int i = 0; i < num; ++i) {
		if ((write_ends[i] = fdopen(p[i].out[1], "w")) == NULL) {
			perror("fdopen");
		}
	}

	/* Parse and distribute */
	do {
		ret = scanf("%[a-zA-Z]", cur);
		if (ret == 1) {
			str_tolower(cur, strlen(cur));
			fputs(cur, write_ends[cur_sorter]);
			cur_sorter = (cur_sorter + 1) % num;
		} else {
			/* need to swallow junk apparently */
		}
			ret = scanf("%*[^a-zA-Z]");
	} while (ret != EOF);

	printf("done parsing\n");
	/* Done parsing, so flush */
	for (int i = 0; i < num; ++i)
		fclose(write_ends[i]);

}

void run_sort(int num, struct pipes *p)
{
	int ret;

	/* Assumes pipes are already set up */
	if ((ret = execlp("sort", "sort", (char *) NULL)) == -1)
		perror("execlp");

	/* call _exit() instead of exit() for children of fork */
	perror("done sorting");
	_exit(EXIT_SUCCESS);
}

void run_suppressor(int num, struct pipes *p)
{
	char cur[MAX_STR_LEN];
	int empty_cnt = 0;
	int first_word;
	int next;
	int occur_cnt;
	FILE *read_ends[num];
	char words[num][MAX_STR_LEN];

	/* Set up and init read pipes */
	for (int i = 0; i < num; ++i) {
		read_ends[i] = fdopen(p[i].in[0], "r");
		fgets(words[i], MAX_STR_LEN, read_ends[i]);
	}

	/* Setup for first word */
	first_word = find_next(num, words);
	strncpy(cur, words[first_word], MAX_STR_LEN);
	str_rm_newline(cur, MAX_STR_LEN);
	occur_cnt = 1;

	while (empty_cnt <= num) {
		if (fgets(words[next], MAX_STR_LEN, read_ends[next]) == NULL) {
			memset(words[next], 0, MAX_STR_LEN);
			++empty_cnt;
		}
		next = find_next(num, words);
		/* Increment count while on same word, otherwise handle end */
		if ((next >= 0) && (strcmp(cur, words[next]) == 0)) {
			++occur_cnt;
		} else {
			printf("%s : %d\n", cur, occur_cnt);
			if (next >= 0) {
				strncpy(cur, words[next], MAX_STR_LEN);
				str_rm_newline(cur, MAX_STR_LEN);
				occur_cnt = 1;
			}
		}
	}

	for (int i = 0; i < num; ++i)
		fclose(read_ends[i]);

	/* call _exit() instead of exit() for children of fork */
	_exit(EXIT_SUCCESS);
}

void sig_handle(int signo)
{
	if (procs != NULL) {

		free(procs);
	}

	exit(1);
}

int spawn_sorts(int num, struct pipes *p)
{
	pid_t child_pid;

	for (int i = 0; i < num; ++i) {
		switch (child_pid = fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			if (p != NULL) {
				close(STDIN_FILENO);
				if (dup2(p[i].in[0], STDIN_FILENO) == -1) {
					perror("dup2");
				}
				close(STDOUT_FILENO);
				if (dup2(p[i].out[1], STDOUT_FILENO) == -1) {
					perror("dup2");
				}
				close_fd(p[i].out[0]);
				close_fd(p[i].in[1]);
			}
			run_sort(num, p);
			break;
		default:
			if (p != NULL) {
				close_fd(p[i].out[1]);
				close_fd(p[i].in[0]);
			}
			break;
		}
	}
	return (EXIT_SUCCESS);
}


int spawn_suppressor(int num, struct pipes *p)
{
	pid_t child_pid;

	for (int i = 0; i < num; ++i) {
		switch (child_pid = fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			run_suppressor(num, p);
			break;
		default:
			break;
		}
	}
	return (EXIT_SUCCESS);
}

void str_tolower(char *str, int len)
{
	for (int i = 0; i < len; ++i)
		str[i] = tolower(str[i]);
}

void str_rm_newline(char *str, int len)
{
	if (str[len-1] == '\n')
		str[len-1] = '\0';
}

int wait_children(int num)
{
	for (int i = 0; i < num; ++i) {
		wait(NULL);
	}

	return (EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int sort_num;

	/* Check input and assign appropriately */
	if (argc != 2) {
		printf("%d: Incorrect argument count\n", argc);
		exit(EXIT_FAILURE);
	}
	sort_num = atoi(argv[1]);
	if (sort_num <= 0) {
		printf("%d: Invalid number of sort processes", sort_num);
		exit(EXIT_FAILURE);
	}

	/* Fill memory */
	struct pipes p[sort_num];
	open_pipes(sort_num, p);

	/* Install a signal handler */
	// INTR, QUIT, HANGUP
	// sigaction
	// signal handler
	// QUIT to children

	/** Run the sorters, get them ready for input */
	spawn_process(sort_num, p, &run_sorts);

	/** Run the parser */
	run_parser(sort_num, p);

	/** Run the suppressor */
	//spawn_process(1, NULL, &run_suppressor);
	run_suppressor(sort_num, p);

	printf("waiting\n");
	/* Cleanup */
	wait_children(sort_num + 1);
	close_pipes(sort_num, p);

	/* return exit status */
	printf("exiting\n");
	return (EXIT_SUCCESS);
}
