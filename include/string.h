#ifndef	_STRING_H
#define	_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#undef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif

#define __NEED_size_t
#include <bits/alltypes.h>

void *memcpy (void *, const void *, size_t);
void *memmove (void *, const void *, size_t);
void *memset (void *, int, size_t);
int memcmp (const void *, const void *, size_t);
void *memchr (const void *, int, size_t);

char *strcpy (char *, const char *);
char *strncpy (char *, const char *, size_t);

char *strcat (char *, const char *);
char *strncat (char *, const char *, size_t);

int strcmp (const char *, const char *);
int strncmp (const char *, const char *, size_t);

int strcoll (const char *, const char *);
size_t strxfrm (char *, const char *, size_t);

char *strchr (const char *, int);
char *strrchr (const char *, int);

size_t strcspn (const char *, const char *);
size_t strspn (const char *, const char *);
char *strpbrk (const char *, const char *);
char *strstr (const char *, const char *);
char *strtok (char *, const char *);

size_t strlen (const char *);

char *strerror (int);


#if defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE) \
 || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE)
char *strtok_r (char *, const char *, char **);
int strerror_r (int, char *, size_t);
char *stpcpy(char *, const char *);
char *stpncpy(char *, const char *, size_t);
size_t strnlen (const char *, size_t);
char *strdup (const char *);
char *strndup (const char *, size_t);
char *strsignal(int);
#endif

#if defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE)
void *memccpy (void *, const void *, int, size_t);
#endif

#ifdef _BSD_SOURCE
size_t strlcat (char *, const char *, size_t);
size_t strlcpy (char *, const char *, size_t);
#endif

#ifdef _GNU_SOURCE
int strcasecmp (const char *, const char *);
int strncasecmp (const char *, const char *, size_t);
char *strchrnul(const char *, int);
char *strcasestr(const char *, const char *);
char *strsep(char **, const char *);
#endif

#ifdef __cplusplus
}
#endif

#endif
