// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/mmo.h"
#include "../common/showmsg.h"
#include "../common/malloc.h"
#include "core.h"
#ifndef MINICORE
#include "../common/db.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/thread.h"
#include "../common/mempool.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include "../common/winapi.h" // Console close event handling
#endif


/// Called when a terminate signal is received.
void (*shutdown_callback)(void) = NULL;

#if defined(BUILDBOT)
	int buildbotflag = 0;
#endif

int runflag = CORE_ST_RUN;
int arg_c = 0;
char **arg_v = NULL;

char *SERVER_NAME = NULL;
char SERVER_TYPE = ATHENA_SERVER_NONE;

static char rA_git_version[50] = "";
static char rA_svn_version[10] = "";

#ifndef MINICORE	// minimalist Core
// Added by Gabuzomeu
//
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//
#ifdef WIN32	// windows don't have SIGPIPE
#define SIGPIPE SIGINT
#endif

#ifndef POSIX
#define compat_signal(signo, func) signal(signo, func)
#else
sigfunc *compat_signal(int signo, sigfunc *func)
{
	struct sigaction sact, oact;

	sact.sa_handler = func;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
#ifdef SA_INTERRUPT
	sact.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

	if (sigaction(signo, &sact, &oact) < 0)
		return (SIG_ERR);

	return (oact.sa_handler);
}
#endif

/*======================================
 *	CORE : Console events for Windows
 *--------------------------------------*/
#ifdef _WIN32
static BOOL WINAPI console_handler(DWORD c_event)
{
    switch(c_event)
    {
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
		if( shutdown_callback != NULL )
			shutdown_callback();
		else
			runflag = CORE_ST_STOP;// auto-shutdown
        break;
	default:
		return FALSE;
    }
    return TRUE;
}

static void cevents_init()
{
	if (SetConsoleCtrlHandler(console_handler,TRUE)==FALSE)
		ShowWarning ("Não foi possível instalar o manipulador do console!\n");
}
#endif

/*======================================
 *	CORE : Signal Sub Function
 *--------------------------------------*/
static void sig_proc(int sn)
{
	static int is_called = 0;

	switch (sn) {
	case SIGINT:
	case SIGTERM:
		if (++is_called > 3)
			exit(EXIT_SUCCESS);
		if( shutdown_callback != NULL )
			shutdown_callback();
		else
			runflag = CORE_ST_STOP;// auto-shutdown
		break;
	case SIGSEGV:
	case SIGFPE:
		do_abort();
		// Pass the signal to the system's default handler
		compat_signal(sn, SIG_DFL);
		raise(sn);
		break;
#ifndef _WIN32
	case SIGXFSZ:
		// ignore and allow it to set errno to EFBIG
		ShowWarning ("Tamanho máximo do arquivo alcançado!\n");
		//run_flag = 0;	// should we quit?
		break;
	case SIGPIPE:
		//ShowInfo ("Broken pipe found... closing socket\n");	// set to eof in socket.c
		break;	// does nothing here
#endif
	}
}

void signals_init (void)
{
	compat_signal(SIGTERM, sig_proc);
	compat_signal(SIGINT, sig_proc);
#ifndef _DEBUG // need unhandled exceptions to debug on Windows
	compat_signal(SIGSEGV, sig_proc);
	compat_signal(SIGFPE, sig_proc);
#endif
#ifndef _WIN32
	compat_signal(SIGILL, SIG_DFL);
	compat_signal(SIGXFSZ, sig_proc);
	compat_signal(SIGPIPE, sig_proc);
	compat_signal(SIGBUS, SIG_DFL);
	compat_signal(SIGTRAP, SIG_DFL);
#endif
}
#endif

const char* get_git_revision(void)
{
	FILE *fp;

	if (*rA_git_version)
		return rA_git_version;

	if ((fp = fopen(".git/refs/heads/master", "r")) != NULL) {
		char line[64];
		char* rev = malloc(sizeof(char)*50);
		if (fgets(line, sizeof(line), fp) && sscanf(line, "%s", rev))
			snprintf(rA_git_version, sizeof(rA_git_version), "%s", rev);
		free(rev);
		fclose(fp);
	} else {
		snprintf(rA_git_version, sizeof(rA_git_version), "no");
	}
	
	if (!(*rA_git_version)) {
		snprintf(rA_git_version, sizeof(rA_git_version), "Desconhecido");
	}

	return rA_git_version;
}

const char* get_svn_revision(void)
{
	FILE *fp;

	if(*rA_svn_version)
		return rA_svn_version;

	if ((fp = fopen(".svn/entries", "r")) != NULL)
	{
		char line[1024];
		int rev;
		// Check the version
		if (fgets(line, sizeof(line), fp))
		{
			if(!ISDIGIT(line[0]))
			{
				// XML File format
				while (fgets(line,sizeof(line),fp))
					if (strstr(line,"revisão=")) break;
				if (sscanf(line," %*[^\"]\"%d%*[^\n]", &rev) == 1) {
					snprintf(rA_svn_version, sizeof(rA_svn_version), "%d", rev);
				}
			} else {
				// Bin File format
				bool fgresult;
				fgresult = ( fgets(line, sizeof(line), fp) != NULL ); // Get the name
				fgresult = ( fgets(line, sizeof(line), fp) != NULL ); // Get the entries kind
				if(fgets(line, sizeof(line), fp)) // Get the rev numver
				{
					snprintf(rA_svn_version, sizeof(rA_svn_version), "%d", atoi(line));
				}
			}
		}
		fclose(fp);
	} else {
		snprintf(rA_svn_version, sizeof(rA_svn_version), "no");
	}
	/**
	 * subversion 1.7 introduces the use of a .db file to store it, and we go through it
	 * TODO: In some cases it may be not accurate
	 **/
	if(!(*rA_svn_version) && ((fp = fopen(".svn/wc.db", "rb")) != NULL || (fp = fopen("../.svn/wc.db", "rb")) != NULL)) {
		char lines[64];
		int revision,last_known = 0;
		while(fread(lines, sizeof(char), sizeof(lines), fp)) {
			if( strstr(lines,"!svn/ver/") ) {
				if (sscanf(strstr(lines,"!svn/ver/"),"!svn/ver/%d/%*s", &revision) == 1) {
					if( revision > last_known ) {
						last_known = revision;
					}
				}
			}
		}
		fclose(fp);
		if( last_known != 0 )
			snprintf(rA_svn_version, sizeof(rA_svn_version), "%d", last_known);
	}
	/**
	 * we definitely didn't find it.
	 **/
	if(!(*rA_svn_version))
		snprintf(rA_svn_version, sizeof(rA_svn_version), "Desconhecido");

	return rA_svn_version;
}

/*======================================
 *	CORE : Display title
 *--------------------------------------*/
static void display_title(void)
{
	//ClearScreen(); // clear screen and go up/left (0, 0 position in text)
	ShowMessage("\n");
	ShowMessage(""CL_WTBL"          (=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=)"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BT_YELLOW"       Equipe Cronus de Desenvolvimento Apresenta        "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BOLD"      _________                                          "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BOLD"      \\_   ___ \\_______  ____   ____  __ __  ______      "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BOLD"      /    \\  \\/\\_  __ \\/  _ \\ /    \\|  |  \\/  ___/      "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BOLD"      \\     \\____|  | \\(  <_> )   |  \\  |  /\\___ \\       "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BOLD"       \\______  /|__|   \\____/|___|  /____//____  >      "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BOLD"              \\/                   \\/           \\/       "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BT_RED"                          Fusion                         "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BOLD"                  www.cronus-emulator.com                "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BT_YELLOW"      Baseado no eAthena (c) 2005-2012 Projeto Cronus    "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_XXBL"          ("CL_BOLD"                                                         "CL_XXBL")"CL_CLL""CL_NORMAL"\n");
	ShowMessage(""CL_WTBL"          (=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=)"CL_CLL""CL_NORMAL"\n\n");

	if( strcmpi(get_git_revision(), "no") != 0 ) {
		ShowInfo("Revisão GIT: '"CL_WHITE"%s"CL_RESET"'.\n", get_git_revision());
	} else if( strcmpi(get_svn_revision(), "no") != 0 ) {
		ShowInfo("Revisão SVN: '"CL_WHITE"%s"CL_RESET"'.\n", get_svn_revision());
	} else {
		ShowInfo("Revisão: Desconhecida.\n");
	}
}

// Warning if executed as superuser (root)
void usercheck(void)
{
#ifndef _WIN32
    if (geteuid() == 0) {
		ShowWarning ("Você está iniciando o Cronus-Emulator com privilégios ROOT.\n");
		ShowWarning ("Isto é inseguro e desnecessário.\n");
    }
#endif
}

/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------*/
int main (int argc, char **argv)
{
	{// initialize program arguments
		char *p1 = SERVER_NAME = argv[0];
		char *p2 = p1;
		while ((p1 = strchr(p2, '/')) != NULL || (p1 = strchr(p2, '\\')) != NULL)
		{
			SERVER_NAME = ++p1;
			p2 = p1;
		}
		arg_c = argc;
		arg_v = argv;
	}

	malloc_init();// needed for Show* in display_title() [FlavioJS]

#ifdef MINICORE // minimalist Core
	display_title();
	usercheck();
	do_init(argc,argv);
	do_final();
#else// not MINICORE
	set_server_type();
	display_title();
	usercheck();

	rathread_init();
	mempool_init();
	db_init();
	signals_init();

#ifdef _WIN32
	cevents_init();
#endif

	timer_init();
	socket_init();

	do_init(argc,argv);

	{// Main runtime cycle
		int next;
		while (runflag != CORE_ST_STOP) {
			next = do_timer(gettick_nocache());
			do_sockets(next);
		}
	}

	do_final();

	timer_final();
	socket_final();
	db_final();
	mempool_final();	
	rathread_final();
#endif

	malloc_final();

	return 0;
}
