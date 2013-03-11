/*
 * =====================================================================================
 *
 *       Filename:  jobs.h
 *
 *    Description:  shell jobs
 *
 *        Version:  1.0
 *        Created:  2012年11月28日 13时45分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#ifndef _JOBS_
#define _JOBS_

#include "queue.h"
#include "command.h"
#include <unistd.h>

typedef struct job_s{
	queue_t queue;
	
	int id;
	pid_t pid;
	cmd_line_t* cmd_line;
}job_t;

job_t *job_init(void);
void job_exit(job_t*);

int job_in(job_t*, pid_t, cmd_line_t *cmd_line);
void job_out(job_t*, pid_t);
int job_count(job_t* jobs);

job_t* job_find_by_pid(job_t* jobs, pid_t pid);
job_t* job_find_by_id(job_t* jobs, int id);

int job_pid_to_id(job_t* jobs, pid_t pid);
pid_t job_id_to_pid(job_t *jobs, int id);

#endif

