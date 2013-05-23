// $Id: chat.c,v 1.7 2003/06/29 05:52:56 lemit Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "chat.h"
#include "npc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

/*==========================================
 * チャットルーム作成
 *------------------------------------------
 */
int chat_createchat(struct map_session_data *sd,int limit,int pub,char* pass,char* title,int titlelen)
{
	struct chat_data *cd;

	cd = malloc(sizeof(*cd));
	if(cd==NULL){
		printf("out of memory : chat_createchat\n");
		exit(1);
	}
	memset(cd,0,sizeof(*cd));

	cd->limit = limit;
	cd->pub = pub;
	cd->users = 1;
	memcpy(cd->pass,pass,8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->owner = (struct block_list **)(&cd->usersd[0]);
	cd->usersd[0] = sd;
	cd->bl.m = sd->bl.m;
	cd->bl.x = sd->bl.x;
	cd->bl.y = sd->bl.y;
	cd->bl.type = BL_CHAT;

	cd->bl.id = map_addobject(&cd->bl);	
	if(cd->bl.id==0){
		clif_createchat(sd,1);
		free(cd);
		return 0;
	}
	pc_setchatid(sd,cd->bl.id);

	clif_createchat(sd,0);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * 既存チャットルームに参加
 *------------------------------------------
 */
int chat_joinchat(struct map_session_data *sd,int chatid,char* pass)
{
	struct chat_data *cd;

	cd=(struct chat_data*)map_id2bl(chatid);
	if(cd==NULL)
		return 1;

	if(cd->bl.m != sd->bl.m || cd->limit <= cd->users){
		clif_joinchatfail(sd,0);
		return 0;
	}
	if(cd->pub==0 && strncmp(pass,cd->pass,8)){
		clif_joinchatfail(sd,1);
		return 0;
	}

	cd->usersd[cd->users] = sd;
	cd->users++;

	pc_setchatid(sd,cd->bl.id);

	// 新たに参加した人には全員のリスト
	clif_joinchatok(sd,cd);

	// 既に中に居た人には追加した人の報告
	clif_addchat(cd,sd);

	// 周囲の人には人数変化報告
	clif_dispchat(cd,0);

	// 満員でイベントが定義されてるなら実行
	if(cd->users>=cd->limit && cd->npc_event[0])
		npc_event(sd,cd->npc_event);
		
	return 0;
}

/*==========================================
 * チャットルームから抜ける
 *------------------------------------------
 */
int chat_leavechat(struct map_session_data *sd)
{
	struct chat_data *cd;
	int i,leavechar;

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL)
		return 1;

	for(i = 0,leavechar=-1;i < cd->users;i++){
		if(cd->usersd[i] == sd){
			leavechar=i;
			break;
		}
	}
	if(leavechar<0)	// そのchatに所属していないらしい (バグ時のみ)
		return -1;

	if(leavechar==0 && cd->users>1 && (*cd->owner)->type==BL_PC){
		// 所有者だった&他に人が居る&PCのチャット
		clif_changechatowner(cd,cd->usersd[1]);
		clif_clearchat(cd,0);
	}

	// 抜けるPCにも送るのでusersを減らす前に実行
	clif_leavechat(cd,sd);

	cd->users--;
	pc_setchatid(sd,0);

	if(cd->users == 0 && (*cd->owner)->type==BL_PC){
			// 全員居なくなった&PCのチャットなので消す
		clif_clearchat(cd,0);
		map_delobject(cd->bl.id);	// freeまでしてくれる
	} else {
		for(i=leavechar;i < cd->users;i++)
			cd->usersd[i] = cd->usersd[i+1];
		if(leavechar==0 && (*cd->owner)->type==BL_PC){
			// PCのチャットなので所有者が抜けたので位置変更
			cd->bl.x=cd->usersd[0]->bl.x;
			cd->bl.y=cd->usersd[0]->bl.y;
		}
		clif_dispchat(cd,0);
	}

	return 0;
}

/*==========================================
 * チャットルームの持ち主を譲る
 *------------------------------------------
 */
int chat_changechatowner(struct map_session_data *sd,char *nextownername)
{
	struct chat_data *cd;
	struct map_session_data *tmp_sd;
	int i,nextowner;

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL || (struct block_list *)sd!=(*cd->owner))
		return 1;

	for(i = 1,nextowner=-1;i < cd->users;i++){
		if(strcmp(cd->usersd[i]->status.name,nextownername)==0){
			nextowner=i;
			break;
		}
	}
	if(nextowner<0) // そんな人は居ない
		return -1;

	clif_changechatowner(cd,cd->usersd[nextowner]);
	// 一旦消す
	clif_clearchat(cd,0);

	// userlistの順番変更 (0が所有者なので)
	tmp_sd = cd->usersd[0];
	cd->usersd[0] = cd->usersd[nextowner];
	cd->usersd[nextowner] = tmp_sd;

	// 新しい所有者の位置へ変更
	cd->bl.x=cd->usersd[0]->bl.x;
	cd->bl.y=cd->usersd[0]->bl.y;

	// 再度表示
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * チャットの状態(タイトル等)を変更
 *------------------------------------------
 */
int chat_changechatstatus(struct map_session_data *sd,int limit,int pub,char* pass,char* title,int titlelen)
{
	struct chat_data *cd;

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL || (struct block_list *)sd!=(*cd->owner))
		return 1;

	cd->limit = limit;
	cd->pub = pub;
	memcpy(cd->pass,pass,8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	clif_changechatstatus(cd);
	clif_dispchat(cd,0);

	return 0;
}

/*==========================================
 * チャットルームから蹴り出す
 *------------------------------------------
 */
int chat_kickchat(struct map_session_data *sd,char *kickusername)
{
	struct chat_data *cd;
	int i,kickuser;

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if(cd==NULL || (struct block_list *)sd!=(*cd->owner))
		return 1;

	for(i = 0,kickuser=-1;i < cd->users;i++){
		if(strcmp(cd->usersd[i]->status.name,kickusername)==0){
			kickuser=i;
			break;
		}
	}
	if(kickuser<0) // そんな人は居ない
		return -1;

	chat_leavechat(cd->usersd[kickuser]);

	return 0;
}

/*==========================================
 * npcチャットルーム作成
 *------------------------------------------
 */
int chat_createcnpchat(struct npc_data *nd,int limit,char* title,int titlelen,const char *ev)
{
	struct chat_data *cd;

	cd = malloc(sizeof(*cd));
	if(cd==NULL){
		printf("out of memory : chat_createchat\n");
		exit(1);
	}
	memset(cd,0,sizeof(*cd));

	cd->limit = limit;
	cd->pub = 1;
	cd->users = 0;
	memcpy(cd->pass,"",8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->bl.m = nd->bl.m;
	cd->bl.x = nd->bl.x;
	cd->bl.y = nd->bl.y;
	cd->bl.type = BL_CHAT;
	cd->owner_ = (struct block_list *)nd;
	cd->owner = &cd->owner_;
	memcpy(cd->npc_event,ev,sizeof(cd->npc_event));

	cd->bl.id = map_addobject(&cd->bl);	
	if(cd->bl.id==0){
		free(cd);
		return 0;
	}
	nd->chat_id=cd->bl.id;

	clif_dispchat(cd,0);

	return 0;
}

