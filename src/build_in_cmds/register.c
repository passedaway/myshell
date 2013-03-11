/*
 * =====================================================================================
 *
 *       Filename:  register.c
 *
 *    Description:  register build in commands 
 *
 *        Version:  1.0
 *        Created:  2012年11月26日 18时07分00秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#include "command.h"
#include "build_in_cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

/* commands do in shell ( same as implemention) */
extern int do_fg(int argc, char **argv);
extern int do_jobs(int argc, char **argv);
extern int do_exit(int argc, char **argv);
extern int do_help(int argc, char **argv);

static int do_cd(int argc, char **argv);
static int do_pwd(int argc, char **argv);

extern int do_unsetenv(int argc, char **argv);
extern int do_printenv(int argc, char **argv);
extern int do_setenv(int argc, char **argv);
extern int do_saveenv(int argc, char **argv);

/* commands do with child fork */
extern int do_ls(int argc, char **argv);

/* 
 * register these cmd to shell, 
 * which id=-1 then do process in the child process 
 * else it will done in shell
 */
void register_build_in_cmd(cmd_t *cmd)
{
	int i = 0;
	cmd_t *tmp;

	cmd_t build_in_cmd[] = {
		{
			.id = 0,
			.name = "help", 
			.usage = "help [command]\nprint usage of all commands, or specical commands\n", 
			.func = do_help,
		},

		{
			.id = 1,
			.name = "pwd",
			.usage = "pwd\nprint the current work dir",
			.func = do_pwd,
		},

		{
			.id = 2,
			.name = "cd",
			.usage = "cd [work dir]\nchange work directory, if no args, then change to user default work directory",
			.func = do_cd,
		},

		{
			.id = 3,
			.name = "exit",
			.usage = "exit\nexit this shell.",
			.func = do_exit,
		},

		{
			.id = 4,
			.name = "fg",
			.usage = "fd [job id]\nresume or continue the stoped job.",
			.func = do_fg,
		},

		{
			.id = 5,
			.name = "jobs",
			.usage = "jobs [job id]\nprint current stoped jobs.",
			.func = do_jobs,
		},

		{
			.id = 6,
			.name = "printenv",
			.usage = "printenv [env]\nprint envirment.",
			.func = do_printenv,
		},

		{
			.id = 7,
			.name = "setenv",
			.usage = "setenv [name] [value]\nset envirment.",
			.func = do_setenv,
		},

		{
			.id = 8,
			.name = "unsetenv",
			.usage = "unsetenv [name]\nunsetenv enviroment.",
			.func = do_unsetenv,
		},

		{
			.id = 9,
			.name = "saveenv",
			.usage = "saveenv\nsaveenv to the file.",
			.func = do_saveenv,
		},

		{
			.id = -1,
			.name = "ls",
			.usage = "ls [directory]\nprint directory entrys.\nif no args, then print current entrys.",
			.func = do_ls,
		},
	};

	for (; i < sizeof(build_in_cmd)/sizeof(cmd_t); i++)
	{
//		printf("register cmd %s\n", build_in_cmd[i].name);
		tmp = cmd_new(build_in_cmd[i].id, build_in_cmd[i].name, build_in_cmd[i].usage, build_in_cmd[i].func);
		if( tmp )
			cmd_register_cmd(cmd, tmp);
	}
}

/* command : pwd */
static int do_pwd(int argc, char **argv)
{
	char *buf = (char *)malloc(_POSIX_PATH_MAX);
	if( buf == NULL )
		return -1;
	memset(buf, 0, _POSIX_PATH_MAX);
	getcwd(buf, _POSIX_PATH_MAX);
	printf("%s\n", buf);
	if( buf )
		free(buf);

	return 0;
}

/* command : cd */
static int do_cd(int argc, char **argv)
{
	char *tmp_dir = argv[1];
	if( tmp_dir == NULL )
		tmp_dir = "";

	if( chdir(tmp_dir) < 0 )
		printf("cd %s: %s\n", tmp_dir, strerror(errno));

	return 0;
}


