// (c) 2008 - 2011 Harmony Project; Daniel Stelter-Gliese / Sirius_White
//
//  - white@siriuswhite.de
//  - ICQ #119-153
//  - MSN msn@siriuswhite.de
//
// This file is NOT public - you are not allowed to distribute it.
#include "../common/cbasetypes.h"
#include "../common/showmsg.h"
#include "../common/db.h"
#include "../common/strlib.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/sql.h"
#include "../common/harmony.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc.h"
#include "clif.h"
#include "atcommand.h"
#include "chrif.h"
#include "harmony.h"

void _FASTCALL harmony_action_request(int fd, int task, int id, intptr data);
void _FASTCALL harmony_log(int fd, const char *msg);

// ----

static SqlStmt* log_stmt = NULL;

enum {
	LOG_SQL = 1,
	LOG_TXT,
	LOG_TRYSQL
};
static int log_method = LOG_TXT;

// ----

void harmony_init() {
	if (logmysql_handle != NULL) {
		log_stmt =  SqlStmt_Malloc(logmysql_handle);
		if (SQL_SUCCESS != SqlStmt_Prepare(log_stmt, "INSERT DELAYED INTO harmony_log (`account_id`, `char_name`, `IP`, `data`) VALUES (?, ?, ?, ?)")) {
			Sql_ShowDebug(logmysql_handle);
			exit(EXIT_FAILURE);
		}
	}

	ea_funcs->action_request = harmony_action_request;
	ea_funcs->player_log = harmony_log;

	harm_funcs->zone_init();
}

void harmony_final() {
	harm_funcs->zone_final();
	if (log_stmt) {
		SqlStmt_Free(log_stmt);
		log_stmt = NULL;
	}
}

void harmony_parse(int fd,struct map_session_data *sd) {
}

int harmony_log_sql(TBL_PC* sd, const char* ip, const char* msg) {
	if (!logmysql_handle || !log_stmt)
		return 0;
	if (SQL_SUCCESS != SqlStmt_BindParam(log_stmt, 0, SQLDT_INT, (void*)&sd->status.account_id, sizeof(sd->status.account_id)) ||
		SQL_SUCCESS != SqlStmt_BindParam(log_stmt, 1, SQLDT_STRING, (void*)&sd->status.name, strlen(sd->status.name)) ||
		SQL_SUCCESS != SqlStmt_BindParam(log_stmt, 2, SQLDT_STRING, (void*)ip, strlen(ip)) ||
		SQL_SUCCESS != SqlStmt_BindParam(log_stmt, 3, SQLDT_STRING, (void*)msg, strlen(msg)) ||
		SQL_SUCCESS != SqlStmt_Execute(log_stmt))
	{
		Sql_ShowDebug(logmysql_handle);
		return 0;
	}
	return 1;
}

int harmony_log_txt(TBL_PC* sd, const char* ip, const char* msg) {
	FILE* logfp;
	static char timestring[255];
	time_t curtime;

	if((logfp = fopen("./log/harmony.log", "a+")) == NULL)
		return 0;
	time(&curtime);
	strftime(timestring, 254, "%m/%d/%Y %H:%M:%S", localtime(&curtime));
	fprintf(logfp,"%s - %s[%d:%d] - %s\t%s\n", timestring, sd->status.name, sd->status.account_id, sd->status.char_id, ip, msg);
	fclose(logfp);
	return 1;
}

void _FASTCALL harmony_log(int fd, const char *msg) {
	char ip[20];
	TBL_PC *sd = (TBL_PC*)session[fd]->session_data;

	if (!sd)
		return;

	sprintf(ip, "%u.%u.%u.%u", CONVIP(session[fd]->client_addr));

	if (log_method == LOG_SQL && !harmony_log_sql(sd, ip, msg)) {
		ShowError("Logging to harmony_log failed. Please check your Harmony setup.\n");
	} else if (log_method == LOG_TRYSQL && log_stmt && harmony_log_sql(sd, ip, msg)) {
		;
	} else if (log_method == LOG_TXT || log_method == LOG_TRYSQL) {
		harmony_log_txt(sd, ip, msg);
	}

	ShowMessage(""CL_MAGENTA"[Harmony]"CL_RESET": %s (Player: %s, IP: %s)\n", msg, sd->status.name, ip);
}

void harmony_action_request_global(int task, int id, intptr data) {
	switch (task) {
	case HARMTASK_LOGIN_ACTION:
		chrif_harmony_request((uint8*)data, id);
		break;
	case HARMTASK_GET_FD:
		{
		TBL_PC *sd = BL_CAST(BL_PC, map_id2bl(id));
		*(int32*)data = (sd ? sd->fd : 0);
		}
		break;
	case HARMTASK_SET_LOG_METHOD:
		log_method = id;
		break;
	default:
		ShowError("Harmony requested unknown action! (ID=%d)\n", task);
		ShowError("This indicates that you are running an incompatible version.\n");
		break;
	}
}

void _FASTCALL harmony_action_request(int fd, int task, int id, intptr data) {
	TBL_PC *sd;

	if (fd == 0) {
		harmony_action_request_global(task, id, data);
		return;
	}

	sd = (TBL_PC *)session[fd]->session_data;
	if (!sd)
		return;

	switch (task) {
	case HARMTASK_DC:
		ShowInfo("-- Harmony requested disconnect.\n");
		set_eof(fd);
		break;
	case HARMTASK_KICK:
		ShowInfo("-- Harmony requested kick.\n");
		if (id == 99)
			set_eof(fd);
		else
			clif_authfail_fd(fd, id);
		break;
	case HARMTASK_ATCMD:
		is_atcommand(fd, sd, (const char*)data, 0);
		break;
	case HARMTASK_MSG:
		clif_displaymessage(fd, (const char*)data);
		break;
	case HARMTASK_PACKET:
		memcpy(WFIFOP(fd, 0), (const void*)data, id);
		WFIFOSET(fd, id);
		break;
	case HARMTASK_IS_ACTIVE:
		*(int32*)data = (sd->bl.prev == NULL || sd->invincible_timer != INVALID_TIMER) ? 0 : 1;
		break;
	case HARMTASK_GET_ID:
		switch (id) {
		case HARMID_AID:
			*(int*)data = sd->status.account_id;
			break;
		case HARMID_GID:
			*(int*)data = sd->status.char_id;
			break;
		case HARMID_GDID:
			*(int*)data = sd->status.guild_id;
			break;
		case HARMID_PID:
			*(int*)data = sd->status.party_id;
			break;
		case HARMID_CLASS:
			*(short*)data = sd->status.class_;
			break;
		case HARMID_GM:
			*(int*)data = pc_isGM(sd);
			break;
		default:
			ShowError("Harmony requested unknown ID! (ID=%d)\n", id);
			ShowError("This indicates that you are running an incompatible version.\n");
			break;
		}
		break;
	default:
		ShowError("Harmony requested unknown action! (ID=%d)\n", task);
		ShowError("This indicates that you are running an incompatible version.\n");
		break;
	}
}

void harmony_logout(TBL_PC* sd) {
	harm_funcs->zone_logout(sd->fd);
}


