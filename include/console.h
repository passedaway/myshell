/*
 * =====================================================================================
 *
 *       Filename:  console.h
 *
 *    Description:  console.h
 *
 *        Version:  1.0
 *        Created:  2012年11月22日 17时18分32秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing ,changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

/* 
 * level   
 * JNP		just newline, then print prompt
 * JP		not-newline, just print prompt
 * JPB		just print buffer,  not do other thing
 * JPF		just print fmt, not do other thing
 * NBF		newline, print buffer, print fmt
 * NBFNP	newline, print buffer, print fmt, newline, print prompt
 * NBFNPB	newline, print buffer, print fmt, newline, print prompt, print buffer
 * NFNP		newline, print fmt_string, then newline, print prompt
 * RPPB		not-newline, reprint prompt, print buffer, with curpos_adjust
 *
 * MAX	define is not over.to be continue 2012年 11月 22日 星期四 18:52:04 CST
 */
typedef enum{
	JNP=0,
	JP,
	JPB,
	JPF,
	NBF,
	NBFNP,
	NBFNPB,
	NFNP,
	RPPB,
	MAX
}level_t;

typedef struct console_s{
	int ttyfd;

	unsigned int *buf; 		/*  recv buffer , this size is 16, incase of sometimes morebytes, one key 4bytes*/
	int count; 				/*  recv buffer size */
	int rpos;				/*  buf read pos */
	int wpos;				/*  buf write pos */

	char *dispbuf; 			/*  display buf */
	int buf_size; 			/*  buffer size  */
	int dstartpos;			/*  display start pos */
	int dendpos;			/*  display end pos */
	int last_endpos;		/*  use this to clear line print */
	int curpos;				/*  cursor pos */
	const char *prompt;		/* maybe use buffer will be safer */

	unsigned int (*get_key)(void);

	int (*print)(level_t level, const char *, ...);

	/*  this all "index" is the offset to the corsor */
	int (*put_c_to_disp)(int c, int index);
	int (*put_s_to_disp)(char *s, int len, int index);
	int (*del_c_from_disp)(int index);
	int (*del_s_from_disp)(int index, int len);
	int (*set_corsor)(int index);
	void (*set_corsor_to_start)(void);
	void (*set_corsor_to_end)(void);
	int (*get_corsor)(void);

	void (*init_dispbuf)(void);
	void (*reset_dispbuf)(void);
	int (*is_dispbuf_empty)(void);

	int (*exit)(struct console_s *);
}console_t;

int console_init(console_t **console, int buf_size);
int console_exit(console_t *console);

void set_tty_attr(int fd);
void clear_tty_attr(int fd);

void dbg_console(console_t *console);
#endif

