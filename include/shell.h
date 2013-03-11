/*
 * =====================================================================================
 *
 *       Filename:  shell.h
 *
 *    Description:  my shell, like cfe or bash
 *
 *        Version:  1.0
 *        Created:  2012年10月11日 18时01分56秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#ifndef __SHELL_H__
#define __SHELL_H__

#include "command.h"

typedef struct {
	const char *prompt;
	int use_color;
	int has_env;
/* is_in_pthread 
 * 0	start thread to start shell
 * nozero  shell_start will not return, until exit
 */
	int is_in_pthread;
}shell_cfg_t;

int shell_init(const shell_cfg_t *);
int shell_exit();

int shell_start();
int shell_stop(int force);
int shell_isrunning();

int shell_register_buildin_cmd(const char *name, const char *usage, int (*func)(int argc, char **argv) );
int shell_register_cmd(const char *name, const char *usage, int (*func)(int argc, char **argv) );
int shell_do_command(cmd_line_t *cmd_line);
#endif

