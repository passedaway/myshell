/*
 * =====================================================================================
 *
 *       Filename:  complete.c
 *
 *    Description:  match command or command auto complete
 *
 *        Version:  1.0
 *        Created:  2012年12月03日 15时10分55秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#include "command.h"

//#include "env.h"
#include "os.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct entry_s{
	queue_t queue;
	char *data;
}entry_t;

static queue_t match_queue = {
	.prev = &match_queue,
	.next = &match_queue,
};
static queue_t *match_pos ;

extern int cmd_match_list_next(char *res);
extern void cmd_match_start(void);

static struct complete_s{
	const char *name;
	/*
	 * res		: out
	 * return	: 
	 * 0	 didnot find next; 
	 * nonzero find next
	 */
	void(* start)(void);
	int(* list_next)(char *res);
	void(* complete)(void);
}gcom[] = {
	{
		"cmd",
		cmd_match_start,
		cmd_match_list_next,
		NULL,
	}
};
/* member current match type */
static int cur_match = 0;
/* current matched results */
static int cur_count = 0;
static char *cur_pattern = 0;

static inline void reset_complete_result(void)
{
	match_pos = &match_queue;
}

static void entry_delete(entry_t *entry)
{
	if( entry )
	{
		if( entry->data )
			FREE(entry->data);
		FREE(entry);
	}
}

static entry_t *entry_new(char *data)
{
	char *tmp = NULL;
	int len = 0;
	entry_t *etmp;

	if( !data )
		return NULL;

	etmp = (entry_t*)MALLOC(sizeof(entry_t));
	if( !etmp )
		return NULL;
	memset(etmp, 0, sizeof(entry_t));

	len = strlen(data) + 1;
	tmp = (char *)MALLOC(len);
	if( !tmp )
		goto err;

	strcpy(tmp, data);
	tmp[len] = 0;
	
	etmp->data = tmp;
	return etmp;

err:
	entry_delete(etmp);
	return NULL;
}

static int compare_ignore_case(const char *pattern ,char *ins)
{
	int c1 = 0, c2 = 0;
	int ret = 0;

	while( *pattern && *ins )
	{
		c1 = *pattern++;
		c2 = *ins++;

		if( c1 <= 'Z' )
			c1 += 0x20;
		if( c2 <= 'Z' )
			c2 += 0x20;

		if( c1 == c2 )
			continue;
		else
			return -1;
	}

	if( *pattern == 0 )
		return 0;

	return 1;
}

int complete_try(const char *pattern, int flags) 
{
	int ret = 0;
	int count = 0;
	char temp[256];
	entry_t *etmp;

	if(flags < 0 ||  flags > sizeof( gcom ) /sizeof(gcom[0]) )
			return -1;
	
	match_pos = &match_queue;

	cur_match = flags;
	if( gcom[flags].start )
		gcom[flags].start();

	while( (ret=gcom[flags].list_next(temp)) > 0 )
	{
		ret = compare_ignore_case(pattern, temp);
//		printf("try match ret=%d pattern=%s temp=%s\n", ret, pattern, temp);
		if( ret == 0 )
		{
			etmp = entry_new(temp);
			if( etmp == NULL )
			{
				printf("error:out of memory\n");
				goto out;
			}

//			printf("matched:%s\n", etmp->data);
			q_enque(&match_queue, (queue_t*)etmp);

			count++;
		}
	}

out:
	cur_count = count;
	return count;
}

char *complete_get_entry(const char *pattern, int getall, int *len)
{
	if( match_pos->next != &match_queue )
	{
		char *tmp = NULL;
		match_pos = match_pos->next;
		tmp = ((entry_t*)match_pos)->data;
		if( !getall )
		{
			tmp += strlen(pattern);
		}

		if( len )
			*len = strlen(tmp);
//		printf("complete_get_entry=%s\n", tmp);
		return tmp;
	}

	return NULL;
}

void complete_over(void)
{
	queue_t *tmp;
	q_foreach(&match_queue, tmp)
	{
		if( tmp != NULL )
		{
			q_deque(tmp);
			entry_delete((entry_t*)tmp);
		}
	}

	if( gcom[cur_match].complete )
		gcom[cur_match].complete();

	q_init(&match_queue);
	reset_complete_result();
	cur_count = 0;
}

int complete_get_almost_likely(const char *pattern, char **out_buf)
{
	int most_match = 0, res_len=0 ,itmp;
	int c = 0;
	char *first = NULL;
	char *res = NULL;
	if( (out_buf == NULL) || (cur_count == 0) )
		return -1;

	*out_buf = NULL;
	reset_complete_result();
	first = complete_get_entry(pattern, 0, &most_match);
	most_match = strlen(first);

	if( cur_count == 1 )
	{
		goto same_out;
	}

	while( NULL != (res=complete_get_entry(pattern, 0, &res_len)) )
	{
		if( res_len < most_match )
			most_match = res_len;

		for(itmp=0; itmp < most_match; itmp++)
		{
			if( *(first+itmp) !=  *(res+itmp) )
				break;
		}

		if( itmp != most_match )
		{
			most_match = itmp;
		}

		if( itmp == 0 )
		{
			most_match = 0;
			goto not_same_out;
		}
	}


same_out:
	*out_buf = first;
not_same_out:
	reset_complete_result();
	return most_match;
}

