/*
 * =====================================================================================
 *
 *       Filename:  env.h
 *
 *    Description:  shell envirment
 *
 *        Version:  1.0
 *        Created:  2012年12月04日 13时13分12秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#ifndef __ENV_H__
#define __ENV_H__

#include "queue.h"

typedef enum{
	ENV_TYPE_BUILD_IN = 1,
	ENV_TYPE_USER_DEFINE,
	ENV_TYPE_SAVE,
	ENV_TYPE_MAX
}env_type_t;

typedef struct env_s{
	queue_t queue;
	char *name;
	char *value;
	env_type_t type;
}env_t;

env_t* env_init(void);
void env_exit(env_t *);

int env_find(const char *name, char **value);
int env_setenv(const char *name, const char *value, env_type_t type);
int env_delenv(const char *name);
int env_sync();

#endif
