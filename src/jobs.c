/*
 * =====================================================================================
 *
 *       Filename:  jobs.c
 *
 *    Description:  shell jobs
 *
 *        Version:  1.0
 *        Created:  2012年11月28日 13时44分44秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#include "os.h"
#include "jobs.h"

#include <stdio.h>
#include <string.h>

static queue_t _this_jobs;
static int _this_job_num = 0;

static void job_delete(job_t *job)
{
	if( job )
	{
		if( job->cmd_line )
		{
			cmd_line_delete(job->cmd_line);
			job->cmd_line = NULL;
		}

		/* just increace, not decreace */
		//_this_job_num--;
		FREE(job);
	}
}

static job_t *job_new(pid_t pid, cmd_line_t *cmd_line)
{
	job_t *tmp = NULL;

	if( !cmd_line )
		return NULL;

	tmp = (job_t*)MALLOC(sizeof(job_t));
	if( !tmp )
		goto err;
	memset(tmp, 0, sizeof(job_t));

	tmp->cmd_line = cmd_line;
	tmp->pid = pid;

	return tmp;

err:
	job_delete(tmp);
	return NULL;
}

job_t *job_init(void)
{
	q_init(&_this_jobs);
	return (job_t*)&_this_jobs;
}

void job_exit(job_t* jobs)
{
	job_t *tmp = NULL;
	if( jobs != (job_t*)&_this_jobs )
		return;

	jobs = (job_t*)&_this_jobs;

	while( !q_empty((queue_t*)&(jobs->queue)) )
	{
		tmp = (job_t*)(_this_jobs.next);
		q_deque((queue_t*)tmp);
		job_delete(tmp);
	}
}

int job_in(job_t* jobs, pid_t pid, cmd_line_t* cmd_line)
{
	job_t *tmp = NULL;

	if( jobs == NULL )
		return -1;
	tmp = job_new(pid, cmd_line);
	if( tmp == NULL )
		return -2;

	tmp->id = q_count((queue_t*)jobs)+1;
	q_enque((queue_t*)jobs, (queue_t*)tmp);
	_this_job_num ++;
	return tmp->id;
}

void job_out(job_t* jobs, pid_t pid)
{
	job_t *tmp = NULL;
	tmp = job_find_by_pid(jobs, pid);
	if ( tmp )
	{
		q_deque((queue_t*)tmp);
		job_delete(tmp);

		if( q_count((queue_t*)jobs) == 0 )
			_this_job_num = 0;
	}
}

int job_count(job_t* jobs)
{
#if 0
	return q_count((queue_t*)jobs)+1;
#else
	return _this_job_num;
#endif
}

job_t* job_find_by_pid(job_t* jobs, pid_t pid)
{
	queue_t *tmp = NULL;
	q_foreach((queue_t*)jobs, tmp)
	{
		if(pid == ((job_t*)tmp)->pid )
			return (job_t*)tmp;
	}
	return NULL;
}

job_t* job_find_by_id(job_t* jobs, int id)
{
	queue_t *tmp = NULL;
	q_foreach((queue_t*)jobs, tmp)
	{
		if( id == ((job_t*)tmp)->id )
			return (job_t*)tmp;
	}

	return NULL;
}

int job_pid_to_id(job_t* jobs, pid_t pid)
{
	job_t *tmp = job_find_by_pid(jobs, pid);
	if( tmp == NULL )
		return -1;

	return tmp->id;
}

pid_t job_id_to_pid(job_t *jobs, int id)
{
	job_t *tmp = job_find_by_id(jobs, id);
	if( tmp == NULL )
		return 0;
	
	return tmp->pid;
}

