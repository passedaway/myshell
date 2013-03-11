/*
 * =====================================================================================
 *
 *       Filename:  os.h
 *
 *    Description:  for debug ,mem, lock & other
 *
 *        Version:  1.0
 *        Created:  2012年11月23日 18时58分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#ifndef _OS_H_
#define _OS_H_

void * _malloc(const char *fuc, int line, int size);
void  _free(const char *fuc, int line, void *ptr);

#ifdef DBG_MEM
#define MALLOC(size)	_malloc(__FUNCTION__, __LINE__, size)
#define FREE(ptr)		_free(__FUNCTION__, __LINE__, ptr)
#else
#include <stdlib.h>
#define MALLOC	malloc
#define FREE	free
#endif

#ifdef DEBUG
#define DBG_Print(fmt, args...)	printf("[DBG][%s][%d][%s]"fmt, __FILE__, __LINE__, __FUNCTION__, ## args )
#else
#define DBG_Print(fmt, ...)	
#endif

#define BUG()	printf("\nbug:%s %d\n", __FUNCTION__, __LINE__)

#define DumpData(b, l)	dumpbuf(__FUNCTION__, __LINE__, b, l)
void dumpbuf(const char *function_name, int line, unsigned char *buf, int len);

#endif

