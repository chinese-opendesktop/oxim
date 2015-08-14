/*
    Copyright (C) 2006 by OXIM TEAM

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/      

#ifdef HPUX
#  define _INCLUDE_POSIX_SOURCE
#endif
#include "oximint.h"

int oxim_check_file_exist(const char *path, const int type)
{
    struct stat buf;

    if (stat(path, &buf) != 0)
	return  False;

    if (type == FTYPE_FILE)
	return  (S_ISREG(buf.st_mode)) ? True : False;
    else if (type == FTYPE_DIR)
	return  (S_ISDIR(buf.st_mode)) ? True : False;
    else
	return  False;
}

/*----------------------------------------------------------------------------

	OXIM data file checking. 
 
----------------------------------------------------------------------------*/

#define return_truefn(true_fn, fn, true_fnsize)				\
{									\
    if (true_fn && true_fnsize > 0)					\
	strncpy(true_fn, fn, true_fnsize);				\
}

int oxim_check_datafile(char *fn, char *sub_path, char *true_fn, int true_fnsize)
{
    char *user_dir = _Config->user_dir;
    char *default_dir = _Config->default_dir;
    char path[BUFFER_LEN], subp[BUFFER_LEN], *s;
/*
 *  For the obsulated path, try to open it directly.
 */
    if (fn[0] == '/') {
	int ret = oxim_check_file_exist(fn, FTYPE_FILE);
	if (ret == True)
	    return_truefn(true_fn, fn, true_fnsize);
	return ret;
    }
/*
 *  Otherwise, search it in user data trees.
 */
    if (sub_path)
	strncpy(subp, sub_path, BUFFER_LEN);
    else
	subp[0] = '\0';

    if (user_dir) {
	while (subp[0] != '\0') {
	    sprintf(path, "%s/%s/%s", user_dir, subp, fn);
	    if (oxim_check_file_exist(path, FTYPE_FILE) == True) {
		return_truefn(true_fn, path, true_fnsize);
		return True;
	    }
	    if ((s = strrchr(subp, '/')) != NULL)
		*s = '\0';
	    else
		subp[0] = '\0';
	}
	sprintf(path, "%s/%s", user_dir, fn);
	if (oxim_check_file_exist(path, FTYPE_FILE) == True) {
	    return_truefn(true_fn, path, true_fnsize);
	    return True;
	}
    }
/*
 *  Otherwise, search it in oxim data trees.
 */
    if (sub_path)
	strncpy(subp, sub_path, BUFFER_LEN);
    else
	subp[0] = '\0';

    while (subp[0] != '\0') {
	sprintf(path, "%s/%s/%s", default_dir, subp, fn);
	if (oxim_check_file_exist(path, FTYPE_FILE) == True) {
	    return_truefn(true_fn, path, true_fnsize);
	    return True;
	}
	if ((s = strrchr(subp, '/')) != NULL)
	    *s = '\0';
	else
	    subp[0] = '\0';
    }
    sprintf(path, "%s/%s", default_dir, fn);
    if (oxim_check_file_exist(path, FTYPE_FILE))
    {
	return_truefn(true_fn, path, true_fnsize);
	return True;
    }

    return False;
}

int oxim_check_cmd_exist(char *cmd)
{
	struct stat buf;
	char paths[BUFFER_LEN];
	char delims[] = ":";
	char *path = NULL;
	strcpy(paths, getenv("PATH"));

	for( path=strtok(paths, delims); path!=NULL; path=strtok(NULL, delims) )
	{
		char program[BUFFER_LEN];
		sprintf(program, "%s/%s\0", path, cmd);
		if( (0==stat(program, &buf)) && (S_ISREG(buf.st_mode)) && (buf.st_mode&S_IXOTH) )
		{
			return True;
		}
	}
	return False;
}
