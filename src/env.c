/*
 * =====================================================================================
 *
 *       Filename:  env.c
 *
 *    Description:  env
 *
 *        Version:  1.0
 *        Created:  2012年12月04日 13时14分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#include "os.h"
#include "env.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define ENV_FILE_NAME	"./.env_save"

static queue_t _this_env;

static void env_delete(env_t* env)
{
	if( env )
	{
		if( env->name )
			FREE(env->name);
		
		if( env->value )
			FREE(env->value);

		FREE(env);
	}
}

static env_t *env_new_by_sp(char *name, char *value)
{
	env_t *tmp = NULL;
	int len = 0;
	
	tmp = (env_t*)MALLOC(sizeof(env_t));
	if( tmp == NULL )
	{
		printf("error: out of memory\n");
		goto err;
	}

	len = strlen(name) + 1;
	tmp->name = (char*)MALLOC(len);
	if( tmp->name == NULL )
	{
		printf("error: out of memory\n");
		goto err;
	}
	strcpy(tmp->name, name);
	tmp->name[len-1] = 0;

	len = strlen(value)+1;
	tmp->value = (char *)MALLOC(len);
	if( value== NULL )
	{
		printf("error: out of memory\n");
		goto err;
	}
	strcpy(tmp->value, value);
	tmp->value[len-1] = 0;

	return tmp;

err:
	env_delete(tmp);
	return NULL;
}

static env_t* env_new(char *line)
{
	int len, itemlen;
	int equal_pos=0, cur_pos=0;
	char *name, *value;
	env_t *tmp = NULL;
	
//	printf("line=%s\n", line);
	len = strlen(line);
	while(equal_pos < len)
	{
		if ( *(line+equal_pos) == '=' )
			break;
		equal_pos++;
	}

	if( (equal_pos == 0) || (equal_pos > len - 1) )
		return NULL;
	
	itemlen = equal_pos;
	name = (char*)MALLOC(itemlen+1);
	if( name == NULL )
	{
		printf("error: out of memory\n");
		goto err;
	}
	
	itemlen = len-equal_pos+1;
	value = (char *)MALLOC(itemlen);
	if( value == NULL )
	{
		printf("error: out of memory\n");
		goto err;
	}

	tmp = (env_t*)MALLOC(sizeof(env_t));
	if( tmp == NULL )
	{
		printf("error: out of memory\n");
		goto err;
	}

	strncpy(name, line, equal_pos);
	name[equal_pos] = 0;

	strncpy(value, line+equal_pos+1, itemlen-1);
	value[itemlen-1] = 0;

	tmp->name = name;
	tmp->value = value;
	tmp->type = ENV_TYPE_BUILD_IN;

	return tmp;
err:
	if( tmp )
		FREE(tmp);
	if( name )
		FREE(name);
	if( value )
		FREE(value);

	return NULL;
}

env_t* env_init(void)
{
	int fd = 0;
	int pos, len, ret, startpos;
	char *buf = NULL;
	env_t *tmpenv=NULL;

	q_init(&_this_env);

	/*  check file exists */
	if ( access(ENV_FILE_NAME, F_OK | W_OK ) )
	{
		printf("\n%s: file not exists. no user define & saved env\n", ENV_FILE_NAME);
		goto out;
	}

	/*  read file, then make env 
	 *  if use fread/fwrite,user can modify, but the length will be limited
	 *  if user read/write, user will not modify it ,just can use command
	 */
	fd = open(ENV_FILE_NAME, O_RDWR);
	if( fd < 0 )
	{
		printf("\nopen file %s error %s\n", ENV_FILE_NAME, strerror(errno));
		goto out;
	}
#define BUF_SIZE 2048

	buf = (char *)MALLOC(BUF_SIZE);
	if( buf == NULL )
	{
		printf("memory out\n");
		goto out;
	}
	memset(buf, 0, BUF_SIZE);

	len = BUF_SIZE;
#if 1
	//printf("will read fd=%d\n", fd);
	startpos = 0;
	while( 0 < (ret = read(fd, buf+startpos, len)) )
	{
	//	DumpData(buf, 256);
	//	printf("read ret=%d\n", ret);
		pos = 0;
		startpos = 0;
		while( pos < ret )
		{
			if(buf[pos] == 0 )
			{
				//printf("pos = %d startpos=%d\n", pos, startpos);
				tmpenv = env_new(buf+startpos);
				if( tmpenv == NULL )
				{
					printf("\nout of memory or arg invalid\n");
					goto out;
				}
				tmpenv->type = ENV_TYPE_SAVE;
				
				q_enque(&_this_env, (queue_t*)tmpenv);

				startpos = pos + 1;
			}

			pos++;
		}
		
		/*  best condition: startpos == ret, that's:last buf is complete ended */
#if 1
		if( startpos != ret )
		{
			//printf("do memmove buf=%p buf+startpos=%p len=%d\n", buf, (buf+startpos), ret-startpos);
			memmove(buf, buf+startpos, ret-startpos);
			startpos = BUF_SIZE - startpos;
			len = BUF_SIZE - startpos;
		}
#endif
	}

#endif
out:
//	printf("out ret=%d error:%s\n", ret, strerror(errno));
	if( buf )
		FREE(buf);
	if( fd )
		close(fd);

	/* init build int types */
	env_setenv("author", "zhaocq@ipanel.cn", ENV_TYPE_BUILD_IN);
	env_setenv("Email", "changqing.1230@163.com", ENV_TYPE_BUILD_IN);

	return (env_t*)&_this_env;
}

void env_exit(env_t *env)
{
	queue_t *tmp;
	if( env != (env_t *) &_this_env )
	{
		/* bug, fixme */
		BUG();
		return ;
	}

	/* env save */
	env_sync();

	q_foreach(&_this_env, tmp)
	{
		if( tmp )
		{
			q_deque(tmp);
			env_delete((env_t*)tmp);
		}
	}
}

int env_find(const char *name, char **value)
{
	queue_t *tmp;
	if( !name || !value )
		return -1;

	q_foreach(&_this_env, tmp)
	{
		if( tmp )
		{
			if( strcmp(((env_t*)tmp)->name, name) == 0 )
			{
				*value = ((env_t*)tmp)->value;
				return 0;
			}
		}
	}

	return -1;
}

int env_setenv(const char *name, const char *value, env_type_t type)
{
	env_t *tmp = NULL;

	if( !name || !value )
		return -1;

	tmp = env_new_by_sp((char *)name, (char *)value);
	if( tmp == NULL )
		return -1;

	tmp->type = type;
	
	q_enque(&_this_env, (queue_t*)tmp);
	return 0;
}

int env_delenv(const char *name)
{
	queue_t *tmp;
	if( !name )
		return -1;

	q_foreach(&_this_env, tmp)
	{
		if( tmp )
		{
			if( strcmp( ((env_t*)tmp)->name, name) == 0 )
			{
				q_deque(tmp);
				env_delete((env_t*)tmp);
				return 0;
			}
		}
	}

	return -1;
}

int env_sync()
{
	int fd = 0;
	char *buf=0, *ptr = 0;
	queue_t *tmp;
	int len = 0;

	fd = open(ENV_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if( fd < 0 )
		return -1;

	buf = (char *)MALLOC(4096);
	if( buf == NULL )
		goto OUT;
	memset(buf, 0, 4096);

	ptr = buf;
	q_foreach(&_this_env, tmp)
	{
		if(tmp)
		{
			if( ((env_t *)tmp) ->type != ENV_TYPE_SAVE )
				continue;
			len = sprintf(ptr, "%s=%s", ((env_t*)tmp)->name, ((env_t*)tmp)->value) + 1;
			ptr += len;
			if( ptr-buf > 1024*3)
			{
				write(fd, buf, ptr-buf);
				memset(buf, 0, ptr-buf);
				len = 0;
				ptr = buf;
			}
		}
	}

	if( buf != ptr )
		write(fd, buf, ptr - buf );

OUT:
	if( buf )
		FREE(buf);

	if( fd )
		close(fd);
	return 0;
}

/* env related commands */
extern int do_printenv(int argc, char **argv)
{
	queue_t *tmp = NULL;
	if( argc != 1 )
	{
		char *value = NULL;
		env_find(argv[1], &value);
		printf("%s = %s\n", argv[1], value);
		return 0;
	}


	printf("%20s  %s\n", "ENV Name", "Value");
	printf("********************  ********************************\n");
	q_foreach(&_this_env, tmp)
	{
		if( tmp )
			printf("%20s  %s\n", ((env_t*)tmp)->name, ((env_t*)tmp)->value);
	}
	printf("********************  ********************************\n");

	return 0;
}

extern int do_setenv(int argc, char **argv)
{
	int ret ;
	char *opts = "s";
	env_type_t type = ENV_TYPE_USER_DEFINE;
	char *name, *value;

	if( (argc < 3) || (argc > 4) )
	{
		goto err;	
	}

	name = argv[1];
	value = argv[2];

	while( -1 != (ret = mygetopt(argc, argv, opts)) )
	{
		if( ret == 's' )
		{
			type = ENV_TYPE_SAVE;
			if( argc < 4 )
				goto err;

			if( argv[1][0] == '-' )
			{
				name = argv[2];
				value = argv[3];
			}else if ( argv[2][0] == '-' ){
				value = argv[3];
			}
		}else
			goto err;
	}

	env_delenv(name);
	env_setenv(name, value, type);
	return 0;

err:
	printf("usage: %s [-s] [name] [value]\n", argv[0]);
	return -1;
}

extern int do_unsetenv(int argc, char **argv)
{
	if( argc < 2 )
	{
		printf("usage: %s [name]\n", argv[0]);
		return 0;
	}

	env_delenv(argv[1]);
	return 0;
}

extern int do_saveenv(int argc, char **argv)
{
	return env_sync();
}

