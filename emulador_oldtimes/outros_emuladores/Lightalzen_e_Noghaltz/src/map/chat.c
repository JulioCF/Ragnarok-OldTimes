// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include <stdio.h>
#include <string.h>

#include "../common/nullpo.h"
#include "../common/malloc.h"
#include "battle.h"
#include "chat.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "npc.h"
#include "atcommand.h"

int chat_triggerevent(struct chat_data *cd);

/*==========================================
 * chatroom creation
 *------------------------------------------
 */
int chat_createchat(struct map_session_data* sd,int limit, int pub, char* pass, char* title, int titlelen)
{
	struct chat_data *cd;

	nullpo_retr(0, sd);

	if (sd->chatID)
		return 0;	//Prevent people abusing the chat system by creating multiple chats, as pointed out by End of Exam. [Skotlex]

	if (map[sd->bl.m].flag.nochat) {
		clif_displaymessage (sd->fd, msg_txt(281));
		return 0; //Can't create chatrooms on this map.
	}

	if ((battle_config.block_area) && ((sd->bl.m == map_mapname2mapid("prontera.gat") && (sd->bl.x >= 148 && sd->bl.x <= 163 && sd->bl.y >= 130 && sd->bl.y <= 232)) ||
	    (sd->bl.m == map_mapname2mapid("prontera.gat") && (sd->bl.x >= 123 && sd->bl.x <= 196 && sd->bl.y >= 182 && sd->bl.y <= 238)))) {
		 clif_displaymessage (sd->fd,"Você não está autorizado a abrir chats ou criar vendas nesta área.");
		 return 0; // can't create chatrooms on prontera square [by  theultramage & JuliosS]
	}
	pc_stop_walking(sd,1);
	cd = (struct chat_data *) aMalloc(sizeof(struct chat_data));

	cd->limit = limit;
	cd->pub = pub;
	cd->users = 1;
	titlelen = cap_value(titlelen, 0, sizeof(cd->title)-1); // empty string achievable by using custom client
	// the following two input strings aren't zero terminated, have to handle it manually
	memcpy(cd->pass, pass, 8); cd->pass[8]= '\0';
	memcpy(cd->title, title, titlelen); cd->title[titlelen] = '\0';

	cd->owner = (struct block_list **)(&cd->usersd[0]);
	cd->usersd[0] = sd;
	cd->bl.m = sd->bl.m;
	cd->bl.x = sd->bl.x;
	cd->bl.y = sd->bl.y;
	cd->bl.type = BL_CHAT;
	cd->bl.next = cd->bl.prev = NULL;
	cd->bl.id = map_addobject(&cd->bl);	
	if(cd->bl.id==0){
		clif_createchat(sd,1);
		aFree(cd);
		return 0;
	}
	pc_setchatid(sd,cd->bl.id);

	clif_createchat(sd,0);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * Šù‘¶ƒ`ƒƒƒbƒgƒ‹[ƒ€‚ÉŽQ‰Á
 *------------------------------------------
 */
int chat_joinchat(struct map_session_data* sd, int chatid, char* pass)
{
	struct chat_data *cd;

	nullpo_retr(0, sd);
	cd = (struct chat_data*)map_id2bl(chatid);

 //No need for a nullpo check. The chatid was sent by the client, if they lag or mess with the packet 
 //a wrong chat id can be received. [Skotlex]
	if (cd == NULL)
		return 1;
	if (cd->bl.type != BL_CHAT || cd->bl.m != sd->bl.m || sd->vender_id || sd->chatID || cd->limit <= cd->users) {
		clif_joinchatfail(sd,0);
		return 0;
	}
	//Allows Gm access to protected room with any password they want by valaris
	if ((cd->pub == 0 && strncmp(pass, (char *)cd->pass, 8) && (pc_isGM(sd) < battle_config.gm_join_chat || !battle_config.gm_join_chat)) ||
		chatid == (int)sd->chatID) //Double Chat fix by Alex14, thx CHaNGeTe
	{
		clif_joinchatfail(sd,1);
		return 0;
	}

	pc_stop_walking(sd,1);
	cd->usersd[cd->users] = sd;
	cd->users++;

	pc_setchatid(sd,cd->bl.id);

	clif_joinchatok(sd,cd);	// V‚½‚ÉŽQ‰Á‚µ‚½l‚É‚Í‘Sˆõ‚ÌƒŠƒXƒg
	clif_addchat(cd,sd);	// Šù‚É’†‚É‹‚½l‚É‚Í’Ç‰Á‚µ‚½l‚Ì•ñ
	clif_dispchat(cd,0);	// ŽüˆÍ‚Ìl‚É‚Íl”•Ï‰»•ñ

	chat_triggerevent(cd); // ƒCƒxƒ“ƒg
	
	return 0;
}

/*==========================================
 * ƒ`ƒƒƒbƒgƒ‹[ƒ€‚©‚ç”²‚¯‚é
 *------------------------------------------
 */
int chat_leavechat(struct map_session_data *sd)
{
	struct chat_data *cd;
	int i,leavechar;

	nullpo_retr(1, sd);

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL) {
		sd->chatID = 0;
		return 1;
	}

	for(i = 0,leavechar=-1;i < cd->users;i++){
		if(cd->usersd[i] == sd){
			leavechar=i;
			break;
		}
	}
	if(leavechar<0)
  	{	//Not found in the chatroom?
		sd->chatID = 0;
		return -1;
	}

	if(leavechar==0 && cd->users>1 && (*cd->owner)->type==BL_PC){
		// Š—LŽÒ‚¾‚Á‚½&‘¼‚Él‚ª‹‚é&PC‚Ìƒ`ƒƒƒbƒg
		clif_changechatowner(cd,cd->usersd[1]);
		clif_clearchat(cd,0);
	}

	// ”²‚¯‚éPC‚É‚à‘—‚é‚Ì‚Åusers‚ðŒ¸‚ç‚·‘O‚ÉŽÀs
	clif_leavechat(cd,sd);

	cd->users--;
	pc_setchatid(sd,0);

	if(cd->users == 0 && (*cd->owner)->type==BL_PC){
		//Delete empty chatroom
		clif_clearchat(cd,0);
		map_delobject(cd->bl.id);
		return 1;
	}
	for(i=leavechar;i < cd->users;i++)
		cd->usersd[i] = cd->usersd[i+1];

	if(leavechar==0 && (*cd->owner)->type==BL_PC){
		//Adjust Chat location after owner has been changed.
		map_delblock( &cd->bl );
		cd->bl.x=cd->usersd[0]->bl.x;
		cd->bl.y=cd->usersd[0]->bl.y;
		map_addblock( &cd->bl );
	}
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * ƒ`ƒƒƒbƒgƒ‹[ƒ€‚ÌŽ‚¿Žå‚ð÷‚é
 *------------------------------------------
 */
int chat_changechatowner(struct map_session_data *sd,char *nextownername)
{
	struct chat_data *cd;
	struct map_session_data *tmp_sd;
	int i, nextowner;

	nullpo_retr(1, sd);

	cd = (struct chat_data*)map_id2bl(sd->chatID);
	if (cd == NULL || (struct block_list *)sd != (*cd->owner))
		return 1;

	for(i = 1,nextowner=-1;i < cd->users;i++){
		if(strcmp(cd->usersd[i]->status.name,nextownername)==0){
			nextowner=i;
			break;
		}
	}
	if(nextowner<0) // ‚»‚ñ‚Èl‚Í‹‚È‚¢
		return -1;

	clif_changechatowner(cd,cd->usersd[nextowner]);
	// ˆê’UÁ‚·
	clif_clearchat(cd,0);

	// userlist‚Ì‡”Ô•ÏX (0‚ªŠ—LŽÒ‚È‚Ì‚Å)
	if( (tmp_sd = cd->usersd[0]) == NULL )
		return 1; //‚ ‚è‚¦‚é‚Ì‚©‚ÈH
	cd->usersd[0] = cd->usersd[nextowner];
	cd->usersd[nextowner] = tmp_sd;

	map_delblock( &cd->bl );
	cd->bl.x=cd->usersd[0]->bl.x;
	cd->bl.y=cd->usersd[0]->bl.y;
	map_addblock( &cd->bl );

	// Ä“x•\Ž¦
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * ƒ`ƒƒƒbƒg‚Ìó‘Ô(ƒ^ƒCƒgƒ‹“™)‚ð•ÏX
 *------------------------------------------
 */
int chat_changechatstatus(struct map_session_data *sd,int limit,int pub,char* pass,char* title,int titlelen)
{
	struct chat_data *cd;

	nullpo_retr(1, sd);

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL || (struct block_list *)sd != (*cd->owner))
		return 1;

	cd->limit = limit;
	cd->pub = pub;
	memcpy(cd->pass,pass,8);
	cd->pass[7]= '\0'; //Overflow check... [Skotlex]
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	clif_changechatstatus(cd);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * ƒ`ƒƒƒbƒgƒ‹[ƒ€‚©‚çR‚èo‚·
 *------------------------------------------
 */
int chat_kickchat(struct map_session_data *sd,char *kickusername)
{
	struct chat_data *cd;
	int i;

	nullpo_retr(1, sd);

	cd = (struct chat_data *)map_id2bl(sd->chatID);
	
	if (!cd) return -1;

	for(i = 0; i < cd->users; i++) {
		if (strcmp(cd->usersd[i]->status.name, kickusername) == 0) {
			if (battle_config.gm_kick_chat && pc_isGM(cd->usersd[i]) >= battle_config.gm_kick_chat)
				//gm kick protection by valaris
				return 0;

			chat_leavechat(cd->usersd[i]);
			return 0;
		}
	}

	return -1;
}

/*==========================================
 * npcƒ`ƒƒƒbƒgƒ‹[ƒ€ì¬
 *------------------------------------------
 */
int chat_createnpcchat(struct npc_data *nd,int limit,int pub,int trigger,const char* title,int titlelen,const char *ev)
{
	struct chat_data *cd;

	nullpo_retr(1, nd);

	cd = (struct chat_data *) aMalloc(sizeof(struct chat_data));

	cd->limit = cd->trigger = limit;
	if(trigger>0)
		cd->trigger = trigger;
	cd->pub = pub;
	cd->users = 0;
	memcpy(cd->pass,"",1);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->bl.m = nd->bl.m;
	cd->bl.x = nd->bl.x;
	cd->bl.y = nd->bl.y;
	cd->bl.type = BL_CHAT;
	cd->bl.prev= cd->bl.next = NULL;
	cd->owner_ = (struct block_list *)nd;
	cd->owner = &cd->owner_;
	if (strlen(ev) > 49)
	{	//npc_event is a char[50]	[Skotlex]
		memcpy(cd->npc_event,ev,49);
		cd->npc_event[49] = '\0';
	} else
		memcpy(cd->npc_event,ev,strlen(ev)+1); //Include the \0

	cd->bl.id = map_addobject(&cd->bl);	
	if(cd->bl.id==0){
		aFree(cd);
		return 0;
	}
	nd->chat_id=cd->bl.id;

	clif_dispchat(cd,0);

	return 0;
}
/*==========================================
 * npcƒ`ƒƒƒbƒgƒ‹[ƒ€íœ
 *------------------------------------------
 */
int chat_deletenpcchat(struct npc_data *nd)
{
	struct chat_data *cd;

	nullpo_retr(0, nd);
	nullpo_retr(0, cd=(struct chat_data*)map_id2bl(nd->chat_id));
	
	chat_npckickall(cd);
	clif_clearchat(cd,0);
	map_delobject(cd->bl.id);	// free‚Ü‚Å‚µ‚Ä‚­‚ê‚é
	nd->chat_id=0;
	
	return 0;
}

/*==========================================
 * ‹K’èl”ˆÈã‚ÅƒCƒxƒ“ƒg‚ª’è‹`‚³‚ê‚Ä‚é‚È‚çŽÀs
 *------------------------------------------
 */
int chat_triggerevent(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	if(cd->users>=cd->trigger && cd->npc_event[0])
		npc_event_do(cd->npc_event);
	return 0;
}

/*==========================================
 * ƒCƒxƒ“ƒg‚Ì—LŒø‰»
 *------------------------------------------
 */
int chat_enableevent(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	cd->trigger&=0x7f;
	chat_triggerevent(cd);
	return 0;
}
/*==========================================
 * ƒCƒxƒ“ƒg‚Ì–³Œø‰»
 *------------------------------------------
 */
int chat_disableevent(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	cd->trigger|=0x80;
	return 0;
}
/*==========================================
 * ƒ`ƒƒƒbƒgƒ‹[ƒ€‚©‚ç‘SˆõR‚èo‚·
 *------------------------------------------
 */
int chat_npckickall(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	while(cd->users>0){
		chat_leavechat(cd->usersd[cd->users-1]);
	}
	return 0;
}
