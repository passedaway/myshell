/*
 * =====================================================================================
 *
 *       Filename:  command.h
 *
 *    Description:  command
 *
 *        Version:  1.0
 *        Created:  2012年11月26日 11时47分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#ifndef _CMD_H_
#define _CMD_H_

#include "queue.h"

typedef struct cmd_line_s{
    int cmd_id;

    char *cmdline;

    int argc;
    char **argv;

    int haspipe; /*  has pipe */
    int hasredirect; /* has redirect */
    int background; /* do this command in background */

    struct cmd_line_s *next;/* one input line, maybe have more then 1 cmds, such as pipe, or ';' */
}cmd_line_t;

cmd_line_t *cmd_line_new(char *buf, int len, int *errcode);
void cmd_line_delete(cmd_line_t*);

typedef struct{
    queue_t queue;
    int id;
    const char *name;
    const char *usage;

    int (*func)(int argc, char **argv);
}cmd_t;

cmd_t* cmd_init(void);
void cmd_exit(cmd_t*);

cmd_t *cmd_new(int id, const char *name, const char *usage, int (*func)(int argc, char **argv));
void cmd_delete(cmd_t *);

cmd_t *cmd_find_cmd(cmd_t *, int cmd_id);
cmd_t *cmd_find_cmd_by_name(cmd_t *, const char *name);
void cmd_register_cmd(cmd_t *, cmd_t *);
void cmd_print_usage(cmd_t *cmd);

/*
 * this id define which use fork & which not use 
 * id > STATIC_CMD, when process is use another cmd(support ctrl+c, ctrl+z, etc)
 * id <= STATIC_CMD, just do it in shell(cannot ctrl+c, & ctrl+z, etc)
 */ 
#define STATIC_CMD 			512
#define USER_BUILDIN_CMD	128	

#endif

