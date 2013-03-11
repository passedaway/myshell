/*
 * =====================================================================================
 *
 *       Filename:  queue.h
 *
 *    Description:  queue.h
 *
 *        Version:  1.0
 *        Created:  2012年10月11日 18时06分29秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

typedef struct queue_s{
	struct queue_s *prev;
	struct queue_s *next;
}queue_t;

#define q_init(q)	(q)->prev = (q), (q)->next = (q)
#define q_empty(q)	((q)->next == (q))

#define q_foreach(q, tmp)	for(tmp=(q)->next; tmp != (q); tmp = tmp->next)

void q_enque(queue_t *q, queue_t *qem);
void q_deque(queue_t *qem);
int q_count(queue_t *q);

#endif
