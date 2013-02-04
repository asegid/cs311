/* This file was automatically generated.  Do not edit! */
int main(int argc,char **argv);

bool check_args(int argc,char **argv);

bool interpret_and_call(int fd,char key,int cnt,char **args);

void usage(void);

bool append_all(int fd,char *self);

bool timeout_add(int fd,time_t timeout);

bool print_archive(int fd,bool verbose);

char *fix_time(char *epoch);

char *fix_perm(char *octal);

bool extract_file(int fd,char *fname);

bool delete_file(int fd,char *fname);

bool delete_bytes(int fd,off_t start,off_t end);

bool append_file(int fd,char *fname);

bool create_header(char *fname,struct ar_hdr *out);

void replace_chars(char *str,int len,char from,char to);

off_t find_header(int fd,char *fname,struct ar_hdr *out);

int create_archive(char *fname);

char *fix_str(char *str,int len,bool backslash);

off_t get_next_header(int fd,struct ar_hdr *out);

bool check_isarchive(int fd);
