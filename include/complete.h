/*
 * =====================================================================================
 *
 *       Filename:  complete.h
 *
 *    Description:  command match / or auto complete
 *
 *        Version:  1.0
 *        Created:  2012年12月03日 15时05分10秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#ifndef _COMPLETE_H_
#define _COMPLETE_H_

/* singleton mode */
/* 
 * pattern		: in
 * flags		: in
 */
int complete_try(const char *pattern, int flags); 
char *complete_get_entry(const char *pattern, int getall, int *res_len);
int complete_get_almost_likely(const char *pattern, char **out_buf);
void complete_over(void);

#endif

