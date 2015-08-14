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

#include <glob.h>
#include "oximint.h"

// static int filter_current = 0;
char **filters = NULL;
int total_filter_num;

#define FILTER_PREFIX "/filters/*"
#define DEFAULT_PATTERN "default"
#define IS_EXEC(buf, command) (0==stat((command), &buf)) && (S_ISREG(buf.st_mode)) && (buf.st_mode&S_IXOTH)

int get_filter_num()
{
    return total_filter_num;
}

void find_filter()
{
    glob_t globbuf;
    struct stat buf;
    int i, a, c = 0, total = 0;
    int filter_num = 0;
    char *dirs[] = {oxim_sys_default_dir(), oxim_user_dir(), NULL};
    globbuf.gl_offs = 2;
    
    filters = (char **)oxim_malloc(BUFFER_LEN * sizeof(char*), False);
    strcpy((char *)&filters[c], DEFAULT_PATTERN);
    c+=BUFFER_LEN;
    
    total_filter_num = 0;
    for(a = 0, total = 2; dirs[a]!=NULL; a++)
    {
		char current_dir[BUFFER_LEN];

		strcpy(current_dir, dirs[a]);
		strcat(current_dir, FILTER_PREFIX);

		int ret = glob(current_dir, GLOB_DOOFFS, NULL, &globbuf);
		if( 0 != ret )
		{
			continue;
		}

		//     char filters[globbuf.gl_pathc][BUFFER_LEN];
		filter_num += globbuf.gl_pathc;
	/*	if(!filters)
			filters = (char **)oxim_malloc(filter_num * BUFFER_LEN * sizeof(char*), True);
		else*/
	// 	    filters = (char **)oxim_realloc(filters, filter_num * BUFFER_LEN * sizeof(char*) + sizeof(char*));
	
		for(i=0; i<=filter_num+1; i++)
		{
			char *command = globbuf.gl_pathv[i];
			if( NULL != command && IS_EXEC(buf, command) )
			{
		// 		char cmd[BUFFER_LEN];
				
		// 		strcpy(cmd, command);
		// 		strcat(cmd, " 2>/dev/null ");
		// 		if( 0 == system(cmd) )
				{
					filters = (char **)oxim_realloc(filters, total * BUFFER_LEN * sizeof(char*));
			// 		strcpy(filters+c, command);
					strcpy((char *)&filters[c], command);
					c+=BUFFER_LEN;
					total++;
					total_filter_num++;
				}
			}
		}
    }
    /*total_filter_num = filter_num;*/
    total_filter_num++;
#if 0
   printf("total=%d, sizeof filters=%d, total_filter_num=%d\n", total, sizeof(**filters), total_filter_num); 
    for(i=0; i<=filter_num*BUFFER_LEN && *(filters+i) != '\0'; i+=BUFFER_LEN)
    {
	puts((char *)&filters[i]);
    }
    puts("=========");
#endif
}

void reload_filter()
{
//    free(filters);
//    filters = NULL;
    memset(&filters, '\0', sizeof(filters));
    //filter_current = 0;
    find_filter();
}

#if 1
/*
 * next: 0 - return current filter name
 *       1 - change to next filter and return filter name
 *      -1 - change to previous filter and return filter name
 * check: checkout executable of the filter.
 * current: pointer to current index.
 */
char** change_filter(int next, int check, int *current)
{
    int index=0;
    char **p;
    struct stat buf;

    if( NULL == filters )
		find_filter();

    if(!next)
    {
	// 	strcpy(cmd, (char *)(filters + (filter_current-1 >0 ? filter_current-1 : 0) * BUFFER_LEN) );
		p = &filters[ ((*current)-1 >0 ? (*current)-1 : 0) * BUFFER_LEN];
    }
    else
    {
		if(next>-1)
		{
			/*if( *(filters + (*current) * BUFFER_LEN) != '\0' )*/
			if(*current < total_filter_num)
				index = (*current)++;
			else
				*current = 1;
		}
		else
		{
			(*current)+=next;
			if(*current<0)
				*current = total_filter_num;
			index = *current;
		}
		
		//     strcpy(cmd, (char *)(filters + index*BUFFER_LEN) );
			p = &filters[index*BUFFER_LEN];
    }

    if(check)
    {
      if( !is_filter_default(current) )
		if( IS_EXEC(buf, (char *)p) )
		{
		  if(0 != system((char *)p) )
			return change_filter(1, False, current);
		}
		else
			return change_filter(1, False, current);
	}

    return p;
}
#endif

#if 0
char** change_filter(int next, int check)
{
    int index=0;
    char cmd[BUFFER_LEN];
    
    if( NULL == filters )
	find_filter();
    
    if(!next)
	return (filters + (filter_current-1 >0 ? filter_current-1 : 0) * BUFFER_LEN);
    
    if( *(filters+filter_current*BUFFER_LEN) != '\0' )
	index = filter_current++;
    else
	filter_current = 1;
    
    return (filters + index*BUFFER_LEN);
    
}
#endif

/*
 * check if filter index is at first position.
 */
int is_filter_default(int *current)
{
    return (0 == strncmp((char *)change_filter(0, False, current), DEFAULT_PATTERN, strlen(DEFAULT_PATTERN)));
}
