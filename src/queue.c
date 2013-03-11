/*
 * =====================================================================================
 *
 *       Filename:  queue.c
 *
 *    Description:  implement queue.h
 *
 *        Version:  1.0
 *        Created:  2012年10月11日 18时09分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#include "queue.h"

/*       |-------<<-----|
 *     p | n ---->>---  p | n
 *     |--------->>-------|
 *       |-------<<-----|
 */

void q_enque(queue_t *q, queue_t *qem)
{
#if 0
	qem->prev = q->prev;
	q->prev->next = qem;
	q->prev = qem;
	qem->next = q;
#else
	q->prev->next = qem;
	qem->next = q;
	qem->prev = q->prev;
	q->prev = qem;
#endif
}

void q_deque(queue_t *qem)
{
	qem->prev->next = qem->next;
	qem->next->prev = qem->prev;
}

int q_count(queue_t *q)
{
	int i = 0;
	queue_t *tmp = q;
	while(tmp->next != q)
	{
		i++;
		tmp = tmp->next;
	}

	return i;
}

