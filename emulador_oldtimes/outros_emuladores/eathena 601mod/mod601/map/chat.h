#ifndef _CHAT_H_
#define _CHAT_H_

#include "map.h"

int chat_createchat(struct map_session_data *,int,int,char*,char*,int);
int chat_joinchat(struct map_session_data *,int,char*);
int chat_leavechat(struct map_session_data* );
int chat_changechatowner(struct map_session_data *,char *);
int chat_changechatstatus(struct map_session_data *,int,int,char*,char*,int);
int chat_kickchat(struct map_session_data *,char *);
int chat_createcnpchat(struct npc_data *nd,int limit,char* title,int titlelen,const char *ev);

#endif
