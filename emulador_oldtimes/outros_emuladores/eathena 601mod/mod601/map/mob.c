// $Id: mob.c,v 1.14 2003/06/30 14:45:10 lemit Exp $
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "timer.h"
#include "db.h"
#include "map.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "mob.h"
#include "itemdb.h"
#include "skill.h"
#include "battle.h"
#include "party.h"
#include "npc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define MIN_MOBTHINKTIME 100

#define MOB_LAZYMOVEPERC 50		// 手抜きモードMOBの移動確率（千分率）
#define MOB_LAZYWARPPERC 20		// 手抜きモードMOBのワープ確率（千分率）

struct mob_db mob_db[2000];

/*==========================================
 * ローカルプロトタイプ宣言 (必要な物のみ)
 *------------------------------------------
 */
static int distance(int,int,int,int);
static int mob_makedummymobdb(int);
static int mob_timer(int,unsigned int,int,int);
int mobskill_use(struct mob_data *md,unsigned int tick,int event);
int mobskill_deltimer(struct mob_data *md );


/*==========================================
 * mobを名前で検索
 *------------------------------------------
 */
int mobdb_searchname(const char *str)
{
	int i;
	for(i=0;i<sizeof(mob_db)/sizeof(mob_db[0]);i++){
		if(strcmpi(mob_db[i].name,str)==0 || strcmp(mob_db[i].jname,str)==0)
			return i;
	}
	return 0;
}
/*==========================================
 * MOB出現用の最低限のデータセット
 *------------------------------------------
 */
int mob_spawn_dataset(struct mob_data *md,const char *mobname,int class)
{
	md->bl.prev=NULL;
	md->bl.next=NULL;
	if(strcmp(mobname,"--en--")==0)
		memcpy(md->name,mob_db[class].name,24);
	else if(strcmp(mobname,"--ja--")==0)
		memcpy(md->name,mob_db[class].jname,24);
	else
		memcpy(md->name,mobname,24);

	md->n = 0;
	md->class=class;
	md->bl.id= npc_get_new_npc_id();

	memset(&md->state,0,sizeof(md->state));
	md->timer = -1;
	md->target_id=0;
	md->attacked_id=0;
	md->speed=mob_db[class].speed;

	return 0;
}


/*==========================================
 * 一発MOB出現(スクリプト用)
 *------------------------------------------
 */
int mob_once_spawn(struct map_session_data *sd,char *mapname,
	int x,int y,const char *mobname,int class,int amount,const char *event)
{
	struct mob_data *md;
	int m,i;
	
	if(strcmp(mapname,"this")==0)
		m=sd->bl.m;
	else
		m=map_mapname2mapid(mapname);

	if(m<0 || amount<=0)
		return 0;

	if(class<=0){	// ランダムに召喚
		int i=0;
		do{
			class=rand()%1000+1000;
		}while( (mob_db[class].max_hp<=0 || !mob_db[class].summonflag || sd->status.base_level < mob_db[class].lv)&& (i++)<1000);
		if(i>=1000)class=1002;	// 強制ポリン
//		printf("mobclass=%d try=%d\n",class,i);
	}
	if(x<=0) x=sd->bl.x;
	if(y<=0) y=sd->bl.y;
	
	for(i=amount;i>0;i--){
		md=malloc(sizeof(struct mob_data));
		if(md==NULL){
			printf("mob_once_spawn: out of memory !\n");
			return 0;
		}
		if(mob_db[class].mode&0x02) {
			md->lootitem=malloc(sizeof(struct item)*LOOTITEM_SIZE);
			if(md->lootitem==NULL){
				printf("mob_once_spawn: out of memory !\n");
			}
		}
		else
			md->lootitem=NULL;

		mob_spawn_dataset(md,mobname,class);
		md->bl.m=m;
		md->bl.x=x;
		md->bl.y=y;

		md->x0=x;
		md->y0=y;
		md->xs=0;
		md->ys=0;
		md->spawndelay1=-1;	// 一度のみフラグ
		md->spawndelay2=-1;	// 一度のみフラグ

		memcpy(md->npc_event,event,sizeof(md->npc_event));

		md->bl.type=BL_MOB;
		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);

	}
	return (amount>0)?md->bl.id:0;
}
/*==========================================
 * 一発MOB出現(スクリプト用＆エリア指定)
 *------------------------------------------
 */
int mob_once_spawn_area(struct map_session_data *sd,char *mapname,
	int x0,int y0,int x1,int y1,
	const char *mobname,int class,int amount,const char *event)
{
	int x,y,i,c,max,lx=-1,ly=-1,id=0;
	int m=map_mapname2mapid(mapname);

	max=(y1-y0+1)*(x1-x0+1)*3;
	if(max>1000)max=1000;

	if(m<0)
		return 0;
	
	for(i=0;i<amount;i++){
		int j=0;
		do{
			x=rand()%(x1-x0+1)+x0;
			y=rand()%(y1-y0+1)+y0;
		}while( ( (c=map_getcell(m,x,y))==1 || c==5)&& (++j)<max );
		if(j>=max){
			if(lx>=0){	// 検索に失敗したので以前に沸いた場所を使う
				x=lx;
				y=ly;
			}else
				return 0;	// 最初に沸く場所の検索を失敗したのでやめる
		}
		id=mob_once_spawn(sd,mapname,x,y,mobname,class,1,event);
		lx=x;
		ly=y;
	}
	return id;
}

/*==========================================
 * mobのadelay所得
 *------------------------------------------
 */
static int mob_get_adelay(struct mob_data *md)
{
	int a=mob_db[md->class].adelay;
	if( md->sc_data[SC_ADRENALINE].timer!=-1)	// アドレナリンラッシュ
		a = a *70/100;
	if( md->sc_data[SC_TWOHANDQUICKEN].timer!=-1)	// 2HQ
		a = a *70/100;
	return a;
}
/*==========================================
 * mobのspeed所得
 *------------------------------------------
 */
static int mob_get_speed(struct mob_data *md)
{
	int a=mob_db[md->class].speed;

	if(md->sc_data[SC_INCREASEAGI].timer!=-1)	// 速度増加
		a -= a*25/100;
	if(md->sc_data[SC_INCREASEAGI].timer!=-1)	// 速度減少
		a = a*125/100;
	if(md->sc_data[SC_QUAGMIRE].timer!=-1)	// クァグマイア(AGI/DEXはbattle.cで)
		a = a*3/2;
	
	return md->speed=a;
}

/*==========================================
 * mobの次の1歩にかかる時間計算
 *------------------------------------------
 */
static int calc_next_walk_step(struct mob_data *md)
{
	if(md->walkpath.path_pos>=md->walkpath.path_len)
		return -1;
	if(md->walkpath.path[md->walkpath.path_pos]&1)
		return mob_get_speed(md)*14/10;
	return mob_get_speed(md);
}

/*==========================================
 * mob歩行処理
 *------------------------------------------
 */
static int mob_walk(struct mob_data *md,unsigned int tick,int data)
{
	int moveblock;
	int i;
	static int dirx[8]={0,-1,-1,-1,0,1,1,1};
	static int diry[8]={1,1,0,-1,-1,-1,0,1};
	int x,y,dx,dy;

	md->state.state=MS_IDLE;
	if(md->walkpath.path_pos>=md->walkpath.path_len || md->walkpath.path_pos!=data)
		return 0;

	if(md->walkpath.path[md->walkpath.path_pos]>=8)
		return 1;

	md->dir=md->walkpath.path[md->walkpath.path_pos];
	x = md->bl.x;
	y = md->bl.y;
	dx = dirx[md->dir];
	dy = diry[md->dir];
	md->walkpath.path_pos++;
	moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

	md->state.state=MS_WALK;
	map_foreachinmovearea(clif_moboutsight,md->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,md);

	x += dx;
	y += dy;
	if(md->min_chase>13)
		md->min_chase--;

	if(moveblock) map_delblock(&md->bl);
	md->bl.x = x;
	md->bl.y = y;
	if(moveblock) map_addblock(&md->bl);

	map_foreachinmovearea(clif_mobinsight,md->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,md);
	md->state.state=MS_IDLE;

	skill_unit_move(&md->bl,tick,1);	// スキルユニットの検査

	if(md->state.state!=MS_DEAD && (i=calc_next_walk_step(md))>0){
		md->timer=add_timer(tick+i,mob_timer,md->bl.id,md->walkpath.path_pos);
		md->state.state=MS_WALK;

		if(md->walkpath.path_pos>=md->walkpath.path_len)
			clif_fixmobpos(md);	// とまったときに位置の再送信
	}
	return 0;
}

/*==========================================
 * mobの攻撃処理
 *------------------------------------------
 */
static int mob_attack(struct mob_data *md,unsigned int tick,int data)
{
	struct map_session_data *sd;
	int mode,race;

	md->min_chase=13;
	md->state.state=MS_IDLE;
	md->state.skillstate=MSS_ATTACK;

	if( md->skilltimer!=-1 )	// スキル使用中
		return 0;

	sd=map_id2sd(md->target_id);
	if(sd==NULL || pc_isdead(sd) || md->bl.m != sd->bl.m || sd->bl.prev == NULL || sd->ghost_timer != -1 ||
	   distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y)>=13){
		md->target_id=0;
		md->state.targettype = NONE_ATTACKABLE;
		return 0;
	}

	mode=mob_db[md->class].mode;
	race=mob_db[md->class].race;
	if( mob_db[md->class].mexp <= 0 && !(mode&0x20) &&
		(sd->sc_data[SC_TRICKDEAD].timer != -1 ||
		 (sd->status.option&0x06 && race!=4 && race!=6)) ) {
		md->target_id=0;
		md->state.targettype = NONE_ATTACKABLE;
		return 0;
	}

	if(distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y) > mob_db[md->class].range+1){
		md->state.state=MS_IDLE;
		return 0;
	}
	md->dir=map_calc_dir(&md->bl, sd->bl.x,sd->bl.y );	// 向き設定

	clif_fixmobpos(md);

	if( mobskill_use(md,tick,-2) )	// スキル使用
		return 0;
	
	if(sd->sc_data[SC_AUTOCOUNTER].timer != -1){
		pc_attack(sd,md->bl.id,0);
		skill_status_change_end(&sd->bl,SC_AUTOCOUNTER,-1);
		return 0;
	}
	if(sd->sc_data[SC_AUTOGUARD].timer != -1){
		pc_attack(sd,md->bl.id,0);
		skill_status_change_end(&sd->bl,SC_AUTOGUARD,-1);
		return 0;
	}
	if(sd->sc_data[SC_REFLECTSHIELD].timer != -1){
		pc_attack(sd,md->bl.id,0);
		skill_status_change_end(&sd->bl,SC_AUTOCOUNTER,-1);
		return 0;
	}

	battle_weapon_attack(&md->bl,&sd->bl,tick,0);

	md->timer=add_timer(tick+mob_get_adelay(md),mob_timer,md->bl.id,0);
	md->state.state=MS_ATTACK;

	return 0;
}


/*==========================================
 * idを攻撃しているPCの攻撃を停止
 * clif_foreachclientのcallback関数
 *------------------------------------------
 */
int mob_stopattacked(struct map_session_data *sd,va_list ap)
{
	int id;

	id=va_arg(ap,int);
	if(sd->attacktarget==id)
		pc_stopattack(sd);
	return 0;
}
/*==========================================
 * 現在動いているタイマを止めて状態を変更
 *------------------------------------------
 */
int mob_changestate(struct mob_data *md,int state,int type)
{
	int i;

	if(md->timer != -1)
		delete_timer(md->timer,mob_timer);
	md->timer=-1;
	md->state.state=state;

	switch(state){
	case MS_WALK:
		if((i=calc_next_walk_step(md))>0){
			i = i/2;
			if(i <= 0)
				i = 1;
			md->timer=add_timer(gettick()+i,mob_timer,md->bl.id,0);
		} else
			md->state.state=MS_IDLE;
		break;
	case MS_ATTACK:
		if(type)
			md->timer=add_timer(gettick()+mob_db[md->class].amotion,mob_timer,md->bl.id,0);
		else
			md->timer=add_timer(gettick()+10,mob_timer,md->bl.id,0);
		break;
	case MS_DEAD:
		mobskill_deltimer(md);
		md->state.skillstate=MSS_DEAD;
		md->last_deadtime=gettick();
		// 死んだのでこのmobへの攻撃者全員の攻撃を止める
		clif_foreachclient(mob_stopattacked,md->bl.id);
		skill_status_change_clear(&md->bl);	// ステータス異常を解除する
		skill_clear_unitgroup(&md->bl);	// 全てのスキルユニットグループを削除する
		skill_cleartimerskill(&md->bl);
		md->hp=md->target_id=md->attacked_id=0;
		md->state.targettype = NONE_ATTACKABLE;
		break;
	}

	return 0;
}

/*==========================================
 * mobのtimer処理 (timer関数)
 * 歩行と攻撃に分岐
 *------------------------------------------
 */
static int mob_timer(int tid,unsigned int tick,int id,int data)
{
	struct mob_data *md;

	md=(struct mob_data*)map_id2bl(id);
	if(md==NULL || md->bl.type!=BL_MOB)
		return 1;

	if(md->timer != tid){
		printf("mob_timer %d != %d\n",md->timer,tid);
		return 0;
	}
	md->timer=-1;

	switch(md->state.state){
	case MS_WALK:
		mob_walk(md,tick,data);
		break;
	case MS_ATTACK:
		mob_attack(md,tick,data);
		break;
	case MS_DELAY:
		mob_changestate(md,MS_IDLE,0);
		break;
	default:
		printf("mob_timer : %d ?\n",md->state.state);
	}

	return 0;
}

/*==========================================
 * mob移動開始
 *------------------------------------------
 */
int mob_walktoxy(struct mob_data *md,int x,int y,int easy)
{
	struct walkpath_data wpd;

	if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,x,y,easy))
		return 1;
	md->to_x=x;
	md->to_y=y;

	memcpy(&md->walkpath,&wpd,sizeof(wpd));

	mob_changestate(md,MS_WALK,0);
	clif_movemob(md);
//	printf("walkstart\n");

	return 0;
}

/*==========================================
 * delay付きmob spawn (timer関数)
 *------------------------------------------
 */
static int mob_delayspawn(int tid,unsigned int tick,int m,int n)
{
	mob_spawn(m);
	return 0;
}

/*==========================================
 * spawnタイミング計算
 *------------------------------------------
 */
int mob_setdelayspawn(int id)
{
	unsigned int spawntime,spawntime1,spawntime2,spawntime3;
	struct mob_data *md;

	md=(struct mob_data*)map_id2bl(id);
	if(md==NULL || md->bl.type!=BL_MOB)
		return -1;

	// 復活しないMOBの処理
	if(md->spawndelay1==-1 && md->spawndelay2==-1 && md->n==0){
		map_deliddb(&md->bl);
		if(md->lootitem) {
			map_freeblock(md->lootitem);
			md->lootitem=NULL;
		}
		map_freeblock(md);	// freeのかわり
		return 0;
	}

	spawntime1=md->last_spawntime+md->spawndelay1;
	spawntime2=md->last_deadtime+md->spawndelay2;
	spawntime3=gettick()+5000;
	// spawntime = max(spawntime1,spawntime2,spawntime3);
	if(DIFF_TICK(spawntime1,spawntime2)>0){
		spawntime=spawntime1;
	} else {
		spawntime=spawntime2;
	}
	if(DIFF_TICK(spawntime3,spawntime)>0){
		spawntime=spawntime3;
	}

	add_timer(spawntime,mob_delayspawn,id,0);
	return 0;
}

/*==========================================
 * mob出現。色々初期化もここで
 *------------------------------------------
 */
int mob_spawn(int id)
{
	int x=0,y=0,i=0,c;
	struct mob_data *md;

	md=(struct mob_data*)map_id2bl(id);
	if(md==NULL || md->bl.type!=BL_MOB)
		return -1;

	md->last_spawntime=gettick();
	if( md->bl.prev!=NULL ){
//		clif_clearchar_area(&md->bl,3);
		map_delblock(&md->bl);
	}
	
	do {
		if(md->x0==0 && md->y0==0){
			x=rand()%(map[md->bl.m].xs-2)+1;
			y=rand()%(map[md->bl.m].ys-2)+1;
		} else {
			x=md->x0+rand()%(md->xs+1)-md->xs/2;
			y=md->y0+rand()%(md->ys+1)-md->ys/2;
		}
		i++;
	} while(((c=map_getcell(md->bl.m,x,y))==1 || c==5) && i<50);

	if(i>=50){
//		printf("MOB spawn error %d @ %s\n",id,map[md->bl.m].name);
		add_timer(gettick()+5000,mob_delayspawn,id,0);
		return 1;
	}

	md->to_x=md->bl.x=x;
	md->to_y=md->bl.y=y;
	md->dir=0;

	map_addblock(&md->bl);

	md->hp = mob_db[md->class].max_hp;
	if(md->hp<=0){
		mob_makedummymobdb(md->class);
		md->hp = mob_db[md->class].max_hp;
	}

	memset(&md->state,0,sizeof(md->state));
	md->attacked_id = 0;
	md->target_id = 0;
	md->move_fail_count = 0;

	md->speed = mob_db[md->class].speed;
	md->def_ele = mob_db[md->class].element;
	md->master_id=0;
	md->master_dist=0;

	md->state.state = MS_IDLE;
	md->state.skillstate = MSS_IDLE;
	md->timer = -1;
	md->last_thinktime = gettick();
	md->next_walktime = gettick()+rand()%50+5000;

	md->skilltimer=-1;
	for(i=0,c=gettick()-1000*3600*10;i<MAX_MOBSKILL;i++)
		md->skilldelay[i] = c;
	md->skillid=0;
	md->skilllv=0;

	memset(md->dmglog,0,sizeof(md->dmglog));
	if(md->lootitem)
		memset(md->lootitem,0,sizeof(md->lootitem));
	md->lootitem_count = 0;
	
	for(i=0;i<MAX_SKILLTIMERSKILL/2;i++)
		md->skilltimerskill[i].timer = -1;

	for(i=0;i<MAX_STATUSCHANGE;i++)
		md->sc_data[i].timer=-1;
	md->sc_count=0;
	md->opt1=md->opt2=md->option=0;

	memset(md->skillunit,0,sizeof(md->skillunit));
	memset(md->skillunittick,0,sizeof(md->skillunittick));

	clif_spawnmob(md);

	return 0;
}

/*==========================================
 * 2点間距離計算
 *------------------------------------------
 */
static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

/*==========================================
 * MOBの攻撃停止
 *------------------------------------------
 */
int mob_stopattack(struct mob_data *md)
{
	md->target_id=0;
	md->state.targettype = NONE_ATTACKABLE;
	md->attacked_id=0;
	return 0;
}
/*==========================================
 * MOBの移動中止
 *------------------------------------------
 */
int mob_stop_walking(struct mob_data *md,int type)
{
	int delay=mob_db[md->class].dmotion;
//	printf("stop walking\n");
	md->walkpath.path_len=0;
	md->to_x=md->bl.x;
	md->to_y=md->bl.y;
	if(type&0x01)
		clif_fixpos(&md->bl);
	if(type&0x02) {
		mob_changestate(md,MS_DELAY,0);
		md->timer=add_timer(gettick()+delay,mob_timer,md->bl.id,0);
		md->state.state=MS_DELAY;
	}
	else
		mob_changestate(md,MS_IDLE,0);

	return 0;
}

/*==========================================
 * 指定IDの存在場所への到達可能性
 *------------------------------------------
 */
int mob_can_reach(struct mob_data *md,struct block_list *bl,int range)
{
	int dx=abs(bl->x - md->bl.x),dy=abs(bl->y - md->bl.y);
	struct walkpath_data wpd;
	if( range>0 && range < ((dx>dy)?dx:dy) )	// 遠すぎる
		return 0;

	if( md->bl.m != bl-> m)	// 違うマップ
		return 0;

	if( md->bl.x==bl->x && md->bl.y==bl->y )	// 同じマス
		return 1;

	// 障害物判定
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x,bl->y,0)!=-1)?1:0;
}

/*==========================================
 * モンスターの攻撃対象決定
 *------------------------------------------
 */
int mob_target(struct mob_data *md,struct block_list *bl,int dist)
{
	struct map_session_data *sd;
	int mode=mob_db[md->class].mode,race=mob_db[md->class].race;


	if(!mode) {
		md->target_id = 0;
		return 0;
	}
	// タゲ済みでタゲを変える気がないなら何もしない
	if( (md->target_id > 0 && md->state.targettype == ATTACKABLE) && ( !(mode&0x04) || rand()%100>25) )
		return 0;

	if(	mob_db[md->class].mexp > 0 || mode&0x20 ||	// MVPMOBなら強制
		(battle_get_sc_data(bl)[SC_TRICKDEAD].timer == -1 &&
		 ( !(*battle_get_option(bl)&0x06) || race==4 || race==6)
		) ){
		if(bl->type == BL_PC) {
			sd = (struct map_session_data *)bl;
			if(sd->ghost_timer != -1)
				return 0;
		}
		
		md->target_id=bl->id;	// 妨害がなかったのでロック
		if(bl->type == BL_PC || bl->type == BL_MOB)
			md->state.targettype = ATTACKABLE;
		else
			md->state.targettype = NONE_ATTACKABLE;
		md->min_chase=dist+13;
		if(md->min_chase>26)
			md->min_chase=26;
	}
	return 0;
}

/*==========================================
 * MOBが現在移動可能な状態にあるかどうか
 *------------------------------------------
 */
int mob_can_move(struct mob_data *md)
{
	if( md->sc_data[SC_ANKLE].timer!=-1 ){	// アンクル中で動けない
		return 0;
	}

	return 1;
}

/*==========================================
 * アクティブモンスターの策敵ルーティン
 *------------------------------------------
 */
static int mob_ai_sub_hard_activesearch(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd=(struct map_session_data *)bl;
	struct mob_data* md;
	int mode,race,dist,*pcc;

	md=va_arg(ap,struct mob_data *);
	pcc=va_arg(ap,int *);
	mode=mob_db[md->class].mode;

	// アクティブでターゲット射程内にいるなら、ロックする
	if( mode&0x04 ){
		if( !pc_isdead(sd) && sd->bl.m == md->bl.m && sd->ghost_timer == -1 &&
			(dist=distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y))<9){

			race=mob_db[md->class].race;
			if(	( mob_db[md->class].mexp > 0 || mode&0x20 ||
				((sd->sc_data[SC_TRICKDEAD].timer == -1 &&
				(!(sd->status.option&0x06) || race==4 || race==6))
				)) && !(sd->status.option&0x40) ){	// 妨害がないか判定

				if( mob_can_reach(md,bl,12) && 		// 到達可能性判定
					rand()%1000<1000/(++(*pcc)) ){	// 範囲内PCで等確率にする
					md->target_id=sd->bl.id;
					md->state.targettype = ATTACKABLE;
					md->min_chase=13;
				}
			}
		}	
	}
	return 0;
}

/*==========================================
 * loot monster item search
 *------------------------------------------
 */
static int mob_ai_sub_hard_lootsearch(struct block_list *bl,va_list ap)
{
	struct mob_data* md;
	int mode,dist,*itc;

	md=va_arg(ap,struct mob_data *);
	itc=va_arg(ap,int *);
	mode=mob_db[md->class].mode;

	if( !md->target_id && mode&0x02){
		if(!md->lootitem || (battle_config.monster_loot_type == 1 && md->lootitem_count >= LOOTITEM_SIZE) )
			return 0;
		if(bl->m == md->bl.m && (dist=distance(md->bl.x,md->bl.y,bl->x,bl->y))<9){
			if( mob_can_reach(md,bl,12) && 		// 到達可能性判定
				rand()%1000<1000/(++(*itc)) ){	// 範囲内PCで等確率にする
				md->target_id=bl->id;
				md->state.targettype = NONE_ATTACKABLE;
				md->min_chase=13;
			}
		}	
	}
	return 0;
}

/*==========================================
 * リンクモンスターの策敵ルーティン
 *------------------------------------------
 */
static int mob_ai_sub_hard_linksearch(struct block_list *bl,va_list ap)
{
	struct mob_data *tmd=(struct mob_data *)bl;
	struct mob_data* md;
	struct block_list *target;
	
	md=va_arg(ap,struct mob_data *);
	target=va_arg(ap,struct block_list *);
	
	// リンクモンスターで射程内に暇な同族MOBがいるなら、ロックさせる
/*	if( (md->target_id > 0 && md->state.targettype == ATTACKABLE) && mob_db[md->class].mode&0x08){
		if( tmd->class==md->class && (!tmd->target_id || md->state.targettype == NONE_ATTACKABLE) && tmd->bl.m == md->bl.m){
			if( mob_can_reach(tmd,target,12) ){	// 到達可能性判定
				tmd->target_id=md->target_id;
				tmd->state.targettype = ATTACKABLE;
				tmd->min_chase=13;
			}
		}	
	}*/
	if( md->attacked_id > 0 && mob_db[md->class].mode&0x08){
		if( tmd->class==md->class && tmd->bl.m == md->bl.m && (!tmd->target_id || md->state.targettype == NONE_ATTACKABLE)){
			if( mob_can_reach(tmd,target,12) ){	// 到達可能性判定
				tmd->target_id=md->attacked_id;
				tmd->state.targettype = ATTACKABLE;
				tmd->min_chase=13;
			}
		}
	}

	return 0;
}
/*==========================================
 * 取り巻きモンスターの主検索
 *------------------------------------------
 */
static int mob_ai_sub_hard_mastersearch(struct block_list *bl,va_list ap)
{
	struct mob_data *mmd=(struct mob_data *)bl;
	struct mob_data *md;
	int mode,race,old_dist;
	unsigned int tick;

	md=va_arg(ap,struct mob_data *);
	tick=va_arg(ap,unsigned int);
	mode=mob_db[md->class].mode;

	// 主ではない
	if(mmd->bl.type!=BL_MOB || mmd->bl.id!=md->master_id)
		return 0;

	// 主との距離を測る
	old_dist=md->master_dist;
	md->master_dist=distance(md->bl.x,md->bl.y,mmd->bl.x,mmd->bl.y);
	
	// 直前まで主が近くにいたのでテレポートして追いかける
	if( old_dist<6 && md->master_dist>15){
		mob_warp(md,mmd->bl.x,mmd->bl.y,3);
		md->state.master_check = 1;
		return 0;
	}

	// 主がいるが、少し遠いので近寄る
	if((!md->target_id || md->state.targettype == NONE_ATTACKABLE) && mob_can_move(md) && 
		(md->walkpath.path_pos>=md->walkpath.path_len || md->walkpath.path_len==0) && md->master_dist<15){
		int i=0,dx,dy,ret;
		if(md->master_dist>4) {
			do {
				if(i<=5){
					dx=mmd->bl.x - md->bl.x;
					dy=mmd->bl.y - md->bl.y;
					if(dx<0) dx+=(rand()%( (dx<-3)?3:-dx )+1);
					else if(dx>0) dx-=(rand()%( (dx>3)?3:dx )+1);
					if(dy<0) dy+=(rand()%( (dy<-3)?3:-dy )+1);
					else if(dy>0) dy-=(rand()%( (dy>3)?3:dy )+1);
				}else{
					dx=mmd->bl.x - md->bl.x + rand()%7 - 3;
					dy=mmd->bl.y - md->bl.y + rand()%7 - 3;
				}

				ret=mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);
				i++;
			} while(ret && i<10);
		}
		else {
			do {
				dx = rand()%9 - 5;
				dy = rand()%9 - 5;
				if( dx == 0 && dy == 0) {
					dx = (rand()%1)? 1:-1;
					dy = (rand()%1)? 1:-1;
				}
				dx += mmd->bl.x;
				dy += mmd->bl.y;

				ret=mob_walktoxy(md,mmd->bl.x+dx,mmd->bl.y+dy,0);
				i++;
			} while(ret && i<10);
		}

		md->next_walktime=tick+500;
		md->state.master_check = 1;
	}

	// 主がいて、主がロックしていて自分はロックしていない
	if( (mmd->target_id>0 && mmd->state.targettype == ATTACKABLE) && (!md->target_id || md->state.targettype == NONE_ATTACKABLE) ){
		struct map_session_data *sd=map_id2sd(mmd->target_id);
		if(sd!=NULL && !pc_isdead(sd) && sd->ghost_timer == -1){

			race=mob_db[md->class].race;
			if(	(mob_db[md->class].mexp > 0 || mode&0x20 ||
				((sd->sc_data[SC_TRICKDEAD].timer == -1 &&
				(!(sd->status.option&0x06) || race==4 || race==6))
				)) && !(sd->status.option&0x40) ){	// 妨害がないか判定
				
				md->target_id=sd->bl.id;
				md->state.targettype = ATTACKABLE;
				md->min_chase=5+distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y);
				md->state.master_check = 1;
			}
		}
	}

	// 主がいて、主がロックしてなくて自分はロックしている
/*	if( (md->target_id>0 && mmd->state.targettype == ATTACKABLE) && (!mmd->target_id || mmd->state.targettype == NONE_ATTACKABLE) ){
		struct map_session_data *sd=map_id2sd(md->target_id);
		if(sd!=NULL && !pc_isdead(sd) && sd->ghost_timer == -1){

			race=mob_db[mmd->class].race;
			if(	mob_db[md->class].mexp > 0 || mode&0x20 ||
				(sd->sc_data[SC_TRICKDEAD].timer == -1 &&
				(!(sd->status.option&0x06) || race==4 || race==6)
				) ){	// 妨害がないか判定
				
				mmd->target_id=sd->bl.id;
				mmd->state.targettype = ATTACKABLE;
				mmd->min_chase=5+distance(mmd->bl.x,mmd->bl.y,sd->bl.x,sd->bl.y);
			}
		}
	}*/
		
	return 0;
}

/*==========================================
 * ロックを止めて待機状態に移る。
 *------------------------------------------
 */
static int mob_unlocktarget(struct mob_data *md,int tick)
{
	md->target_id=0;
	md->state.targettype = NONE_ATTACKABLE;
	md->state.skillstate=MSS_IDLE;
	md->next_walktime=tick+rand()%3000+3000;
	return 0;
}
/*==========================================
 * ランダム歩行
 *------------------------------------------
 */
static int mob_randomwalk(struct mob_data *md,int tick)
{
	const int retrycount=20;
	int speed=mob_get_speed(md);
	if(DIFF_TICK(md->next_walktime,tick)<0){
		int i,x,y,c,d=12-md->move_fail_count;
		if(d<5) d=5;
		for(i=0;i<retrycount;i++){	// 移動できる場所の探索
			int r=rand();
			x=md->bl.x+r%(d*2+1)-d;
			y=md->bl.y+r/(d*2+1)%(d*2+1)-d;
			if((c=map_getcell(md->bl.m,x,y))!=1 && c!=5 && mob_walktoxy(md,x,y,1)==0){
				md->move_fail_count=0;
				break;
			}
			if(i+1>=retrycount){
				md->move_fail_count++;
				if(md->move_fail_count>10){
					printf("MOB cant move. random spawn %d, class = %d\n",md->bl.id,md->class);
					md->move_fail_count=0;
					mob_spawn(md->bl.id);
				}
			}
		}
		for(i=c=0;i<md->walkpath.path_len;i++){	// 次の歩行開始時刻を計算
			if(md->walkpath.path[i]&1)
				c+=speed*14/10;
			else
				c+=speed;
		}
		md->next_walktime = tick+rand()%3000+3000+c;
		md->state.skillstate=MSS_WALK;
		return 1;
	}
	return 0;
}

/*==========================================
 * PCが近くにいるMOBのAI
 *------------------------------------------
 */
static int mob_ai_sub_hard(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	struct map_session_data *sd;
	struct block_list *bl_item;
	struct flooritem_data *fitem;
	unsigned int tick;
	int i,dx,dy,ret,dist;
	int attack_type=0;
	int mode,race;

	md=(struct mob_data*)bl;
	tick=va_arg(ap,unsigned int);


	if(DIFF_TICK(tick,md->last_thinktime)<MIN_MOBTHINKTIME)
		return 0;
	md->last_thinktime=tick;

	if( md->skilltimer!=-1 || md->bl.prev==NULL ){	// スキル詠唱中か死亡中
		if(DIFF_TICK(tick,md->next_walktime)>MIN_MOBTHINKTIME)
			md->next_walktime=tick;
		return 0;
	}

	mode=mob_db[md->class].mode;
	race=mob_db[md->class].race;
	
	// 異常
	if( md->opt1>0 || md->state.state==MS_DELAY){
		return 0;
	}

	if(!mode && md->target_id > 0)
		md->target_id = 0;

	if(md->attacked_id > 0 && mode&0x08){	// リンクモンスター
		sd=map_id2sd(md->attacked_id);
		if(sd) {
			if(sd->ghost_timer == -1) {
				map_foreachinarea(mob_ai_sub_hard_linksearch,md->bl.m,
					md->bl.x-13,md->bl.y-13,
					md->bl.x+13,md->bl.y+13,
					BL_MOB,md,&sd->bl);
			}
		}
	}
	
	// まず攻撃されたか確認（アクティブなら25%の確率でターゲット変更）
	if( mode>0 && md->attacked_id>0 && (!md->target_id || md->state.targettype == NONE_ATTACKABLE
		|| (mob_db[md->class].mode&0x04 && rand()%100<25 )  )){
		sd=map_id2sd(md->attacked_id);
		if(sd==NULL || md->bl.m != sd->bl.m || sd->bl.prev == NULL || sd->ghost_timer != -1 ||
			(dist=distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y))>=32){
			md->attacked_id=0;
		} else {
			md->target_id=md->attacked_id; // set target
			md->state.targettype = ATTACKABLE;
			attack_type = 1;
			md->attacked_id=0;
			md->min_chase=dist+13;
			if(md->min_chase>26)
				md->min_chase=26;
		}
	}

	md->state.master_check = 0;
	// 取り巻きモンスターの主の検索
	if( md->master_id > 0 )
		map_foreachinarea(mob_ai_sub_hard_mastersearch,md->bl.m,
						  md->bl.x-AREA_SIZE*2,md->bl.y-AREA_SIZE*2,
						  md->bl.x+AREA_SIZE*2,md->bl.y+AREA_SIZE*2,
						  BL_MOB,md,tick);

	// アクティヴモンスターの策敵
	if( (!md->target_id || md->state.targettype == NONE_ATTACKABLE) && mode&0x04 && !md->state.master_check &&
		battle_config.monster_active_enable){
		i=0;
		map_foreachinarea(mob_ai_sub_hard_activesearch,md->bl.m,
						  md->bl.x-AREA_SIZE*2,md->bl.y-AREA_SIZE*2,
						  md->bl.x+AREA_SIZE*2,md->bl.y+AREA_SIZE*2,
						  BL_PC,md,&i);
	}
	
	// ルートモンスターのアイテムサーチ
	if( !md->target_id && mode&0x02 && !md->state.master_check){
		i=0;
		map_foreachinarea(mob_ai_sub_hard_lootsearch,md->bl.m,
						  md->bl.x-AREA_SIZE*2,md->bl.y-AREA_SIZE*2,
						  md->bl.x+AREA_SIZE*2,md->bl.y+AREA_SIZE*2,
						  BL_ITEM,md,&i);
	}

	// 攻撃対象が居るなら攻撃
	if(md->target_id > 0){
		sd=map_id2sd(md->target_id);
		if(sd) {
			if(sd->bl.m != md->bl.m || sd->bl.prev == NULL ||
				 (dist=distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y))>=md->min_chase){
			// 別マップか、視界外
				mob_unlocktarget(md,tick);

			} else if( (!(mode&0x20) &&
					(sd->sc_data[SC_TRICKDEAD].timer != -1 ||
					 (sd->status.option&0x06 && race!=4 && race!=6)
					))  ){
			// スキルなどによる策敵妨害
				mob_unlocktarget(md,tick);

			} else if(!battle_check_range(&md->bl,sd->bl.x,sd->bl.y,mob_db[md->class].range)){

				// 攻撃範囲外なので移動
				if(!(mode&1)){	// 移動しないモード
					mob_unlocktarget(md,tick);
					return 0;
				}
								
				if( !mob_can_move(md) )	// 動けない状態にある
					return 0;

				md->state.skillstate=MSS_CHASE;	// 突撃時スキル
				mobskill_use(md,tick,-1);
						
//				if(md->timer != -1 && (DIFF_TICK(md->next_walktime,tick)<0 || distance(md->to_x,md->to_y,sd->bl.x,sd->bl.y)<2) )
				if(md->timer != -1 && md->state.state!=MS_ATTACK && (DIFF_TICK(md->next_walktime,tick)<0 || distance(md->to_x,md->to_y,sd->bl.x,sd->bl.y)<2) )
					return 0; // 既に移動中

				if( !mob_can_reach(md,&sd->bl,(md->min_chase>13)?md->min_chase:13) ){
					// 移動できないのでタゲ解除（IWとか？）
					mob_unlocktarget(md,tick);
					
				}else{
					// 追跡
					md->next_walktime=tick+500;
					i=0;
					do {
						if(i==0){	// 最初はAEGISと同じ方法で検索
							dx=sd->bl.x - md->bl.x;
							dy=sd->bl.y - md->bl.y;
							if(dx<0) dx++;
							else if(dx>0) dx--;
							if(dy<0) dy++;
							else if(dy>0) dy--;
						}else{	// だめならAthena式(ランダム)
							dx=sd->bl.x - md->bl.x + rand()%3 - 1;
							dy=sd->bl.y - md->bl.y + rand()%3 - 1;
						}
/*						if(path_search(&md->walkpath,md->bl.m,md->bl.x,md->bl.y,md->bl.x+dx,md->bl.y+dy,0)){
							dx=sd->bl.x - md->bl.x;
							dy=sd->bl.y - md->bl.y;
							if(dx<0) dx--;
							else if(dx>0) dx++;
							if(dy<0) dy--;
							else if(dy>0) dy++;
						}*/
						ret=mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);
						i++;
					} while(ret && i<5);
	
					if(ret){ // 移動不可能な所からの攻撃なら2歩下る
						if(dx<0) dx=2;
						else if(dx>0) dx=-2;
						if(dy<0) dy=2;
						else if(dy>0) dy=-2;
						mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);
					}
				}

			} else { // 攻撃射程範囲内
				md->state.skillstate=MSS_ATTACK;

				if(md->state.state==MS_WALK){	// 歩行中なら停止
					mob_stop_walking(md,1);
				}
				if(md->state.state==MS_ATTACK)
					return 0; // 既に攻撃中
				mob_changestate(md,MS_ATTACK,attack_type);

/*				if(mode&0x08){	// リンクモンスター
					map_foreachinarea(mob_ai_sub_hard_linksearch,md->bl.m,
						md->bl.x-13,md->bl.y-13,
						md->bl.x+13,md->bl.y+13,
						BL_MOB,md,&sd->bl);
				}*/
			}
			return 0;
		}
		else {	// ルートモンスター処理
			bl_item = map_id2bl(md->target_id);
			
			if(bl_item == NULL || bl_item->type != BL_ITEM ||bl_item->m != md->bl.m ||
				 (dist=distance(md->bl.x,md->bl.y,bl_item->x,bl_item->y))>=md->min_chase || !md->lootitem){
				 // 遠すぎるかアイテムがなくなった
				mob_unlocktarget(md,tick);

			}
			else if(dist){
				if(!(mode&1)){	// 移動しないモード
					mob_unlocktarget(md,tick);
					return 0;
				}
				if( !mob_can_move(md) )	// 動けない状態にある
					return 0;

				md->state.skillstate=MSS_LOOT;	// ルート時スキル使用
				mobskill_use(md,tick,-1);

//				if(md->timer != -1 && (DIFF_TICK(md->next_walktime,tick)<0 || distance(md->to_x,md->to_y,bl_item->x,bl_item->y)<2) )
				if(md->timer != -1 && md->state.state!=MS_ATTACK && (DIFF_TICK(md->next_walktime,tick)<0 || distance(md->to_x,md->to_y,bl_item->x,bl_item->y) <= 0))
					return 0; // 既に移動中


				md->next_walktime=tick+500;
				dx=bl_item->x - md->bl.x;
				dy=bl_item->y - md->bl.y;
/*				if(path_search(&md->walkpath,md->bl.m,md->bl.x,md->bl.y,md->bl.x+dx,md->bl.y+dy,0)){
					dx=bl_item->x - md->bl.x;
					dy=bl_item->y - md->bl.y;
				}*/
				ret=mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);

				if(ret){
					// 移動できないのでタゲ解除（IWとか？）
					mob_unlocktarget(md,tick);
				}

			} else {	// アイテムまでたどり着いた
				if(md->state.state==MS_ATTACK)
					return 0; // 攻撃中
				if(md->state.state==MS_WALK){	// 歩行中なら停止
					mob_stop_walking(md,1);
				}
				
				fitem = (struct flooritem_data *)bl_item;
				if(md->lootitem_count < LOOTITEM_SIZE)
					memcpy(&md->lootitem[md->lootitem_count++],&fitem->item_data,sizeof(md->lootitem[0]));
				else if(battle_config.monster_loot_type == 1 && md->lootitem_count >= LOOTITEM_SIZE) {
					mob_unlocktarget(md,tick);
					return 0;
				}
				else {
					if(md->lootitem[0].card[0] == (short)0xff00)
						intif_delete_petdata(*((long *)(&md->lootitem[0].card[2])));
					for(i=0;i<LOOTITEM_SIZE-1;i++)
						memcpy(&md->lootitem[i],&md->lootitem[i+1],sizeof(md->lootitem[0]));
					memcpy(&md->lootitem[LOOTITEM_SIZE-1],&fitem->item_data,sizeof(md->lootitem[0]));
				}
				map_clearflooritem(bl_item->id);
				mob_unlocktarget(md,tick);
				
			}
			return 0;
		}
	}

	// 歩行時/待機時スキル使用
	if( mobskill_use(md,tick,-1) )
		return 0;

	// 歩行処理
	if( mode&1 && mob_can_move(md) &&	// 移動可能MOB&動ける状態にある
		(md->master_id==0 || md->master_dist>10) ){	//取り巻きMOBじゃない
	
		// ランダム移動
		if( mob_randomwalk(md,tick) )
			return 0;
	}
	
	// 歩き終わってるので待機
	if( md->walkpath.path_len==0 || md->walkpath.path_pos>=md->walkpath.path_len )
		md->state.skillstate=MSS_IDLE;
	return 0;
}


/*==========================================
 * PC視界内のmob用まじめ処理(foreachclient)
 *------------------------------------------
 */
static int mob_ai_sub_foreachclient(struct map_session_data *sd,va_list ap)
{
	unsigned int tick;

	tick=va_arg(ap,unsigned int);
	map_foreachinarea(mob_ai_sub_hard,sd->bl.m,
					  sd->bl.x-AREA_SIZE*2,sd->bl.y-AREA_SIZE*2,
					  sd->bl.x+AREA_SIZE*2,sd->bl.y+AREA_SIZE*2,
					  BL_MOB,tick);

	return 0;
}

/*==========================================
 * PC視界内のmob用まじめ処理 (interval timer関数)
 *------------------------------------------
 */
static int mob_ai_hard(int tid,unsigned int tick,int id,int data)
{
	clif_foreachclient(mob_ai_sub_foreachclient,tick);

	return 0;
}

/*==========================================
 * 手抜きモードMOB AI（近くにPCがいない）
 *------------------------------------------
 */
static int mob_ai_sub_lazy(void * key,void * data,va_list app)
{
	struct mob_data *md=data;
	unsigned int tick;
	va_list ap;

	if(md==NULL || md->bl.type!=BL_MOB)
		return 0;
	ap=va_arg(app,va_list);
	tick=va_arg(ap,unsigned int);

	if(DIFF_TICK(tick,md->last_thinktime)<MIN_MOBTHINKTIME*10)
		return 0;
	md->last_thinktime=tick;

	if(md->bl.prev==NULL || md->skilltimer!=-1){
		if(DIFF_TICK(tick,md->next_walktime)>MIN_MOBTHINKTIME*10)
			md->next_walktime=tick;
		return 0;
	}

	if(DIFF_TICK(md->next_walktime,tick)<0 &&
		(mob_db[md->class].mode&1) && mob_can_move(md) ){

		if( map[md->bl.m].users>0 ){
			// 同じマップにPCがいるので、少しましな手抜き処理をする
		
			// 時々移動する
			if(rand()%1000<MOB_LAZYMOVEPERC)
				mob_randomwalk(md,tick);
			
			// 召喚MOBでなく、BOSSでもないMOBは時々、沸きなおす
			else if( rand()%1000<MOB_LAZYWARPPERC && md->x0<=0 &&
				mob_db[md->class].mexp<=0 )
				mob_spawn(md->bl.id);
				
		}else{
			// 同じマップにすらPCがいないので、とっても適当な処理をする
			
			// 召喚MOBでない場合、時々移動する（歩行ではなくワープで処理軽減）
			if( md->x0<=0 && rand()%1000<MOB_LAZYWARPPERC )
				mob_warp(md,-1,-1,-1);
		}
		
		md->next_walktime = tick+rand()%10000+5000;
	}
	return 0;
}

/*==========================================
 * PC視界外のmob用手抜き処理 (interval timer関数)
 *------------------------------------------
 */
static int mob_ai_lazy(int tid,unsigned int tick,int id,int data)
{
	map_foreachiddb(mob_ai_sub_lazy,tick);

	return 0;
}


/*==========================================
 * delay付きitem drop用構造体
 * timer関数に渡せるのint 2つだけなので
 * この構造体にデータを入れて渡す
 *------------------------------------------
 */
struct delay_item_drop {
	int m,x,y;
	int nameid,amount;
};

struct delay_item_drop2 {
	int m,x,y;
	struct item item_data;
};

/*==========================================
 * delay付きitem drop (timer関数)
 *------------------------------------------
 */
static int mob_delay_item_drop(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop *ditem;
	struct item temp_item;

	ditem=(struct delay_item_drop *)id;

	memset(&temp_item,0,sizeof(temp_item));
	temp_item.nameid = ditem->nameid;
	temp_item.amount = ditem->amount;
	temp_item.identify = !itemdb_isequip(temp_item.nameid);
	map_addflooritem(&temp_item,1,ditem->m,ditem->x,ditem->y);

	free(ditem);
	return 0;
}

/*==========================================
 * delay付きitem drop (timer関数) - lootitem
 *------------------------------------------
 */
static int mob_delay_item_drop2(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop2 *ditem;

	ditem=(struct delay_item_drop2 *)id;

	map_addflooritem(&ditem->item_data,ditem->item_data.amount,ditem->m,ditem->x,ditem->y);

	free(ditem);
	return 0;
}

/*==========================================
 * mdを消す
 *------------------------------------------
 */
int mob_delete(struct mob_data *md)
{
	mob_changestate(md,MS_DEAD,0);
	clif_clearchar_area(&md->bl,1);
	map_delblock(&md->bl);
	mob_setdelayspawn(md->bl.id);
	return 0;
}

int mob_catch_delete(struct mob_data *md)
{
	mob_changestate(md,MS_DEAD,0);
	clif_clearchar_area(&md->bl,0);
	map_delblock(&md->bl);
	mob_setdelayspawn(md->bl.id);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md = (struct mob_data *)bl;
	int id;
	id=va_arg(ap,int);
	if(md->master_id > 0 && md->master_id == id ) {
	mob_changestate(md,MS_DEAD,0);
	clif_clearchar_area(&md->bl,1);
	map_delblock(&md->bl); 
}
//		mob_damage(NULL,md,md->hp);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave(struct mob_data *md)
{
	map_foreachinarea(mob_deleteslave_sub, md->bl.m,
		0,0,map[md->bl.m].xs-1,map[md->bl.m].ys-1,
		BL_MOB,md->bl.id);
	return 0;
}

/*==========================================
 * mdにsdからdamageのダメージ
 *------------------------------------------
 */
int mob_damage(struct map_session_data *sd,struct mob_data *md,int damage)
{
	int i,sum,count,minpos,mindmg;
	struct map_session_data *tmpsd[DAMAGELOG_SIZE];
	struct {
		struct party *p;
		int id,base_exp,job_exp;
	} pt[DAMAGELOG_SIZE];
	int pnum=0;
	int mvp_damage=0;
	struct map_session_data *mvp_sd=sd;

//	printf("mob_damage %d %d %d\n",md->hp,mob_db[md->class].max_hp,damage);
	if(md->bl.prev==NULL){
		printf("mob_damage : BlockError!!\n");return 0;
	}

	if(md->state.state==MS_DEAD || md->hp<=0) {
		if(md->bl.prev != NULL) {
			mob_changestate(md,MS_DEAD,0);
			mobskill_use(md,gettick(),-1);	// 死亡時スキル
			clif_clearchar_area(&md->bl,1);
			map_delblock(&md->bl);
			mob_setdelayspawn(md->bl.id);
		}
		return 0;
	}

	if(md->state.state==MS_WALK){
//		mob_changestate(md,MS_IDLE,0);
//		clif_fixpos(&md->bl);
		mob_stop_walking(md,3);
	}

	if(md->hp>mob_db[md->class].max_hp)
		md->hp=mob_db[md->class].max_hp;

	// over kill分は丸める
	if(damage>md->hp)
		damage=md->hp;

	if(sd!=NULL){
		for(i=0,minpos=0,mindmg=30000;i<DAMAGELOG_SIZE;i++){
			if(md->dmglog[i].id==sd->bl.id)
				break;
			if(md->dmglog[i].id==0){
				minpos=i;
				mindmg=0;
			} else if(md->dmglog[i].dmg<mindmg){
				minpos=i;
				mindmg=md->dmglog[i].dmg;
			}
		}
		if(i<DAMAGELOG_SIZE)
			md->dmglog[i].dmg+=damage;
		else {
			md->dmglog[minpos].id=sd->bl.id;
			md->dmglog[minpos].dmg=damage;
		}

		md->attacked_id = sd->bl.id;
	}

	md->hp-=damage;
	//printf("%d : %d/%d\n",md->bl.id,md->hp,mob_db[md->class].max_hp);


	if(md->hp>0){

		return 0;
	}

	// ----- ここから死亡処理 -----

	mob_changestate(md,MS_DEAD,0);
	mobskill_use(md,gettick(),-1);	// 死亡時スキル

	memset(tmpsd,0,sizeof(tmpsd));
	memset(pt,0,sizeof(pt));

	// map外に消えた人は計算から除くので
	// overkill分は無いけどsumはmax_hpとは違う
	for(i=0,sum=0,count=0;i<DAMAGELOG_SIZE;i++){
		if(md->dmglog[i].id==0)
			continue;
		tmpsd[i] = map_id2sd(md->dmglog[i].id);
		if(tmpsd[i] == 0)
			continue;
		count++;
		if(tmpsd[i]->bl.m != md->bl.m)
			continue;
		sum+=md->dmglog[i].dmg;
		
		if(mvp_damage<md->dmglog[i].dmg){
			mvp_sd=tmpsd[i];
			mvp_damage=md->dmglog[i].dmg;
		}
	}
	// div0対策
	if(sum==0)
		sum=mob_db[md->class].max_hp;

	// 経験値の分配
	for(i=0;i<DAMAGELOG_SIZE;i++){
		int per,pid,base_exp,job_exp,flag=1;
		struct party *p;
		if(tmpsd[i]==NULL || tmpsd[i]->bl.m != md->bl.m)
			continue;

		per=md->dmglog[i].dmg*256*(9+count)/10/sum;
		if(per>512) per=512;
		if(per<1) per=1;
		base_exp=mob_db[md->class].base_exp*per/256;
		job_exp=mob_db[md->class].job_exp*per/256;

		if((pid=tmpsd[i]->status.party_id)>0){	// パーティに入っている
			int j=0;
			for(j=0;j<pnum;j++)	// 公平パーティリストにいるかどうか
				if(pt[j].id==pid)
					break;
			if(j==pnum){	// いないときは公平かどうか確認
				if((p=party_search(pid))!=NULL && p->exp!=0){
					pt[pnum].id=pid;
					pt[pnum].p=p;
					pt[pnum].base_exp=base_exp;
					pt[pnum].job_exp=job_exp;
					pnum++;
					flag=0;
				}
			}else{	// いるときは公平
				pt[j].base_exp+=base_exp;
				pt[j].job_exp+=job_exp;
				flag=0;
			}
		}
		if(flag)	// 各自所得
			pc_gainexp(tmpsd[i],base_exp,job_exp);
	}
	// 公平分配
	for(i=0;i<pnum;i++)
		party_exp_share(pt[i].p,md->bl.m,pt[i].base_exp,pt[i].job_exp);

	// item drop
	for(i=0;i<8;i++){
		struct delay_item_drop *ditem;

		if(mob_db[md->class].dropitem[i].nameid==0 ||
		   mob_db[md->class].dropitem[i].p<rand()%10000)
			continue;

		ditem=malloc(sizeof(*ditem));
		if(ditem==NULL){
			printf("out of memory : mob_damage\n");
			exit(1);
		}

		ditem->nameid=mob_db[md->class].dropitem[i].nameid;
		ditem->amount=1;
		ditem->m=md->bl.m;
		ditem->x=md->bl.x;
		ditem->y=md->bl.y;
		add_timer(gettick()+500+i,mob_delay_item_drop,(int)ditem,0);
	}
	if(md->lootitem) {
		for(i=0;i<md->lootitem_count;i++) {
			struct delay_item_drop2 *ditem;

			ditem=malloc(sizeof(*ditem));
			if(ditem==NULL){
				printf("out of memory : mob_damage\n");
				exit(1);
			}
			memcpy(&ditem->item_data,&md->lootitem[i],sizeof(md->lootitem[0]));
			ditem->m=md->bl.m;
			ditem->x=md->bl.x;
			ditem->y=md->bl.y;
			add_timer(gettick()+500+i,mob_delay_item_drop2,(int)ditem,0);
		}
	}
	
	// mvp処理
	if( mob_db[md->class].mexp ){
		int flag = 0;
		clif_mvp_effect(mvp_sd);					// エフェクト
		for(i=0;i<3;i++){
			struct item item;
			int ret;
			if( mob_db[md->class].mvpitem[i].nameid <=0 ||
				mob_db[md->class].mvpitem[i].p<rand()%10000 )
				continue;
			memset(&item,0,sizeof(item));
			item.nameid=mob_db[md->class].mvpitem[i].nameid;
			item.identify=1;
			clif_mvp_item(mvp_sd,item.nameid);
			if(mvp_sd->weight*2 > mvp_sd->max_weight)
				map_addflooritem(&item,1,mvp_sd->bl.m,mvp_sd->bl.x,mvp_sd->bl.y);
			else if((ret = pc_additem(mvp_sd,&item,1))) {
				clif_additem(sd,0,0,ret);
				map_addflooritem(&item,1,mvp_sd->bl.m,mvp_sd->bl.x,mvp_sd->bl.y);
			}
			flag = 1;
			break;
		}
		if(!flag) {
			clif_mvp_exp(mvp_sd,mob_db[md->class].mexp);	// 無条件で経験値
			pc_gainexp(mvp_sd,mob_db[md->class].mexp,0);
		}
	}

	if(md->npc_event[0]){	// SCRIPT実行
//		printf("mob_damage : run event : %s\n",md->npc_event);
		npc_event(sd,md->npc_event);
	}

	clif_clearchar_area(&md->bl,1);
	map_delblock(&md->bl);
	mob_deleteslave(md);
	mob_setdelayspawn(md->bl.id);

	return 0;
}

/*==========================================
 * mob回復
 *------------------------------------------
 */
int mob_heal(struct mob_data *md,int heal)
{
	md->hp+=heal;
	if( mob_db[md->class].max_hp < md->hp )
		md->hp=mob_db[md->class].max_hp;
	return 0;
}

/*==========================================
 * mobワープ
 *------------------------------------------
 */
int mob_warp(struct mob_data *md,int x,int y,int type)
{
	int m,i=0,c,xs=0,ys=0,bx=x,by=y;
	if( md==NULL || md->bl.prev==NULL )
		return 0;

	if(type>0)
		clif_clearchar_area(&md->bl,type);
	map_delblock(&md->bl);
	m=md->bl.m;
	
	if(bx>0 && by>0){	// 位置指定の場合周囲９セルを探索
		xs=ys=9;
	}

	while( ( x<0 || y<0 || ((c=read_gat(m,x,y))==1 || c==5) ) && (i++)<1000 ){
		if( xs>0 && ys>0 && i<250 ){	// 指定位置付近の探索
			x=bx+rand()%xs-xs/2;
			y=by+rand()%ys-ys/2;
		}else{			// 完全ランダム探索
			x=rand()%(map[m].xs-2)+1;
			y=rand()%(map[m].ys-2)+1;
		}
	}
	md->dir=0;
	if(i<1000){
		md->bl.x=x;
		md->bl.y=y;
	}else
		printf("MOB %d warp failed, class = %d\n",md->bl.id,md->class);

	md->target_id=0;	// タゲを解除する
	md->state.targettype=NONE_ATTACKABLE;
	md->attacked_id=0;
	md->state.state=MS_IDLE;
	md->state.skillstate=MSS_IDLE;
	
	if(type>0 && i==1000)
		printf("MOB %d warp to (%d,%d), class = %d\n",md->bl.id,x,y,md->class);
	
	map_addblock(&md->bl);
	if(type>0)
		clif_spawnmob(md);
	return 0;
}

/*==========================================
 * 画面内の取り巻きの数計算用(foreachinarea)
 *------------------------------------------
 */
int mob_countslave_sub(struct block_list *bl,va_list ap)
{
	int id,*c;
	id=va_arg(ap,int);
	c=va_arg(ap,int *);
	if( ((struct mob_data *)bl)->master_id==id )
		(*c)++;
	return 0;
}
/*==========================================
 * 画面内の取り巻きの数計算
 *------------------------------------------
 */
int mob_countslave(struct mob_data *md)
{
	int c=0;
	map_foreachinarea(mob_countslave_sub, md->bl.m,
		md->bl.x-AREA_SIZE,md->bl.y-AREA_SIZE,
		md->bl.x+AREA_SIZE,md->bl.y+AREA_SIZE,BL_MOB,
		md->bl.id,&c );
	return c;
}
/*==========================================
 * 手下MOB召喚
 *------------------------------------------
 */
int mob_summonslave(struct mob_data *md2,int class,int amount,int flag)
{
	struct mob_data *md;
	int bx=md2->bl.x,by=md2->bl.y,m=md2->bl.m;
	
	if(class<=1000 || class>2000)	// 値が異常なら召喚を止める
		return 0;
	
	for(;amount>0;amount--){
		int x=0,y=0,c=0,i=0;
		md=malloc(sizeof(struct mob_data));
		if(md==NULL){
			printf("mob_once_spawn: out of memory !\n");
			return 0;
		}
		if(mob_db[class].mode&0x02) {
			md->lootitem=malloc(sizeof(struct item)*LOOTITEM_SIZE);
			if(md->lootitem==NULL){
				printf("mob_once_spawn: out of memory !\n");
			}
		}
		else
			md->lootitem=NULL;

		while((x<=0 || y<=0 || (c=map_getcell(m,x,y))==1 || c==5 ) && (i++)<50){
			x=rand()%9-4+bx;
			y=rand()%9-4+by;
		}
		if(i>=50){
			x=bx;
			y=by;
		}
		
		mob_spawn_dataset(md,"--ja--",class);
		md->bl.m=m;
		md->bl.x=x;
		md->bl.y=y;

		md->x0=x;
		md->y0=y;
		md->xs=0;
		md->ys=0;
		md->spawndelay1=-1;	// 一度のみフラグ
		md->spawndelay2=-1;	// 一度のみフラグ

		memset(md->npc_event,0,sizeof(md->npc_event));
		md->bl.type=BL_MOB;
		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);

		if(flag)
			md->master_id=md2->bl.id;
	}
	return 0;
}

/*==========================================
 * 自分をロックしているPCの数を数える(foreachclient)
 *------------------------------------------
 */
int mob_counttargeted_sub(struct map_session_data *sd,va_list ap)
{
	int id,*c;
	id=va_arg(ap,int);
	c=va_arg(ap,int *);
	if( sd->attacktarget==id )
		(*c)++;
	return 0;
}
/*==========================================
 * 自分をロックしているPCの数を数える
 *------------------------------------------
 */
int mob_counttargeted(struct mob_data *md)
{
	int c=0;
	clif_foreachclient(mob_counttargeted_sub,md->bl.id,&c);
	return c;
}

//
// MOBスキル
//

/*==========================================
 * スキル使用（詠唱完了、ID指定）
 *------------------------------------------
 */
int mobskill_castend_id( int tid, unsigned int tick, int id,int data )
{
	struct mob_data* md=NULL;
	struct block_list *bl;
	
	if( (md=(struct mob_data *)map_id2bl(id))==NULL ||
		md->bl.type!=BL_MOB || md->bl.prev==NULL)
		return 0;
	
	if( md->skilltimer != tid )	// タイマIDの確認
		return 0;
	md->skilltimer=-1;
	md->last_thinktime=tick + mob_get_adelay(md);

	md->skilldelay[md->skillidx]=tick;
	
	bl=map_id2bl(md->skilltarget);
	if(bl==NULL || bl->prev==NULL)
		return 0;
	if(md->bl.m != bl->m)
		return 0;
		
	printf("MOB skill castend skill=%d, class = %d\n",md->skillid,md->class);

	switch( skill_get_nk(md->skillid) )
	{
	// 攻撃系/吹き飛ばし系
	case 0:	case 2:
		skill_castend_damage_id(&md->bl,bl,md->skillid,md->skilllv,tick,0);
		break;
	case 1:// 支援系
		if( (md->skillid==28 || md->skillid==54)&& battle_get_elem_type(bl)==9 )
			skill_castend_damage_id(&md->bl,bl,md->skillid,md->skilllv,tick,0);
		else
			skill_castend_nodamage_id(&md->bl,bl,md->skillid,md->skilllv,tick,0);
		break;
	}
	

	return 0;
}

/*==========================================
 * スキル使用（詠唱完了、場所指定）
 *------------------------------------------
 */
int mobskill_castend_pos( int tid, unsigned int tick, int id,int data )
{
	struct mob_data* md=NULL;
	
	if( (md=(struct mob_data *)map_id2bl(id))==NULL ||
		md->bl.type!=BL_MOB || md->bl.prev==NULL )
		return 0;
	
	if( md->skilltimer != tid )	// タイマIDの確認
		return 0;
	md->skilltimer=-1;
	md->skilldelay[md->skillidx]=tick;

	printf("MOB skill castend skill=%d, class = %d\n",md->skillid,md->class);

	skill_castend_pos2(&md->bl,md->skillx,md->skilly,md->skillid,md->skilllv,tick,0);

	return 0;
}


/*==========================================
 * スキル使用（詠唱開始、ID指定）
 *------------------------------------------
 */
int mobskill_use_id(struct mob_data *md,struct block_list *target,int skill_idx)
{
	int casttime;
	struct mob_skill *ms=&mob_db[md->class].skill[skill_idx];
	int skill_id=ms->skill_id, skill_lv=ms->skill_lv;

	if(target==NULL && (target=map_id2bl(md->target_id))==NULL)
		return 0;
	
	if( target->prev==NULL || md->bl.prev==NULL )
		return 0;
	
	// 沈黙や異常
	if( md->opt1>0 || md->option&6 || md->sc_data[SC_DIVINA].timer!=-1 )
		return 0;
		
	// 射程と障害物チェック
	if(!battle_check_range(&md->bl,target->x,target->y,skill_get_range(skill_id)))
		return 0;
	
//	casttime=skill_castfix(&md->bl, skill_get_cast( skill_id,skill_lv) );
//	delay=skill_delayfix(&md->bl, skill_get_delay( skill_id,skill_lv) );
//	sd->skillcastcancel=1;

	casttime=ms->casttime;
	md->state.skillcastcancel=ms->cancel;
	md->skilldelay[skill_idx]=gettick();
	
	printf("MOB skill use target_id=%d skill=%d lv=%d cast=%d, class = %d\n"
		,target->id,skill_id,skill_lv,casttime,md->class);

	if( casttime>0 ){ 	// 詠唱が必要
		struct mob_data *md2;
		clif_skillcasting( &md->bl,
			md->bl.id, target->id, 0,0, skill_id,casttime);
		
		// 詠唱反応モンスター
		if( target->type==BL_MOB && mob_db[(md2=(struct mob_data *)target)->class].mode&0x10 &&
			md2->state.state!=MS_ATTACK){
				md2->target_id=md->bl.id;
				md->state.targettype = ATTACKABLE;
				md2->min_chase=13;
		}
	}
	
	if( casttime<=0 )	// 詠唱の無いものはキャンセルされない
		md->state.skillcastcancel=0;

	md->skilltarget	= target->id;
	md->skillx		= 0;
	md->skilly		= 0;
	md->skillid		= skill_id;
	md->skilllv		= skill_lv;
	md->skillidx	= skill_idx;

	if( casttime>0 ){
		md->skilltimer =
			add_timer( gettick()+casttime, mobskill_castend_id, md->bl.id, 0 );
	}else{
		md->skilltimer = -1;
		mobskill_castend_id(md->skilltimer,gettick(),md->bl.id, 0);
	}

//	sd->canmove_tick=gettick()+casttime+delay;

	return 0;
}
/*==========================================
 * スキル使用（場所指定）
 *------------------------------------------
 */
int mobskill_use_pos( struct mob_data *md,
	int skill_x, int skill_y, int skill_idx)
{
	int casttime=0;
	struct mob_skill *ms=&mob_db[md->class].skill[skill_idx];
	int skill_num=ms->skill_id, skill_lv=ms->skill_lv;
	
	if( md->bl.prev==NULL )
		return 0;

	if( md->opt1>0 || md->option&6 || md->sc_data[SC_DIVINA].timer!=-1 )
		return 0;	// 異常や沈黙など

	// 射程と障害物チェック
	if(!battle_check_range(&md->bl,skill_x,skill_y,skill_get_range(skill_num)))
		return 0;

//	casttime=skill_castfix(&sd->bl, skill_get_cast( skill_num,skill_lv) );
//	delay=skill_delayfix(&sd->bl, skill_get_delay( skill_num,skill_lv) );
	casttime=ms->casttime;
	md->skilldelay[skill_idx]=gettick();
	md->state.skillcastcancel=ms->cancel;

	printf("MOB skill use target_pos=(%d,%d) skill=%d lv=%d cast=%d, class = %d\n",
		skill_x,skill_y,skill_num,skill_lv,casttime,md->class);

	if( casttime>0 )	// 詠唱が必要
		clif_skillcasting( &md->bl,
			md->bl.id, 0, skill_x,skill_y, skill_num,casttime);

	if( casttime<=0 )	// 詠唱の無いものはキャンセルされない
		md->state.skillcastcancel=0;


	md->skillx		= skill_x;
	md->skilly		= skill_y;
	md->skilltarget	= 0;
	md->skillid		= skill_num;
	md->skilllv		= skill_lv;
	md->skillidx	= skill_idx;
	if( casttime>0 ){
		md->skilltimer =
			add_timer( gettick()+casttime, mobskill_castend_pos, md->bl.id, 0 );
	}else{
		md->skilltimer = -1;
		mobskill_castend_pos(md->skilltimer,gettick(),md->bl.id, 0);
	}

//	sd->canmove_tick=gettick()+casttime+delay;

	return 0;
}

/*==========================================
 * スキル使用判定
 *------------------------------------------
 */
int mobskill_use(struct mob_data *md,unsigned int tick,int event)
{
	struct mob_skill *ms=mob_db[md->class].skill;
//	struct block_list *target=NULL;
	int i;

	if(battle_config.mob_skill_use == 0)
		return 0;
	
	for(i=0;i<mob_db[md->class].maxskill;i++){
		int c2=ms[i].cond2,flag=0;
	
		// ディレイ中
		if( DIFF_TICK(tick,md->skilldelay[i])<ms[i].delay )
			continue;

		// 状態判定
		if( ms[i].state>=0 && ms[i].state!=md->state.skillstate )
			continue;

		// 条件判定
		flag=(event==ms[i].cond1);
		if(!flag){
			switch( ms[i].cond1 ){
			case MSC_ALWAYS:
				flag=1; break;
			case MSC_MYHPLTMAXRATE:		// HP< maxhp%
				flag=( md->hp < mob_db[md->class].max_hp*c2/100 ); break;
			case MSC_SLAVELT:		// slave < num
				flag=( mob_countslave(md) < c2 ); break;
			case MSC_ATTACKPCGT:	// attack pc > num
				flag=( mob_counttargeted(md) > c2 ); break;
			case MSC_SLAVELE:		// slave <= num
				flag=( mob_countslave(md) <= c2 ); break;
			case MSC_ATTACKPCGE:	// attack pc >= num
				flag=( mob_counttargeted(md) >= c2 ); break;
			case MSC_SKILLUSED:		// specificated skill used
				flag=( (event&0xffff)==MSC_SKILLUSED && (event>>16)==c2); break;
			}
		}
	
		// 確率判定
		if( flag && rand()%1000 < ms[i].permillage ){

			if( skill_get_inf(ms[i].skill_id)&2 ){				
				// 場所指定
				struct block_list *bl;
				int x=0,y=0;
				if( ms[i].target<2 ){
					bl=((ms[i].target==MST_TARGET)?map_id2bl(md->target_id):&md->bl);
					if(bl!=NULL){
						x=bl->x; y=bl->y;
					}
				}
				if( x<=0 || y<=0 )
					continue;

				mobskill_use_pos(md,x,y,i);

			}else{
				// ID指定
				if( ms[i].target<2 )
					mobskill_use_id(md,((ms[i].target==MST_TARGET)?NULL:&md->bl),i);
			}
			return 1;
		}
	}
	
	return 0;
}
/*==========================================
 * スキル使用イベント処理
 *------------------------------------------
 */
int mobskill_event(struct mob_data *md,int flag)
{
	if(flag==-1 && mobskill_use(md,gettick(),MSC_CASTTARGETED))
		return 1;
	if( (flag&BF_SHORT) && mobskill_use(md,gettick(),MSC_CLOSEDATTACKED))
		return 1;
	if( (flag&BF_LONG) && mobskill_use(md,gettick(),MSC_LONGRANGEATTACKED))
		return 1;
	return 0;
}
/*==========================================
 * スキル用タイマー削除
 *------------------------------------------
 */
int mobskill_deltimer(struct mob_data *md )
{
	if( md->skilltimer!=-1 ){
		if( skill_get_inf( md->skillid )&2 )
			delete_timer( md->skilltimer, mobskill_castend_pos );
		else
			delete_timer( md->skilltimer, mobskill_castend_id );
		md->skilltimer=-1;
	}
	return 0;
}
//
// 初期化
//
/*==========================================
 * 未設定mobが使われたので暫定初期値設定
 *------------------------------------------
 */
static int mob_makedummymobdb(int class)
{
	int i;

	sprintf(mob_db[class].name,"mob%d",class);
	sprintf(mob_db[class].jname,"mob%d",class);
	mob_db[class].lv=1;
	mob_db[class].max_hp=1000;
	mob_db[class].max_sp=1;
	mob_db[class].base_exp=2;
	mob_db[class].job_exp=1;
	mob_db[class].range=1;
	mob_db[class].atk1=7;
	mob_db[class].atk2=10;
	mob_db[class].def=0;
	mob_db[class].mdef=0;
	mob_db[class].str=1;
	mob_db[class].agi=1;
	mob_db[class].vit=1;
	mob_db[class].int_=1;
	mob_db[class].dex=6;
	mob_db[class].luk=2;
	mob_db[class].range2=10;
	mob_db[class].range3=10;
	mob_db[class].size=0;
	mob_db[class].race=0;
	mob_db[class].element=0;
	mob_db[class].mode=0;
	mob_db[class].speed=300;
	mob_db[class].adelay=1000;
	mob_db[class].amotion=500;
	mob_db[class].dmotion=500;
	mob_db[class].dropitem[0].nameid=909;	// Jellopy
	mob_db[class].dropitem[0].p=1000;
	for(i=1;i<8;i++){
		mob_db[class].dropitem[i].nameid=0;
		mob_db[class].dropitem[i].p=0;
	}
	// Item1,Item2
	mob_db[class].mexp=0;
	mob_db[class].mexpper=0;
	for(i=0;i<3;i++){
		mob_db[class].mvpitem[i].nameid=0;
		mob_db[class].mvpitem[i].p=0;
	}
	mob_db[class].summonflag=0;
	return 0;
}

/*==========================================
 * db/mob_db.txt読み込み
 *------------------------------------------
 */
static int mob_readdb(void)
{
	FILE *fp;
	char line[1024];

	memset(mob_db,0,sizeof(mob_db));
	fp=fopen("db/mob_db.txt","r");
	if(fp==NULL){
		printf("can't read db/mob_db.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		int class,i;
		char *str[55],*p,*np;

		if(line[0] == '/' && line[1] == '/')
			continue;

		for(i=0,p=line;i<55;i++){
			if((np=strchr(p,','))!=NULL){
				str[i]=p;
				*np=0;
				p=np+1;
			} else
				str[i]=p;
		}

		class=atoi(str[0]);
		if(class>=2000 || class==0)
			continue;

		memcpy(mob_db[class].name,str[1],24);
		memcpy(mob_db[class].jname,str[2],24);
		mob_db[class].lv=atoi(str[3]);
		mob_db[class].max_hp=atoi(str[4]);
		mob_db[class].max_sp=atoi(str[5]);
		mob_db[class].base_exp=atoi(str[6])*
				battle_config.base_exp_rate/100;
		if(mob_db[class].base_exp <= 0)
			mob_db[class].base_exp = 1;
		mob_db[class].job_exp=atoi(str[7])*
				battle_config.job_exp_rate/100;
		if(mob_db[class].job_exp <= 0)
			mob_db[class].job_exp = 1;
		mob_db[class].range=atoi(str[8]);
		mob_db[class].atk1=atoi(str[9]);
		mob_db[class].atk2=atoi(str[10]);
		mob_db[class].def=atoi(str[11]);
		mob_db[class].mdef=atoi(str[12]);
		mob_db[class].str=atoi(str[13]);
		mob_db[class].agi=atoi(str[14]);
		mob_db[class].vit=atoi(str[15]);
		mob_db[class].int_=atoi(str[16]);
		mob_db[class].dex=atoi(str[17]);
		mob_db[class].luk=atoi(str[18]);
		mob_db[class].range2=atoi(str[19]);
		mob_db[class].range3=atoi(str[20]);
		mob_db[class].size=atoi(str[21]);
		mob_db[class].race=atoi(str[22]);
		mob_db[class].element=atoi(str[23]);
		mob_db[class].mode=atoi(str[24]);
		mob_db[class].speed=atoi(str[25]);
		mob_db[class].adelay=atoi(str[26]);
		mob_db[class].amotion=atoi(str[27]);
		mob_db[class].dmotion=atoi(str[28]);

		for(i=0;i<8;i++){
			mob_db[class].dropitem[i].nameid=atoi(str[29+i*2]);
			mob_db[class].dropitem[i].p=atoi(str[30+i*2])*battle_config.item_rate/100;
		}
		// Item1,Item2
		mob_db[class].mexp=atoi(str[47])*battle_config.mvp_exp_rate/100;
		mob_db[class].mexpper=atoi(str[48]);
		for(i=0;i<3;i++){
			mob_db[class].mvpitem[i].nameid=atoi(str[49+i*2]);
			mob_db[class].mvpitem[i].p=atoi(str[50+i*2])*battle_config.mvp_item_rate/100;
		}
		if(battle_config.mvp_hp_rate!=100 && mob_db[class].mexp)
			if((mob_db[class].max_hp=mob_db[class].max_hp*
				battle_config.mvp_hp_rate/100)<=0)
				mob_db[class].max_hp=1;
		mob_db[class].summonflag=0;
		mob_db[class].maxskill=0;
	}
	fclose(fp);
	printf("read db/mob_db.txt done\n");
	return 0;
}
/*==========================================
 * db/mob_branch.txt読み込み
 *------------------------------------------
 */
static int mob_readbranch(void)
{
	FILE *fp;
	char line[1024];

	fp=fopen("db/mob_branch.txt","r");
	if(fp==NULL){
		printf("can't read db/mob_branch.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		int class;
		if(line[0] == '/' && line[1] == '/')
			continue;
		if( (class=atoi(line))>1000 && class<2000 )
			mob_db[class].summonflag=1;
	}
	fclose(fp);
	printf("read db/mob_branch.txt done\n");
	return 0;
}
/*==========================================
 * db/mob_skill_db.txt読み込み
 *------------------------------------------
 */
static int mob_readskilldb(void)
{
	FILE *fp;
	char line[1024];
	int i;
	
	static struct {
		char str[32];
		int id;
	} cond1[] = {
		{	"always",			MSC_ALWAYS				},
		{	"myhpltmaxrate",	MSC_MYHPLTMAXRATE		},
		{	"friendhpltmaxrate",MSC_FRIENDHPLTMAXRATE	},
		{	"mystatuseq",		MSC_MYSTATUSEQ			},
		{	"mystatusne",		MSC_MYSTATUSNE			},
		{	"friendstatuseq",	MSC_FRIENDSTATUSEQ		},
		{	"friendstatusne",	MSC_FRIENDSTATUSNE		},
		{	"attackpcgt",		MSC_ATTACKPCGT			},
		{	"attackpcge",		MSC_ATTACKPCGE			},
		{	"slavelt",			MSC_SLAVELT				},
		{	"slavele",			MSC_SLAVELE				},
		{	"closedattacked",	MSC_CLOSEDATTACKED		},
		{	"longrangeattacked",MSC_LONGRANGEATTACKED	},
		{	"skillused",		MSC_SKILLUSED			},
		{	"casttargeted",		MSC_CASTTARGETED		},
	};

	fp=fopen("db/mob_skill_db.txt","r");
	if(fp==NULL){
		printf("can't read db/mob_skill_db.txt\n");
		return 0;
	}
	while(fgets(line,1020,fp)){
		char *sp[16],*p;
		int mob_id;
		struct mob_skill *ms;

		if(line[0] == '/' && line[1] == '/')
			continue;

		memset(sp,0,sizeof(sp));
		for(i=0,p=line;i<13 && p;i++){
			sp[i]=p;
			if((p=strchr(p,','))!=NULL)
				*p++=0;
		}
		if( (mob_id=atoi(sp[0]))<=0 )
			continue;
		
		for(i=0;i<MAX_MOBSKILL;i++)
			if( (ms=&mob_db[mob_id].skill[i])->skill_id == 0)
				break;
		if(i==MAX_MOBSKILL){
			printf("mob_skill: readdb: too many skill ! [%s] in %d[%s]\n",
				sp[1],mob_id,mob_db[mob_id].jname);
			continue;
		}
		
		ms->state=atoi(sp[2]);
		if( strcmp(sp[2],"any")==0 ) ms->state=-1;
		if( strcmp(sp[2],"idle")==0 ) ms->state=MSS_IDLE;
		if( strcmp(sp[2],"walk")==0 ) ms->state=MSS_WALK;
		if( strcmp(sp[2],"attack")==0 ) ms->state=MSS_ATTACK;
		if( strcmp(sp[2],"dead")==0 ) ms->state=MSS_DEAD;
		if( strcmp(sp[2],"loot")==0 ) ms->state=MSS_LOOT;
		if( strcmp(sp[2],"chase")==0 ) ms->state=MSS_CHASE;
		ms->skill_id=atoi(sp[3]);
		ms->skill_lv=atoi(sp[4]);
		ms->permillage=atoi(sp[5]);
		ms->casttime=atoi(sp[6]);
		ms->delay=atoi(sp[7]);
		ms->cancel=atoi(sp[8]);
		if( strcmp(sp[8],"yes")==0 ) ms->cancel=1;
		ms->target=atoi(sp[9]);
		if( strcmp(sp[9],"self")==0 )ms->target=MST_SELF;
		if( strcmp(sp[9],"target")==0 )ms->target=MST_TARGET;
		ms->cond1=-1;
		for(i=0;i<sizeof(cond1)/sizeof(cond1[0]);i++){
			if( strcmp(sp[10],cond1[i].str)==0)
				ms->cond1=cond1[i].id;
		}
		ms->cond2=atoi(sp[11]);
		ms->val1=atoi(sp[12]);
		mob_db[mob_id].maxskill=i+1;
	}
	fclose(fp);
	printf("read db/mob_skill_db.txt done\n");
	return 0;
}
/*==========================================
 * mob周り初期化
 *------------------------------------------
 */
int do_init_mob(void)
{
	add_timer_func_list(mob_timer,"mob_timer");
	add_timer_func_list(mob_delayspawn,"mob_delayspawn");
	add_timer_func_list(mob_delay_item_drop,"mob_delay_item_drop");
	add_timer_func_list(mob_delay_item_drop2,"mob_delay_item_drop2");
	add_timer_func_list(mob_ai_hard,"mob_ai_hard");
	add_timer_func_list(mob_ai_lazy,"mob_ai_lazy");
	add_timer_func_list(mobskill_castend_id,"mobskill_castend_id");
	add_timer_func_list(mobskill_castend_pos,"mobskill_castend_pos");
	add_timer_interval(gettick()+100,mob_ai_hard,0,0,MIN_MOBTHINKTIME);
	add_timer_interval(gettick()+1000,mob_ai_lazy,0,0,MIN_MOBTHINKTIME*10);

	mob_readdb();
	mob_readbranch();
	mob_readskilldb();

	return 0;
}
