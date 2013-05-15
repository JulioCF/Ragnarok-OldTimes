#ifndef WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif


ACMD_FUNC(reloadgrfintegrity) {
	harm_funcs->zone_grf_reload();
	clif_displaymessage(fd, "GRF integrity definitions reloaded.");
	return 0;
}

ACMD_FUNC(reloadharmony) {
	harm_funcs->zone_reload();
	clif_displaymessage(fd, "Harmony configuration reloaded.");
	return 0;
}

ACMD_FUNC(showautoban) {
	harm_funcs->zone_autoban_show(fd);
	return 0;
}

ACMD_FUNC(liftautoban) {
	uint32 ip;

	if (!message || !*message) {
	__usage:
		clif_displaymessage(fd, "Usage:");
		clif_displaymessage(fd, "	@liftautoban <IP address>");
		return -1;
	}
	
	if ( (ip=inet_addr(message)) != INADDR_NONE)
		ip = htonl(ip);
	else if (stricmp(message, "all") == 0)
		ip = 0;
	else
		goto __usage;

	harm_funcs->zone_autoban_lift(ip);

	sprintf(atcmd_output, "Unblocked %u.%u.%u.%u.", CONVIP(ip));
	clif_displaymessage(fd, atcmd_output);

	return 0;	
}

/*==========================================
 * Network Information
 *------------------------------------------*/
int atcommand_netinfo(const int fd, struct map_session_data *sd, const char *command, const char *message)
{
	uint32 ip;
	int8 mac_address[20];

	nullpo_retr(-1,sd);
	sprintf(atcmd_output, "Network Information Request for %s (AID %d | CID %d)", sd->status.name, sd->status.account_id, sd->status.char_id);
	clif_displaymessage(fd,atcmd_output);
	harm_funcs->zone_get_mac_address(sd->fd, mac_address);
	sprintf(atcmd_output, "- Mac Address : %s", mac_address);
	clif_displaymessage(fd,atcmd_output);

	ip = session[sd->fd]->client_addr;
	sprintf(atcmd_output, "- IP Address : %d.%d.%d.%d", CONVIP(ip));
	clif_displaymessage(fd,atcmd_output);

	return 0;
}

/*==========================================
 * Mac Address Ban
 *------------------------------------------*/
int atcommand_showmacban(const int fd, struct map_session_data *sd, const char *command, const char *message) {
	SqlStmt* stmt = SqlStmt_Malloc(mmysql_handle);
	char mac_address[20];
	char comment[90];

	if (SQL_SUCCESS != SqlStmt_Prepare(stmt, "SELECT `mac`, `comment` FROM mac_bans ORDER BY `mac` DESC LIMIT 100") ||
		SQL_SUCCESS != SqlStmt_Execute(stmt) ||
		SQL_SUCCESS != SqlStmt_BindColumn(stmt, 0,  SQLDT_STRING, mac_address, sizeof(mac_address), NULL, NULL) ||
		SQL_SUCCESS != SqlStmt_BindColumn(stmt, 1,  SQLDT_STRING, comment, sizeof(comment), NULL, NULL)) {
			Sql_ShowDebug(mmysql_handle);
	} else {
		clif_displaymessage(fd, "Mac address bans:");
		while (SQL_SUCCESS == SqlStmt_NextRow(stmt)) {
			// Gravity client crashes when this sequence occurs on a bad position :(
			char *p = comment;
			while ((p = strstr(p, " : ")) != NULL)
				*(++p) = ';';

			sprintf(atcmd_output, "%-20s%s", mac_address, comment);
			clif_displaymessage(fd, atcmd_output);
		}
	}
	SqlStmt_Free(stmt);

	return 0;
}

int atcommand_macban(const int fd, struct map_session_data *sd, const char *command, const char *message)
{
	char mac_address[20];
	char buf1[400];
	char buf2[400];
	TBL_PC *src_sd = (TBL_PC*) session[fd]->session_data;
	size_t len;

	harm_funcs->zone_get_mac_address(sd->fd, mac_address);
	len = snprintf(buf1, sizeof(buf1)/2, "%s (%u) banned %s (%u) : %s", src_sd->status.name, src_sd->status.account_id, sd->status.name, sd->status.account_id, strlen(message) > 1 ? message : "(no comment)");
	Sql_EscapeStringLen(mmysql_handle, buf2, buf1, len);
	sprintf(buf1, "INSERT INTO `mac_bans` (`mac`, `comment`) VALUES ('%s', '%s')", mac_address, buf2);

	if( SQL_ERROR == Sql_QueryStr(mmysql_handle, buf1) )
	{
		Sql_ShowDebug(mmysql_handle);
		return 1;
	}

	sprintf(atcmd_output, "Mac Address %s banned.", mac_address);
	clif_displaymessage(fd, atcmd_output);
	return 0;
}

int atcommand_liftmacban(const int fd, struct map_session_data *sd, const char *command, const char *message)
{
	char buf[41];
	size_t len = min(20, strlen(message));

	Sql_EscapeStringLen(mmysql_handle, buf, message, len);
	sprintf(atcmd_output, "DELETE FROM `mac_bans` WHERE `mac` = '%s'", buf);

	if( SQL_ERROR == Sql_QueryStr(mmysql_handle, atcmd_output) )
	{
		Sql_ShowDebug(mmysql_handle);
		return 1;
	}

	sprintf(atcmd_output, "Mac Address %s unbanned.", message);
	clif_displaymessage(fd, atcmd_output);
	return 0;
}


