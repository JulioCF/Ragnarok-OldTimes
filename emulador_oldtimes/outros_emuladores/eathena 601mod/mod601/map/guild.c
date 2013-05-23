#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guild.h"
#include "db.h"
#include "timer.h"
#include "pc.h"
#include "map.h"
#include "intif.h"
#include "clif.h"
#include "socket.h"
#include "battle.h"

static struct dbt *guild_db;
static struct dbt *guild_expcache_db;

// ギルドのEXPキャッシュのフラッシュに関連する定数
#define GUILD_PAYEXP_INVERVAL 10000	// 間隔(キャッシュの最大生存時間、ミリ秒)
#define GUILD_PAYEXP_LIST 8192	// キャッシュの最大数

// ギルドのEXPキャッシュ
struct guild_expcache {
	int guild_id, account_id, char_id, exp;
};

// ギルドスキルdbのアクセサ（今は直打ちで代用）
int guild_skill_get_inf(int id){ return 0; }
int guild_skill_get_sp(int id,int lv){ return 0; }
int guild_skill_get_range(int id){ return 0; }
int guild_skill_get_max(int id){ return (id==10004)?10:1; }

// ギルドスキルがあるか確認
int guild_checkskill(struct guild *g,int id){ return g->skill[id-10000].lv; }


int guild_payexp_timer(int tid,unsigned int tick,int id,int data);

// 初期化
void do_init_guild(void)
{
	guild_db=numdb_init();
	guild_expcache_db=numdb_init();

	add_timer_func_list(guild_payexp_timer,"guild_payexp_timer");
	add_timer_interval(gettick()+GUILD_PAYEXP_INVERVAL,guild_payexp_timer,0,0,GUILD_PAYEXP_INVERVAL);
}

// 検索
struct guild *guild_search(int guild_id)
{
	return numdb_search(guild_db,guild_id);
}

// ログイン中のギルドメンバーの１人のsdを返す
struct map_session_data *guild_getavailablesd(struct guild *g)
{
	int i;
	for(i=0;i<g->max_member;i++)
		if(g->member[i].sd!=NULL)
			return g->member[i].sd;
	return NULL;
}

// ギルドメンバーのインデックスを返す
int guild_getindex(struct guild *g,int account_id,int char_id)
{
	int i;
	if(g==NULL)
		return -1;
	for(i=0;i<g->max_member;i++)
		if( g->member[i].account_id==account_id &&
			g->member[i].char_id==char_id )
			return i;
	return -1;
}
// ギルドメンバーの役職を返す
int guild_getposition(struct map_session_data *sd,struct guild *g)
{
	int i;
	if(g==NULL && (g=guild_search(sd->status.guild_id))==NULL)
		return -1;
	for(i=0;i<g->max_member;i++)
		if( g->member[i].account_id==sd->status.account_id &&
			g->member[i].char_id==sd->status.char_id )
			return g->member[i].position;
	return -1;
}

// メンバー情報の作成
void guild_makemember(struct guild_member *m,struct map_session_data *sd)
{
	memset(m,0,sizeof(struct guild_member));
	m->account_id	=sd->status.account_id;
	m->char_id		=sd->status.char_id;
	m->hair			=sd->status.hair;
	m->hair_color	=sd->status.hair_color;
	m->gender		=sd->sex;
	m->class		=sd->status.class;
	m->lv			=sd->status.base_level;
	m->exp			=0;
	m->exp_payper	=0;
	m->online		=1;
	m->position		=MAX_GUILDPOSITION-1;
	memcpy(m->name,sd->status.name,24);
	return;
}
// ギルド競合確認
int guild_check_conflict(struct map_session_data *sd)
{
	intif_guild_checkconflict(sd->status.guild_id,
		sd->status.account_id,sd->status.char_id);
	return 0;
}

// ギルドのEXPキャッシュをinter鯖にフラッシュする
int guild_payexp_timer_sub(void *key,void *data,va_list ap)
{
	int i, *dellist,*delp, dataid=(int)key;
	struct guild_expcache *c=(struct guild_expcache *)data;
	struct guild *g;
	dellist=va_arg(ap,int *);
	delp=va_arg(ap,int *);
	
	if( *delp>=GUILD_PAYEXP_LIST || (g=guild_search(c->guild_id))==NULL )
		return 0;
	if( ( i=guild_getindex(g,c->account_id,c->char_id) )<0 )
		return 0;
	
	g->member[i].exp+=c->exp;
	intif_guild_change_memberinfo(g->guild_id,c->account_id,c->char_id,
		GMI_EXP,&g->member[i].exp,sizeof(g->member[i].exp));
	c->exp=0;

	dellist[(*delp)++]=dataid;
	free(c);
	return 0;
}
int guild_payexp_timer(int tid,unsigned int tick,int id,int data)
{
	int dellist[GUILD_PAYEXP_LIST],delp=0,i;
	numdb_foreach(guild_expcache_db,guild_payexp_timer_sub,
		dellist,&delp);
	for(i=0;i<delp;i++)
		numdb_erase(guild_expcache_db,dellist[i]);
//	printf("guild exp %d charactor's exp flushed !\n",delp);
	return 0;
}

//------------------------------------------------------------------------

// 作成要求
int guild_create(struct map_session_data *sd,char *name)
{
	if(sd->status.guild_id==0){
		if(!battle_config.guild_emperium_check || pc_search_inventory(sd,714) != -1) {
			struct guild_member m;
			guild_makemember(&m,sd);
			m.position=0;
			intif_guild_create(name,&m);
		} else
			clif_guild_created(sd,3);	// エンペリウムがいない
	}else
		clif_guild_created(sd,1);	// すでに所属している

	return 0;
}

// 作成可否
int guild_created(int account_id,int guild_id)
{
	struct map_session_data *sd;
	sd=map_id2sd(account_id);
	if(sd==NULL)
		return 0;
	if(guild_id>0) {
			struct guild *g;
			sd->status.guild_id=guild_id;
			sd->guild_sended=0;
			if((g=numdb_search(guild_db,guild_id))!=NULL){
				printf("guild: id already exists!\n");
				exit(0);
			}
			clif_guild_created(sd,0);
			if(battle_config.guild_emperium_check)
				pc_delitem(sd,pc_search_inventory(sd,714),1,0);	// エンペリウム消耗
	} else {
		clif_guild_created(sd,2);	// 作成失敗（同名ギルド存在）
	}
	return 0;
}

// 情報要求
int guild_request_info(int guild_id)
{
//	printf("guild_request_info\n");
	return intif_guild_request_info(guild_id);
}

// 所属キャラの確認
int guild_check_member(const struct guild *g)
{
	int i;
	struct map_session_data *sd;
	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth){
			if(sd->status.guild_id==g->guild_id){
				int j,f=1;
				for(j=0;j<MAX_GUILD;j++){	// データがあるか
					if(	g->member[j].account_id==sd->status.account_id &&
						g->member[j].char_id==sd->status.char_id)
						f=0;
				}
				if(f){
					sd->status.guild_id=0;
					sd->guild_sended=0;
					sd->guild_emblem_id=0;
					printf("guild: check_member %d[%s] is not member\n",
						sd->status.account_id,sd->status.name);
				}
			}
		}
	}
	return 0;
}
// 情報所得失敗（そのIDのキャラを全部未所属にする）
int guild_recv_noinfo(int guild_id)
{
	int i;
	struct map_session_data *sd;
	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth){
			if(sd->status.guild_id==guild_id)
				sd->status.guild_id=0;
		}
	}
	return 0;
}
// 情報所得
int guild_recv_info(struct guild *sg)
{
	struct guild *g,before;
	int i,bm,m;
	
	if((g=numdb_search(guild_db,sg->guild_id))==NULL){
		g=malloc(sizeof(struct guild));
		if(g==NULL){
			printf("guild_recv_info: out of memory!\n");
			exit(0);
		}
		numdb_insert(guild_db,sg->guild_id,g);
		before=*sg;
		
		
		// 最初のロードなのでユーザーのチェックを行う
		guild_check_member(sg);
	}else
		before=*g;
	memcpy(g,sg,sizeof(struct guild));
	
	for(i=bm=m=0;i<g->max_member;i++){	// sdの設定と人数の確認
		if(g->member[i].account_id>0){
			struct map_session_data *sd = map_id2sd(g->member[i].account_id);
			g->member[i].sd=(sd!=NULL &&
				sd->status.char_id==g->member[i].char_id &&
				sd->status.guild_id==g->guild_id)?	sd:NULL;
			m++;
		}else
			g->member[i].sd=NULL;
		if(before.member[i].account_id>0)
			bm++;
	}

	for(i=0;i<g->max_member;i++){	// 情報の送信
		struct map_session_data *sd = g->member[i].sd;
		if( sd==NULL )
			continue;
		
		if(	before.guild_lv!=g->guild_lv || bm!=m ||
			before.max_member!=g->max_member ){
			clif_guild_basicinfo(sd);	// 基本情報送信
			clif_guild_emblem(sd,g);	// エンブレム送信
		}
		
		if(bm!=m){		// メンバー情報送信
			clif_guild_memberlist(g->member[i].sd);
		}
		
		if( before.skill_point!=g->skill_point)
			clif_guild_skillinfo(sd);	// スキル情報送信
		
		if( sd->guild_sended==0){	// 未送信なら所属情報も送る
			clif_guild_belonginfo(sd,g);
			clif_guild_notice(sd,g);
			sd->guild_emblem_id=g->emblem_id;
			sd->guild_sended=1;
		}
	}
	
	return 0;
}


// ギルドへの勧誘
int guild_invite(struct map_session_data *sd,int account_id)
{
	struct map_session_data *tsd= map_id2sd(account_id);
	struct guild *g=guild_search(sd->status.guild_id);
	int i;
	
	if(tsd==NULL || g==NULL)
		return 0;
	if( tsd->status.guild_id>0 || tsd->guild_invite>0 ){	// 相手の所属確認
		clif_guild_inviteack(sd,0);
		return 0;
	}
	
	// 定員確認
	for(i=0;i<g->max_member;i++)
		if(g->member[i].account_id==0)
			break;
	if(i==g->max_member){
		clif_guild_inviteack(sd,3);
		return 0;
	}

	tsd->guild_invite=sd->status.guild_id;
	tsd->guild_invite_account=sd->status.account_id;

	clif_guild_invite(tsd,g);
	return 0;
}
// ギルド勧誘への返答
int guild_reply_invite(struct map_session_data *sd,int guild_id,int flag)
{
	struct map_session_data *tsd= map_id2sd( sd->guild_invite_account );

	if(sd->guild_invite!=guild_id)	// 勧誘とギルドIDが違う
		return 0;

	if(flag==1){	// 承諾
		struct guild_member m;
		struct guild *g;
		int i;

		// 定員確認
		if( (g=guild_search(tsd->status.guild_id))==NULL ){
			sd->guild_invite=0;
			sd->guild_invite_account=0;
			return 0;
		}
		for(i=0;i<g->max_member;i++)
			if(g->member[i].account_id==0)
				break;
		if(i==g->max_member){
			sd->guild_invite=0;
			sd->guild_invite_account=0;
			clif_guild_inviteack(tsd,3);
			return 0;
		}


		//inter鯖へ追加要求
		guild_makemember(&m,sd);
		intif_guild_addmember( sd->guild_invite, &m );
		return 0;
	}else{		// 拒否
		sd->guild_invite=0;
		sd->guild_invite_account=0;
		if(tsd==NULL)
			return 0;
		clif_guild_inviteack(tsd,1);
	}
	return 0;
}
// ギルドメンバが追加された
int guild_member_added(int guild_id,int account_id,int char_id,int flag)
{
	struct map_session_data *sd= map_id2sd(account_id),*sd2;
	struct guild *g;

	if( (g=guild_search(guild_id))==NULL )
		return 0;

	if((sd==NULL || sd->guild_invite==0) && flag==0){
		// キャラ側に登録できなかったため脱退要求を出す
		printf("guild: member added error %d is not online\n",account_id);
 		intif_guild_leave(guild_id,account_id,char_id,0,"**登録失敗**");
		return 0;
	}
	sd->guild_invite=0;
	sd->guild_invite_account=0;

	sd2=map_id2sd(sd->guild_invite_account);
	
	if(flag==1){	// 失敗
		if( sd2!=NULL )
			clif_guild_inviteack(sd2,3);
		return 0;
	}
	
		// 成功
	sd->guild_sended=0;
	sd->status.guild_id=guild_id;

	if( sd2!=NULL )
		clif_guild_inviteack(sd2,2);
	
	// いちおう競合確認
	guild_check_conflict(sd);

	return 0;
}

// ギルド脱退要求
int guild_leave(struct map_session_data *sd,int guild_id,
	int account_id,int char_id,const char *mes)
{
	struct guild *g = guild_search(sd->status.guild_id);
	int i;
	if(g==NULL)
		return 0;
	
	if(	sd->status.account_id!=account_id ||
		sd->status.char_id!=char_id || sd->status.guild_id!=guild_id)
		return 0;
	
	for(i=0;i<g->max_member;i++){	// 所属しているか
		if(	g->member[i].account_id==sd->status.account_id &&
			g->member[i].char_id==sd->status.char_id ){
			intif_guild_leave(g->guild_id,sd->status.account_id,sd->status.char_id,0,mes);
			return 0;
		}
	}
	return 0;
}
// ギルド追放要求
int guild_explusion(struct map_session_data *sd,int guild_id,
	int account_id,int char_id,const char *mes)
{
	struct guild *g = guild_search(sd->status.guild_id);
	int i,ps;
	if(g==NULL)
		return 0;

	if(	sd->status.guild_id!=guild_id)
		return 0;

	if( (ps=guild_getposition(sd,g))<0 || !(g->position[ps].mode&0x0010) )
		return 0;	// 処罰権限無し

	for(i=0;i<g->max_member;i++){	// 所属しているか
		if(	g->member[i].account_id==account_id &&
			g->member[i].char_id==char_id ){
			intif_guild_leave(g->guild_id,account_id,char_id,1,mes);
			return 0;
		}
	}
	return 0;
}
// ギルドメンバが脱退した
int guild_member_leaved(int guild_id,int account_id,int char_id,int flag,
	const char *name,const char *mes)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct guild *g=guild_search(guild_id);
	int i;
	
	if(g!=NULL){
		int i;
		for(i=0;i<g->max_member;i++)
			if(	g->member[i].account_id==account_id &&
				g->member[i].char_id==char_id ){
				struct map_session_data *sd2=sd;
				if(sd2==NULL)
					sd2=guild_getavailablesd(g);
				if(sd2!=NULL){
					if(flag==0)
						clif_guild_leave(sd2,name,mes);
					else
						clif_guild_explusion(sd2,name,mes,account_id);
				}
				g->member[i].account_id=0;
				g->member[i].sd=NULL;
			}
	}
	if(sd!=NULL && sd->status.guild_id==guild_id){
		sd->status.guild_id=0;
		sd->guild_emblem_id=0;
		sd->guild_sended=0;
	}

	// メンバーリストを全員に再通知
	for(i=0;i<g->max_member;i++){
		if( g->member[i].sd!=NULL )
			clif_guild_memberlist(g->member[i].sd);
	}

	return 0;
}
// ギルドメンバのオンライン状態/Lv更新送信
int guild_send_memberinfoshort(struct map_session_data *sd,int online)
{
	struct guild *g;
	if(sd->status.guild_id<=0)
		return 0;
	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	
	intif_guild_memberinfoshort(g->guild_id,
		sd->status.account_id,sd->status.char_id,online,sd->status.base_level);

	if( !online ){	// ログアウトするならsdをクリアして終了
		int i=guild_getindex(g,sd->status.account_id,sd->status.char_id);
		if(i>=0)
			g->member[i].sd=NULL;
		return 0;
	}

	if( sd->guild_sended!=0 )	// ギルド初期送信データは送信済み
		return 0;

	// 競合確認	
	guild_check_conflict(sd);
	
	// あるならギルド初期送信データ送信
	if( (g=guild_search(sd->status.guild_id))!=NULL ){
		guild_check_member(g);	// 所属を確認する
		if(sd->status.guild_id==g->guild_id){
			clif_guild_belonginfo(sd,g);
			clif_guild_notice(sd,g);
			sd->guild_sended=1;
			sd->guild_emblem_id=g->emblem_id;
		}
	}
	return 0;
}
// ギルドメンバのオンライン状態/Lv更新通知
int guild_recv_memberinfoshort(int guild_id,int account_id,int char_id,int online,int lv)
{
	int i,alv,c,idx=0,om=0,oldonline=-1;
	struct guild *g=guild_search(guild_id);
	if(g==NULL)
		return 0;
	for(i=0,alv=0,c=0,om=0;i<g->max_member;i++){
		struct guild_member *m=&g->member[i];
		if(m->account_id==account_id && m->char_id==char_id ){
			oldonline=m->online;
			m->online=online;
			m->lv=lv;
			idx=i;
		}
		if(m->account_id>0){
			alv+=m->lv;
			c++;
		}
		if(m->online)
			om++;
	}
	if(idx==g->max_member){
		printf("guild: not found member %d,%d on %d[%s]\n",
			account_id,char_id,guild_id,g->name);
		return 0;
	}
	g->average_lv=alv/c;
	g->connect_member=om;

	if(oldonline!=online)	// オンライン状態が変わったので通知
		clif_guild_memberlogin_notice(g,idx,online);

	for(i=0;i<g->max_member;i++){	// sd再設定
		struct map_session_data *sd= map_id2sd(g->member[i].account_id);
		g->member[i].sd=(sd!=NULL &&
			sd->status.char_id==g->member[i].char_id &&
			sd->status.guild_id==guild_id)?sd:NULL;
	}
	
	// ここにクライアントに送信処理が必要
	
	return 0;
}
// ギルド会話送信
int guild_send_message(struct map_session_data *sd,char *mes,int len)
{
	if(sd->status.guild_id==0)
		return 0;
	intif_guild_message(sd->status.guild_id,sd->status.account_id,mes,len);
	return 0;
}
// ギルド会話受信
int guild_recv_message(int guild_id,int account_id,char *mes,int len)
{
	struct guild *g;
	if( (g=guild_search(guild_id))==NULL)
		return 0;
	clif_guild_message(g,account_id,mes,len);
	return 0;
}
// ギルドメンバの役職変更
int guild_change_memberposition(int guild_id,int account_id,int char_id,int idx)
{
	return intif_guild_change_memberinfo(
		guild_id,account_id,char_id,GMI_POSITION,&idx,sizeof(idx));
}
// ギルドメンバの役職変更通知
int guild_memberposition_changed(struct guild *g,int idx,int pos)
{
	g->member[idx].position=pos;
	clif_guild_memberpositionchanged(g,idx);
	return 0;
}
// ギルド役職変更
int guild_change_position(struct map_session_data *sd,int idx,
	int mode,int exp_mode,const char *name)
{
	struct guild_position p;
	if(exp_mode>100)exp_mode=100;
	if(exp_mode<0)exp_mode=0;
	p.mode=mode;
	p.exp_mode=exp_mode;
	memcpy(p.name,name,24);
	return intif_guild_position(sd->status.guild_id,idx,&p);
}
// ギルド役職変更通知
int guild_position_changed(int guild_id,int idx,struct guild_position *p)
{
	struct guild *g=guild_search(guild_id);
	if(g==NULL)
		return 0;
	memcpy(&g->position[idx],p,sizeof(struct guild_position));
	clif_guild_positionchanged(g,idx);
	return 0;
}
// ギルド告知変更
int guild_change_notice(struct map_session_data *sd,int guild_id,const char *mes1,const char *mes2)
{
	if(guild_id!=sd->status.guild_id)
		return 0;
	return intif_guild_notice(guild_id,mes1,mes2);
}
// ギルド告知変更通知
int guild_notice_changed(int guild_id,const char *mes1,const char *mes2)
{
	int i;
	struct map_session_data *sd;
	struct guild *g=guild_search(guild_id);
	if(g==NULL)
		return 0;

	memcpy(g->mes1,mes1,60);
	memcpy(g->mes2,mes2,120);

	for(i=0;i<g->max_member;i++){
		if((sd=g->member[i].sd)!=NULL)
			clif_guild_notice(sd,g);
	}
	return 0;
}
// ギルドエンブレム変更
int guild_change_emblem(struct map_session_data *sd,int len,const char *data)
{
	return intif_guild_emblem(sd->status.guild_id,len,data);
}
// ギルドエンブレム変更通知
int guild_emblem_changed(int len,int guild_id,int emblem_id,const char *data)
{
	int i;
	struct map_session_data *sd;
	struct guild *g=guild_search(guild_id);
	if(g==NULL)
		return 0;
	
	memcpy(g->emblem_data,data,len);
	g->emblem_len=len;
	g->emblem_id=emblem_id;
	
	for(i=0;i<g->max_member;i++){
		if((sd=g->member[i].sd)!=NULL){
			sd->guild_emblem_id=emblem_id;
			clif_guild_belonginfo(sd,g);
			clif_guild_emblem(sd,g);
		}
	}
	return 0;
}

// ギルドのEXP上納
int guild_payexp(struct map_session_data *sd,int exp)
{
	struct guild *g;
	struct guild_expcache *c;
	int per,exp2;
	if(sd->status.guild_id==0 || (g=guild_search(sd->status.guild_id))==NULL )
		return 0;
	if( (per=g->position[guild_getposition(sd,g)].exp_mode)<=0 )
		return 0;
	if( per>100 )per=100;

	if( (exp2=exp*per/100)<=0 )
		return 0;
	
	if( (c=numdb_search(guild_expcache_db,sd->status.char_id))==NULL ){
		c=malloc(sizeof(struct guild_expcache));
		if(c==NULL){
			printf("guild_payexp: out of memory !\n");
			return 0;
		}
		c->guild_id=sd->status.guild_id;
		c->account_id=sd->status.account_id;
		c->char_id=sd->status.char_id;
		c->exp=exp2;
		numdb_insert(guild_expcache_db,c->char_id,c);
	}else{
		c->exp+=exp2;
	}
	return exp2;
}

// スキルポイント割り振り
int guild_skillup(struct map_session_data *sd,int skill_num)
{
	struct guild *g;
	int idx;
	if(sd->status.guild_id==0 || (g=guild_search(sd->status.guild_id))==NULL)
		return 0;
	if(strcmp(sd->status.name,g->master))
		return 0;
	
	if( g->skill_point>0 &&
		g->skill[(idx=skill_num-10000)].id!=0 &&
		g->skill[idx].lv < guild_skill_get_max(skill_num) ){
		intif_guild_skillup(g->guild_id,skill_num,sd->status.account_id);
	}
	return 0;
}
// スキルポイント割り振り通知
int guild_skillupack(int guild_id,int skill_num,int account_id)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct guild *g=guild_search(guild_id);
	int i;
	if(g==NULL)
		return 0;
	if(sd!=NULL)
		clif_guild_skillup(sd,skill_num,g->skill[skill_num-10000].lv);
	// 全員に通知
	for(i=0;i<g->max_member;i++)
		if((sd=g->member[i].sd)!=NULL)
			clif_guild_skillinfo(sd);
	return 0;
}

// ギルド同盟数所得
int guild_get_alliance_count(struct guild *g,int flag)
{
	int i,c;
	for(i=c=0;i<MAX_GUILDALLIANCE;i++){
		if(	g->alliance[i].guild_id>0 &&
			g->alliance[i].opposition==flag )
			c++;
	}
	return c;
}
// ギルド同盟要求
int guild_reqalliance(struct map_session_data *sd,int account_id)
{
	struct map_session_data *tsd= map_id2sd(account_id);
	struct guild *g[2];
	int i;
	
	if(tsd==NULL || tsd->status.guild_id<=0)
		return 0;
	
	g[0]=guild_search(sd->status.guild_id);
	g[1]=guild_search(tsd->status.guild_id);
	
	if(g[0]==NULL || g[1]==NULL)
		return 0;
	
	if( guild_get_alliance_count(g[0],0)>3 )	// 同盟数確認
		clif_guild_allianceack(sd,4);
	if( guild_get_alliance_count(g[1],0)>3 )
		clif_guild_allianceack(sd,3);
	
	if( tsd->guild_alliance>0 ){	// 相手が同盟要請状態かどうか確認
		clif_guild_allianceack(sd,1);
		return 0;
	}

	for(i=0;i<MAX_GUILDALLIANCE;i++){	// すでに同盟状態か確認
		if(	g[0]->alliance[i].guild_id==tsd->status.guild_id &&
			g[0]->alliance[i].opposition==0){
			clif_guild_allianceack(sd,0);
			return 0;
		}
	}
	
	tsd->guild_alliance=sd->status.guild_id;
	tsd->guild_alliance_account=sd->status.account_id;

	clif_guild_reqalliance(tsd,sd->status.account_id,g[0]->name);
	return 0;
}
// ギルド勧誘への返答
int guild_reply_reqalliance(struct map_session_data *sd,int account_id,int flag)
{
	struct map_session_data *tsd= map_id2sd( account_id );

	if(sd->guild_alliance!=tsd->status.guild_id)	// 勧誘とギルドIDが違う
		return 0;

	if(flag==1){	// 承諾
		int i;
		
		struct guild *g;	// 同盟数再確認
		if( (g=guild_search(sd->status.guild_id))==NULL ||
			guild_get_alliance_count(g,0)>3 ){
			clif_guild_allianceack(sd,4);
			clif_guild_allianceack(tsd,3);
			return 0;
		}
		if( (g=guild_search(tsd->status.guild_id))==NULL ||
			guild_get_alliance_count(g,0)>3 ){
			clif_guild_allianceack(sd,3);
			clif_guild_allianceack(tsd,4);
			return 0;
		}
		
		// 敵対関係なら敵対を止める
		g=guild_search(sd->status.guild_id);
		for(i=0;i<MAX_GUILDALLIANCE;i++){
			if(	g->alliance[i].guild_id==tsd->status.guild_id &&
				g->alliance[i].opposition==1)
				intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
					sd->status.account_id,tsd->status.account_id,9 );
		}
		g=guild_search(tsd->status.guild_id);
		for(i=0;i<MAX_GUILDALLIANCE;i++){
			if(	g->alliance[i].guild_id==sd->status.guild_id &&
				g->alliance[i].opposition==1)
				intif_guild_alliance( tsd->status.guild_id,sd->status.guild_id,
					tsd->status.account_id,sd->status.account_id,9 );
		}

		// inter鯖へ同盟要請
		intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
			sd->status.account_id,tsd->status.account_id,0 );
		return 0;
	}else{		// 拒否
		sd->guild_alliance=0;
		sd->guild_alliance_account=0;
		if(tsd!=NULL)
			clif_guild_allianceack(tsd,3);
	}
	return 0;
}
// ギルド関係解消
int guild_delalliance(struct map_session_data *sd,int guild_id,int flag)
{
	intif_guild_alliance( sd->status.guild_id,guild_id,
		sd->status.account_id,0,flag|8 );
	return 0;
}
// ギルド敵対
int guild_opposition(struct map_session_data *sd,int char_id)
{
	struct map_session_data *tsd=map_id2sd(char_id);
	struct guild *g=guild_search(sd->status.guild_id);
	int i;
	
	if(g==NULL || tsd==NULL)
		return 0;	
	
	if( guild_get_alliance_count(g,1)>3 )	// 敵対数確認
		clif_guild_oppositionack(sd,1);
	
	for(i=0;i<MAX_GUILDALLIANCE;i++){	// すでに関係を持っているか確認
		if(g->alliance[i].guild_id==tsd->status.guild_id){
			if(g->alliance[i].opposition==1){	// すでに敵対
				clif_guild_oppositionack(sd,2);
				return 0;
			}else	// 同盟破棄
				intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
					sd->status.account_id,tsd->status.account_id,8 );
		}
	}

	// inter鯖に敵対要請
	intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
			sd->status.account_id,tsd->status.account_id,1 );
	return 0;
}
// ギルド同盟/敵対通知
int guild_allianceack(int guild_id1,int guild_id2,int account_id1,int account_id2,
	int flag,const char *name1,const char *name2)
{
	struct guild *g[2];
	int guild_id[2]={guild_id1,guild_id2};
	const char *guild_name[2]={name1,name2};
	struct map_session_data *sd[2]={map_id2sd(account_id1),map_id2sd(account_id2)};
	int j,i;
	
	g[0]=guild_search(guild_id1);
	g[1]=guild_search(guild_id2);
	
	if(sd[0]!=NULL && (flag&0x0f)==0){
		sd[0]->guild_alliance=0;
		sd[0]->guild_alliance_account=0;
	}
	
	if(flag&0x70){	// 失敗
		for(i=0;i<2-(flag&1);i++)
			if( sd[i]!=NULL )
				clif_guild_allianceack(sd[i],((flag>>4)==i+1)?3:4);
		return 0;
	}
//	printf("guild alliance_ack %d %d %d %d %d %s %s\n",guild_id1,guild_id2,
//		account_id1,account_id2,flag,name1,name2);

	if(!(flag&0x08)){	// 関係追加
		for(i=0;i<2-(flag&1);i++)
			if(g[i]!=NULL)
				for(j=0;j<MAX_GUILDALLIANCE;j++)
					if(g[i]->alliance[j].guild_id==0){
						g[i]->alliance[j].guild_id=guild_id[1-i];
						memcpy(g[i]->alliance[j].name,guild_name[1-i],24);
						g[i]->alliance[j].opposition=flag&1;
						break;
					}
	}else{				// 関係解消
		for(i=0;i<2-(flag&1);i++){
			if(g[i]!=NULL)
				for(j=0;j<MAX_GUILDALLIANCE;j++)
					if(	g[i]->alliance[j].guild_id==guild_id[1-i] &&
						g[i]->alliance[j].opposition==(flag&1)){
						g[i]->alliance[j].guild_id=0;
						break;
					}
			if( sd[i]!=NULL )	// 解消通知
				clif_guild_delalliance(sd[i],guild_id[1-i],(flag&1));
		}
	}
	
	if((flag&0x0f)==0){			// 同盟通知
		if( sd[1]!=NULL )
			clif_guild_allianceack(sd[1],2);
	}else if((flag&0x0f)==1){	// 敵対通知
		if( sd[0]!=NULL )
			clif_guild_oppositionack(sd[0],0);
	}


	for(i=0;i<2-(flag&1);i++){	// 同盟/敵対リストの再送信
		struct map_session_data *sd;
		if(g[i]!=NULL)
			for(j=0;j<g[i]->max_member;j++)
				if((sd=g[i]->member[j].sd)!=NULL)
					clif_guild_allianceinfo(sd);
	}
	return 0;
}
// ギルド解散通知用
int guild_broken_sub(void *key,void *data,va_list ap)
{
	struct guild *g=(struct guild *)data;
	int guild_id=va_arg(ap,int);
	int i,j;
	struct map_session_data *sd=NULL;
	
	for(i=0;i<MAX_GUILDALLIANCE;i++){	// 関係を破棄
		if(g->alliance[i].guild_id==guild_id){
			for(j=0;j<g->max_member;j++)
				if( (sd=g->member[j].sd)!=NULL )
					clif_guild_delalliance(sd,guild_id,g->alliance[i].opposition);
			g->alliance[i].guild_id=0;
		}
	}
	return 0;
}
// ギルド解散通知
int guild_broken(int guild_id,int flag)
{
	struct guild *g=guild_search(guild_id);
	struct map_session_data *sd;
	int i;
	if(flag!=0 || g==NULL)
		return 0;
	
	for(i=0;i<g->max_member;i++){	// ギルド解散を通知
		if((sd=g->member[i].sd)!=NULL){
			sd->status.guild_id=0;
			sd->guild_sended=0;
			clif_guild_broken(g->member[i].sd,0);
		}
	}
	
	numdb_foreach(guild_db,guild_broken_sub,guild_id);
	numdb_erase(guild_db,guild_id);
	free(g);
	return 0;
}

// ギルド解散
int guild_break(struct map_session_data *sd,char *name)
{
	struct guild *g;
	int i;
	if( (g=guild_search(sd->status.guild_id))==NULL )
		return 0;
	if(strcmp(g->name,name)!=0)
		return 0;
	if(strcmp(sd->status.name,g->master)!=0)
		return 0;
	for(i=0;i<g->max_member;i++){
		if(	g->member[i].account_id>0 && (
			g->member[i].account_id!=sd->status.account_id ||
			g->member[i].char_id!=sd->status.char_id ))
			break;
	}
	if(i<g->max_member){
		clif_guild_broken(sd,2);
		return 0;
	}
	
	intif_guild_break(g->guild_id);
	return 0;
}

