#ifndef _STDIO_H
#define _STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_FILE
#define __NEED_va_list
#define __NEED_size_t

#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
 || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE)
#define __NEED_ssize_t
#define __NEED_off_t
#endif

#include <bits/alltypes.h>

#undef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif

#undef EOF
#define EOF (-1)

#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 1024
#define FILENAME_MAX 4095
#define FOPEN_MAX 1000
#define TMP_MAX 10000
#define L_tmpnam 20

typedef union {
	char __opaque[16];
	double __align;
} fpos_t;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;

#define stdin  (stdin)
#define stdout (stdout)
#define stderr (stderr)

FILE *fopen(const char *, const char *);
FILE *freopen(const char *, const char *, FILE *);
int fclose(FILE *);

int remove(const char *);
int rename(const char *, const char *);

int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
void clearerr(FILE *);

int fseek(FILE *, long, int);
long ftell(FILE *);
void rewind(FILE *);

int fgetpos(FILE *, fpos_t *);
int fsetpos(FILE *, const fpos_t *);

size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);

int fgetc(FILE *);
int getc(FILE *);
int getchar(void);
int ungetc(int, FILE *);

int fputc(int, FILE *);
int putc(int, FILE *);
int putchar(int);

char *fgets(char *, int, FILE *);
char *gets(char *);

int fputs(const char *, FILE *);
int puts(const char *);

int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int sprintf(char *, const char *, ...);
int snprintf(char *, size_t, const char *, ...);

int vprintf(const char *, va_list);
int vfprintf(FILE *, const char *, va_list);
int vsprintf(char *, const char *, va_list);
int vsnprintf(char *, size_t, const char *, va_list);

int scanf(const char *, ...);
int fscanf(FILE *, const char *, ...);
int sscanf(const char *, const char *, ...);
int vscanf(const char *, va_list);
int vfscanf(FILE *, const char *, va_list);
int vsscanf(const char *, const char *, va_list);

void perror(const char *);

int setvbuf(FILE *, char *, int, size_t);
void setbuf(FILE *, char *);

char *tmpnam(char *);
FILE *tmpfile(void);

#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
 || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE)
FILE *fdopen(int, const char *);
FILE *popen(const char *, const char *);
int pclose(FILE *);
int fileno(FILE *);
int fseeko(FILE *, off_t, int);
off_t ftello(FILE *);
int dprintf(int, const char *, ...);
int vdprintf(int, const char *, va_list);
void flockfile(FILE *);
int ftrylockfile(FILE *);
void funlockfile(FILE *);
int getc_unlocked(FILE *);
int getchar_unlocked(void);
int putc_unlocked(int, FILE *);
int putchar_unlocked(int);
ssize_t getdelim(char **, size_t *, int, FILE *);
ssize_t getline(char **, size_t *, FILE *);
int renameat(int, const char *, int, const char *);
char *ctermid(char *);
#define L_ctermid 20
#endif


#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE)
#define P_tmpdir "/tmp"
char *tempnam(const char *, const char *);
#endif

#if defined(_GNU_SOURCE)
#define L_cuserid 20
char *cuserid(char *);
#undef off64_t
#define off64_t off_t
int asprintf(char **, const char *, ...);
int vasprintf(char **, const char *, va_list);
void setlinebuf(FILE *);
void setbuffer(FILE *, char *, size_t);
#endif

#ifdef __cplusplus
}
#endif

#endif
