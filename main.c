/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  main entry
 *
 *        Version:  1.0
 *        Created:  2012年11月22日 17时08分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <unistd.h>

#include "shell.h"

static int _test_cmd(int argc, char **argv)
{
	int i = 0;
	printf("_test_cmd: argc = %d\n", argc);
	for( ; i < argc ; i++)
	{
		printf("argv %d = %s\n", i, argv[i]);
	}

	return 0;
}

static int _help_cmd(int argc, char **argv)
{
	printf("%s:%d\n", __FUNCTION__, argc);
	printf("this is my help .\n");
	return 0;
}

int main(void)
{
	shell_cfg_t cfg = {
		.prompt = "zSimpleSh $ ",
		.use_color = 0,
		.has_env = 0,
	/*  shell in a thread , or not*/
		.is_in_pthread = 0,
	};
	int ret = 0;

	shell_init(&cfg);

	printf("shell will start\n");
	shell_register_cmd("cp", "just test this shell", _test_cmd);

	/* this is test the shell_register cmd, for dumplicate name registion */
	ret = shell_register_cmd("help1", "my self print", _help_cmd);
	printf("shell_register_cmd: dumplicate cmds registe ret = %d \n",ret);

	if( cfg.is_in_pthread )
	{
		shell_start();

		//printf("shell_isrunning() %d \n", shell_isrunning());
		while( shell_isrunning() )
		{
			sleep(1);
		}
		shell_stop(1);
	}else{
		/*  shell_start will not back */
		shell_start();
	}

	shell_exit();

	return 0;
}
