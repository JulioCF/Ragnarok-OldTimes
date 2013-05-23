#include "../common/harmony.h"
#include "../common/socket.h"

BUILDIN_FUNC(harm_map_multicount)
{
	const char *var = script_getstr(st,2);
	struct map_session_data *sd = script_rid2sd(st);
	int cnt = 0, m;

	if( sd && (m = map_mapname2mapid(var)) >= 0 )
	{
		struct map_session_data* pl_sd;
		struct s_mapiterator* iter;
		int8 s_mac_address[20], t_mac_address[20];
		bool byip = false;

		harm_funcs->zone_get_mac_address(sd->fd,s_mac_address);

		if( !strncmp(s_mac_address,"00:00:00:", 8) || !strncmp(s_mac_address,"0:0:0:", 6) || !strcmpi(s_mac_address,"00:53:45:00:00:00") )
			byip = true; // Ignored Mac Address

		iter = mapit_getallusers();
		for( pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter) )
		{
			if( pl_sd->bl.m != m )
				continue;
			if( !byip )
			{
				harm_funcs->zone_get_mac_address(pl_sd->fd,t_mac_address);
				if( !strcmpi(s_mac_address,t_mac_address) )
					cnt++;
			}
			else if( session[sd->fd]->client_addr == session[pl_sd->fd]->client_addr )
				cnt++;
		}
		mapit_free(iter);
	}

	script_pushint(st,cnt);
	return 0;
}

BUILDIN_FUNC(harm_bg_multicount)
{
	struct map_session_data *sd = script_rid2sd(st);
	int m = map_mapname2mapid("bat_room");
	int cnt = 0;
	
	if (sd)
	{
		struct map_session_data* pl_sd;
		struct s_mapiterator* iter;
		int8 s_mac_address[20], t_mac_address[20];
		bool byip = false;

		harm_funcs->zone_get_mac_address(sd->fd,s_mac_address);

		if( !strncmp(s_mac_address,"00:00:00:", 8) || !strncmp(s_mac_address,"0:0:0:", 6) || !strcmpi(s_mac_address,"00:53:45:00:00:00") )
			byip = true; // Ignored Mac Address

		iter = mapit_getallusers();
		for( pl_sd = (TBL_PC*)mapit_first(iter); mapit_exists(iter); pl_sd = (TBL_PC*)mapit_next(iter) )
		{
			if( pl_sd->bl.m != m && !map[pl_sd->bl.m].flag.battleground )
				continue;
			if( !byip )
			{
				harm_funcs->zone_get_mac_address(pl_sd->fd,t_mac_address);
				if( !strcmpi(s_mac_address,t_mac_address) )
					cnt++;
			}
			else if( session[sd->fd]->client_addr == session[pl_sd->fd]->client_addr )
				cnt++;
		}
		mapit_free(iter);
	}

	script_pushint(st,cnt);
	return 0;
}



