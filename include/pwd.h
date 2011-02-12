#ifndef _PWD_H
#define _PWD_H

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_size_t
#define __NEED_uid_t
#define __NEED_gid_t

#include <bits/alltypes.h>

struct passwd
{
	char *pw_name;
	char *pw_passwd;
	uid_t pw_uid;
	gid_t pw_gid;
	char *pw_gecos;
	char *pw_dir;
	char *pw_shell;
};

void setpwent (void);
void endpwent (void);
struct passwd *getpwent (void);

struct passwd *getpwuid (uid_t);
struct passwd *getpwnam (const char *);
int getpwuid_r (uid_t, struct passwd *, char *, size_t, struct passwd **);
int getpwnam_r (const char *, struct passwd *, char *, size_t, struct passwd **);

#ifdef __cplusplus
}
#endif

#endif
