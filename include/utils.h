/*
 * =====================================================================================
 *
 *       Filename:  utils.h
 *
 *    Description:  my utils
 *
 *        Version:  1.0
 *        Created:  2012年11月27日 15时36分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#ifndef _MY_UTILS_H_
#define _MY_UTILS_H_

char* mystrchr(char *str, char c);
char *mystrcpy(char *dest, char *src);
char *mystrncpy(char *dest, char *src, int size);
char *mystrcat(char *dest, char *src);
int mystrcmp(char *s1, char *s2);

void *mymemset(void *src, int c, int size);
void *mymemcpy(void *dest, void *src, int size);
void *mymemmove(void *dest, void *src, int size);
int mygetopt(int argc, char **argv, char *opts);

#endif

