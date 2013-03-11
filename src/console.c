#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "os.h"
#include "console.h"

#define INPUT_BUF_SIZE	16
#define DISP_BUF_SIZE 	256

static console_t *_this_console;

static struct termios term;

static char *_s_pbuf = 0;

static void set_console(int fd, int flag)
{
	if ( flag == 1 )
	{
		/* set to un normal mode */
		tcgetattr(fd, &term);
		term.c_lflag &= ~ECHO;
		term.c_lflag &= ~ICANON;
		tcsetattr(fd, TCSAFLUSH, &term);
	}else{
		/*  set to normal mode */
		tcgetattr(fd, &term);
		term.c_lflag |= ECHO;
		term.c_lflag |= ICANON;
		tcsetattr(fd, TCSAFLUSH, &term);
	}
}
/****************************************************************************
 * console_t buf operation function 
 * ***************************************************************************/
#define put_key(c, key)	 do{\
						c->buf[c->wpos] = key; \
						c->wpos++; \
						c->wpos %= INPUT_BUF_SIZE; \
					}while(0)

static inline unsigned int get_key(console_t *con)
{
	unsigned int key = con->buf[con->rpos]; 
	con->rpos++; 
	con->rpos %= INPUT_BUF_SIZE; 
	return key;
}

#define can_read(c)		(c->wpos != c->rpos)
static inline int can_write(console_t *con)
{
	unsigned char keywpos = con->wpos;
	unsigned char keyrpos = con->rpos;

	keywpos = (keywpos+1) % INPUT_BUF_SIZE;
	
	return keywpos != keyrpos;
}


/* **********console  get key function*************************** */
static unsigned int _get_key()
{
//	printf("\nwpos = %d rpos=%d\n", _this_console->wpos, _this_console->rpos);
	if( !can_read(_this_console) )
	{
		unsigned int data=0;
		set_console(0, 1);
		read(0, &data, 4);
		set_console(0, 0);
		put_key(_this_console, data);
//		printf("\nkey = 0x%x\n", data);
	}

	return get_key(_this_console);
}

/* **********console print function*************************** */
static int _just_print_prompt(int newline)
{
	int ret = 0;
	fflush(stderr);
	ret = fprintf(stderr, "%c%s", newline?'\n':'\r', _this_console->prompt);
	fflush(stderr);
	return ret; 
}

static int _just_print_buffer(int newline, int curpos_adjust)
{
	int i = 0, len = _this_console->last_endpos;

	if( !newline && (_this_console->last_endpos > _this_console->dendpos) && len)
	{
		/*  not newline ,need clear last line info */
		char *buf;
		char buf_a[64] = {0};

		if( len < 31 )
		{
			buf = buf_a;
		}else
		{
#if 0
			buf=(char *)MALLOC(len*2+1);
			if( buf == NULL )
				return -1;
			//memset(buf, 0, len*2+1);
#else
			/*
			 * if 0 code is error
			 * this is a bug, has been fixed.
			 * malloc memory len should (len+1)*2+1, not len*2 + 1
			 *
			 * 2012-12-05 09:57:25 zhaocq 
			 */
			buf=(char *)MALLOC((len+1)*2+1);
			if( buf == NULL )
				return -1;
#endif
		}

		len += 1;
		memset(buf, ' ', len);
		memset(buf+len,'\b', len);
		buf[2*len] = 0;

		fprintf(stderr, "%s", buf);
		fflush(stderr);
		if( buf != buf_a )
		{
			FREE(buf);
			buf=NULL;
		}
	}

	if( _this_console->dendpos > _this_console->dstartpos )
	{
		_this_console->dispbuf[_this_console->dendpos] = 0;
		fprintf(stderr, "%c%s", newline?'\n':0, _this_console->dispbuf + _this_console->dstartpos);

		if( curpos_adjust )
		{
			for(i = 0; i < _this_console->dendpos-_this_console->curpos; i++)
				fprintf(stderr, "%c", '\b');
		}
		fflush(stderr);
	}
	
	return _this_console->curpos-_this_console->dstartpos;
}

/* 
 * level   
 * 0	just newline, then print prompt
 * 1	just print buffer,  not do other thing
 * 2	just print fmt, not do other thing
 * 3	newline, print buffer, print fmt
 * 4	newline, print buffer, print fmt, newline, print prompt
 * 5	newline, print buffer, print fmt, newline, print prompt, print buffer
 * 6	newline, print fmt_string, then newline, print prompt
 * 7	not-newline, reprint prompt, print buffer, with curpos_adjust
 *
 * MAX	define is not over.to be continue 2012年 11月 22日 星期四 18:52:04 CST
typedef enum{
	JNP=0,
	JPB,
	JPF,
	NBF,
	NBFNP,
	NBFNPB,
	NFNP,
	}
 */
static int _con_print(level_t level, const char *fmt, ...)
{
	va_list args;
	char *ptr = NULL;
	int len = 0;
	char *_pbuf = NULL;
	int ret = -1;

	int newline = 0;
	int isprintprompt = 0;
	int isprintbuffer = 0;
	int isprintfmt = 0;
	int isreprintbuf = 0;
	int isprintfmt_newline = 0;
	int isfresh_line = 0;
	int isadjustcursor = 0;

	/* check arg valid */

	if( !_this_console || _this_console->dispbuf == NULL )
		return -1;

	/*  process level */
	switch( level )
	{
		case JPB:
			isprintbuffer = 1;
			//isadjustcursor = 1;
			break;

		case JPF:
			isprintfmt = 1;
			break;

		case NBFNPB:
			isreprintbuf = 1;
		case NBFNP:
			isprintprompt = 1;
		case NBF:
			newline = 1;
			isprintfmt = 1;
			isprintbuffer = 1;
			break;

		case NFNP:
			newline = 1;
			isprintfmt = 1;
			isprintprompt = 1;
			isprintfmt_newline = 1;
			break;

		case RPPB:
			isfresh_line = 1;
			isadjustcursor = 1;
			break;

		default:
		case JNP:
			newline = 1;
		case JP:
			isprintprompt = 1;
			break;
	}

	/*  refresh line info*/
	if( isfresh_line == 1 )
	{
		_just_print_prompt(0);
		isprintbuffer = 1;
		newline = 0;
	}

	/* print buffer */
	if( isprintbuffer )
		ret = _just_print_buffer(newline, isadjustcursor);

	/* print fmt */
	if ( isprintfmt )
	{
		_pbuf = _s_pbuf;
		ptr = _pbuf;
		va_start(args, fmt);
		len = vsprintf(ptr, fmt, args);
		va_end(args);
		ptr += len;

		/*  this newline is for fmt newline */
		fprintf(stderr, "%c%s", isprintfmt_newline?'\n':0, _pbuf);
		fflush(stderr);
	}

	/*  print prompt */
	if( isprintprompt )
		ret = _just_print_prompt(newline);

	/*  need re-print buffer */
	if( isreprintbuf )
		ret = _just_print_buffer(0, isadjustcursor);

	return ret;
}

static int _is_dispbuf_empty(void)
{
	return _this_console->dstartpos == _this_console->dendpos;
}

static void _init_dispbuf(void)
{
	_this_console->curpos = sprintf(_this_console->dispbuf, "%s", _this_console->prompt);
	_this_console->dstartpos = 0;
	_this_console->dendpos = _this_console->curpos;
}

static void _reset_dispbuf(void)
{
	_this_console->curpos = 0;
	_this_console->dstartpos = 0;
	_this_console->last_endpos = _this_console->dendpos;
	_this_console->dendpos = 0;
}

/*  this all "index" is the offset to the corsor */
static int _set_corsor(int index)
{
	int tmp = _this_console->curpos + index;
	if( (tmp < _this_console->dstartpos) || (tmp > _this_console->dendpos ) )
		return -1;

	if( index == 0 )
		return 0;

	_this_console->curpos += index;
	return 0;
}

static void _set_corsor_to_start(void)
{
	_this_console->curpos = _this_console->dstartpos;
}

static void _set_corsor_to_end(void)
{
	_this_console->curpos = _this_console->dendpos;
}

static int _get_corsor()
{
	return _this_console->curpos;
}

static int _put_c_to_dispbuf(int c, int index)
{
	int tmp = _this_console->curpos + index;
	if( (tmp < _this_console->dstartpos) || (tmp > _this_console->dendpos ) )
		return -1;

	if( _this_console->dendpos + 1 > _this_console->buf_size )
	{
		BUG();
		return -1;
	}

	if( !(_this_console->curpos == _this_console->dendpos) )
	{
	/*  cursor is not at the end of line */
	/*  cursor is not at end, or index is not -1, need memove */
		memmove(_this_console->dispbuf + tmp + 1, 
				_this_console->dispbuf + tmp,
					_this_console->dendpos - tmp);
	}

	/*  put the char to index, buf index is ok */
	_this_console->last_endpos = _this_console->dendpos;
	_this_console->dispbuf[tmp] = c;
	_this_console->curpos ++;
	_this_console->dendpos ++;
	return 0;
}

static int _put_s_to_dispbuf(char *s, int len, int index)
{
	int tmp = _this_console->curpos + index;
	if( !s || !len )
		return -1;

	if( (tmp < _this_console->dstartpos) || (tmp > _this_console->dendpos ) )
		return -1;

	if( ( (tmp+len) > _this_console->buf_size) ||
		(_this_console->dendpos+len >= _this_console->buf_size)	)
	{
		return -1;
	}

	_this_console->last_endpos = _this_console->dendpos;
	if( _this_console->curpos != _this_console->dendpos )
		memmove(_this_console->dispbuf+tmp+len, _this_console->dispbuf+tmp, _this_console->dendpos-tmp);
	memcpy(_this_console->dispbuf + tmp, s, len);
	_this_console->dendpos += len;
	_this_console->curpos += len;
	return 0;
}

static int _del_c_from_dispbuf(int index)
{
	int tmp = _this_console->curpos + index;
	if( (tmp < _this_console->dstartpos) || (tmp > _this_console->dendpos ) )
		return -1;

	if( _this_console->dendpos == 0 )
		return 0;

	if( !(_this_console->curpos == _this_console->dendpos) )
	{
	/*  cursor is not at the end of line */
	/*  cursor is not at end, or index is not -1, need memove */
		memmove(_this_console->dispbuf + tmp, 
				_this_console->dispbuf + tmp+1,
					_this_console->dendpos - tmp);
	}

	_this_console->last_endpos = _this_console->dendpos;
	_this_console->curpos--;
	_this_console->dendpos--;
	return 0;
}

static int _del_s_from_dispbuf(int index, int len)
{
	int tmp = _this_console->curpos + index;
	if( len <= 0 )
		return 0;

	if( (tmp < _this_console->dstartpos) || (tmp > _this_console->dendpos ) )
		return -1;

	if( _this_console->dendpos == 0 )
		return 0;

	if( (tmp+len) > _this_console->dendpos )
		len = _this_console->dendpos - tmp;

	if( _this_console->dendpos != (tmp+len) )
	{
		/* need memmove */
		memmove(_this_console->dispbuf+tmp, _this_console->dispbuf+tmp+len, 
				_this_console->dendpos-tmp-len);
	}

	if ( len == 1 )
		return _del_c_from_dispbuf(index);

	/*  to do  */
	_this_console->last_endpos = _this_console->dendpos;
	_this_console->dendpos -= len;
	return 0;
}

int console_init(console_t **console, int buf_size)
{
	int ret = -1;
	console_t *con = NULL;
	int fd = 0;

	if( console == NULL )
		return -1;

	if( _this_console )
	{
		*console = _this_console;
		return 0;
	}

	/* allocat console , buf, disp buf */

	if( buf_size < 64 || buf_size > 1024)
		buf_size = 512;
	con = (console_t*) MALLOC ( sizeof(console_t));
	if( con == NULL )
		goto OUT;
	memset(con, 0, sizeof(console_t));
	con->buf = (unsigned int*)MALLOC(INPUT_BUF_SIZE*sizeof(unsigned int));
	if( con->buf == NULL )
		goto OUT;
	memset(con->buf, 0, INPUT_BUF_SIZE*sizeof(unsigned int));

	con->buf_size = buf_size;
	con->dispbuf = (char *)MALLOC(buf_size);
	if( con->dispbuf == NULL )
		goto OUT;
	memset(con->dispbuf, 0, buf_size);

	_s_pbuf = (char *)MALLOC(512);
	if( _s_pbuf == NULL )
		goto OUT;
	memset(_s_pbuf, 0, 512);

	
	/* init console */
	fd = open("/dev/tty", O_RDWR);
	if( fd < 0 )
		goto OUT;
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	/*  set console attr */
	set_tty_attr(fd);
	con->ttyfd = fd;

	/*  init get,print,init_dispbuf,reset_dispbuf,printbuf */
	con->get_key = _get_key;
	con->print = _con_print;
	con->init_dispbuf = _init_dispbuf;
	con->reset_dispbuf = _reset_dispbuf;
	con->put_c_to_disp = _put_c_to_dispbuf;
	con->put_s_to_disp = _put_s_to_dispbuf;
	con->del_c_from_disp = _del_c_from_dispbuf;
	con->del_s_from_disp = _del_s_from_dispbuf;
	con->set_corsor = _set_corsor;
	con->set_corsor_to_end = _set_corsor_to_end;
	con->set_corsor_to_start = _set_corsor_to_start;
	con->get_corsor = _get_corsor;
	con->is_dispbuf_empty = _is_dispbuf_empty;

	con->exit = console_exit;
	*console = con;
	_this_console = con;
	return 0;

	/* ERROR exit */
OUT:
	console_exit(con);
	return ret;
}

int console_exit(console_t *console)
{
	if( console != _this_console )
		return -1;

	if( _this_console )
	{
		/* roll back console info */
		if( _this_console->ttyfd )
		{
			clear_tty_attr(_this_console->ttyfd);
			close(_this_console->ttyfd);
			_this_console->ttyfd = 0;
		}

		if( console->buf )
			FREE( console->buf );

		if( console->dispbuf )
			FREE( console->dispbuf );

		FREE( console );
		_this_console = NULL;
	}

	if( _s_pbuf )
	{
		FREE(_s_pbuf);
		_s_pbuf = NULL;
	}

	return 0;
}

void dbg_console(console_t *console)
{
	if( console == NULL )
	{
		printf("\nDBG: console is NULL\n");
		return;
	}

	printf("\nDBG: dstartpos=%d  dendpos=%d  curpos=%d\n",
			console->dstartpos, console->dendpos, console->curpos);
	if( console->dstartpos < console->dendpos )
	{
		int i=0;
		for (i=console->dstartpos; i<console->dendpos; i++)
		{
			printf("DBG:index=%d data=0x%x\n", i, console->dispbuf[i]);
		}
	}
}

/* 
 * clear_tty_attr
 */
void clear_tty_attr(int fd)
{
	struct termios term;

	tcgetattr(fd, &term);
	term.c_iflag |= IXON;
	term.c_iflag |= IXOFF;
	term.c_lflag |= ISIG;
	tcsetattr(fd, TCSAFLUSH, &term);
}

void set_tty_attr(int fd)
{
	struct termios term;
	tcgetattr(fd, &term);
	term.c_iflag &= ~IXON;	/* c_iflag ctrl + q */
	term.c_iflag &= ~IXOFF;	/* c_iflag ctrl + s */          
	term.c_lflag &= ~ISIG;	/* c_lflag ctrl+z ctrl+c ctrl+\ */
	tcsetattr(fd, TCSAFLUSH, &term);
}
