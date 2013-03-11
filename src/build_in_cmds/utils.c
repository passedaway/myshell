/*
 * =====================================================================================
 *
 *       Filename:  utils.c
 *
 *    Description:  some utils
 *
 *        Version:  1.0
 *        Created:  2012年11月27日 14时43分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#include "utils.h"

#ifndef NULL
#define NULL	(void*)0
#endif

char* mystrchr(char *str, char c)
{
	char *tmp = str;
	if( str == NULL )
		return NULL;

	while( (*tmp != 0) && (*tmp != c) )
		tmp++;

	if( *tmp == 0 )
		return NULL;

	return tmp;
}

char *mystrcpy(char *dest, char *src)
{
	char *tmp = dest;
	if( !dest || !src )
		return NULL;

	while( *src != 0 )
		*dest++ = *src++;

	*dest = 0;

	return tmp;
}

char *mystrncpy(char *dest, char *src, int size)
{
	char *tmp = dest;
	if( !dest || !src )
		return NULL;

	while( (size != 0) && (*src!=0) )
		size--, *dest++ = *src++;

	*dest = 0;
	return tmp;
}

char *mystrcat(char *dest, char *src)
{
	char *ret= dest;
	if( !dest || !src )
		return NULL;

	while( *dest++ != 0 )
		;
	while( *src != 0 )
		*dest++ = *src++;
	*dest = 0;

	return ret;
}

int mystrcmp(char *s1, char *s2)
{
	if( s1 == s2 )
		return 0;

	if( s1 && !s2 )
		return 1;

	if( !s1 && s2 )
		return -1;

	while( *s1 && *s2 && (*s1 == *s2) )
		s1++, s2++;

	if( *s1 == *s2 )
		return 0;

	if( *s1 > *s2 )
		return 1;
	else
		return -1;

	return 0;
}


void *mymemset(void *src, int c, int size)
{
	char *tmp = (char *)src;
	int i = 0;

	if( src == NULL )
		return NULL;
	for( ; i < size; i++ )
		tmp[i] = (char)c;

	return tmp;
}

void *mymemcpy(void *dest, void *src, int size)
{
	int i = 0;
	if(dest == src)
		return dest;

	if( !dest || !src )
		return NULL;

	if( size <= 0 )
		return NULL;

	for(; i < size; i++)
		((char *)dest)[i] = ((char *)src)[i];

	return dest;
}

void *mymemmove(void *dest, void *src, int size)
{
	if(dest == src )
		return dest;

	if( !dest || !src )
		return NULL;

	if( size <= 0 )
		return NULL;


	return NULL;
}

int mygetopt(int argc, char **argv, char *opts)
{
	static int sp = 0;
	static int optindex = 1;
	char *tmp = NULL;

	while(1)
	{
		if( optindex >= argc )
		{
			optindex = 1;
			sp = 0;
			return -1;
		}

		sp++;
		if( (argv[optindex][0] == '-') && (argv[optindex][sp] != 0) )
		{
			tmp = mystrchr(opts, argv[optindex][sp]);
			if( tmp )
				return argv[optindex][sp];
		}

		optindex++;
		sp = 0;
	}
}


