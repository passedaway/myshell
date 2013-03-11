/*
 * =====================================================================================
 *
 *       Filename:  os.c
 *
 *    Description:  for debug memory
 *
 *        Version:  1.0
 *        Created:  2012年11月23日 19时06分07秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#include "os.h"

#include <stdio.h>
#include <stdlib.h>

void * _malloc(const char *fuc, int line, int size)
{
	char *ptr;
	ptr = (char *)malloc(size);
	printf("DBG:malloc:[%s][%d] size=%d ptr=%p\n", fuc, line,  size, ptr);
	return ptr;
}

void  _free(const char *fuc, int line, void *ptr)
{
	printf("DBG:free:[%s][%d] ptr=%p\n", fuc, line,  ptr);
	if(ptr)
		free(ptr);
}

void dumpbuf(const char *function_name, int line, unsigned char *buf, int len)
{
#define IS_PRINTF(c)	((c)>=0x20 && (c)<=0x7E)

	int i = 0;
	printf("\n[%s][%d]buf=%p len=%d\n", function_name, line, buf, len);
	for(i=0; i < len; i++)
	{
		printf("%02x%s", (unsigned char )buf[i], (i+1)%16? " ": " | ");
		if( (i+1)%16 == 0 )
		{
			int j = i - 15;
			for(; j <= i; j++ )
				printf("%c", IS_PRINTF(buf[j])?(unsigned char)buf[j]:'.' );
			printf("\n");
		}
	}

	if( i%16 != 0 )
	{
		int j = 16 - (i+1)%16;
		for(; j >= 0; j--)
			printf("   ");
		printf("| ");

		j = i - (i+1)%16 + 1;
		for(; j<i; j++)
			printf("%c", IS_PRINTF((unsigned char)buf[j])?(unsigned char)buf[j]:'.' );

		printf("\n");
	}

	printf("\n[%s][%d] dump over\n", function_name, line);
}
