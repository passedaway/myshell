/*
 * =====================================================================================
 *
 *       Filename:  ls.c
 *
 *    Description:  realize ls command
 *
 *        Version:  1.0
 *        Created:  2012年11月27日 10时39分13秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include "utils.h"

static int print_file_with_color(char *name, mode_t mode, char *linkname)
{
	char *pstr = NULL, *nstr=NULL, *exstr=NULL ;
	nstr = "\033[0m";
	if( S_ISDIR(mode) )
	{
		/*  bule  */
		pstr = "\033[34;1m";
	}else if (S_ISLNK(mode) ){
		/* color same with commenting */
		pstr = "\033[36;1m";
		if(linkname){
			nstr = "\033[0m -> ";
			exstr = linkname;
		}
	}else{
		/* can execuble , green */
		if ( mode & S_IXUSR )
			pstr = "\033[32;1m";
		else
			nstr=NULL;
	}

	printf("%s%s%s%s",pstr?pstr:"", name, nstr?nstr:"", exstr?exstr:"");
	return 0;
}

static int print_file_info(char *fullname, char *name, int flag)
{
	struct stat fstat;
	struct passwd *pwd = NULL;
	struct group *grp = NULL;
	char mode[11] = {0};
	char *link_name = NULL;
	struct tm* ltm = NULL;

	if ( (name == NULL)  && (fullname == NULL) )
		return -1;
	if( name == NULL )
		name = fullname;
	if( fullname == NULL )
		fullname = name;

	if( lstat(fullname, &fstat) < 0 )
	{
		printf("%s : %s\n", fullname, strerror(errno));
		return -1;
	}

	if( !(flag&0x02) )
	{
		print_file_with_color(name, fstat.st_mode, link_name);
		return 0;
	}
	
	/*  check file type */
	if( S_ISDIR(fstat.st_mode) )
		mode[0] = 'd';
	else if( S_ISREG(fstat.st_mode) )
		mode[0] = '-';
	else if( S_ISCHR(fstat.st_mode) )
		mode[0] = 'c';
	else if( S_ISBLK(fstat.st_mode) )
		mode[0] = 'b';
	else if( S_ISFIFO(fstat.st_mode) )
		mode[0] = 'f';
	else if( S_ISLNK(fstat.st_mode) )
		mode[0] = 'l';
	else if( S_ISSOCK(fstat.st_mode) )
		mode[0] = 's';
	else
		mode[0] = '*';

	/* link mode, get the link file name*/
	if( mode[0] == 'l' )
	{
		link_name = (char *)malloc(_POSIX_PATH_MAX + 1);
		if( link_name == NULL )
			return -1;
		memset(link_name, 0, _POSIX_PATH_MAX+1);
		readlink(fullname, link_name, _POSIX_PATH_MAX);
	}

	/*  check file permission */
	if( S_IRUSR & fstat.st_mode )
		mode[1] = 'r';
	else
		mode[1] = '-';

	if( S_IWUSR & fstat.st_mode )
		mode[2] = 'w';
	else
		mode[2] = '-';

	if( S_IXUSR & fstat.st_mode )
		mode[3] = 'x';
	else
		mode[3] = '-';

	if( S_IRGRP & fstat.st_mode )
		mode[4] = 'r';
	else
		mode[4] = '-';

	if( S_IWGRP & fstat.st_mode )
		mode[5] = 'w';
	else
		mode[5] = '-';

	if( S_IXGRP & fstat.st_mode )
		mode[6] = 'x';
	else
		mode[6] = '-';

	if( S_IROTH & fstat.st_mode )
		mode[7] = 'r';
	else
		mode[7] = '-';

	if( S_IWOTH & fstat.st_mode )
		mode[8] = 'w';
	else
		mode[8] = '-';

	if( S_IXOTH & fstat.st_mode )
		mode[9] = 'x';
	else
		mode[9] = '-';

	/* get user name, pwd->pw_name */
	pwd = getpwuid(fstat.st_uid);
	if( pwd == NULL )
	{
		perror("get username by uid");
	}

	/* get group name , grp->gr_name*/
	grp = getgrgid(fstat.st_gid);
	if( grp == NULL )
	{
		perror("get group name by gid\n");
	}

	/* time */

	printf("%s %4d %8lu %8s %8s ", mode, fstat.st_nlink, fstat.st_size,
			pwd?pwd->pw_name:"NULL", grp?grp->gr_name:"NULL" ); 

	ltm = localtime(&fstat.st_ctime);
	if( ltm )
	{
		printf("%4d-%02d-%02d %02d:%02d ", ltm->tm_year+1900, ltm->tm_mon+1, 
				ltm->tm_mday, ltm->tm_hour+1, ltm->tm_min);

	}

	print_file_with_color(name, fstat.st_mode, link_name);
	printf("\n");

	/*  this is for debug */
//	sleep(1);
	return 0;
}

int do_ls(int argc, char **argv)
{
	char *dir_name = argv[1];
	char *fullpath = NULL, *ptr;
	int len = 0;
	DIR *dir = NULL;
	struct dirent *pdirent;
	struct stat fstat;
	int is_dir = 0;

	char *opts = "-als";
	int flag;
	int ret = 0, i;

	while( -1 != (ret=mygetopt(argc, argv, opts)) )
	{
		printf("ret = %d\n", ret);
		switch( ret )
		{
			case 'a':
				flag |= 1;
				break;
			case 'l':
				flag |= 1<<1;
				break;
			case 's':
				flag |= 1<<2;
				break;
			default:
				break;
		}

	}
	
	/*  find list dirname or filename */
	i = 1;
	while( (i<argc) && (argv[i][0] == '-') )
		i++;
	
	if( i == argc )
		dir_name = ".";
	else
		dir_name = argv[i];

	if ( lstat(dir_name, &fstat) < 0 )
		goto err;

	if( S_ISDIR(fstat.st_mode) )
	{
		is_dir = 1;
	}

	if( is_dir )
	{
		fullpath = (char *)malloc(_POSIX_PATH_MAX+1);
		if( fullpath == NULL )
		{
			goto err;
		}
		strcpy(fullpath, dir_name);
		len = strlen(dir_name);
		fullpath[len] = 0;
		ptr = fullpath+len;
		if( fullpath[len-1] !='/')
		{
			fullpath[len] = '/';
			ptr++;
		}

		dir = opendir(dir_name);
		if( dir == NULL )
			goto err;
		while( NULL != (pdirent = readdir(dir) ) )
		{
#if 0
			printf("%s\n", pdirent->d_name);
#else
			if( (0 == strcmp(pdirent->d_name, ".") )
					|| (0 == strcmp(pdirent->d_name, "..") ) )
				continue;

			strcpy(ptr, pdirent->d_name);
//			printf("pdirent->d_name:%s fullpath=%s\n", pdirent->d_name, fullpath);
			//print_file_info(pdirent->d_name);
			print_file_info(fullpath, pdirent->d_name, flag);
			if( !(flag&0x02) )
				printf("    \t");
#endif
		}
	}else{
		print_file_info(dir_name, NULL, flag);
	}

	if( !(flag & 0x02) )
		printf("\n");


	if( dir )
		closedir(dir);
	return 0;
err:
	printf("ls %s: %s\n", dir_name, strerror(errno));
	return -1;
}


