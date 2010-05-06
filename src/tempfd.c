/*
 * Copyright (C) 2010 Conrad Parker
 */

/*
 * Create a temporary file; return its fd.
 *
 * This is based on advice from:
 * http://www.dwheeler.com/secure-programs/Secure-Programs-HOWTO/avoid-race.html
 * and the mkstemp() man page
 */

#include <stdio.h>
#include <stdlib.h>

static int raw_tempfd(char *pattern)
{
	int temp_fd;
	mode_t old_mode;

	old_mode = umask(077);	/* Create file with restrictive permissions */
	temp_fd = mkstemp(pattern);
	(void) umask(old_mode);
	if (temp_fd == -1) {
		perror("Couldn't open temporary file");
		exit(1);
	}
	if (unlink(pattern) == -1) {
		perror("Couldn't unlink temporary file");
		exit(1);
	}
	return temp_fd;

}

/*
 * Given a "tag" (a relative filename ending in XXXXXX),
 * create a temporary file using the tag.  The file will be created
 * in the directory specified in the environment variables
 * TMPDIR or TMP, if defined and we aren't setuid/setgid, otherwise
 * it will be created in /tmp.  Note that root (and su'd to root)
 * _will_ use TMPDIR or TMP, if defined.
 * 
 */
int create_tempfd (char *tag)
{
	char *tmpdir = NULL;
	char *pattern;
	int temp_fd;

	if ((getuid() == geteuid()) && (getgid() == getegid())) {
		if (!((tmpdir = getenv("TMPDIR")))) {
			tmpdir = getenv("TMP");
		}
	}
	if (!tmpdir) {
		tmpdir = "/tmp";
	}

	pattern = malloc(strlen(tmpdir) + strlen(tag) + 2);
	if (!pattern) {
		perror ("Could not malloc tempfile pattern");
		exit (1);
	}

	strcpy(pattern, tmpdir);
	strcat(pattern, "/");
	strcat(pattern, tag);

	temp_fd = raw_tempfd (pattern);

	free(pattern);

	return temp_fd;
}
