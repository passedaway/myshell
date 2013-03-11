/*
 * =====================================================================================
 *
 *       Filename:  shell.c
 *
 *    Description:  implement shell.h
 *
 *        Version:  1.0
 *        Created:  2012年11月22日 17时15分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhao, Changqing (NO), changqing.1230@163.com
 *        Company:  iPanel TV inc.
 *
 * =====================================================================================
 */
#include "os.h"		/* include debug define */
#include "console.h"
#include "command.h"
#include "jobs.h"
#include "env.h"

#if 0
#include "history.h"
#endif

#include "complete.h"
#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/prctl.h>

/* *********************key defention ************************** */
#ifdef LITTLE_ENDIAN
/*  little endian(x86,armle,mipsel,etc.) */
#define F1	0x504f1b
#define F2	0x514f1b
#define F3	0x524f1b
#define F4	0x534f1b
#define F5	0x35315b1b
#define F6	0x37315b1b
#define F7	0x38315b1b
#define F8	0x39315b1b
#define F9	0x30325b1b
#define F10	0x31325b1b
#define F11	0x33325b1b
#define F12	0x34325b1b
/* arrowhead left,right,up,down, delete */
#define AU	0x415b1b
#define AD	0x425b1b
#define AR	0x435b1b
#define AL	0x445b1b
#define DEL	0x7e335b1b
#else
/*  big endian(sparc,armbe, mipsbe,etc.) */
/*  if the bytes is 00, that's the last byte is 0 */
#define F1	0x1b4f5000
#define F2	0x1b4f5100
#define F3	0x1b4f5200
#define F4	0x1b4f5300
#define F5	0x1b5b3135
#define F6	0x1b5b3137
#define F7	0x1b5b3138
#define F8	0x1b5b3139
#define F9	0x1b5b3230
#define F10	0x1b5b3231
#define F11	0x1b5b3233
#define F12	0x1b5b3234
/* arrowhead left,right,up,down, delete */
#define AU	0x1b5b4100
#define AD	0x1b5b4200
#define AR	0x1b5b4300
#define AL	0x1b5b4400
#define DEL	0x1b5b337e
#endif

#define BACKSPCAE	0x7F
/*  ctrl+j == Enter */
#ifdef CTRL
#undef CTRL
#endif
#define CTRL(ch)	(ch-'A'+1)
#define CAN_PRINTF(ch) 	((ch>=0x20) && (key<=0x7E))

/*****************************  module data************************** */

static shell_cfg_t _def_cfg = {
	.prompt = "mysh",
	.use_color = 0, 
	.has_env = 1,
};

typedef struct shell_data_s{
	shell_cfg_t *cfg;
	console_t *console;
	cmd_t *cmd;
	job_t *jobs;
	env_t *env;
#if 0
	history_t *history;
#endif

	pthread_t task_id;
	int task_run;
	int init;
}shell_data_t;
static shell_data_t  gd;
static pid_t cur_term_pgid;

static int waitchild(int *pid, int *status, int flag);
static void process_waitchild_result(\
		cmd_line_t *cmd_line, pid_t pid,\
		int rettype, int status, int from_dofg);
static int give_terminal_to(int fd, pid_t pgrp);
static void signal_handler(int signo);
static void sigchild_handler(int signo);
static void init_sig(void);
static void clear_sig(void);
static void do_cmd_in_child(cmd_t *cmd, cmd_line_t *cmd_line);
static void* _shell_task(void* args);
static void _do_fg(job_t *);

int shell_init(const shell_cfg_t * cfg)
{
	int ret = 0;
	shell_cfg_t *_cfg = (shell_cfg_t *)cfg;
	if(cfg == NULL)
	{
		/*  default config */
		_cfg = &_def_cfg;
	}
	ret = console_init(&(gd.console), 256);
	if( ret )
	{
		printf("error: console init ret=%d\n", ret);
		return -1;
	}

	/*  init command */
	gd.cmd = cmd_init();

	/*  init jobs manager */
	gd.jobs = job_init();

	/*  init env manager */
	gd.env = env_init();

	(gd.console)->prompt = _cfg->prompt;

	/* register signal hander */
	init_sig();

	gd.cfg = _cfg;
	gd.init = 1;
	return 0;
}

int shell_exit()
{
	/* thread_exit */
	shell_stop(1);

	/* console exit */
	console_exit(gd.console);

	/*  command exit */
	cmd_exit(gd.cmd);
	
	/* jobs exit */
	job_exit(gd.jobs);

	/* env exit */
	env_exit(gd.env);

	gd.init = 0;
	gd.cfg = NULL;

	return 0;
}

int shell_start()
{
	gd.console->print(0, "");
	gd.task_run = 1;
	if( gd.cfg->is_in_pthread )
		pthread_create(&gd.task_id, NULL, &_shell_task, (void *)(gd.console));
	else
		_shell_task(NULL);

	return 0;
}

int shell_stop(int force)
{
	if( gd.cfg->is_in_pthread )
	{
		gd.task_run = 0;
		pthread_cancel(gd.task_id);
	}
	
	return 0;
}

int shell_isrunning()
{
	if( gd.cfg->is_in_pthread )
		return gd.init & gd.task_run;
	else
		return 0;
}

static int _shell_register_cmd(int id,
		const char *name, 
		const char *usage, 
		int (*func)(int argc, char **argv) )
{
	if( name && func )
	{
		cmd_t *cmd = NULL;
		/*  check if already has this cmdname */
		cmd = cmd_find_cmd_by_name(gd.cmd, name);
		if( cmd != NULL )
			return -1;

		/*  allocate memory, then init cmd  */
		cmd = cmd_new(id, name, usage, func);
		if( cmd == NULL )
			return -1;

		/*  register to the cmd list(queue) */
		cmd_register_cmd(gd.cmd, cmd);
		return 0;
	}

	return -1;

}

int shell_register_cmd(
		const char *name, 
		const char *usage, 
		int (*func)(int argc, char **argv) )
{
	return _shell_register_cmd(-1, name, usage, func);
}

int shell_register_buildin_cmd(const char *name, const char *usage, int (*func)(int argc, char **argv) )
{
	return _shell_register_cmd(USER_BUILDIN_CMD, name, usage, func);
}


int shell_do_command(cmd_line_t *cmd_line)
{
	int ret = -1;
	cmd_t *cmd = NULL;

	if( cmd_line == NULL )
		return -1;

	cmd = cmd_find_cmd_by_name(gd.cmd, cmd_line->argv[0]);
	if( cmd == NULL )
	{
#if 0
		/*  find cmd in other module */
		/*  if command is null, in do_cmd_in_child function, it will in child do it, 
		 *  such as: fork, then execv
		 */
		gd.console->print(NFNP, "%s: command not found", cmd_line->argv[0]);
		ret = -2;
		goto out;
#endif
	}


	/* cmd is NULL : do_cmd_in_child will do it use exec */
	if( !cmd || (cmd->id > STATIC_CMD) )
	{
		/*  do command in child, do release or in job in parrent */
		do_cmd_in_child(cmd, cmd_line);
	}
	else
	{
		/*  just do it (NIKE) */
		ret = cmd->func(cmd_line->argc, cmd_line->argv);

		/*  release this command line */
		/* if in child , the wait process status will do this */
		if( cmd_line )
		{
			cmd_line_delete(cmd_line);
			cmd_line = NULL;
		}

		if( ret == 0xA5 )
			return ret;
	}

	/* process more then one cmd */
//	shell_do_command(cmd_line->next);

	gd.console->print(JP, "");
	gd.console->reset_dispbuf();
	return ret;
}

/*
 * give terimal to the specil process
 */
int give_terminal_to(int fd, pid_t pgrp)
{
	sigset_t set, oldset;
	int ret = 0;

	sigemptyset(&set);
	sigaddset(&set, SIGTTOU);
	sigaddset(&set, SIGTTIN);
	sigaddset(&set, SIGTSTP);
	sigaddset(&set, SIGCHLD);

	sigprocmask(SIG_BLOCK, &set, &oldset);
	if( tcsetpgrp(fd, pgrp) < 0 )
	{
		printf("error:%s:fd=%d pgrp=%d tcgetattr:%s\n", __FUNCTION__, \
				fd, pgrp, strerror(errno));
		ret = -1;
	}

	sigprocmask(SIG_SETMASK, &oldset, NULL);

	cur_term_pgid = pgrp;
	return ret;
}


/* 
 * flag 
 * 0	normal
 * 1	wait for stop
 * 2	wait for continue & nowait
 * ret 
 * 1	normal exit, status = exit value
 * 2	stoped, status = stop value
 * 3	continued, stauts = continued status
 * 4	othre	stauts = status
 */
int waitchild(int *pid, int *status, int flag)
{
	int s = 0;
	int ret = 0;
	siginfo_t sigs;

	if( flag == 1 )
		waitpid(*pid, &s, WUNTRACED);
	else if (flag == 2)
	{
	//	waitpid(pid, &s, WCONTINUED | WNOWAIT);
		waitid(P_ALL, 0, &sigs, WCONTINUED | WNOWAIT | WNOHANG);
	}
	else 
		waitpid(*pid, &s, 0);

	if( flag == 2 )
	{
		ret = 3;
		*pid = sigs.si_pid;
		s = sigs.si_status;
		DBG_Print("waitpid: waitid2 sigs.si_pid=%d si_status=%d\n",\
				sigs.si_pid, sigs.si_status);
	} 
	
	if (WIFCONTINUED(s) )
	{
		ret = 3;
		*status = s;
	}
	else if( WIFEXITED(s) )
	{
		ret = 1;
		*status = WEXITSTATUS(s);
	}else if( WIFSTOPPED(s) ){
		ret = 2;
		*status = WSTOPSIG(s);
	}else{
		ret = 4;
		*status = s;
	}

	DBG_Print("pid=%d stauts=%d 0x%x\n",  *pid, s, s);
	return ret;
}

void process_waitchild_result(
		cmd_line_t *cmd_line,
		pid_t pid, 
		int rettype, 
		int status, 
		int from_dofg)
{	
	int delete_cmdline = 1;
	
	/*  normal exit */
	if( rettype == 1 )
	{
		if( status == 0x5A )
		{
			gd.console->print(JPB, "");
			gd.console->print(JPF, ": command not found.");
			gd.console->print(JNP, "");
		}else
			gd.console->print(NFNP, "cmd normal exit. ret=%d", status);
	}
	/* child stoped */
	else if(rettype == 2){
		/*  parrent add it to jobs */
		int id = 0;
		if( !from_dofg )
		{
			id = job_in(gd.jobs, pid, cmd_line);
			delete_cmdline = 0;
			gd.console->print(JPF, "\n[%d]+ stoped\t\t%s", id, cmd_line->cmdline);
		}else{
			job_t *job = NULL;
			job = job_find_by_pid(gd.jobs, pid);
			if( job )
				gd.console->print(JPF, "\n[%d]+ stoped\t\t%s", \
						job->id, job->cmd_line->cmdline);
			else
				/*  should never come here */
				gd.console->print(JPF, "\nerror: no such job\n");
		}
		gd.console->print(JNP, "");
	}
	/* child abnormal */
	else{
		gd.console->print(NFNP, "cmd abnormal exit. ret=%d", status);
	}

	/* come from fg, check this */
	if( from_dofg && (rettype != 2) )
	{
		/* job out from job queue */
		job_out(gd.jobs, pid);
	}

	if( !from_dofg && delete_cmdline )
		if( cmd_line )
			cmd_line_delete(cmd_line);
}

void signal_handler(int signo)
{
	DBG_Print("pid=%d signo=%d\n", getpid(), signo);
	if( signo == SIGINT )
	{
		gd.console->print(JPF,"^C");
		gd.console->reset_dispbuf();
		gd.console->print(JNP,"");
		gd.console->exit(gd.console);
		exit(0);
		return;
	}

	if( signo == SIGUSR1 || signo == SIGUSR2 )
	{
		printf("\n signal = %d",signo);
		return;
	}

	printf("\n unprocess signal : signo=%d exit.\n", signo);
	exit(0);
}

void sigchild_handler(int signo)
{
	pid_t pid;
	int status, rettype;

	DBG_Print("\nsignal child received\n");
	if( cur_term_pgid == getpid() )
	{

		rettype = waitchild(&pid, &status, 2);
		DBG_Print("rettype=%d pid=%d status=%d\n", rettype, pid, status);
		/* no child has be continued */
		if( pid == 0 )
			return;
		else{
			/* child has CONTINUED by other process,
			 * but it still "stoped" in this shell data
			 * so, the shell re-stopped it
			 * printf("child %d  statux = %d has continued by other process."
			 * "just re-stop it.\n", pid, status );
			 */
			kill(pid, SIGTSTP);
		}
	}
}

void init_sig(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGCONT, signal_handler);
	signal(SIGCHLD, sigchild_handler);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
}

void clear_sig(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGCONT, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
}

void do_cmd_in_child(cmd_t *cmd, cmd_line_t *cmd_line)
{
	pid_t child_pid;
	int ret = 0;

	/*  fork, then child do cmd & exit */
	child_pid = fork();
	if( child_pid < 0 )
	{
		printf("error:system error. %s\n", strerror(errno));
	}
	else if( child_pid == 0 )
	{
		/*  child process */
		pid_t thispid = getpid();

		/*  new process group */
		setpgid(thispid, thispid);
		
		/*  get tty control */
		give_terminal_to(gd.console->ttyfd, thispid);

		/*  reset signal */
		clear_sig();

		/* reset tty mode */
		clear_tty_attr(gd.console->ttyfd);

		/*  now do child process */
		if( cmd )
		{
#if 0
			/*  replace commandname in kernel */
			prctl(PR_SET_NAME, "child", NULL, NULL, NULL);
#endif
			/*  just do cmd func ,may by call exec */
			ret = cmd->func(cmd_line->argc, cmd_line->argv);
			exit(ret);
		}else{
			/* exec */
#if 0
			int ret = 
#endif
			execvp(cmd_line->argv[0], cmd_line->argv);
#if 0
			if( ret ==  -1 )
			{
				//printf("error: %s\n", strerror(errno));
				gd.console->print(NFNP, "%s: command not found", cmd_line->argv[0]);
			}
#endif
			/* incase of error, command not found */
			exit(0x5A);
		}
	}
	else
	{
		/*  parrent  */
		int status, type;

		/*  new process group, same with child , becase cannot determian which be run first*/
		setpgid(child_pid, child_pid);

		/*  wait child */
		type = waitchild(&child_pid, &status, 1);

		/*  control terminal */
		give_terminal_to(gd.console->ttyfd, getpid());

		/*  reset tty attr */
		set_tty_attr(gd.console->ttyfd);

		/*  process type  1 2 */
		process_waitchild_result(cmd_line, child_pid, type, status, 0);
	}

	/*  incase of child didnot exit */
	if( child_pid == 0 )
		exit(0);
}


static void _do_match(void)
{
	char tmp[256];
	int ret ;

	int match_chose = 0;
	/* need find the flag */
	/*
	 * flag
	 * 0	command
	 * 1	path or filename
	 * 2	env
	 */
	/* need get flag & the pattern */
	{
		memcpy(tmp,\
				gd.console->dispbuf+gd.console->dstartpos,\
				gd.console->curpos-gd.console->dstartpos  );
		tmp[gd.console->dstartpos + gd.console->curpos] = 0;
		match_chose = 0;
	}

	/*  then match */
	ret = complete_try(tmp, match_chose);
	//printf("ret = %d\n",ret);
	
	/* analyse resutl */
	/*
	 * ret < 1		no matched
	 * ret == 1		just match one
	 * ret > 1		match more
	 */
	/* didnot matched */
	if( ret < 1 )
	{
		return;
	}
	/* only one matched */
	else if( ret == 1 )
	{
		int len = 0;
		char *ctmp = complete_get_entry(tmp, 0, &len);
		gd.console->put_s_to_disp(ctmp, len, 0);
		gd.console->print(RPPB, "");
	}
	/* matched many */
	else{
		char *res;
		int retlen = complete_get_almost_likely(tmp, &res);
		//printf("\nretlen=%d res=%s\n",retlen, res);
		/*
		 * try to almost likeley
		 * ret < 0 	: error - should not call this function
		 * ret == 0 : didnot have almost likely
		 * ret > 0  : almost likely len is ret, string address in res
		 */
		if( retlen < 0 )
		{
			/* should never come here, some thing error */
			return;
		}
		else if( retlen == 0 )
		{
			/* bash will display them ,or print display them all? */
			int i = 0;
			gd.console->print(JPF,"\n");
			for( ; i < ret; i++)
				gd.console->print(JPF, "%s%c",\
						complete_get_entry(tmp, 1, NULL), \
						(i+1)%5?'\t':'\n' );

			if( i%5 )
				gd.console->print(JPF, "\n");
			
		}else{
			/* almost likely string is res, len is retlen, copy to dispbuffer */
			gd.console->put_s_to_disp(res, retlen, 0);
		}

		gd.console->print(RPPB, "");
	}

	/* complete over */
	complete_over();
}

void* _shell_task(void* args)
{
	int key = 0;
	int lock_screen = 0;
	int cmd_ret = 0;
	int q_nums=0;	/* if press q four times , then quit */

	/*  do shell */
	while( gd.task_run )
	{
		key = gd.console->get_key();

		/*  for ctrl + s & ctrl + q */
		if( lock_screen == 1 )
		{
			if( key == CTRL('Q') )
				lock_screen = 0;

			continue;
		}

		/* for quit */
		if( key == 'q' )
		{
			if( q_nums == 0 )
				if( gd.console->get_corsor()==0)
					q_nums++;
				else
					q_nums = 0;
			else
				q_nums++;

			if( q_nums == 4 )
			{
				gd.task_run = 0;
				gd.console->print(JPF, "\b\b\bquit.\n");
				break;
			}
		}

		switch( key )
		{
			case '\t':
				/*  do match */
				/* just do some print & maybe modify display buf. NOT Do other things */
				_do_match();
				continue;

			case '\n':
			case '\r':
				{
#if 0
				/* this all fake processas */
#if 1
				gd.console->print(NBF, ": command not found");
				/*  print promt */
				gd.console->print(JPP, "");
				gd.console->reset_dispbuf();
#elif 0
				/*  this is same with, 3 + 0 */
				gd.console->print(NBFNP, ":command not found");
				gd.console->reset_dispbuf();
#else
				gd.console->print(NBFNPB, ":match table is more...");
				/*  this is not reset dispbuf */
#endif

#endif
				/* do cmds */
				/* now it's time to realize my command module 2012-11-23 Fri 18:29:35 CST */
					if( gd.console->is_dispbuf_empty() )
					{
						gd.console->print(JNP, "");
						continue;
					}

					{
						cmd_line_t *cmd_line = NULL;
						int retcode=0;

						gd.console->dispbuf[gd.console->dendpos+1] = 0;
						cmd_line = cmd_line_new(gd.console->dispbuf+gd.console->dstartpos, \
											gd.console->dendpos-gd.console->dstartpos, &retcode );
						if( cmd_line == NULL )
						{
							/*
							 * should process 
							 * quotes : ' , "
							 * braces : {
							 * brackets : [
							 * parentheses : (
							 * 
							 */
							char *_tmpbuf = (char*)MALLOC(retcode+1);
							memset(_tmpbuf, ' ', retcode);
							_tmpbuf[retcode] = 0;
							/* command line not construct success */
							gd.console->print(NBFNP,"\n%s^:syntax error", _tmpbuf);
							gd.console->reset_dispbuf();
							FREE(_tmpbuf);

							continue;
						}

						/* do the cmd */
						gd.console->print(JPF, "\n");
						cmd_ret = shell_do_command(cmd_line);
						if( cmd_ret == 0xA5)
						{
							gd.task_run = 0;
							return NULL;
						}
					}
				//gd.console->print(JNP, "");
					continue;
				}

			case F1:
			case F2:
			case F3:
			case F4:
			case F5:
			case F6:
			case F7:
			case F8:
			case F9:
			case F10:
			case F11:
			case F12:
				continue;
				
			//case CTRL('A'):/* set corsor to start */
			//case CTRL('B'):/* move corsor back */
			//case CTRL('C'):/*  print ^C then print promte */
			//case CTRL('D'):/* do delete */
			//case CTRL('E'):/* set corsor to end */
			//case CTRL('F'):/*  move corsor forward */
			case CTRL('G'):
			//case CTRL('H'):/* do backspace */
			//case CTRL('I'):/* it is \t */
			//case CTRL('J'):/* it is \n */
			//case CTRL('K'):/* delete data from cursor to end  */
			case CTRL('L'):/*  bash:clear this screen */
			//case CTRL('M'):/* it is \r */
			//case CTRL('N'):/* next command */
			case CTRL('O'):/* in bash, it do enter, but i do nothing */
			//case CTRL('P'):/* prevous command */
			//case CTRL('Q'):/* in bash, unlock screen, i re-do it, termios.c_lflag */
			case CTRL('R'):/* in bash, it do search in history */
			//case CTRL('S'):/* in bash, lock screen , i re-do it */
			case CTRL('T'):/* bash do: swap the cursor char with the previous */
			//case CTRL('U'):/* cancel line */
			case CTRL('V'):
			case CTRL('W'):
			case CTRL('X'):
			//case CTRL('Y'):
			case CTRL('Z'):
				continue;

			case CTRL('Y'): /* for test */
				dbg_console(gd.console);
				gd.console->put_s_to_disp("abcd", 4, 0);
				dbg_console(gd.console);
				gd.console->print(RPPB, "");
				break;

				/*  process ctrl + XX */

			case CTRL('C'):
				gd.console->print(JPF,"^C");
				gd.console->reset_dispbuf();
				gd.console->print(JNP,"");
				continue;

			case CTRL('S'):/* this is need modify terimnal attr.
							  stty -a  to see the reason. 
							  in bash, lock screen , i do it*/
				lock_screen = 1;
			case CTRL('Q'):/* in bash, unlock screen, i can do it*/
				continue;

			case CTRL('U'):/* cancel line */
				gd.console->reset_dispbuf();
				gd.console->print(RPPB, "");
				continue;

			case CTRL('D'):
			case DEL:
				/* move corsor right */
				if( gd.console->set_corsor(1) )
				/* if corsor is at end, then continue */
					continue;

				/*  then delete */
			case CTRL('H'):
			case BACKSPCAE:
				/*  do delete  */
				gd.console->del_c_from_disp(-1);
				/*  then printf */
				//gd.console->print(2,"\b \b");
				gd.console->print(RPPB,"");
				//dbg_console(gd.console);
				continue;

			case CTRL('K'):/* delete data from cursor to end  */
				//dbg_console(gd.console);
				gd.console->del_s_from_disp(0, gd.console->dendpos - gd.console->curpos);
				//dbg_console(gd.console);
				gd.console->print(RPPB, "");
				continue;

			case AR:
			case CTRL('F'):
				key = 1;
			case AL:
			case CTRL('B'):
				gd.console->set_corsor(key==1?1:-1);
				gd.console->print(RPPB,"");
				continue;

			case CTRL('A'):
				gd.console->set_corsor_to_start();
				gd.console->print(RPPB,"");
				continue;

			case CTRL('E'):
				gd.console->set_corsor_to_end();
				gd.console->print(RPPB,"");
				continue;

			case CTRL('P'):
			case AU:
				/*  enable history mode, then list prev command line*/
				continue;

			case CTRL('N'):
			case AD:
				/* if in history mode, then list next command line*/
				continue;

			default:
				break;
		}

		/*  incase of some special key 2012-12-03 11:05:43 CST */
		if( !CAN_PRINTF(key) )
			continue;

		/* put this key to display buf */
		gd.console->put_c_to_disp(key, 0);

#if 0
		gd.console->print(2, "%c", key);
#else
		gd.console->print(RPPB, "");
#endif
	}

	return NULL;
}

void _do_fg(job_t *job)
{
	int status = 0;
	int rettype = 0;
	int pid = job->pid;


	/* child & parrent use one tty, so here need clear tty attr */
	clear_tty_attr(gd.console->ttyfd);
	/*  give terminal to child pid */
	give_terminal_to(gd.console->ttyfd, pid);

	/* send SIGCONT to child */
	kill(pid, SIGCONT);

	/* now child will run */
	/*  wait child */
	rettype = waitchild(&pid, &status, 1);

	/*  control terminal */
	give_terminal_to(gd.console->ttyfd, getpid());

	/*  set tty attr */
	set_tty_attr(gd.console->ttyfd);

	/*  process type  1 2 */
	process_waitchild_result(job->cmd_line, pid, rettype, status, 1);
}

/* **********from here they are some commands build in shell ********** */
/* such as: fg, jobs, exit, help
 * these are close with the shell'gd data, so I put them implemention to here
 * fg,jobs	such fork, gd.jobs, and terminal control
 * exit		its return value, and need check stoped jobs
 * help		show all register cmd, gd.cmds
 *
 * such as: cd , it also build in shell ,but its implemention no need to put here,
 * actually, it's in build_in_cmds/register.c
 * ************************ some commands realize ********************* */
/*  cmd :fg */
int do_fg(int argc, char **argv)
{
	int id = -1;
	int len ;
	job_t *job = NULL;

	len = job_count(gd.jobs);
	if( argc > 1 )
	{
		id = atoi(argv[1]);
		if( id > len )
		{
			goto err;
		}
	}

	if ( id != -1 )
		job = job_find_by_id(gd.jobs, id);
	else
	{
		int i = 1;
		for(; i <= len; i++)
		{
			job = job_find_by_id(gd.jobs,i);
			if( job )
				break;
		}
	}
	if( !job )
		goto err;

	_do_fg(job);

	return 0;

err:
	printf("fg: %d: no such job.\n", id);
	return 0;
}

/*  cmd :jobs */
int do_jobs(int argc, char **argv)
{
	int id = -1, i = 0, len = 0;
	job_t *_job;

	if ( argc > 1 )
		id = atoi(argv[1]);
	
	if ( id != -1 )
	{
		_job = job_find_by_id(gd.jobs, id);
		if( _job )
			printf("[%d]+ stoped\t\t%s\n", _job->id, _job->cmd_line->cmdline);
		else
			printf("%d\tno such job has stopped\n", id);

		return 0;
	}

	len = job_count(gd.jobs);
	for(i=1; i <= len; i++)
	{
		_job = job_find_by_id(gd.jobs, i);
		if( _job )
			printf("[%d]+ stoped\t\t%s\n", _job->id, _job->cmd_line->cmdline);
	}

	return 0;
}

/*  cmd :exit */
int do_exit(int argc, char **argv)
{
	printf("exit\n");
	if( job_count(gd.jobs) == 0 )
		return 0xA5;
	else
		gd.console->print(JPF, "shell has stoped jobs.\n");

	return 0;
}

/*  cmd :help */
int do_help(int argc, char **argv)
{
	queue_t *tmp = NULL;
	cmd_t *cmd = gd.cmd;
	if( cmd == NULL )
	{
		printf("No commands.\n");
		return -1;
	}

	if( argc > 1 )
	{
		tmp = (queue_t*)cmd_find_cmd_by_name(cmd, argv[1]);
		if( tmp == NULL )
		{
			printf("help: %s : no such entry.\n", argv[1]);
			return 0;
		}
		cmd_print_usage((cmd_t*)tmp);
		return 0;
	}

	q_foreach((queue_t*)cmd, tmp)
	{
		if( tmp )
		{
			cmd_print_usage((cmd_t*)tmp);
			printf("\n");
		}
	}

	return 0;
}

