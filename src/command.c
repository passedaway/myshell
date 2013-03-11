/*
 * =====================================================================================
 *
 *       Filename:  command.c
 *
 *    Description:  command function
 *
 *        Version:  1.0
 *        Created:  2012年11月26日 13时32分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#include "command.h"
#include "os.h"
#include "build_in_cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static queue_t _this_cmd;
static int _this_cmd_id;

static void cmd_line_delete_one(cmd_line_t* tmp)
{
	if( tmp )
	{
		if( tmp->argv )
		{
			int i = 0;
			for( ;i < tmp->argc; i++ )
			{
				if( tmp->argv[i] )
					FREE(tmp->argv[i]);
			}

			FREE(tmp->argv);
		}

		if ( tmp->cmdline )
			FREE(tmp->cmdline);

		FREE(tmp);
	}
}

static cmd_line_t *cmd_line_new_one(char *buf, int len, int argc)
{
	cmd_line_t *tmp = NULL;
	char *start, *cur;
	int clen;

//	printf("\n%s:buf=%s len=%d argc=%d\n", __FUNCTION__, buf, len, argc);

	if( buf == NULL )
		return NULL;

	tmp = (cmd_line_t*)MALLOC(sizeof(cmd_line_t));
	if( tmp == NULL )
		return NULL;
	memset(tmp, 0, sizeof(cmd_line_t));

	tmp->cmdline = (char *)MALLOC(len+1);
	if( tmp->cmdline == NULL )
		goto OUT;
	strncpy(tmp->cmdline, buf, len);
	tmp->cmdline[len] = 0;

	/* split buf, with spcae or \t, endwith 0 or \n */
	cur = buf;
	clen = len;

#if 0
	/*1. need  get argc */
	while( clen )
	{
		if( (*cur == ' ') || (*cur == '\t') )
		{
			cur++;
			clen--;
			continue;
		}

		while(clen && ( (*cur != ' ') && (*cur != '\t') ) )
			cur++, clen--;
		argc++;
	}
#endif
	/* argc get from caller */
	tmp->argc = argc;

	//printf("buf=%s argc=%d\n", buf, tmp->argc);
	/*2. according argc, then allocate argv */
	tmp->argv = (char **)MALLOC(sizeof(char*) * (tmp->argc+1));
	if( tmp->argv == NULL )
		goto OUT;
	memset( tmp->argv, 0, sizeof(char*) * (tmp->argc+1) );
	
	/*3. allocate & fill argv[i] */
	start = buf;
	cur = buf;
	argc = 0;
	while(len)
	{
		if( (*cur == ' ') || (*cur == '\t') ) 
		{
			len--,cur++;
			continue;
		}

		start = cur;
		while( len && ((*cur != ' ')&&*cur != '\t') )
			len--, cur++;
		clen = cur - start + 1;
		tmp->argv[argc] = (char *)MALLOC( clen );
		if( tmp->argv[argc] == NULL )
			goto OUT;
		memcpy(tmp->argv[argc], start, clen-1);
		tmp->argv[argc][clen-1] = 0;

		//printf("argc=%d argv=%s\n", argc, tmp->argv[argc]);
		argc++;
	}

	//printf("buf=%s argv[0] = %s argc=%d\n", buf, tmp->argv[0], argc);
	return tmp;

OUT:
	cmd_line_delete_one(tmp);
	return NULL;
}

cmd_line_t *cmd_line_new(char *buf, int len, int *errcode)
{
	char *start, *cur;
	int clen = 0;
	int haspipe = 0, hasredict = 0, inbackground = 0;
	cmd_line_t *ret, *tmp, *end;
	int single_quotation_marks=0, double_quotation_marks=0;
	int argc = 0;

#define CHECK_QUOTATION_PAIRS()	( ((single_quotation_marks % 2)==0) && ((double_quotation_marks % 2 ) == 0))
#define LIST_IN()	do {\
						if( ret == NULL )\
						{\
							ret = tmp;\
						}else{\
							end->next = tmp;\
						}\
						end = tmp; }while(0)

	/* 
	 * check if has pipe, redirect or background 
	 * and will call cmd_line_new_one new one cmd_line_t
	 */
	if( buf == NULL)
		return NULL;

	if( len <= 0 )
		len = strlen(buf);

	/* init tmp var */
	cur = buf;
	clen = len;
	ret = tmp = end = NULL;

	/* ignore start space */
	while( clen )
	{
		if( (*cur==' ') || (*cur=='\t') )
			clen--, cur++;
		else
			break;
	}
	if( clen == 0 )
		goto err;
	start = cur;

	while( clen )
	{
		switch(*cur)
		{
			case '|' :
			case '&' :
			case ';' :
				/* check if single_quotation_marks & double_quotation_marks are pairs */
				if( !CHECK_QUOTATION_PAIRS() )
				{
					break;
				}

				if( *cur == '|' )
				{
					haspipe++;
					/* cannot has pipe & background */
					if( inbackground )
					{
						goto err;
					}
				}
				else if( *cur == '&' )
				{
					inbackground++;
					/* cannot has pipe & background */
					if( haspipe )
						goto err;
				}

#if 0
				printf("start=%p buf=%p argc=%d clen-1=%d argc=%d\n", start, buf, argc, clen-1, argc);
				printf("haspipe = %d inbackground = %d"
						" single_quotation_marks=%d double_quotation_marks=%d\n",
						haspipe, inbackground, single_quotation_marks, double_quotation_marks);
#endif
				tmp = cmd_line_new_one(start, cur-start, argc);
				if( tmp == NULL )
				{
					printf(" %d | error\n", __LINE__);
					break;
				}

				tmp->background = inbackground;
				tmp->haspipe = haspipe;
				single_quotation_marks = double_quotation_marks = 0;
				start = cur+1;
				argc = 0;
				LIST_IN();

				if( *cur == ';' ){
					/*
					 * "one" command(maybe many) termineted 
					 * command can not in background & with multicommand
					 */
					if( inbackground )
						goto err;
					haspipe = 0;
				}
				break;

			case '(' :
			case ')' :
				/* ( or ) must in ' ' or " " */
				if( CHECK_QUOTATION_PAIRS() )
					goto err;
				break;

			case '\'':
				if( double_quotation_marks == 0 )
					single_quotation_marks++;
				break;
				
			case '\"':
				if( single_quotation_marks == 0 )
					double_quotation_marks++;
				break;

			case ' ':
			case '\t':
				if( CHECK_QUOTATION_PAIRS() )
				{
					argc++;
					while( clen )
					{
						if( (*(cur+1) == ' ') || (*(cur+1) == '\t') ) 
							clen--, cur++;
						else
							break;
					}
				}
				break;
		}

		cur++, clen--;
	}

	if( !CHECK_QUOTATION_PAIRS() )
	{
		//printf("quotation not pairs: single_quotation_marks=%d double_quotation_marks=%d\n", single_quotation_marks, double_quotation_marks);
		goto err;
	}
	//printf("start=%p buf=%p len=%d argc=%d\n", start, buf, len, argc);
	if( start != (buf+len) )
	{
		tmp = cmd_line_new_one(start, buf+len - start, argc+1);
		if( tmp == NULL )
			goto err;
		LIST_IN();
	}

	return ret;
err:
	if( errcode )
		*errcode = cur-buf;
	cmd_line_delete(ret);
	return NULL;
}

void cmd_line_delete(cmd_line_t *cmd_line)
{
	cmd_line_t *tmp;
	tmp = cmd_line;
	while( tmp )
	{
		tmp = cmd_line->next;
		cmd_line_delete_one(cmd_line);
		cmd_line = tmp;
	}
}

cmd_t *cmd_new(int id, const char *name, const char *usage, int (*func)(int argc, char **argv))
{
	cmd_t *tmp = NULL;

	if ( (name == NULL)  ||  (func == NULL) )
		return NULL;

	tmp = (cmd_t *)MALLOC(sizeof(cmd_t));
	if( tmp == NULL )
		return NULL;

	tmp->id = id;
	tmp->name = name;
	tmp->usage = usage; /*  usage can be NULL */
	tmp->func = func;

	return tmp;
}

void cmd_delete(cmd_t *tmp)
{
	if( tmp )
		FREE(tmp);
}

cmd_t* cmd_init(void)
{
	_this_cmd_id = STATIC_CMD+1;
	q_init((&_this_cmd));
	register_build_in_cmd((cmd_t*)&_this_cmd);
	return (cmd_t*)&_this_cmd;
}

void cmd_exit(cmd_t* cmd)
{
	cmd_t *tmp=NULL;
	_this_cmd_id = STATIC_CMD+1;
	if( cmd != (cmd_t*)&_this_cmd )
	{
		/* fixme!!! the exit cmd should be same with _this_cmd */
		return;
	}
	cmd = (cmd_t*)&_this_cmd;

	while( !q_empty(&_this_cmd) )
	{
		tmp = (cmd_t*)(_this_cmd.next);
		q_deque((queue_t*)tmp);
		cmd_delete(tmp);
	}
}


cmd_t *cmd_find_cmd(cmd_t *cmd, int cmd_id)
{
	queue_t *tmp;
	if( q_empty(&cmd->queue) )
		return NULL;

	q_foreach((queue_t*)cmd, tmp)
	{
		if(cmd_id == ((cmd_t*)tmp)->id)
			return (cmd_t*)tmp;
	}

	return NULL;
}

cmd_t *cmd_find_cmd_by_name(cmd_t *cmd, const char *name)
{
	queue_t *tmp;
	DBG_Print("name=%s\n", name);
	if( q_empty(&cmd->queue) )
		return NULL;
	q_foreach((queue_t*)cmd, tmp)
	{
		DBG_Print("queue.next=%p prev=%p name=%s\n", cmd->queue.next, cmd->queue.prev, ((cmd_t*)tmp)->name);
		if(strcmp(name, ((cmd_t*)tmp)->name) == 0 )
			return (cmd_t*)tmp;

	}
	return NULL;
}

void cmd_register_cmd(cmd_t *cmd, cmd_t *cmd_item)
{
	if( cmd == NULL )
		return;
	
	if( cmd_item->id == -1 )
		cmd_item->id = _this_cmd_id++;
	q_enque((queue_t*)cmd, (queue_t*)cmd_item);
}

void cmd_print_usage(cmd_t *cmd)
{
	int i, len;
	char *tmp;
	if( cmd == NULL )
		return;

//#ifdef DEBUG
#if 1
	printf("id=%d %s:\n\tusage: ", cmd->id, cmd->name);
#else
	printf("%s:\n\tusage: ", cmd->name);
#endif
	for( i = 0, len = strlen(cmd->usage), tmp=(char *)cmd->usage; i < len; i++, tmp++)
	{
		if( *tmp == '\n' || *tmp == '\r')
			printf("\n\t");
		else
			printf("%c", *tmp);
	}

	tmp--;
	if( *tmp != '\n') /* const data ,must not need modify */
		printf("\n");
}

/* ***********************for match****************************** */
static queue_t *cmd_match_pos;
void cmd_match_start(void)
{
	cmd_match_pos = &_this_cmd;
}

int cmd_match_list_next(char *res)
{
	if( cmd_match_pos->next != &_this_cmd )
	{
		cmd_match_pos = cmd_match_pos->next;
		strcpy(res, ((cmd_t*)cmd_match_pos)->name);
		return 1;
	}

	return 0;
}

void cmd_match_complete(void)
{

}

