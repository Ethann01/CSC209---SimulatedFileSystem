#include <stdio.h>
#include "simfstypes.h"

/* File system operations */
void printfs(char *);
void initfs(char *);
void createfile(char *, char*);
void deletefile(char *, char*);
void writefile(char *, char *, int start, int length);
void readfile(char *, char *, int start, int length);
/* Internal functions */
FILE *openfs(char *filename, char *mode);
void closefs(FILE *fp);