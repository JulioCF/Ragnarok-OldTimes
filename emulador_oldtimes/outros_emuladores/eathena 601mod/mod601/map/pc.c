// $Id: pc.c,v 1.26 2003/07/04 15:26:33 lemit Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "timer.h"
#include "db.h"
#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "npc.h"
#include "mob.h"
#include "pet.h"
#include "itemdb.h"
#include "script.h"
#include "battle.h"
#include "skill.h"
#include "party.h"
#include "guild.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "vending.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define PVP_CALCRANK_INTERVAL 1000	// PVP順位計算の間隔

static int max_weight_base[MAX_PC_CLASS];
static double hp_coefficient[MAX_PC_CLASS];
static int sp_coefficient[MAX_PC_CLASS];
static int aspd_base[MAX_PC_CLASS][20];
static char job_bonus[MAX_PC_CLASS][MAX_LEVEL];
static int exp_table[6][MAX_LEVEL];
static struct {
	int id;
	int max;
	struct {
		short id,lv;
	} need[6];
} skill_tree[MAX_PC_CLASS][90];


static int atkmods[3][20];	// 武器ATKサイズ修正(size_fix.txt)
static int refinebonus[5][3];	// 精錬ボーナステーブル(refine_db.txt)
static int percentrefinery[5][10];	// 精錬成功率(refine_db.txt)

static int dirx[8]={0,-1,-1,-1,0,1,1,1};
static int diry[8]={1,1,0,-1,-1,-1,0,1};

static char GM_account_filename[1024] = "conf/GM_account.txt";
static struct dbt *gm_account_db;

void pc_set_gm_account_fname(char *str)
{
	strcpy(GM_account_filename,str);
}

int pc_isGM(struct map_session_data *sd)
{
	struct gm_account *p;
	p = numdb_search(gm_account_db,sd->status.account_id);
	if( p == NULL)
		return 0;
	return p->level;
}


static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}


static int pc_ghost_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;

	sd=(struct map_session_data *)map_id2sd(id);
	if(sd==NULL || sd->bl.type!=BL_PC)
		return 1;

	if(sd->ghost_timer != tid){
		printf("ghost_timer %d != %d\n",sd->ghost_timer,tid);
		return 0;
	}
	sd->ghost_timer=-1;

	return 0;
}


int pc_setghosttimer(struct map_session_data *sd,int val)
{
	if(sd->ghost_timer != -1)
		delete_timer(sd->ghost_timer,pc_ghost_timer);
	sd->ghost_timer = add_timer(gettick()+val,pc_ghost_timer,sd->bl.id,0);
	return 0;
}


int pc_delghosttimer(struct map_session_data *sd)
{
	if(sd->ghost_timer != -1) {
		delete_timer(sd->ghost_timer,pc_ghost_timer);
		sd->ghost_timer = -1;
	}
	return 0;
}

static int pc_spiritball_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	int i;

	sd=(struct map_session_data *)map_id2sd(id);
	if(sd==NULL || sd->bl.type!=BL_PC)
		return 1;

	if(sd->spirit_timer[0] != tid){
		printf("spirit_timer %d != %d\n",sd->spirit_timer[0],tid);
		return 0;
	}
	sd->spirit_timer[0]=-1;
	for(i=1;i<10;i++) {
		sd->spirit_timer[i-1] = sd->spirit_timer[i];
		sd->spirit_timer[i] = -1;
	}
	sd->spiritball--;
	if(sd->spiritball < 0)
		sd->spiritball = 0;
	clif_spiritball(sd);

	return 0;
}


int pc_addspiritball(struct map_session_data *sd,int interval,int max)
{
	int i;

	if(max > 10)
		max = 10;
	if(sd->spiritball < 0)
		sd->spiritball = 0;

	if(sd->spiritball >= max) {
		if(sd->spirit_timer[0] != -1) {
			delete_timer(sd->spirit_timer[0],pc_spiritball_timer);
			sd->spirit_timer[0] = -1;
		}
		for(i=1;i<max;i++) {
			sd->spirit_timer[i-1] = sd->spirit_timer[i];
			sd->spirit_timer[i] = -1;
		}
	}
	else
		sd->spiritball+=sd->skilllv;
	if (sd->spiritball > max) sd->spiritball = max;

	sd->spirit_timer[sd->spiritball-1] = add_timer(gettick()+interval,pc_spiritball_timer,sd->bl.id,0);
	clif_spiritball(sd);

	return 0;
}


int pc_delspiritball(struct map_session_data *sd,int count,int type)
{
	int i;

	if(sd->spiritball <= 0) {
		sd->spiritball = 0;
		return 0;
	}

	if(count > sd->spiritball)
		count = sd->spiritball;
	sd->spiritball -= count;
	if(count > 10)
		count = 10;

	for(i=0;i<count;i++) {
		if(sd->spirit_timer[i] != -1) {
			delete_timer(sd->spirit_timer[i],pc_spiritball_timer);
			sd->spirit_timer[i] = -1;
		}
	}
	for(i=count;i<10;i++) {
		sd->spirit_timer[i-count] = sd->spirit_timer[i];
		sd->spirit_timer[i] = -1;
	}

	if(!type)
		clif_spiritball(sd);

	return 0;
}

int pc_setrestartvalue(struct map_session_data *sd,int type)
{
	//-----------------------
	// 死亡した
	if( pc_check_equip_dcard(sd,4144) ) {	// オシリスカード
		sd->status.hp=sd->status.max_hp;
		sd->status.sp=sd->status.max_sp;
	}
	else {
		if(sd->status.class == 0 && battle_config.restart_hp_rate < 50) {	// ノービス
			sd->status.hp=(sd->status.max_hp)/2;
		}
		else {
			if(battle_config.restart_hp_rate <= 0)
				sd->status.hp = 1;
			else {
				sd->status.hp = sd->status.max_hp * battle_config.restart_hp_rate /100;
				if(sd->status.hp <= 0)
					sd->status.hp = 1;
			}
		}
		if(battle_config.restart_sp_rate > 0) {
			int sp = sd->status.max_sp * battle_config.restart_sp_rate /100;
			if(sd->status.sp < sp)
				sd->status.sp = sp;
		}
	}
	if(!type)
		clif_updatestatus(sd,SP_HP);
	if(!type)
		clif_updatestatus(sd,SP_SP);


	if(!(battle_config.death_penalty_type&1) ) {
		if(sd->status.class && !map[sd->bl.m].flag.nopenalty){
			if(battle_config.death_penalty_type&2 && battle_config.death_penalty_base > 0)
				sd->status.base_exp -= (int)((double)pc_nextbaseexp(sd) * (double)battle_config.death_penalty_base/10000.);
			else if(battle_config.death_penalty_base > 0) {
				if(pc_nextbaseexp(sd) > 0)
					sd->status.base_exp -= (int)((double)sd->status.base_exp * (double)battle_config.death_penalty_base/10000.);
			}
			if(sd->status.base_exp < 0)
				sd->status.base_exp = 0;
			if(!type)
				clif_updatestatus(sd,SP_BASEEXP);

			if(battle_config.death_penalty_type&2 && battle_config.death_penalty_job > 0)
				sd->status.job_exp -= (int)((double)pc_nextjobexp(sd) * (double)battle_config.death_penalty_job/10000.);
			else if(battle_config.death_penalty_job > 0) {
				if(pc_nextjobexp(sd) > 0)
					sd->status.job_exp -= (int)((double)sd->status.job_exp * (double)battle_config.death_penalty_job/10000.);
			}
			if(sd->status.job_exp < 0)
				sd->status.job_exp = 0;
			if(!type)
				clif_updatestatus(sd,SP_JOBEXP);
		}
	}

	return 0;
}


/*==========================================
 * ローカルプロトタイプ宣言 (必要な物のみ)
 *------------------------------------------
 */
static int pc_walktoxy_sub(struct map_session_data *);


/*==========================================
 * saveに必要なステータス修正を行なう
 *------------------------------------------
 */
int pc_makesavestatus(struct map_session_data *sd)
{
	// 服の色は色々弊害が多いので保存対象にはしない
	//This line disabled clothes color saving - Sara
	//sd->status.clothes_color=0;

	// 死亡状態だったのでhpを1、位置をセーブ場所に変更
	if(pc_isdead(sd)){
		pc_setrestartvalue(sd,1);
		memcpy(&sd->status.last_point,&sd->status.save_point,sizeof(sd->status.last_point));
	} else {
		memcpy(sd->status.last_point.map,sd->mapname,16);
		sd->status.last_point.x = sd->bl.x;
		sd->status.last_point.y = sd->bl.y;
	}

	// セーブ禁止マップだったので指定位置に移動
	if(map[sd->bl.m].flag.nosave){
		struct map_data *m=&map[sd->bl.m];
		if(strcmp(m->save.map,"SavePoint")==0)
			memcpy(&sd->status.last_point,&sd->status.save_point,sizeof(sd->status.last_point));
		else
			memcpy(&sd->status.last_point,&m->save,sizeof(sd->status.last_point));
	}

	return 0;
}


/*==========================================
 * 接続時の初期化
 *------------------------------------------
 */
int pc_setnewpc(struct map_session_data *sd,int account_id,int char_id,int login_id1,int client_tick,int sex,int fd)
{
	sd->bl.id        = account_id;
	sd->char_id      = char_id;
	sd->login_id1    = login_id1;
	sd->client_tick  = client_tick;
	sd->sex          = sex;
	sd->state.auth   = 0;
	sd->bl.type      = BL_PC;
	sd->canact_tick = sd->canmove_tick = gettick();
	sd->state.waitingdisconnect=0;

	return 0;
}


/*==========================================
 * session idに問題無し
 * char鯖から送られてきたステータスを設定
 *------------------------------------------
 */
int pc_authok(int id,struct mmo_charstatus *st)
{
	struct map_session_data *sd;
	struct party *p;
	struct guild *g;
	int i;
	unsigned long tick = gettick();

	sd = map_id2sd(id);
	if(sd==NULL)
		return 1;
	if(sd->new_fd){
		// 2重login状態だったので、両方落す
		clif_authfail_fd(sd->fd,2);	// same id
		clif_authfail_fd(sd->new_fd,2);	// same id
		return 1;
	}
	memcpy(&sd->status,st,sizeof(*st));

	if(sd->status.sex != sd->sex){
		clif_authfail_fd(sd->fd,0);
		return 1;
	}

	memset(&sd->state,0,sizeof(sd->state));
	// 基本的な初期化
	sd->speed = DEFAULT_WALK_SPEED;
	sd->state.dead_sit=0;
	sd->dir=0;
	sd->head_dir=0;
	sd->state.auth=1;
	sd->walktimer=-1;
	sd->attacktimer=-1;
	sd->skilltimer=-1;
	sd->skillitem=-1;
	sd->skillitemlv=-1;
	sd->ghost_timer=-1;

	sd->deal_locked =0;
	sd->trade_partner=0;

	sd->inchealhptick = 0;
	sd->inchealsptick = 0;
	sd->hp_sub = 0;
	sd->sp_sub = 0;
	sd->inchealspirittick = 0;
	sd->canact_tick = tick;
	sd->canmove_tick = tick;

	sd->spiritball = 0;
	sd->combo_flag = 0;
	sd->combo_delay1 = 0;
	sd->combo_delay2 = 0;
	sd->combo_delay3 = 0;
	sd->triple_delay = 0;
	sd->skill_old = 0;
	for(i=0;i<10;i++)
		sd->spirit_timer[i] = -1;
	for(i=0;i<MAX_SKILLTIMERSKILL;i++)
		sd->skilltimerskill[i].timer = -1;

	// pet
	sd->petDB = NULL;
	sd->pet_npcdata = NULL;
	sd->pet_hungry_timer = -1;
	memset(&sd->pet,0,sizeof(struct s_pet));

	// ステータス異常の初期化
	for(i=0;i<MAX_STATUSCHANGE;i++)
		sd->sc_data[i].timer=-1;
	sd->sc_count=0;
	sd->status.option&=0xfff8;

	// スキルユニット関係の初期化
	memset(sd->skillunit,0,sizeof(sd->skillunit));
	memset(sd->skillunittick,0,sizeof(sd->skillunittick));

	// パーティー関係の初期化
	sd->party_sended=0;
	sd->party_invite=0;
	sd->party_x=-1;
	sd->party_y=-1;
	sd->party_hp=-1;

	// ギルド関係の初期化
	sd->guild_sended=0;
	sd->guild_invite=0;
	sd->guild_alliance=0;

	// イベント関係の初期化
	memset(sd->eventqueue,0,sizeof(sd->eventqueue));
	for(i=0;i<MAX_EVENTTIMER;i++)
		sd->eventtimer[i] = -1;

	// 位置の設定
	pc_setpos(sd,sd->status.last_point.map ,
		sd->status.last_point.x , sd->status.last_point.y, 0);

	// pet
	if(sd->status.pet_id)
		intif_request_petdata(sd->status.account_id,sd->status.char_id,sd->status.pet_id);

	// パーティ、ギルドデータの要求
	if( sd->status.party_id>0 && (p=party_search(sd->status.party_id))==NULL)
		party_request_info(sd->status.party_id);
	if( sd->status.guild_id>0 && (g=guild_search(sd->status.guild_id))==NULL)
		guild_request_info(sd->status.guild_id);

	// pvpの設定
	sd->pvp_rank=0;
	sd->pvp_point=0;
	sd->pvp_timer=-1;

	clif_authok(sd);
	map_addnickdb(sd);

	pc_calcstatus(sd,1);
	pc_equiplookall(sd);

	return 0;
}


/*==========================================
 * session idに問題ありなので後始末
 *------------------------------------------
 */
int pc_authfail(int id)
{
	struct map_session_data *sd;

	sd = map_id2sd(id);
	if(sd==NULL)
		return 1;
	if(sd->new_fd){
		// 2重login状態だったので、新しい接続のみ落す
		clif_authfail_fd(sd->new_fd,0);

		sd->new_fd=0;
		return 0;
	}
	clif_authfail_fd(sd->fd,0);
	return 0;
}


/*==========================================
 * 覚えられるスキルの計算
 *------------------------------------------
 */
int pc_calc_skilltree(struct map_session_data *sd)
{
	int i,id=0,flag;
	int c=sd->status.class;
	for(i=0;i<MAX_SKILL;i++){
		sd->status.skill[i].id=0;
		if (sd->status.skill[i].flag){	// cardスキルなら、
			sd->status.skill[i].lv=(sd->status.skill[i].flag==1)?0:sd->status.skill[i].flag-2;	// 本当のlvに
			sd->status.skill[i].flag=0;	// flagも0にしておく
		}
	}

	if (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill){
		// 全てのスキル
		for(i=1;i<158;i++)
			sd->status.skill[i].id=i;
		for(i=210;i<291;i++)
			sd->status.skill[i].id=i;
		for(i=304;i<MAX_SKILL;i++)
			sd->status.skill[i].id=i;
	}else{
		// 通常の計算
		do{
			flag=0;
			for(i=0;(id=skill_tree[c][i].id)>0;i++){
				int j,f=1;
				for(j=0;j<5;j++)
					if( skill_tree[c][i].need[j].id &&
						pc_checkskill(sd,skill_tree[c][i].need[j].id) <
						skill_tree[c][i].need[j].lv
					 ) f=0;
				if(f && sd->status.skill[id].id==0 ){
					sd->status.skill[id].id=id;
					flag=1;
				}
			}
		}while(flag);
	}
//	printf("calc skill_tree\n");
	return 0;
}



/*==========================================
 * 重量アイコンの確認
 *------------------------------------------
 */
int pc_checkweighticon(struct map_session_data *sd)
{
	int flag=0;
	if(sd->weight*2 >= sd->max_weight)
		flag=1;
	if(sd->weight*10 >= sd->max_weight*9)
		flag=2;

	if(flag==1){
		if(sd->sc_data[SC_WEIGHT50].timer==-1)
			skill_status_change_start(&sd->bl,SC_WEIGHT50,0,0);
	}else{
		skill_status_change_end(&sd->bl,SC_WEIGHT50,-1);
	}
	if(flag==2){
		if(sd->sc_data[SC_WEIGHT90].timer==-1)
			skill_status_change_start(&sd->bl,SC_WEIGHT90,0,0);
	}else{
		skill_status_change_end(&sd->bl,SC_WEIGHT90,-1);
	}
	return 0;
}


/*==========================================
 * パラメータ計算
 * first==0の時、計算対象のパラメータが呼び出し前から
 * 変 化した場合自動でsendするが、
 * 能動的に変化させたパラメータは自前でsendするように
 *------------------------------------------
 */
int pc_calcstatus(struct map_session_data* sd,int first)
{
	int i,bl,item_id;
	struct map_session_data before;
	int weapontype;
	int weapontype1=0, weapontype2=0;
	int skill;
	int atk_ele=0;	//一時的な右手属性

	memcpy(&before,sd,sizeof(before));

	pc_calc_skilltree(sd);	// スキルツリーの計算

	sd->max_weight = max_weight_base[sd->status.class]+sd->status.str*300;
	if( (skill=pc_checkskill(sd,MC_INCCARRY))>0 )	// 所持量増加
		sd->max_weight += skill*1000;

	sd->weight=0;
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==0)
			continue;
		sd->weight+=itemdb_weight(sd->status.inventory[i].nameid)*sd->status.inventory[i].amount;
	}

	sd->cart_max_weight=80000;
	sd->cart_weight=0;
	sd->cart_max_num=100;
	sd->cart_num=0;
	for(i=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid==0)
			continue;
		sd->cart_weight+=itemdb_weight(sd->status.cart[i].nameid)*sd->status.cart[i].amount;
		sd->cart_num++;
	}

	for(i=0;i<6;i++)
		sd->paramb[i]=0;
	sd->hit = 0;
	sd->flee = 0;
	sd->flee2 = 0;
	sd->critical = 0;
	sd->aspd = 0;
	sd->watk = 0;
	sd->def = 0;
	sd->mdef = 0;
	sd->watk2 = 0;
	sd->def2 = 0;
	sd->mdef2 = 0;
	sd->status.max_hp = 0;
	sd->status.max_sp = 0;
	sd->status.weapon = 0;
	sd->status.shield = 0;
	sd->status.shoes = 0;
	sd->status.head_top = 0;
	sd->status.head_mid = 0;
	sd->status.head_bottom = 0;
	sd->attackrange = 1;
	sd->atk_ele = 0;
	sd->def_ele = 0;
	sd->star =0;
	sd->overrefine =0;
	sd->matk1 =0;
	sd->matk2 =0;
	weapontype = 0;
	sd->speed = DEFAULT_WALK_SPEED ;
	sd->hprate=100;
	sd->sprate=100;
	sd->castrate=100;
	sd->dsprate=100;
	memset(sd->addele,0,sizeof(sd->addele));
	memset(sd->addrace,0,sizeof(sd->addrace));
	memset(sd->addsize,0,sizeof(sd->addsize));
	memset(sd->subele,0,sizeof(sd->subele));
	memset(sd->subrace,0,sizeof(sd->subrace));
	memset(sd->addeff,0,sizeof(sd->addeff));
	memset(sd->reseff,0,sizeof(sd->reseff));

	sd->wcard_count=0;
	sd->dcard_count=0;

	sd->watk_ = 0;			//二刀流用(仮)
	sd->watk_2 = 0;
	sd->atk_ele_ = 0;
	sd->star_ = 0;
	sd->overrefine_ = 0;

	// 装備品によるステータス変化はここで実行
	for(i=0;i<MAX_INVENTORY;i++){
		int nameid=sd->status.inventory[i].nameid,ep=sd->status.inventory[i].equip;

		if(nameid==0 || ep==0 || itemdb_type(nameid)==10)	// 矢は考慮外
			continue;
		sd->watk += itemdb_atk(nameid);
		sd->def += itemdb_def(nameid);
		run_script(itemdb_equipscript(nameid),0,sd->bl.id,0);

		if(itemdb_type(nameid)==4){	// 武器 現在2刀流は考慮外
			int wlv=itemdb_wlv(nameid),r;

//			sd->status.weapon = weapontype = itemdb_look(nameid);

			//	二刀流武器look修正
			if (ep==0x20){
				weapontype2 = itemdb_look(nameid);

				//二刀流用データ入力
				sd->watk_ = itemdb_atk(nameid);
				sd->watk_2 = (r=sd->status.inventory[i].refine)*	// 精錬攻撃力
					refinebonus[wlv][0];
				if( (r-=refinebonus[wlv][2])>0 )	// 過剰精錬ボーナス
					sd->overrefine_ = r*refinebonus[wlv][1];

				if(sd->status.inventory[i].card[0]==0x00ff){	// 製造武器
					sd->star_ = (sd->status.inventory[i].card[1]>>8);	// 星のかけら
					sd->atk_ele_= (sd->status.inventory[i].card[1]&0x0f);	// 属 性
				}
				if(sd->atk_ele != 0)	//スクリプトで属性を取得したら
					sd->atk_ele_ = sd->atk_ele;
			}
			else{	//二刀流武器以外
				weapontype1 = itemdb_look(nameid);
				sd->status.weapon = weapontype = itemdb_look(nameid);	

				if(sd->status.inventory[i].card[0]==0x00ff){	// 製造武器
					sd->atk_ele= (sd->status.inventory[i].card[1]&0x0f);	// 属 性
				}
				if(sd->atk_ele != 0)	//スクリプトで属性を取得したら
					atk_ele = sd->atk_ele;			
			}

			sd->attackrange = itemdb_range(nameid);

			sd->watk2 += (r=sd->status.inventory[i].refine)*	// 精錬攻撃力
				refinebonus[wlv][0];
			if( (r-=refinebonus[wlv][2])>0 )	// 過剰精錬ボーナス
				sd->overrefine += r*refinebonus[wlv][1];

			if(sd->status.inventory[i].card[0]==0x00ff){	// 製造武器
				sd->star += (sd->status.inventory[i].card[1]>>8);	// 星のかけら
			}
		} else if(itemdb_type(nameid)==5){ // 防具
			if(ep&0x1)
				sd->status.head_bottom = itemdb_look(nameid);
			else if(ep&0x20)
				sd->status.shield = itemdb_look(nameid);
			else if(ep&0x40)
				sd->status.shoes = itemdb_look(nameid);
			else if(ep&0x100)
				sd->status.head_top = itemdb_look(nameid);
			else if(ep&0x200)
				sd->status.head_mid = itemdb_look(nameid);

			// 精錬防御力
			sd->def += sd->status.inventory[i].refine*refinebonus[0][0];
		}
	}
	for(i=0;i<6;i++)
		sd->parame[i] = sd->paramb[i];

	if(sd->status.weapon==11 && pc_checkequip(sd,0x8000)!=0){ // 矢
		if(atk_ele == 0){		//まだ属性が入っていない
		run_script(itemdb_equipscript(pc_checkequip(sd,0x8000)),0,sd->bl.id,0);
			atk_ele = sd->atk_ele;	//矢の属性を適用
		}
	}

	sd->atk_ele = atk_ele;		//右手属性を入れる

	// 武器ATKサイズ補正
	/*
	sd->atkmods[0] = atkmods[0][weapontype];
	sd->atkmods[1] = atkmods[1][weapontype];
	sd->atkmods[2] = atkmods[2][weapontype];
	*/

	//	二刀流最終 weapontype 検査
	if ((weapontype1 == 0)
	&&	(weapontype2 != 0)){// 左手武器 Only
		sd->status.weapon = weapontype = weapontype2;
	}
	else if ((weapontype1 == 1)
		 &&	 (weapontype2 == 1)){// 双短剣
		sd->status.weapon = weapontype = 0x11;
	}
	else if ((weapontype1 == 2)
		 &&	 (weapontype2 == 2)){// 双単手剣
		sd->status.weapon = weapontype = 0x12;
	}
	else if ((weapontype1 == 6)
	 	 &&	 (weapontype2 == 6)){// 双単手斧
		sd->status.weapon = weapontype = 0x13;
	}
	else if ((weapontype1 == 1)
	 	 &&	 (weapontype2 == 2)){// 短剣 - 単手剣
		sd->status.weapon = weapontype = 0x14;
	}
	else if ((weapontype1 == 2)
	 	 &&	 (weapontype2 == 1)){// 単手剣 - 短剣
		sd->status.weapon = weapontype = 0x14;
	}
	else if ((weapontype1 == 1)
	 	 &&	 (weapontype2 == 6)){// 短剣 - 斧
		sd->status.weapon = weapontype = 0x15;
	}
	else if ((weapontype1 == 6)
	 	 &&	 (weapontype2 == 1)){// 斧 - 短剣
		sd->status.weapon = weapontype = 0x15;
	}
	else if ((weapontype1 == 2)
	 	 &&	 (weapontype2 == 6)){// 単手剣 - 斧
		sd->status.weapon = weapontype = 0x16;
	}
	else if ((weapontype1 == 6)
	 	 &&	 (weapontype2 == 2)){// 斧 - 単手剣
		sd->status.weapon = weapontype = 0x16;
	}

	// 武器ATKサイズ補正 (右手)
	sd->atkmods[0] = atkmods[0][weapontype1];
	sd->atkmods[1] = atkmods[1][weapontype1];
	sd->atkmods[2] = atkmods[2][weapontype1];
	//武器ATKサイズ補正 (左手)
	sd->atkmods_[0] = atkmods[0][weapontype2];
	sd->atkmods_[1] = atkmods[1][weapontype2];
	sd->atkmods_[2] = atkmods[2][weapontype2];

	// jobボーナス分
	for(i=0;i<sd->status.job_level && i<MAX_LEVEL;i++)
		if(job_bonus[sd->status.class][i])
			sd->paramb[job_bonus[sd->status.class][i]-1]++;

	if( (skill=pc_checkskill(sd,AC_OWL))>0 )	// ふくろうの目
		sd->paramb[4] += skill;

	// ステータス変化による基本パラメータ補正
	if(sd->sc_count){
		if(sd->sc_data[SC_INCREASEAGI].timer!=-1){	// 速度増加
			sd->paramb[1]+= 2+sd->sc_data[SC_INCREASEAGI].val1;
			sd->speed -= sd->speed *25/100;
		}
		if(sd->sc_data[SC_DECREASEAGI].timer!=-1)	// 速度減少(agiはbattle.cで)
			sd->speed = sd->speed *125/100;

		if(sd->sc_data[SC_BLESSING].timer!=-1){	// ブレッシング
			sd->paramb[0]+= sd->sc_data[SC_BLESSING].val1;
			sd->paramb[3]+= sd->sc_data[SC_BLESSING].val1;
			sd->paramb[4]+= sd->sc_data[SC_BLESSING].val1;
		}
		if(sd->sc_data[SC_CONCENTRATE].timer!=-1){	// 集中力向上
			sd->paramb[1]+= (sd->status.agi+sd->paramb[1])*(2+sd->sc_data[SC_CONCENTRATE].val1)/100;
			sd->paramb[4]+= (sd->status.dex+sd->paramb[4])*(2+sd->sc_data[SC_CONCENTRATE].val1)/100;
		}
		if(sd->sc_data[SC_GLORIA].timer!=-1)	// グロリア
			sd->paramb[5]+= 30;
		if(sd->sc_data[SC_LOUD].timer!=-1)	// ラウドボイス
			sd->paramb[0]+= 4;
		if(sd->sc_data[SC_QUAGMIRE].timer!=-1)	// クァグマイア(AGI/DEXはbattle.cで)
			sd->speed = sd->speed*3/2;
	}

	for(i=0;i<MAX_INVENTORY;i++){
		int nameid=sd->status.inventory[i].nameid,ep;

		if(nameid==0 || (ep=sd->status.inventory[i].equip)==0)
			continue;

		if(itemdb_type(nameid)==4){	// 武器
			if(sd->status.inventory[i].card[0]!=0x00ff){
				int j;
				for(j=0;j<itemdb_slot(nameid);j++){	// カード
					int c=sd->status.inventory[i].card[j];
					if(c>0){
						run_script(itemdb_equipscript(c),0,sd->bl.id,0);
						sd->wcard[sd->wcard_count++]=c;
					}
				}
			}
		} else if(itemdb_type(nameid)==5){ // 防具
			int j;
			for(j=0;j<itemdb_slot(nameid);j++){	// カード
				int c=sd->status.inventory[i].card[j];
				if(c>0){
					run_script(itemdb_equipscript(c),0,sd->bl.id,0);
					sd->dcard[sd->dcard_count++]=c;
				}
			}
		}
	}

	sd->paramc[0]=sd->status.str+sd->paramb[0];
	sd->paramc[1]=sd->status.agi+sd->paramb[1];
	sd->paramc[2]=sd->status.vit+sd->paramb[2];
	sd->paramc[3]=sd->status.int_+sd->paramb[3];
	sd->paramc[4]=sd->status.dex+sd->paramb[4];
	sd->paramc[5]=sd->status.luk+sd->paramb[5];

	sd->matk1+=sd->paramc[3]+(sd->paramc[3]/5)*(sd->paramc[3]/5);
	sd->matk2+=sd->paramc[3]+(sd->paramc[3]/7)*(sd->paramc[3]/7);
	sd->hit += sd->paramc[4] + sd->status.base_level;
	sd->flee += sd->paramc[1] + sd->status.base_level;
	sd->def2 += sd->paramc[2];
	sd->mdef2 += sd->paramc[3];
	sd->flee2 += sd->paramc[5]/10+1;
	sd->critical += (sd->paramc[5]*3/10)+1;

//	sd->aspd += 10 * aspd_base[sd->status.class][weapontype] * (250 - sd->paramc[1] - sd->paramc[4]/4) /250;

	// 二刀流 ASPD 修正
	if (weapontype<=16)
		sd->aspd = aspd_base[sd->status.class][weapontype]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[sd->status.class][weapontype]/1000;
	else
		sd->aspd = 1.4 * (
			(aspd_base[sd->status.class][weapontype1]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[sd->status.class][weapontype1]/1000) +
			(aspd_base[sd->status.class][weapontype2]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[sd->status.class][weapontype2]/1000)
			) / 2;

	//攻撃速度増加

	if(sd->aspd < 100) sd->aspd = 100;	// 最大ASPDを190に制限( (2000-[190]*10)=100 )
	sd->amotion = sd->aspd;
	sd->dmotion = 800-sd->paramc[1]*4;
	if(sd->dmotion<400)
		sd->dmotion = 400;

	if ((item_id=pc_checkequip(sd,2)) != -1) {
		if (item_id==1165 ) 	// Masamune - Def = 0
			sd->def = 0;
	}

	if(pc_isriding(sd))							// 騎兵修練
		sd->aspd = sd->aspd*(100 + 10*(5 - pc_checkskill(sd,KN_CAVALIERMASTERY)))/ 100;

	if( (skill=pc_checkskill(sd,AC_VULTURE))>0 && sd->status.weapon == 11){	// ワシの目
		sd->hit += skill;
		sd->attackrange += skill;
	}
	if( (skill=pc_checkskill(sd,TF_MISS))>0 )	// 回避率増加
		sd->flee += skill*3;
	if( (skill=pc_checkskill(sd,MO_DODGE))>0 )	// 見切り
		sd->flee += (skill*3)>>1;
	if( (skill=pc_checkskill(sd,BS_WEAPONRESEARCH))>0)	// 武器研究の命中率増加
		sd->hit += skill*2;
	if(sd->status.option&2 && (skill = pc_checkskill(sd,RG_TUNNELDRIVE))>0 )	// トンネルドライブ	// トンネルドライブ
		sd->speed += sd->speed*(20-skill)/40;
	if (pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)	// カートによる速度低下
		sd->speed = sd->speed + (10-skill) * 0.1 * DEFAULT_WALK_SPEED;
	else if (pc_isriding(sd))	// ペコペコ乗りによる速度増加
		sd->speed = sd->speed - 0.17 * DEFAULT_WALK_SPEED;

	if((skill=pc_checkskill(sd,CR_TRUST))>0) { // フェイス
		sd->status.max_hp += skill*200;
		sd->addele[6] += skill*5;
	}

	bl=sd->status.base_level;

	{// 最大HP計算（オーバーフロー対策付き）
		int hp = (int)
			(((double)(35 + bl * 5) + ((double)((1 + bl) * bl / 2) * hp_coefficient[sd->status.class])) * (1 + (double)sd->paramc[2]/100.))
			+ sd->status.max_hp;
		if(sd->hprate!=100)
			hp=hp*sd->hprate/100;
		sd->status.max_hp = ((hp>32000)?32000:hp);
	}

	// 最大SP計算
	sd->status.max_sp += ((sp_coefficient[sd->status.class] * bl / 10) + 10) * (100 + sd->paramc[3]) / 100 + sd->parame[3];
	if(sd->sprate!=100)
		sd->status.max_sp=sd->status.max_sp*sd->sprate/100;

	if(sd->status.hp>sd->status.max_hp)
		sd->status.hp=sd->status.max_hp;
	if(sd->status.sp>sd->status.max_sp)
		sd->status.sp=sd->status.max_sp;

	// 種族耐性（これでいいの？ ディバインプロテクションと同じ処理がいるかも）
	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){	// ドラゴノロジー
		sd->addrace[9]+=skill*4;
		sd->subrace[9]+=skill*4;
	}

	// スキルやステータス異常による残りのパラメータ補正
	if(sd->sc_count){
		// ATK/DEF変化形
		if(sd->sc_data[SC_ANGELUS].timer!=-1)	// エンジェラス
			sd->def2 = sd->def2*(110+5*sd->sc_data[SC_ANGELUS].val1)/100;
		if(sd->sc_data[SC_IMPOSITIO].timer!=-1)	// インポシティオマヌス
			sd->watk += sd->sc_data[SC_IMPOSITIO].val1*5;
		if(sd->sc_data[SC_PROVOKE].timer!=-1){	// プロボック
			sd->def2 = sd->def2*(100-6*sd->sc_data[SC_PROVOKE].val1)/100;
			sd->watk = sd->watk*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
		}
		item_id = pc_checkequip(sd,2);		// RoVeRT
		if(sd->sc_data[SC_DRUMBATTLE].timer!=-1 && (item_id!=-1 && itemdb_wlv(item_id)==4)){	// 戦 太鼓の響き
			sd->watk += sd->sc_data[SC_DRUMBATTLE].val2;
			sd->def  += sd->sc_data[SC_DRUMBATTLE].val3;
		}
		if(sd->sc_data[SC_NIBELUNGEN].timer!=-1)	// ニーベルングの指輪
			sd->watk += sd->sc_data[SC_NIBELUNGEN].val2;
		if(sd->sc_data[SC_ETERNALCHAOS].timer!=-1)	// エターナルカオス
			sd->def=0;

		// ASPD/移動速度変化系
		if(sd->sc_data[SC_TWOHANDQUICKEN].timer!=-1)
			sd->aspd = sd->aspd*70/100;
		if(sd->sc_data[SC_ADRENALINE].timer!=-1)
			sd->aspd = sd->aspd*70/100;
		if(sd->sc_data[SC_SPEARSQUICKEN].timer!=-1)	// スピアクィッケン
			sd->aspd = sd->aspd * sd->sc_data[SC_SPEARSQUICKEN].val2/100;
		if(sd->sc_data[SC_ASSNCROS].timer!=-1)		// 夕陽のアサシンクロス
			sd->aspd = sd->aspd * sd->sc_data[SC_ASSNCROS].val2/100;
		if(sd->sc_data[SC_DONTFORGETME].timer!=-1){		// 私を忘れないで
			sd->aspd = sd->aspd * sd->sc_data[SC_DONTFORGETME].val2/100;
			sd->speed= sd->speed* sd->sc_data[SC_DONTFORGETME].val3/100;
		}
		if(	sd->sc_data[i=SC_SPEEDPOTION2].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION1].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION0].timer!=-1)	// 増 速ポーション
			sd->aspd = sd->aspd * sd->sc_data[i].val2/100;

		// HIT/FLEE変化系
		if(sd->sc_data[SC_WHISTLE].timer!=-1){  // 口笛
			sd->flee += sd->sc_data[SC_WHISTLE].val2 * sd->flee/100;
			sd->flee2+= sd->sc_data[SC_WHISTLE].val2 * sd->flee2/100;
		}
		if(sd->sc_data[SC_HUMMING].timer!=-1)  // ハミング
			sd->hit += sd->sc_data[SC_HUMMING].val2 * sd->hit/100;
		if(sd->sc_data[SC_BLIND].timer!=-1){	// 暗黒
			sd->hit -= sd->hit*25/100;
			sd->flee -= sd->flee*25/100;
		}

		// 耐性
		if(sd->sc_data[SC_SIEGFRIED].timer!=-1){  // 不死身のジークフリード
			sd->addele[1] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
			sd->addele[3] += sd->sc_data[SC_SIEGFRIED].val2;	// 火
		}
		if(sd->sc_data[SC_PROVIDENCE].timer!=-1){	// プロヴィデンス
			sd->addele[6] += sd->sc_data[SC_PROVIDENCE].val2;	// 対 聖属性
			sd->addrace[6] += sd->sc_data[SC_PROVIDENCE].val2;	// 対 ?魔
		}

		// その他
		if(sd->sc_data[SC_APPLEIDUN].timer!=-1){	// イドゥンの林檎
			int max_hp=sd->status.max_hp;
			max_hp += (sd->sc_data[SC_APPLEIDUN].val2 * max_hp)/100;
			sd->status.max_hp=(max_hp>32000)?32000:max_hp;
		}
		if(sd->sc_data[SC_SERVICE4U].timer!=-1)	// サービスフォーユー
			sd->status.max_sp +=
				(sd->sc_data[SC_SERVICE4U].val2 * sd->status.max_sp)/100;

		if(sd->sc_data[SC_FORTUNE].timer!=-1)	// 幸運のキス
			sd->critical += sd->sc_data[SC_FORTUNE].val1 * sd->critical/100;

		if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer!=-1)	// 爆裂波動
			sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val2 * sd->critical/1000;

		if(sd->sc_data[SC_STEELBODY].timer!=-1){	// 金剛
			sd->def = 90;
			sd->mdef = 90;
			sd->aspd = sd->aspd * 125 / 100;
			sd->speed = sd->speed * 125 / 100;
		}
/*		if(sd->sc_data[SC_DEFENDER].timer!=-1)	// Defender
			sd->def2 += ((sd->status.vit+20)*skill*10)/100;
		if(sd->sc_data[SC_CASTCANCEL].timer!=-1){	// Cast Cancel
			skill_castcancel(&sd->bl);
			sd->status.sp -= ((100)-(skill*15))/100;
		}
		if(sd->sc_data[SC_SPELLBREAKER].timer!=-1){	// Spellbreaker
			skill_castcancel(&sd->bl);
			sd->status.sp = ((skill*25-25)/sd->status.max_sp)/100;
		}
		*/
/*		if(sd->sc_data[SC_VOLCANO].timer!=-1)	// エンチャントポイズン(属性はbattle.cで)
			sd->addeff[2]+=sd->sc_data[SC_VOLCANO].val2;//% of granting
		if(sd->sc_data[SC_DELUGE].timer!=-1)	// エンチャントポイズン(属性はbattle.cで)
			sd->addeff[0]+=sd->sc_data[SC_DELUGE].val2;//% of granting
		*/
	}

	if(sd->skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0) {
		sd->prev_speed = sd->speed;
		sd->speed = sd->speed*(175 - skill*5)/100;
	}
	clif_updatestatus(sd,SP_SPEED);

	if(first) {
		clif_updatestatus(sd,SP_MAXHP);
		clif_updatestatus(sd,SP_MAXSP);
		clif_updatestatus(sd,SP_HP);
		clif_updatestatus(sd,SP_SP);
		return 0;
	}

	if( memcmp(before.status.skill,sd->status.skill,sizeof(sd->status.skill)) )
		clif_skillinfoblock(sd);	// スキル送信

	if(before.weight != sd->weight)
		clif_updatestatus(sd,SP_WEIGHT);
	if(before.max_weight != sd->max_weight)
		clif_updatestatus(sd,SP_MAXWEIGHT);
	for(i=0;i<6;i++)
		if(before.paramb[i] != sd->paramb[i])
			clif_updatestatus(sd,SP_STR+i);
	if(before.hit != sd->hit)
		clif_updatestatus(sd,SP_HIT);
	if(before.flee != sd->flee)
		clif_updatestatus(sd,SP_FLEE1);
	if(before.aspd != sd->aspd)
		clif_updatestatus(sd,SP_ASPD);
	if(before.watk != sd->watk || before.paramc[0]!=sd->paramc[0])
		clif_updatestatus(sd,SP_ATK1);
	if(before.def != sd->def)
		clif_updatestatus(sd,SP_DEF1);
	if(before.watk2 != sd->watk2)
		clif_updatestatus(sd,SP_ATK2);
	if(before.def2 != sd->def2)
		clif_updatestatus(sd,SP_DEF2);
	if(before.flee2 != sd->flee2)
		clif_updatestatus(sd,SP_FLEE2);
	if(before.critical != sd->critical)
		clif_updatestatus(sd,SP_CRITICAL);
	if(before.attackrange != sd->attackrange)
		clif_updatestatus(sd,SP_ATTACKRANGE);
	if(before.matk1 != sd->matk1)
		clif_updatestatus(sd,SP_MATK1);
	if(before.matk2 != sd->matk2)
		clif_updatestatus(sd,SP_MATK2);
	if(before.mdef != sd->mdef)
		clif_updatestatus(sd,SP_MDEF1);
	if(before.mdef2 != sd->mdef2)
		clif_updatestatus(sd,SP_MDEF2);

	if(before.status.max_hp != sd->status.max_hp)
		clif_updatestatus(sd,SP_MAXHP);
	if(before.status.max_sp != sd->status.max_sp)
		clif_updatestatus(sd,SP_MAXSP);
	if(before.status.hp != sd->status.hp)
		clif_updatestatus(sd,SP_HP);
	if(before.status.sp != sd->status.sp)
		clif_updatestatus(sd,SP_SP);

	if(before.cart_num != before.cart_num || before.cart_max_num != before.cart_max_num ||
		before.cart_weight != before.cart_weight || before.cart_max_weight != before.cart_max_weight )
		clif_updatestatus(sd,SP_CARTINFO);

	// 装 備品による見た目の変化分
	if(before.status.weapon != sd->status.weapon)
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	if(before.status.shield != sd->status.shield)
		clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
	if(before.status.shoes != sd->status.shoes)
		clif_changelook(&sd->bl,LOOK_SHOES,sd->status.shoes);
	if(before.status.head_bottom != sd->status.head_bottom)
		clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
	if(before.status.head_top != sd->status.head_top)
		clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
	if(before.status.head_mid != sd->status.head_mid)
		clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);

	return 0;
}


/*==========================================
 * 装 備品による能力等のボーナス設定
 *------------------------------------------
 */
int pc_bonus(struct map_session_data *sd,int type,int val)
{
	switch(type){
	case SP_STR:
	case SP_AGI:
	case SP_VIT:
	case SP_INT:
	case SP_DEX:
	case SP_LUK:
		sd->paramb[type-SP_STR]+=val;
		break;
	case SP_ATK2:
		sd->watk2+=val;
		break;
	case SP_MATK1:
		break;
	case SP_DEF1:
		sd->def+=val;
		break;
	case SP_MDEF1:
		sd->mdef+=val;
		break;
	case SP_HIT:
		sd->hit+=val;
		break;
	case SP_FLEE1:
		sd->flee+=val;
		break;
	case SP_FLEE2:
		sd->flee2+=val;
		break;
	case SP_CRITICAL:
		sd->critical+=val;
		break;
	case SP_ATKELE:
		sd->atk_ele=val;
		break;
	case SP_DEFELE:
		sd->def_ele=val;
		break;
	case SP_MAXHP:
		sd->status.max_hp+=val;
		break;
	case SP_MAXSP:
		sd->status.max_sp+=val;
		break;
	case SP_CASTRATE:
		sd->castrate+=val;
		break;
	case SP_MAXHPRATE:
		sd->hprate+=val;
		break;
	case SP_MAXSPRATE:
		sd->sprate+=val;
		break;
	case SP_SPRATE:
		sd->dsprate+=val;
		break;
	case SP_ATTACKRANGE:		// bAtkRange = 56
		sd->attackrange+=val;
		break;
	case SP_ASPD:
		sd->aspd-=val*10;
		break;
	case SP_SPEED:			// bSpeed = 0
		sd->speed -= DEFAULT_WALK_SPEED *val/100;
		break;
	default:
		printf("pc_bonus: unknown type %d %d !\n",type,val);
		break;
	}

	return 0;
}


/*==========================================
 * 装 備品による能力等のボーナス設定
 *------------------------------------------
 */
int pc_bonus2(struct map_session_data *sd,int type,int type2,int val)
{
	switch(type){
	case SP_ADDELE:
		sd->addele[type2]+=val;
		break;
	case SP_ADDRACE:
		sd->addrace[type2]+=val;
		break;
	case SP_ADDSIZE:
		sd->addsize[type2]+=val;
		break;
	case SP_SUBELE:
		sd->subele[type2]+=val;
		break;
	case SP_SUBRACE:
		sd->subrace[type2]+=val;
		break;
	case SP_ADDEFF:
		sd->addeff[type2]+=val;
		break;
	case SP_RESEFF:
		sd->reseff[type2]+=val;
		break;
	default:
		printf("pc_bonus2: unknown type %d %d %d!\n",type,type2,val);
		break;
	}

	return 0;
}


/*==========================================
 * スクリプトによるスキル所得
 *------------------------------------------
 */
int pc_skill(struct map_session_data *sd,int id,int level,int flag)
{
	if(level>10){
		printf("support card skill only!\n");
		return 0;
	}
	if(!flag && sd->status.skill[id].id == id){	// クエスト所得ならここで条件を確認して送信する
		sd->status.skill[id].lv=level;
		pc_calcstatus(sd,0);
		clif_skillinfoblock(sd);
	}
	else if(sd->status.skill[id].lv < level){	// 覚えられるがlvが小さいなら
		if(sd->status.skill[id].id==id)
			sd->status.skill[id].flag=sd->status.skill[id].lv+2;	// lvを記憶
		else {
			sd->status.skill[id].id=id;
			sd->status.skill[id].flag=1;	// cardスキルとする
		}
		sd->status.skill[id].lv=level;
	}

	return 0;
}


/*==========================================
 * カード挿入
 *------------------------------------------
 */
int pc_insert_card(struct map_session_data *sd,int idx_card,int idx_equip)
{
	if(idx_card >= 0 && idx_card < MAX_INVENTORY && idx_equip >= 0 && idx_equip < MAX_INVENTORY) {
		int i;
		int nameid=sd->status.inventory[idx_equip].nameid;
		int cardid=sd->status.inventory[idx_card].nameid;
		int ep=itemdb_equip(cardid);

		if( nameid <= 0 || ( itemdb_type(nameid)!=4 && itemdb_type(nameid)!=5)||	// 装 備じゃない
			( sd->status.inventory[idx_equip].identify==0 ) ||		// 未鑑定
			( sd->status.inventory[idx_equip].card[0]==0x00ff) ||		// 製造武器
			( (itemdb_equip(nameid)&ep)==0 ) ||					// 装 備個所違い
			( itemdb_type(nameid)==4 && ep==32) ||			// 両 手武器と盾カード
			( sd->status.inventory[idx_equip].card[0]==(short)0xff00) || sd->status.inventory[idx_equip].equip > 0){

			clif_insert_card(sd,idx_equip,idx_card,1);
			return 0;
		}
		for(i=0;i<itemdb_slot(nameid);i++){
			if( sd->status.inventory[idx_equip].card[i] == 0){
			// 空きスロットがあったので差し込む
				sd->status.inventory[idx_equip].card[i]=cardid;

			// カードは減らす
				sd->status.inventory[idx_card].amount-=1;
				sd->weight -= itemdb_weight(sd->status.inventory[idx_card].nameid);
				if(sd->status.inventory[idx_card].amount==0){
					memset(&sd->status.inventory[idx_card],0,sizeof(sd->status.inventory[0]));
				}
				clif_insert_card(sd,idx_equip,idx_card,0);
				return 0;
			}
		}
	}
	else
		clif_insert_card(sd,idx_equip,idx_card,1);

	return 0;
}


/*==========================================
 * 武器装備カード検索
 *------------------------------------------
 */
int pc_check_equip_wcard(struct map_session_data *sd,int cardid)
{
	int i;
	for(i=0;i<sd->wcard_count;i++)
		if(sd->wcard[i]==cardid)
			return 1;

	return 0;
}


/*==========================================
 * 防具装備カード検索
 *------------------------------------------
 */
int pc_check_equip_dcard(struct map_session_data *sd,int cardid)
{
	int i;
	for(i=0;i<sd->dcard_count;i++)
		if(sd->dcard[i]==cardid)
			return 1;

	return 0;
}


/*==========================================
 * カード装備検索
 *------------------------------------------
 */
int pc_check_equip_card(struct map_session_data *sd,int cardid)
{
	return pc_check_equip_wcard(sd,cardid) || pc_check_equip_dcard(sd,cardid);
}


//
// アイテム物
//


/*==========================================
 * スキルによる買い値修正
 *------------------------------------------
 */
int pc_modifybuyvalue(struct map_session_data *sd,int orig_value)
{
	int skill;
	if((skill=pc_checkskill(sd,MC_DISCOUNT))>0)	// ディスカウント
		orig_value-=(int)((double)orig_value*(5+skill*2-(skill==10))/100);
	if((skill=pc_checkskill(sd,RG_COMPULSION))>0)	// コムパルションディスカウント
		orig_value-=(int)((double)orig_value*(5+skill*4)/100);

	return orig_value;
}


/*==========================================
 * スキルによる売り値修正
 *------------------------------------------
 */
int pc_modifysellvalue(struct map_session_data *sd,int orig_value)
{
	int skill;
	if((skill=pc_checkskill(sd,MC_OVERCHARGE))>0)	// オーバーチャージ
		orig_value+=(int)((double)orig_value*(5+skill*2-(skill==10))/100);

	return orig_value;
}


/*==========================================
 * アイテムを買った時に、新しいアイテム欄を使うか、
 * 3万個制限にかかるか確認
 *------------------------------------------
 */
int pc_checkadditem(struct map_session_data *sd,int nameid,int amount)
{
	int i;

	if(itemdb_isequip(nameid))
		return ADDITEM_NEW;

	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==nameid){
			if(sd->status.inventory[i].amount+amount > MAX_AMOUNT)
				return ADDITEM_OVERAMOUNT;
			return ADDITEM_EXIST;
		}
	}

	return ADDITEM_NEW;
}


/*==========================================
 * 空きアイテム欄の個数
 *------------------------------------------
 */
int pc_inventoryblank(struct map_session_data *sd)
{
	int i,b;
	for(i=0,b=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==0)
			b++;
	}

	return b;
}


/*==========================================
 * お金を払う
 *------------------------------------------
 */
int pc_payzeny(struct map_session_data *sd,int zeny)
{
	if(sd->status.zeny<zeny)
		return 1;
	sd->status.zeny-=zeny;
	clif_updatestatus(sd,SP_ZENY);

	return 0;
}


/*==========================================
 * お金を得る
 *------------------------------------------
 */
int pc_getzeny(struct map_session_data *sd,int zeny)
{
	if(sd->status.zeny+zeny > MAX_ZENY)
		return 1;
	sd->status.zeny+=zeny;
	clif_updatestatus(sd,SP_ZENY);

	return 0;
}


/*==========================================
 * アイテムを探して、インデックスを返す
 *------------------------------------------
 */
int pc_search_inventory(struct map_session_data *sd,int item_id)
{
	int i;
	for(i=0;i<MAX_INVENTORY;i++) {
		if(sd->status.inventory[i].nameid == item_id &&
		 (sd->status.inventory[i].amount > 0 || item_id >= 0))
			return i;
	}

	return -1;
}


/*==========================================
 * アイテム追加。個数のみitem構造体の数字を無視
 *------------------------------------------
 */
int pc_additem(struct map_session_data *sd,struct item *item_data,int amount)
{
	int i,w;

	if(amount <= 0) return 1;
	if((w = itemdb_weight(item_data->nameid)*amount) + sd->weight > sd->max_weight)
		return 2;

	i = -1;
	if(!itemdb_isequip(item_data->nameid)){
		// 装 備品ではないので、既所有品なら個数のみ変化させる
		i = pc_search_inventory(sd,item_data->nameid);
		if(i >= 0) {
			if(sd->status.inventory[i].amount+amount > MAX_AMOUNT)
				return 5;
			sd->status.inventory[i].amount+=amount;
			clif_additem(sd,i,amount,0);
		}
	}
	if(i < 0){
		// 装 備品か未所有品だったので空き欄へ追加
		i = pc_search_inventory(sd,0);
		if(i >= 0) {
			memcpy(&sd->status.inventory[i],item_data,sizeof(sd->status.inventory[0]));
			sd->status.inventory[i].amount=amount;
			clif_additem(sd,i,amount,0);
		}
		else return 4;
	}
	sd->weight += w;
	clif_updatestatus(sd,SP_WEIGHT);

	return 0;
}


/*==========================================
 * アイテムを減らす
 *------------------------------------------
 */
int pc_delitem(struct map_session_data *sd,int n,int amount,int type)
{
	if(sd->status.inventory[n].nameid==0 ||
	   sd->status.inventory[n].amount<amount)
		return 1;

	sd->status.inventory[n].amount-=amount;
	sd->weight -= itemdb_weight(sd->status.inventory[n].nameid)*amount ;
	if(sd->status.inventory[n].amount==0){
		memset(&sd->status.inventory[n],0,sizeof(sd->status.inventory[0]));
	}
	if(!type)
		clif_delitem(sd,n,amount);
	clif_updatestatus(sd,SP_WEIGHT);

	return 0;
}


/*==========================================
 * アイテムを落す
 *------------------------------------------
 */
int pc_dropitem(struct map_session_data *sd,int n,int amount)
{
	if(sd->status.inventory[n].nameid==0 ||
	   sd->status.inventory[n].amount<amount)
		return 1;
	map_addflooritem(&sd->status.inventory[n],amount,sd->bl.m,sd->bl.x,sd->bl.y);
	pc_delitem(sd,n,amount,0);

	return 0;
}


/*==========================================
 * アイテムを拾う
 *------------------------------------------
 */
int pc_takeitem(struct map_session_data *sd,struct flooritem_data *fitem)
{
	int flag;
	if((flag = pc_additem(sd,&fitem->item_data,fitem->item_data.amount))){
		// 重量overで取得失敗
		clif_additem(sd,0,0,flag);
	} else {
		/* 取得成功 */
		if(sd->attacktimer != -1)
			pc_stopattack(sd);
		clif_takeitem(&sd->bl,&fitem->bl);
		map_clearflooritem(fitem->bl.id);
	}

	return 0;
}


/*==========================================
 * アイテムを使う
 *------------------------------------------
 */
int pc_useitem(struct map_session_data *sd,int n)
{
	int nameid,amount;

	if(sd->status.inventory[n].nameid==0||
		sd->status.inventory[n].amount<1||
	((sd->status.inventory[n].nameid==604)&&(map[sd->bl.m].flag.nobranch)) ){
		clif_useitemack(sd,n,0,0);
		return 1;
	}

	nameid = sd->status.inventory[n].nameid;
	amount = sd->status.inventory[n].amount;

	sd->status.inventory[n].amount--;
	sd->weight -= itemdb_weight(nameid) ;
	if(sd->status.inventory[n].amount==0){
		memset(&sd->status.inventory[n],0,sizeof(sd->status.inventory[0]));
	}
	clif_useitemack(sd,n,amount-1,1);

	run_script(itemdb_usescript(nameid),0,sd->bl.id,0);

	clif_updatestatus(sd,SP_WEIGHT);

	return 0;
}


/*==========================================
 * カートアイテム追加。個数のみitem構造体の数字を無視
 *------------------------------------------
 */
int pc_cart_additem(struct map_session_data *sd,struct item *item_data,int amount)
{
	int i;

	if(itemdb_weight(item_data->nameid)*amount + sd->cart_weight > sd->cart_max_weight)
		return 1;

	i=MAX_CART;
	if(!itemdb_isequip(item_data->nameid)){
		// 装 備品ではないので、既所有品なら個数のみ変化させる
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid==item_data->nameid){
				if(sd->status.cart[i].amount+amount > MAX_AMOUNT)
					return 1;
				sd->status.cart[i].amount+=amount;
				clif_cart_additem(sd,i,amount,0);
				break;
			}
		}
	}
	if(i==MAX_CART){
		// 装 備品か未所有品だったので空き欄へ追加
		for(i=MAX_CART-1;i>=0;i--){
			if(sd->status.cart[i].nameid==0 &&
			   (i==0 || sd->status.cart[i-1].nameid)){
				memcpy(&sd->status.cart[i],item_data,sizeof(sd->status.cart[0]));
				sd->status.cart[i].amount=amount;
				sd->cart_num++;				
				clif_cart_additem(sd,i,amount,0);
				break;
			}
		}
		if(i<0)
			return 1;
	}
	sd->cart_weight += itemdb_weight(item_data->nameid)*amount ;
	clif_updatestatus(sd,SP_CARTINFO);

	return 0;
}


/*==========================================
 * カートアイテムを減らす
 *------------------------------------------
 */
int pc_cart_delitem(struct map_session_data *sd,int n,int amount)
{
	if(sd->status.cart[n].nameid==0 ||
	   sd->status.cart[n].amount<amount)
		return 1;

	sd->status.cart[n].amount-=amount;
	sd->cart_weight -= itemdb_weight(sd->status.cart[n].nameid)*amount ;
	if(sd->status.cart[n].amount==0){
		memset(&sd->status.cart[n],0,sizeof(sd->status.cart[0]));
		sd->cart_num--;
	}
	clif_cart_delitem(sd,n,amount);
	clif_updatestatus(sd,SP_CARTINFO);

	return 0;
}


/*==========================================
 * カートへアイテム移動
 *------------------------------------------
 */
int pc_putitemtocart(struct map_session_data *sd,int idx,int amount)
{
	struct item *item_data=&sd->status.inventory[idx];
	if( item_data->nameid==0 || item_data->amount<amount || sd->vender_id )
		return 1;
	if(pc_cart_additem(sd,item_data,amount)==0)
		return pc_delitem(sd,idx,amount,0);

	return 1;
}


/*==========================================
 * カートからアイテム移動
 *------------------------------------------
 */
int pc_getitemfromcart(struct map_session_data *sd,int idx,int amount)
{
	struct item *item_data=&sd->status.cart[idx];
	int flag;
	if( item_data->nameid==0 || item_data->amount<amount || sd->vender_id )
		return 1;
	if((flag = pc_additem(sd,item_data,amount)) == 0)
		return pc_cart_delitem(sd,idx,amount);

	clif_additem(sd,0,0,flag);
	return 1;
}


/*==========================================
 * アイテム鑑定
 *------------------------------------------
 */
int pc_item_identify(struct map_session_data *sd,int idx)
{
	int flag=1;

	if(idx >= 0 && idx < MAX_INVENTORY) {
		if(sd->status.inventory[idx].nameid > 0 && sd->status.inventory[idx].identify == 0 ){
			flag=0;
			sd->status.inventory[idx].identify=1;
		}
		clif_item_identified(sd,idx,flag);
	}
	else
		clif_item_identified(sd,idx,flag);

	return !flag;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_steal_item(struct map_session_data *sd,struct block_list *bl)
{
	if(sd != NULL && bl != NULL && bl->type == BL_MOB) {
		int i,skill,rate,itemid,flag;
		struct mob_data *md;
		md=(struct mob_data *)bl;
		if(mob_db[md->class].mexp <= 0) {  // && !md->state.steal_flag  (Not RO friendly)
			skill = pc_checkskill(sd,TF_STEAL) * 50;
			for(i=0;i<8;i++) {
				itemid = mob_db[md->class].dropitem[i].nameid;
				if(itemid > 0 && itemdb_type(itemid) != 6) {
					rate = (mob_db[md->class].dropitem[i].p * (sd->status.base_level*4 + sd->paramc[4]*3 + skill))/1000;
					if(rand()%10000 < rate) {
						struct item tmp_item;
						memset(&tmp_item,0,sizeof(tmp_item));
						tmp_item.nameid = itemid;
						tmp_item.amount = 1;
						tmp_item.identify = 1;
						flag = pc_additem(sd,&tmp_item,1);
						if(flag)
							clif_additem(sd,0,0,flag);
						md->state.steal_flag = 1;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_steal_coin(struct map_session_data *sd,struct block_list *bl)
{
	if(sd != NULL && bl != NULL && bl->type == BL_MOB) {
		int rate,skill;
		struct mob_data *md;
		md=(struct mob_data *)bl;
		if(!md->state.steal_coin_flag) {
			skill = pc_checkskill(sd,RG_STEALCOIN)*10;
			rate = skill + (sd->status.base_level - mob_db[md->class].lv)*3 + sd->paramc[4]*2 + sd->paramc[5]*2;
			if(rand()%1000 < rate) {
				pc_getzeny(sd,mob_db[md->class].lv*10 + rand()%100);
				md->state.steal_coin_flag = 1;
				return 1;
			}
		}
	}

	return 0;
}
//
//
//
/*==========================================
 * PCの位置設定
 *------------------------------------------
 */
int pc_setpos(struct map_session_data *sd,char *mapname_org,int x,int y,int clrtype)
{
	char mapname[24];
	int m,c;

	if(sd->chatID)	// チャットから出る
		chat_leavechat(sd);
	if(sd->trade_partner)	// 取引を中断する
		trade_tradecancel(sd);
	storage_storage_quitsave(sd);	// 倉庫を開いてるなら保存する
	skill_castcancel(&sd->bl);	// 詠唱を中断する

	if(sd->party_invite>0)	// パーティ勧誘を拒否する
		party_reply_invite(sd,sd->party_invite_account,0);
	if(sd->guild_invite>0)	// ギルド勧誘を拒否する
		guild_reply_invite(sd,sd->guild_invite,0);
	if(sd->guild_alliance>0)	// ギルド同盟勧誘を拒否する
		guild_reply_reqalliance(sd,sd->guild_alliance_account,0);

	memcpy(mapname,mapname_org,16);
	mapname[16]=0;
	if(strstr(mapname,".gat")==NULL && strlen(mapname)<11){
		strcat(mapname,".gat");
	}
	m=map_mapname2mapid(mapname);
	if(m<0){
		if(sd->mapname[0]){
			int ip,port;
			if(map_mapname2ipport(mapname,&ip,&port)==0){
				clif_clearchar_area(&sd->bl,clrtype&0xffff);
				map_delblock(&sd->bl);
				memcpy(sd->mapname,mapname,16);
				sd->bl.x=x;
				sd->bl.y=y;
				sd->state.waitingdisconnect=1;
				chrif_save(sd);
				chrif_changemapserver(sd,mapname,x,y,ip,port);
				return 0;
			}
		}
#if 0
		clif_authfail_fd(sd->fd,0);	// cancel
		clif_setwaitclose(sd->fd);
#endif
		return 1;
	}

	if(x <0 || x >= map[m].xs || y <0 || y >= map[m].ys)
		x=y=0;
	if((x==0 && y==0) || (c=read_gat(m,x,y))==1 || c==5){
		if(x||y) printf("stacked (%d,%d)\n",x,y);
		do {
			x=rand()%(map[m].xs-2)+1;
			y=rand()%(map[m].ys-2)+1;
		} while((c=read_gat(m,x,y))==1 || c==5);
	}

	if(sd->mapname[0] && sd->bl.prev != NULL){
		clif_clearchar_area(&sd->bl,clrtype&0xffff);
		// printf("pc.c 63 clif_clearchar_area\n");
		map_delblock(&sd->bl);
		// pet
		if(sd->status.pet_id && sd->pet_npcdata) {
			if(sd->pet_npcdata->bl.m != m && sd->pet.intimate <= 0) {
				pet_remove_map(sd);
				intif_delete_petdata(sd->status.pet_id);
				sd->status.pet_id = 0;
				sd->pet_npcdata = NULL;
				sd->petDB = NULL;
				chrif_save(sd);
			}
			else if(sd->pet.intimate > 0) {
				pet_changestate(sd->pet_npcdata,MS_IDLE);
				clif_clearchar_area(&sd->pet_npcdata->bl,clrtype&0xffff);
				map_delblock(&sd->pet_npcdata->bl);
			}
		}
		clif_changemap(sd,mapname,x,y);
	}

	memcpy(sd->mapname,mapname,16);
	sd->bl.m = m;
	sd->bl.x = x;
	sd->bl.y = y;

	sd->walkpath.path_half=0;
	sd->walkpath.path_len=0;
	sd->walkpath.path_pos=0;

	if(sd->status.pet_id && sd->pet_npcdata && sd->pet.intimate > 0) {
		sd->pet_npcdata->bl.m = m;
		sd->pet_npcdata->bl.x = sd->pet_npcdata->to_x = x;
		sd->pet_npcdata->bl.y = sd->pet_npcdata->to_y = y;
		sd->pet_npcdata->dir = sd->dir;
		sd->pet_npcdata->walkpath.path_half=0;
		sd->pet_npcdata->walkpath.path_len=0;
		sd->pet_npcdata->walkpath.path_pos=0;
	}

//	map_addblock(&sd->bl);	/// ブロック登録とspawnは
//	clif_spawnpc(sd);		// clif_parse_LoadEndAckで行う

	return 0;
}


/*==========================================
 * PCのランダムワープ
 *------------------------------------------
 */
int pc_randomwarp(struct map_session_data *sd,int type)
{
	int x,y,c,i=0;
	int m=sd->bl.m;
	
	if(map[sd->bl.m].flag.noteleport)	// テレポート禁止
		return 0;
	
	do{
		x=rand()%(map[m].xs-2)+1;
		y=rand()%(map[m].ys-2)+1;
	}while( ((c=read_gat(m,x,y))==1 || c==5) && (i++)<1000 );
	
	if(i<1000)
		pc_setpos(sd,map[m].name,x,y,type);

	return 0;
}


/*==========================================
 * 現在位置のメモ
 *------------------------------------------
 */
int pc_memo(struct map_session_data *sd,int i)
{
	int skill=pc_checkskill(sd,AL_WARP);
	int j;
	
	if(skill<2 || i<-1 || i>2 || map[sd->bl.m].flag.nomemo){
		clif_skill_memo(sd,1);
		return 0;
	}

	for(j=0;j<3;j++){
		if(strcmp(sd->status.memo_point[i].map,map[sd->bl.m].name)==0){
			i=j;
			break;
		}
	}

	if(i==-1){
		for(i=skill-3;i>=0;i--){
			memcpy(&sd->status.memo_point[i+1],&sd->status.memo_point[i],
				sizeof(struct point));
		}
		i=0;
	}
	memcpy(sd->status.memo_point[i].map,map[sd->bl.m].name,16);
	sd->status.memo_point[i].x=sd->bl.x;
	sd->status.memo_point[i].y=sd->bl.y;

	clif_skill_memo(sd,0);

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_can_reach(struct map_session_data *sd,int x,int y)
{
	struct walkpath_data wpd;

	if( sd->bl.x==x && sd->bl.y==y )	// 同じマス
		return 1;

	// 障害物判定
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,x,y,0)!=-1)?1:0;
}

//
// 歩 行物
//
/*==========================================
 * 次の1歩にかかる時間を計算
 *------------------------------------------
 */
static int calc_next_walk_step(struct map_session_data *sd)
{
	if(sd->walkpath.path_pos>=sd->walkpath.path_len)
		return -1;
	if(sd->walkpath.path[sd->walkpath.path_pos]&1)
		return sd->speed*14/10;
		
	return sd->speed;
}


/*==========================================
 * 半歩進む(timer関数)
 *------------------------------------------
 */
static int pc_walk(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	int i;
	int x,y,dx,dy;

	sd=map_id2sd(id);
	if(sd==NULL)
		return 0;

	if(sd->walktimer != tid){
		printf("pc_walk %d != %d\n",sd->walktimer,tid);
		return 0;
	}
	sd->walktimer=-1;
	if(sd->walkpath.path_pos>=sd->walkpath.path_len || sd->walkpath.path_pos!=data)
		return 0;

	sd->walkpath.path_half ^= 1;
	if(sd->walkpath.path_half==0){ // マス目中心へ到着
		sd->walkpath.path_pos++;
		if(sd->state.change_walk_target){
			pc_walktoxy_sub(sd);
			return 0;
		}
	} else { // マス目境界へ到着
		int moveblock;

		if(sd->walkpath.path[sd->walkpath.path_pos]>=8)
			return 1;

		x = sd->bl.x;
		y = sd->bl.y;
		sd->dir=sd->head_dir=sd->walkpath.path[sd->walkpath.path_pos];
		dx = dirx[(int)sd->dir];
		dy = diry[(int)sd->dir];
		moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

		sd->walktimer = 1;
		map_foreachinmovearea(clif_pcoutsight,sd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,0,sd);

		x += dx;
		y += dy;

		if(moveblock) map_delblock(&sd->bl);
		sd->bl.x = x;
		sd->bl.y = y;
		if(moveblock) map_addblock(&sd->bl);

		map_foreachinmovearea(clif_pcinsight,sd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,0,sd);
		sd->walktimer = -1;

		if(sd->status.party_id>0){	// パーティのＨＰ情報通知検査
			struct party *p=party_search(sd->status.party_id);
			if(p!=NULL){
				int flag=0;
				map_foreachinmovearea(party_send_hp_check,sd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id,&flag);
				if(flag)
					sd->party_hp=-1;
			}
		}
		if(sd->status.option&4)	// クローキングの消滅検査
			skill_check_cloaking(&sd->bl);

		skill_unit_move(&sd->bl,tick,1);	// スキルユニットの検査

		if(map_getcell(sd->bl.m,x,y)&0x80)
			npc_touch_areanpc(sd,sd->bl.m,x,y);
	}
	if((i=calc_next_walk_step(sd))>0) {
		i = i/2;
		if(i <= 0)
			i = 1;
		sd->walktimer=add_timer(tick+i,pc_walk,id,sd->walkpath.path_pos);
	}

	return 0;
}


/*==========================================
 * 移動可能か確認して、可能なら歩行開始
 *------------------------------------------
 */
static int pc_walktoxy_sub(struct map_session_data *sd)
{
	struct walkpath_data wpd;
	int i;

	if(path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y,0))
		return 1;
	memcpy(&sd->walkpath,&wpd,sizeof(wpd));

	clif_walkok(sd);
	clif_movechar(sd);

	sd->state.change_walk_target=0;

	if((i=calc_next_walk_step(sd))>0){
		i = i/4;
		if(i <= 0)
			i = 1;
		sd->walktimer=add_timer(gettick()+i,pc_walk,sd->bl.id,0);
		if(sd->status.pet_id && sd->pet_npcdata && sd->pet.intimate > 0 && sd->petDB) {
			if(distance(sd->bl.x,sd->bl.y,sd->pet_npcdata->bl.x,sd->pet_npcdata->bl.y) > 9) {
				sd->pet_npcdata->speed = sd->speed/2;
				if(sd->pet_npcdata->speed <= 0)
					sd->pet_npcdata->speed = 1;
			}
			else
				sd->pet_npcdata->speed = sd->petDB->speed;
			pet_walktoxy(sd->pet_npcdata,sd->to_x,sd->to_y,0,sd->walkpath.path[sd->walkpath.path_len-1]);
		}
	}

	return 0;
}


/*==========================================
 * pc歩 行要求
 *------------------------------------------
 */
int pc_walktoxy(struct map_session_data *sd,int x,int y)
{

	sd->to_x=x;
	sd->to_y=y;
	
	if(sd->walktimer != -1 && sd->state.change_walk_target==0){
		// 現在歩いている最中の目的地変更なのでマス目の中心に来た時に
		// timer関 ?からpc_walktoxy_subを呼ぶようにする
		sd->state.change_walk_target=1;
	} else {
		pc_walktoxy_sub(sd);
	}

	return 0;
}

/*==========================================
 * 歩 行停止
 *------------------------------------------
 */
int pc_stop_walking(struct map_session_data *sd,int type)
{
	if(sd->walktimer != -1){
		delete_timer(sd->walktimer,pc_walk);
		sd->walktimer=-1;
		sd->walkpath.path_len=0;
		if(type&0x01)
			clif_fixpos(&sd->bl);
		if(sd->status.pet_id && sd->pet_npcdata) {
			pet_stop_walking(sd,sd->dir);
		}
	}
	if(type&0x02 && battle_config.pc_damage_delay)
		sd->canmove_tick = gettick() + sd->dmotion;

	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */
int pc_movepos(struct map_session_data *sd,int dst_x,int dst_y)
{
	int moveblock;
	int dx,dy,dist;

	struct walkpath_data wpd;

	if(path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,dst_x,dst_y,0))
		return 1;

	sd->dir = sd->head_dir = map_calc_dir(&sd->bl, dst_x,dst_y);

	dx = dst_x - sd->bl.x;
	dy = dst_y - sd->bl.y;
	dist = distance(sd->bl.x,sd->bl.y,dst_x,dst_y);

	moveblock = ( sd->bl.x/BLOCK_SIZE != dst_x/BLOCK_SIZE || sd->bl.y/BLOCK_SIZE != dst_y/BLOCK_SIZE);

	map_foreachinmovearea(clif_pcoutsight,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,dx,dy,0,sd);

	if(moveblock) map_delblock(&sd->bl);
	sd->bl.x = dst_x;
	sd->bl.y = dst_y;
	if(moveblock) map_addblock(&sd->bl);

	map_foreachinmovearea(clif_pcinsight,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,0,sd);

	if(sd->status.party_id>0){	// パーティのＨＰ情報通知検査
		struct party *p=party_search(sd->status.party_id);
		if(p!=NULL){
			int flag=0;
			map_foreachinmovearea(party_send_hp_check,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id,&flag);
			if(flag)
				sd->party_hp=-1;
		}
	}

	if(sd->status.option&4)	// クローキングの消滅検査
		skill_check_cloaking(&sd->bl);

	skill_unit_move(&sd->bl,gettick(),dist+5);	// スキルユニットの検査

	if(map_getcell(sd->bl.m,sd->bl.x,sd->bl.y)&0x80)
		npc_touch_areanpc(sd,sd->bl.m,sd->bl.x,sd->bl.y);

	return 0;
}

//
// 武器戦闘
//
/*==========================================
 * スキルの検索 所有していた場合Lvが返る
 *------------------------------------------
 */
int pc_checkskill(struct map_session_data *sd,int skill_id)
{
	if( skill_id>=10000 ){
		struct guild *g;
		if( sd->status.guild_id>0 && (g=guild_search(sd->status.guild_id))!=NULL)
			return guild_checkskill(g,skill_id);
		return 0;
	}

	if(sd->status.skill[skill_id].id == skill_id)
		return (sd->status.skill[skill_id].lv);

	return 0;
}

/*==========================================
 * 武器変更によるスキルの継続チェック
 * 引数：
 *   struct map_session_data *sd	セッションデータ
 *   int nameid						装備品ID
 * 返り値：
 *   0		変更なし
 *   -1		スキルを解除
 *------------------------------------------
 */
int pc_checkallowskill(struct map_session_data *sd,int nameid)
{
	// 騎士・クルセイダー系スキルのチェック
	if(sd->sc_data[SC_TWOHANDQUICKEN].timer!=-1){	// 2HQ
		if(sd->status.weapon != 3){	// 両手剣か
			skill_status_change_end(&sd->bl,SC_TWOHANDQUICKEN,-1);	// 2HQを解除
			return -1;
		}
	}
	if(sd->sc_data[SC_SPEARSQUICKEN].timer!=-1 ){	// スピアクィッケン
		if(sd->status.weapon != 5){	// 槍か
			skill_status_change_end(&sd->bl,SC_SPEARSQUICKEN,-1);	// スピアクイッケンを解除
			return -1;
		}
	}
/*
	if(sd->sc_data[SC_AUTOGUARD].timer!=-1){	// オートガード
		skill_status_change_end(&sd->bl,SC_AUTOGUARD,-1);
		return -1;
	}
*/
	return 0;
}


/*==========================================
 * 装 備品のチェック
 *------------------------------------------
 */
int pc_checkequip(struct map_session_data *sd,int pos)
{
	int i;
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid &&
		   (sd->status.inventory[i].equip&pos)){
			return (sd->status.inventory[i].nameid);
		}
	}

	return -1;
}


/*==========================================
 * PCの攻撃 (timer関数)
 *------------------------------------------
 */
int pc_attack_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	struct block_list *bl;
//	struct WeaponDamage wd;
	int dist,skill;

	sd=map_id2sd(id);
	if(sd == NULL)
		return 0;
	sd->attacktimer=-1;

	if(sd->bl.prev == NULL)
		return 0;

	bl=map_id2bl(sd->attacktarget);
	if(bl==NULL || bl->prev == NULL)
		return 0;

	// 同じmapでないなら攻撃しない
	// PCが死んでても攻撃しない
	if(sd->bl.m != bl->m || pc_isdead(sd))
		return 0;

	if( sd->opt1>0 || sd->status.option&6)	// 異常などで攻撃できない
		return 0;

	dist = distance(sd->bl.x,sd->bl.y,bl->x,bl->y);
	if( dist > sd->attackrange+1 ){	// 届 かないので移動
		clif_movetoattack(sd,bl);
		return 0;
	}

	sd->dir=sd->head_dir=map_calc_dir(&sd->bl, bl->x,bl->y );	// 向き設定

	// 実 際に攻撃する
 	if (sd->combo_flag == 1)
 		sd->combo_flag = 2;
	else if (sd->combo_flag == 3)
		sd->combo_flag = 4;
	else
		battle_weapon_attack(&sd->bl,bl,tick,0);

	if (sd->triple_delay > 0) {
		sd->attackabletime = tick + sd->triple_delay;
		sd->combo_delay1 = tick + sd->triple_delay - 300;
		sd->triple_delay = 0;
	}
	else if (sd->combo_flag == 2) {
		sd->attackabletime = tick + sd->combo_delay1;
		sd->combo_flag = 0;
	}
	else if (sd->combo_flag == 4) {
		sd->attackabletime = tick + sd->combo_delay2;
		sd->combo_flag = 0;
	}
	else if(sd->skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0 ) // フリーキャスト
		sd->attackabletime = tick + ((sd->aspd<<1)*(150 - skill*5)/100);
	else
		sd->attackabletime = tick + (sd->aspd<<1);

	if(sd->state.attack_continue){
		sd->attacktimer=add_timer(sd->attackabletime,pc_attack_timer,sd->bl.id,0);
	}

	return 0;
}


/*==========================================
 * 攻撃要求
 * typeが1なら継続攻撃
 *------------------------------------------
 */
int pc_attack(struct map_session_data *sd,int target_id,int type)
{
	struct block_list *bl;
	int d;

	bl=map_id2bl(target_id);
	if(bl==NULL || (bl->type!=BL_MOB && bl->type!=BL_PC))
		return 1;
	if(sd->attacktimer != -1)
		pc_stopattack(sd);
	sd->attacktarget=target_id;
	sd->state.attack_continue=type;

	d=DIFF_TICK(sd->attackabletime,gettick());
	if(d>0 && d<2000){	// 攻撃delay中
		sd->attacktimer=add_timer(sd->attackabletime,pc_attack_timer,sd->bl.id,0);
	} else {
		// 本来timer関数なので引数を合わせる
		pc_attack_timer(-1,gettick(),sd->bl.id,0);
	}

	return 0;
}


/*==========================================
 * 継 ?攻撃停止
 *------------------------------------------
 */
int pc_stopattack(struct map_session_data *sd)
{
	if(sd->attacktimer != -1) {
		delete_timer(sd->attacktimer,pc_attack_timer);
		sd->attacktimer=-1;
	}
	sd->attacktarget=0;
	sd->state.attack_continue=0;

	return 0;
}


int pc_checkbaselevelup(struct map_session_data *sd)
{
	int next = pc_nextbaseexp(sd);

	if(sd->status.base_exp >= next && next > 0){
		// base側レベルアップ処理
		sd->status.base_exp -= next;

		sd->status.base_level ++;
		clif_updatestatus(sd,SP_BASELEVEL);
		clif_updatestatus(sd,SP_NEXTBASEEXP);
		if(sd->status.base_level>=99)
			sd->status.status_point += 22;
		else
			sd->status.status_point += (sd->status.base_level+14) / 5 ;
		clif_updatestatus(sd,SP_STATUSPOINT);
		pc_calcstatus(sd,0);
		pc_heal(sd,sd->status.max_hp,sd->status.max_sp);

		clif_misceffect(&sd->bl,0);
		return 1;
	}

	return 0;
}


int pc_checkjoblevelup(struct map_session_data *sd)
{
	int next = pc_nextjobexp(sd);

	if(sd->status.job_exp >= next && next > 0){
		// job側レベルアップ処理
		sd->status.job_exp -= next;
		sd->status.job_level ++;
		clif_updatestatus(sd,SP_JOBLEVEL);
		clif_updatestatus(sd,SP_NEXTJOBEXP);
		sd->status.skill_point ++;
		clif_updatestatus(sd,SP_SKILLPOINT);
		pc_calcstatus(sd,0);

		clif_misceffect(&sd->bl,1);
		return 1;
	}

	return 0;
}


/*==========================================
 * 経 ?値取得
 *------------------------------------------
 */
int pc_gainexp(struct map_session_data *sd,int base_exp,int job_exp)
{
	if(sd->bl.prev == NULL || pc_isdead(sd))
		return 0;

	if(sd->status.guild_id>0){	// ギルドに上納
		base_exp-=guild_payexp(sd,base_exp);
		if(base_exp < 0)
			base_exp = 0;
	}

	sd->status.base_exp += base_exp;
	if(sd->status.base_exp < 0)
		sd->status.base_exp = 0;
	//printf("base %d/%d ",sd->status.base_exp,pc_nextbaseexp(sd));
	while(pc_checkbaselevelup(sd)) ;

	clif_updatestatus(sd,SP_BASEEXP);

	sd->status.job_exp += job_exp;
	if(sd->status.job_exp < 0)
		sd->status.job_exp = 0;
	//printf("job %d/%d\n",sd->status.job_exp,pc_nextjobexp(sd));
	while(pc_checkjoblevelup(sd)) ;

	clif_updatestatus(sd,SP_JOBEXP);

	return 0;
}


/*==========================================
 * base level側必要経験値計算
 *------------------------------------------
 */
int pc_nextbaseexp(struct map_session_data *sd)
{
	int i;

	if(sd->status.base_level>=MAX_LEVEL || sd->status.base_level<=0)
		return 0;

	if(sd->status.class==0) i=0;
	else if(sd->status.class<=6) i=1;
	else i=2;

	return exp_table[i][sd->status.base_level-1];
}


/*==========================================
 * job level側必要経験値計算
 *------------------------------------------
 */
int pc_nextjobexp(struct map_session_data *sd)
{
	int i;

	if(sd->status.job_level>=MAX_LEVEL || sd->status.job_level<=0)
		return 0;

	if(sd->status.class==0) i=3;
	else if(sd->status.class<=6) i=4;
	else i=5;

	return exp_table[i][sd->status.job_level-1];
}


/*==========================================
 * 必要ステータスポイント計算
 *------------------------------------------
 */
int pc_need_status_point(struct map_session_data *sd,int type)
{
	int val;

	if(type<SP_STR || type>SP_LUK)
		return -1;
	val =
		type==SP_STR ? sd->status.str :
		type==SP_AGI ? sd->status.agi :
		type==SP_VIT ? sd->status.vit :
		type==SP_INT ? sd->status.int_:
		type==SP_DEX ? sd->status.dex : sd->status.luk;

	return (val+9)/10+1;
}


/*==========================================
 * 能力値成長
 *------------------------------------------
 */
int pc_statusup(struct map_session_data *sd,int type)
{
	int need,val;

	need=pc_need_status_point(sd,type);
	if(type<SP_STR || type>SP_LUK || need<0 || need>sd->status.status_point){
		clif_statusupack(sd,type,0,0);
		return 1;
	}
	sd->status.status_point-=need;
	switch(type){
	case SP_STR:
		val= ++sd->status.str;
		break;
	case SP_AGI:
		val= ++sd->status.agi;
		break;
	case SP_VIT:
		val= ++sd->status.vit;
		break;
	case SP_INT:
		val= ++sd->status.int_;
		break;
	case SP_DEX:
		val= ++sd->status.dex;
		break;
	case SP_LUK:
		val= ++sd->status.luk;
		break;
	}
	if(need!=pc_need_status_point(sd,type)){
		clif_updatestatus(sd,type-SP_STR+SP_USTR);
	}
	clif_updatestatus(sd,SP_STATUSPOINT);
	clif_updatestatus(sd,type);
	pc_calcstatus(sd,0);
	clif_statusupack(sd,type,1,val);

	return 0;
}


/*==========================================
 * スキルポイント割り振り
 *------------------------------------------
 */
int pc_skillup(struct map_session_data *sd,int skill_num)
{
	if( skill_num>=10000 ){
		guild_skillup(sd,skill_num);
		return 0;
	}

	if( sd->status.skill_point>0 &&
		sd->status.skill[skill_num].id!=0 &&
		sd->status.skill[skill_num].lv < skill_get_max(skill_num) )
	{
		sd->status.skill[skill_num].lv++;
		sd->status.skill_point--;
		pc_calcstatus(sd,0);
		clif_skillup(sd,skill_num);
		clif_updatestatus(sd,SP_SKILLPOINT);
		clif_skillinfoblock(sd);
	}

	return 0;
}


/*==========================================
 * /resetstate
 *------------------------------------------
 */
int pc_resetstate(struct map_session_data* sd)
{
	#define sumsp(a) ((a)*((a-2)/10+2) - 5*((a-2)/10)*((a-2)/10) - 6*((a-2)/10) -2)
	int add=0;

	add += sumsp(sd->status.str);
	add += sumsp(sd->status.agi);
	add += sumsp(sd->status.vit);
	add += sumsp(sd->status.int_);
	add += sumsp(sd->status.dex);
	add += sumsp(sd->status.luk);
	sd->status.status_point+=add;

	clif_updatestatus(sd,SP_STATUSPOINT);

	sd->status.str=1;
	sd->status.agi=1;
	sd->status.vit=1;
	sd->status.int_=1;
	sd->status.dex=1;
	sd->status.luk=1;

	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);

	pc_calcstatus(sd,0);

	return 0;
}


/*==========================================
 * /resetskill 
 *------------------------------------------
 */
int pc_resetskill(struct map_session_data* sd)
{
	int  i,skill;
	for(i=1;i<MAX_SKILL;i++){
		if( (skill = pc_checkskill(sd,i)) > 0) {
			if(!(skill_get_inf2(i)&0x01) || battle_config.quest_skill_learn == 1) {
				if(!sd->status.skill[i].flag)
					sd->status.skill_point += skill;
				else if(sd->status.skill[i].flag > 2) {
					sd->status.skill_point += (sd->status.skill[i].flag - 2);
				}
				sd->status.skill[i].lv = 0;
			}
			else if(battle_config.quest_skill_reset == 1)
				sd->status.skill[i].lv = 0;
			sd->status.skill[i].flag = 0;
		}
	}
	clif_updatestatus(sd,SP_SKILLPOINT);
	clif_skillinfoblock(sd);  
	pc_calcstatus(sd,0);

	return 0;
}


/*==========================================
 * pcにダメージを与える
 *------------------------------------------
 */
int pc_damage(struct block_list *src,struct map_session_data *sd,int damage)
{
	// 既 に死んでいたら無効
	if(pc_isdead(sd))
		return 0;
	// 座ってたら立ち上がる
	if(pc_issit(sd))
		pc_setstand(sd);
	// 歩 いていたら足を止める
	if(sd->sc_data[SC_ENDURE].timer == -1 && !pc_check_equip_card(sd,4123))
		pc_stop_walking(sd,3);

	sd->status.hp-=damage;
	if(sd->status.hp>0){
		// まだ生きているならHP更新
		clif_updatestatus(sd,SP_HP);
		
		if(sd->status.hp<sd->status.max_hp>>2 && pc_checkskill(sd,SM_AUTOBERSERK)>0 &&
			(sd->sc_data[SC_PROVOKE].timer==-1 || sd->sc_data[SC_PROVOKE].val2==0 ))
			// オートバーサーク発動
			skill_status_change_start(&sd->bl,SC_PROVOKE,10,1);

		return 0;
	}
	sd->status.hp = 0;
	pc_setdead(sd);
	if(sd->vender_id)
		vending_closevending(sd);
	if(sd->status.pet_id && sd->pet_npcdata) {
		if(sd->petDB) {
			sd->pet.intimate -= sd->petDB->die;
			if(sd->pet.intimate < 0)
				sd->pet.intimate = 0;
			clif_send_petdata(sd,1,sd->pet.intimate);
		}
	}

	pc_stop_walking(sd,0);
	skill_castcancel(&sd->bl);	// 詠唱の中止
	clif_clearchar_area(&sd->bl,1);
	skill_status_change_clear(&sd->bl);	// ステータス異常を解除する
	clif_updatestatus(sd,SP_HP);
	pc_calcstatus(sd,0);

	if(battle_config.death_penalty_type&1) {
		if(sd->status.class && !map[sd->bl.m].flag.nopenalty){
			if(battle_config.death_penalty_type&2 && battle_config.death_penalty_base > 0)
				sd->status.base_exp -= (int)((double)pc_nextbaseexp(sd) * (double)battle_config.death_penalty_base/10000.);
			else if(battle_config.death_penalty_base > 0) {
				if(pc_nextbaseexp(sd) > 0)
					sd->status.base_exp -= (int)((double)sd->status.base_exp * (double)battle_config.death_penalty_base/10000.);
			}
			if(sd->status.base_exp < 0)
				sd->status.base_exp = 0;
			clif_updatestatus(sd,SP_BASEEXP);

			if(battle_config.death_penalty_type&2 && battle_config.death_penalty_job > 0)
				sd->status.job_exp -= (int)((double)pc_nextjobexp(sd) * (double)battle_config.death_penalty_job/10000.);
			else if(battle_config.death_penalty_job > 0) {
				if(pc_nextjobexp(sd) > 0)
					sd->status.job_exp -= (int)((double)sd->status.job_exp * (double)battle_config.death_penalty_job/10000.);
			}
			if(sd->status.job_exp < 0)
				sd->status.job_exp = 0;
			clif_updatestatus(sd,SP_JOBEXP);
		}
	}

	// pvp
	if( map[sd->bl.m].flag.pvp){
		sd->pvp_point-=5;
		if(src && src->type==BL_PC )
			((struct map_session_data *)src)->pvp_point++;
	}

	return 0;
}


//
// script関 連
//
/*==========================================
 * script用PCステータス読み出し
 *------------------------------------------
 */
int pc_readparam(struct map_session_data *sd,int type)
{
	int val=0;
	switch(type){
	case SP_SKILLPOINT:
		val= sd->status.skill_point;
		break;
	case SP_STATUSPOINT:
		val= sd->status.status_point;
		break;
	case SP_ZENY:
		val= sd->status.zeny;
		break;
	case SP_BASELEVEL:
		val= sd->status.base_level;
		break;
	case SP_JOBLEVEL:
		val= sd->status.job_level;
		break;
	case SP_CLASS:
		val= sd->status.class;
		break;
	case SP_SEX:
		val= sd->sex;
		break;
	case SP_WEIGHT:
		val= sd->weight;
		break;
	case SP_MAXWEIGHT:
		val= sd->max_weight;
		break;
	}

	return val;
}


/*==========================================
 * script用PCステータス設定
 *------------------------------------------
 */
int pc_setparam(struct map_session_data *sd,int type,int val)
{
	switch(type){
	case SP_SKILLPOINT:
		sd->status.skill_point = val;
		break;
	case SP_STATUSPOINT:
		sd->status.status_point = val;
		break;
	case SP_ZENY:
		sd->status.zeny = val;
		break;
		
	}
	clif_updatestatus(sd,type);

	return 0;
}


/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int pc_heal(struct map_session_data *sd,int hp,int sp)
{
//	printf("heal %d %d\n",hp,sp);
	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}
	if(hp+sd->status.hp>sd->status.max_hp)
		hp=sd->status.max_hp-sd->status.hp;
	if(sp+sd->status.sp>sd->status.max_sp)
		sp=sd->status.max_sp-sd->status.sp;
	sd->status.hp+=hp;
	if(sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL,sd,1);
		hp = 0;
	}
	sd->status.sp+=sp;
	if(sd->status.sp <= 0)
		sd->status.sp = 0;
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return hp + sp;
}


/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int pc_itemheal(struct map_session_data *sd,int hp,int sp)
{
	int bonus;
//	printf("heal %d %d\n",hp,sp);
	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}
	if(hp > 0) {
		bonus = sd->paramc[2] + 100 + pc_checkskill(sd,4)*10;
		if(bonus != 100)
			hp = hp * bonus / 100;
		bonus = 100 + pc_checkskill(sd,227)*5;
		if(bonus != 100)
			hp = hp * bonus / 100;
	}
	if(sp > 0) {
		bonus = 100 + pc_checkskill(sd,227)*5;
		if(bonus != 100)
			sp = sp * bonus / 100;
	}
	if(hp+sd->status.hp>sd->status.max_hp)
		hp=sd->status.max_hp-sd->status.hp;
	if(sp+sd->status.sp>sd->status.max_sp)
		sp=sd->status.max_sp-sd->status.sp;
	sd->status.hp+=hp;
	if(sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL,sd,1);
		hp = 0;
	}
	sd->status.sp+=sp;
	if(sd->status.sp <= 0)
		sd->status.sp = 0;
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return 0;
}


/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int pc_percentheal(struct map_session_data *sd,int hp,int sp)
{
	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}
	if(hp) {
		if(hp >= 100) {
			sd->status.hp = sd->status.max_hp;
		}
		else if(hp <= -100) {
			sd->status.hp = 0;
			pc_damage(NULL,sd,1);
		}
		else {
			sd->status.hp += sd->status.max_hp*hp/100;
			if(sd->status.hp > sd->status.max_hp)
				sd->status.hp = sd->status.max_hp;
			if(sd->status.hp <= 0) {
				sd->status.hp = 0;
				pc_damage(NULL,sd,1);
				hp = 0;
			}
		}
	}
	if(sp) {
		if(sp >= 100) {
			sd->status.sp = sd->status.max_sp;
		}
		else if(sp <= -100) {
			sd->status.sp = 0;
		}
		else {
			sd->status.sp += sd->status.max_sp*sp/100;
			if(sd->status.sp > sd->status.max_sp)
				sd->status.sp = sd->status.max_sp;
			if(sd->status.sp < 0)
				sd->status.sp = 0;
		}
	}
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return 0;
}


/*==========================================
 * 職変更
 *------------------------------------------
 */
int pc_jobchange(struct map_session_data *sd,int job)
{
	int i;

	if((sd->status.sex == 0 && job == 19) || (sd->status.sex == 1 && job == 20))
		return 1;

	sd->status.class=job;
	clif_changelook(&sd->bl,LOOK_BASE,sd->status.class);

	sd->status.job_level=1;
	sd->status.job_exp=0;
	clif_updatestatus(sd,SP_JOBLEVEL);
	clif_updatestatus(sd,SP_JOBEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);
	pc_calcstatus(sd,0);

	for(i=0;i<MAX_INVENTORY;i++) pc_unequipitem(sd,i);	// 装備外し

	return 0;
}


/*==========================================
 * 見た目変更
 *------------------------------------------
 */
int pc_equiplookall(struct map_session_data *sd)
{
	clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
	clif_changelook(&sd->bl,LOOK_SHOES,sd->status.shoes);
	clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
	clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
	clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);

	return 0;
}


/*==========================================
 * 見た目変更
 *------------------------------------------
 */
int pc_changelook(struct map_session_data *sd,int type,int val)
{
	switch(type){
	case LOOK_HAIR:
		sd->status.hair=val;
		break;
	case LOOK_WEAPON:
		sd->status.weapon=val;
		break;
	case LOOK_HEAD_BOTTOM:
		sd->status.head_bottom=val;
		break;
	case LOOK_HEAD_TOP:
		sd->status.head_top=val;
		break;
	case LOOK_HEAD_MID:
		sd->status.head_mid=val;
		break;
	case LOOK_HAIR_COLOR:
		sd->status.hair_color=val;
		break;
	case LOOK_CLOTHES_COLOR:
		sd->status.clothes_color=val;
		break;
	case LOOK_SHIELD:
		sd->status.shield=val;
		break;
	case LOOK_SHOES:
		sd->status.shoes=val;
		break;
	}
	clif_changelook(&sd->bl,type,val);

	return 0;
}


/*==========================================
 * 付属品(鷹,ペコ,カート)設定
 *------------------------------------------
 */
int pc_setoption(struct map_session_data *sd,int type)
{
	sd->status.option=type;
	clif_changeoption(&sd->bl);
	pc_calcstatus(sd,0);

	return 0;
}


/*==========================================
 * カート設定
 *------------------------------------------
 */
int pc_setcart(struct map_session_data *sd,int type)
{
	int cart[6]={0x0000,0x0008,0x0080,0x0100,0x0200,0x0400};

	if(pc_checkskill(sd,MC_PUSHCART)>0){ // プッシュカートスキル所持
		if(!pc_iscarton(sd)){ // カートを付けていない
			pc_setoption(sd,cart[type]);
			clif_cart_itemlist(sd);
			clif_cart_equiplist(sd);
			clif_updatestatus(sd,SP_CARTINFO);
			clif_status_change(&sd->bl,0x0c,0);
		}else{
			pc_setoption(sd,cart[type]);
		}
	}

	return 0;
}


/*==========================================
 * 鷹設定
 *------------------------------------------
 */
int pc_setfalcon(struct map_session_data *sd)
{
	if(pc_checkskill(sd,HT_FALCON)>0){	// ファルコンマスタリースキル所持
		pc_setoption(sd,0x0010);
	}

	return 0;
}


/*==========================================
 * ペコペコ設定
 *------------------------------------------
 */
int pc_setriding(struct map_session_data *sd)
{
	if(pc_checkskill(sd,KN_RIDING)>0){ // ライディングスキル所持
		pc_setoption(sd,0x0020);
	}

	return 0;
}


/*==========================================
 * script用変数の値を読む
 *------------------------------------------
 */
int pc_readreg(struct map_session_data *sd,int reg)
{
	int i;

	for(i=0;i<sd->reg_num;i++)
		if(sd->reg[i].index==reg)
			return sd->reg[i].data;

	return 0;
}


/*==========================================
 * script用変数の値を設定
 *------------------------------------------
 */
int pc_setreg(struct map_session_data *sd,int reg,int val)
{
	int i;

	for(i=0;i<sd->reg_num;i++)
		if(sd->reg[i].index==reg){
			sd->reg[i].data = val;
			return 0;
		}
	sd->reg_num++;
	sd->reg=realloc(sd->reg,sizeof(sd->reg[0])*sd->reg_num);
	if(sd->reg==NULL){
		printf("out of memory : pc_setreg\n");
		exit(1);
	}
	sd->reg[i].index=reg;
	sd->reg[i].data=val;

	return 0;
}


/*==========================================
 * script用グローバル変数の値を読む
 *------------------------------------------
 */
int pc_readglobalreg(struct map_session_data *sd,char *reg)
{
	int i;

	for(i=0;i<sd->status.global_reg_num;i++){
		if(strcmp(sd->status.global_reg[i].str,reg)==0)
			return sd->status.global_reg[i].value;
	}

	return 0;
}


/*==========================================
 * script用グローバル変数の値を設定
 *------------------------------------------
 */
int pc_setglobalreg(struct map_session_data *sd,char *reg,int val)
{
	int i;

	if(val==0){
		for(i=0;i<sd->status.global_reg_num;i++){
			if(strcmp(sd->status.global_reg[i].str,reg)==0){
				sd->status.global_reg[i]=sd->status.global_reg[sd->status.global_reg_num-1];
				sd->status.global_reg_num--;
				break;
			}
		}
		return 0;
	}
	for(i=0;i<sd->status.global_reg_num;i++){
		if(strcmp(sd->status.global_reg[i].str,reg)==0){
			sd->status.global_reg[i].value=val;
			return 0;
		}
	}
	if(sd->status.global_reg_num<GLOBAL_REG_NUM){
		strcpy(sd->status.global_reg[i].str,reg);
		sd->status.global_reg[i].value=val;
		sd->status.global_reg_num++;
		return 0;
	}
	printf("pc_setglobalreg : couldn't set %s (GLOBAL_REG_NUM = %d)\n", reg, GLOBAL_REG_NUM);

	return 1;
}


/*==========================================
 * 精錬成功率
 *------------------------------------------
 */
int pc_percentrefinery(struct map_session_data *sd,struct item *item)
{
	int percent=percentrefinery[itemdb_wlv(item->nameid)][(int)item->refine];
	if(pc_checkskill(sd,BS_WEAPONRESEARCH)>0)	// 武器研究スキル所持
		if(percent<100)
			percent+=10;

	return percent;
}


/*==========================================
 * 装 備品インデックス
 *------------------------------------------
 */
int pc_equipitemindex(struct map_session_data *sd,int pos)
{
	int i;
	for(i=0;i<MAX_INVENTORY;i++)
		if( sd->status.inventory[i].nameid && (sd->status.inventory[i].equip&pos) )
			break;

	return i;
}


/*==========================================
 * イベントタイマー処理
 *------------------------------------------
 */
int pc_eventtimer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	int i;
	if(sd==NULL)
		return 0;
	
	for(i=0;i<MAX_EVENTTIMER;i++){
		if( sd->eventtimer[i]==tid ){
			sd->eventtimer[i]=-1;
			npc_event(sd,(const char *)data);
			break;
		}
	}
	free((void *)data);
	if(i==MAX_EVENTTIMER)
		printf("pc_eventtimer: no such event timer\n");

	return 0;
}


/*==========================================
 * イベントタイマー追加
 *------------------------------------------
 */
int pc_addeventtimer(struct map_session_data *sd,int tick,const char *name)
{
	int i;
	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]==-1 )
			break;
	if(i<MAX_EVENTTIMER){
		char *evname=malloc(24);
		if(evname==NULL){
			printf("pc_addeventtimer: out of memory !\n");exit(1);
		}
		memcpy(evname,name,24);
		sd->eventtimer[i]=add_timer(gettick()+tick,
			pc_eventtimer,sd->bl.id,(int)evname);
	}else
		printf("pc_addtimer: event timer is full !\n");

	return 0;
}


/*==========================================
 * イベントタイマー削除
 *------------------------------------------
 */
int pc_deleventtimer(struct map_session_data *sd,const char *name)
{
	int i;
	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 && strcmp(
			(char *)(get_timer(sd->eventtimer[i])->data), name)==0 ){
				delete_timer(sd->eventtimer[i],pc_eventtimer);
				sd->eventtimer[i]=-1;
				break;
		}

	return 0;
}


/*==========================================
 * イベントタイマーカウント値追加
 *------------------------------------------
 */
int pc_addeventtimercount(struct map_session_data *sd,const char *name,int tick)
{
	int i;
	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 && strcmp(
			(char *)(get_timer(sd->eventtimer[i])->data), name)==0 ){
				addtick_timer(sd->eventtimer[i],tick);
				break;
		}

	return 0;
}


/*==========================================
 * イベントタイマー全削除
 *------------------------------------------
 */
int pc_cleareventtimer(struct map_session_data *sd)
{
	int i;
	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 ){
			delete_timer(sd->eventtimer[i],pc_eventtimer);
			sd->eventtimer[i]=-1;
		}

	return 0;
}


//
// 装 備物
//
/*==========================================
 * アイテムを装備する
 *------------------------------------------
 */
int pc_equipitem(struct map_session_data *sd,int n,int pos)
{
	int i,nameid;
	struct item_data *id;
	nameid=sd->status.inventory[n].nameid;
	id=itemdb_search(nameid);
	printf("equip %d %x:%x\n",n,itemdb_equip(nameid),pos);
	if((itemdb_equip(nameid)&pos)==0
	 || (id->sex!=2 && sd->sex!=id->sex)
	 || sd->status.base_level<id->elv
	 || ((1<<sd->status.class)&id->class)==0
	  ){
		clif_equipitemack(sd,n,0,0);	// fail
		return 0;
	}
	if(pos==0x88){ // アクセサリ用例外処理
		int epor=0;
		for(i=0;i<100;i++){
			if(sd->status.inventory[i].nameid &&
			   sd->status.inventory[i].equip)
				epor |= sd->status.inventory[i].equip;
		}
		epor &= 0x88;
		pos = epor == 0x08 ? 0x80 : 0x08;
	}

	// 二刀流処理
	if ((pos==0x22) // 一応、装備要求箇所が二刀流武器かチェックする
	 &&	(id->equip==2)	// 単 手武器
	 &&	(sd->status.class == 12) // Assassin
	 &&	(pc_checkskill(sd, AS_LEFT) != -1) // 左手修錬有
	){
		int tpos=0;
		for(i=0;i<100;i++){
			if(sd->status.inventory[i].nameid &&
			   sd->status.inventory[i].equip)
				tpos |= sd->status.inventory[i].equip;
		}
		tpos &= 0x02;
		pos = tpos == 0x02 ? 0x20 : 0x02;
	}
	
	int arrow=pc_search_inventory(sd,pc_checkequip(sd,0x8000));
	for(i=0;i<100;i++){
		if(sd->status.inventory[i].nameid &&
		   (sd->status.inventory[i].equip&pos)){
			pc_unequipitem(sd,i);
		}
	}
	// 弓矢装備
	if(pos==0x8000){
		clif_arrowequip(sd,n);
		clif_arrow_fail(sd,3);	// 3=矢が装備できました
	}
	else
		clif_equipitemack(sd,n,pos,1);

	sd->status.inventory[n].equip=pos;
	pc_checkallowskill(sd,nameid);	// 装備品でスキルか解除されるかチェック
	if (itemdb_look(sd->status.inventory[n].nameid) == 11 && arrow){
		clif_arrowequip(sd,arrow);
		sd->status.inventory[arrow].equip=32768;
	}

	pc_calcstatus(sd,0);

	return 0;
}


/*==========================================
 * 装 備した物を外す
 *------------------------------------------
 */
int pc_unequipitem(struct map_session_data *sd,int n)
{
	int arrow=pc_search_inventory(sd,pc_checkequip(sd,0x8000));
	//printf("unequip %d %x:%x\n",n,itemdb_equippoint(sd->status.inventory[n].nameid),sd->status.inventory[n].equip);
	if(sd->status.inventory[n].equip){
		clif_unequipitemack(sd,n,sd->status.inventory[n].equip,1);
		sd->status.inventory[n].equip=0;
		clif_unequipitemack(sd,arrow,sd->status.inventory[arrow].equip,1);	// 矢も外す
		sd->status.inventory[arrow].equip=0;
	} else {
		clif_unequipitemack(sd,n,0,0);
	}
	pc_calcstatus(sd,0);

	return 0;
}


/*==========================================
 * アイテムのindex番号を詰めたり
 * 装 備品の装備可能チェックを行なう
 *------------------------------------------
 */
int pc_checkitem(struct map_session_data *sd)
{
	int i,j;

	// 所持品空き詰め
	for(i=j=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==0)
			continue;
		if(i>j){
			memcpy(&sd->status.inventory[j],&sd->status.inventory[i],sizeof(struct item));
			sd->status.inventory[i].nameid=0;
		}
		j++;
	}
	// カート内空き詰め
	for(i=j=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid==0)
			continue;
		if(i>j){
			memcpy(&sd->status.cart[j],&sd->status.cart[i],sizeof(struct item));
			sd->status.cart[i].nameid=0;
		}
		j++;
	}
	// 装 備位置チェック
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==0)
			continue;
		if(sd->status.inventory[i].equip & ~itemdb_equippoint(sd,sd->status.inventory[i].nameid))
			sd->status.inventory[i].equip=0;

	}

	return 0;
}


int pc_checkoverhp(struct map_session_data *sd)
{
	if(sd->status.hp == sd->status.max_hp)
		return 1;
	if(sd->status.hp > sd->status.max_hp) {
		sd->status.hp = sd->status.max_hp;
		clif_updatestatus(sd,SP_HP);
		return 2;
	}

	return 0;
}

int pc_checkoversp(struct map_session_data *sd)
{
	if(sd->status.sp == sd->status.max_sp)
		return 1;
	if(sd->status.sp > sd->status.max_sp) {
		sd->status.sp = sd->status.max_sp;
		clif_updatestatus(sd,SP_SP);
		return 2;
	}

	return 0;
}


/*==========================================
 * PVP順位計算用(foreachinarea)
 *------------------------------------------
 */
int pc_calc_pvprank_sub(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd1=(struct map_session_data *)bl,*sd2=NULL;
	sd2=va_arg(ap,struct map_session_data *);
	if( sd1->pvp_point > sd2->pvp_point )
		sd2->pvp_rank++;
	return 0;
}
/*==========================================
 * PVP順位計算
 *------------------------------------------
 */
int pc_calc_pvprank(struct map_session_data *sd)
{
	int old=sd->pvp_rank;
	struct map_data *m=&map[sd->bl.m];
	if( !(m->flag.pvp) )
		return 0;
	sd->pvp_rank=1;
	map_foreachinarea(pc_calc_pvprank_sub,sd->bl.m,0,0,m->xs,m->ys,BL_PC,sd);
	if(old!=sd->pvp_rank || sd->pvp_lastusers!=m->users)
		clif_pvpset(sd,sd->pvp_rank,sd->pvp_lastusers=m->users,0);
	return sd->pvp_rank;
}
/*==========================================
 * PVP順位計算(timer)
 *------------------------------------------
 */
int pc_calc_pvprank_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	if(sd==NULL)
		return 0;
	sd->pvp_timer=-1;
	if( pc_calc_pvprank(sd)>0 )
		sd->pvp_timer=add_timer(
			gettick()+PVP_CALCRANK_INTERVAL,
			pc_calc_pvprank_timer,id,data);
	return 0;
}

//
// 自然回復物
//
/*==========================================
 * SP回復量計算
 *------------------------------------------
 */
static int natural_heal_tick,natural_heal_prev_tick,natural_heal_diff_tick;
static int pc_spheal(struct map_session_data *sd)
{
	int a;

	a = natural_heal_diff_tick;
	if(pc_issit(sd)) a += a;
	if( sd->sc_data[SC_MAGNIFICAT].timer!=-1 )	// マグニフィカート
		a += a;

	return a;
}

/*==========================================
 * HP回復量計算
 *------------------------------------------
 */
static int pc_hpheal(struct map_session_data *sd)
{
	int a;

	a = natural_heal_diff_tick;
	if(pc_issit(sd)) a += a;
	if( sd->sc_data[SC_MAGNIFICAT].timer!=-1 )	// RoVeRT
		a += a;

	return a;
}

static int pc_natural_heal_hp(struct map_session_data *sd)
{
	int bhp;
	int inc_num,bonus,skill,hp_flag;

	if (sd->sc_data[SC_TRICKDEAD].timer != -1)		// RoVeRT
		return 0;

	if(pc_checkoverhp(sd)) {
		sd->hp_sub = sd->inchealhptick = 0;
		return 0;
	}

	bhp=sd->status.hp;
	hp_flag = (pc_checkskill(sd,SM_MOVINGRECOVERY) > 0 && sd->walktimer != -1);

	if(sd->walktimer == -1) {
		inc_num = pc_hpheal(sd);
		sd->hp_sub += inc_num;
		sd->inchealhptick += inc_num;
	}
	else if(hp_flag) {
		inc_num = pc_hpheal(sd);
		sd->hp_sub += inc_num;
		sd->inchealhptick = 0;
	}
	else {
		sd->hp_sub = sd->inchealhptick = 0;
		return 0;
	}

	if(sd->hp_sub >= 4000) {
		bonus = 1 + (sd->paramc[2]/6) + (sd->status.max_hp/200);
		if(hp_flag) {
			bonus /= 4;
			if(bonus <= 0) bonus = 1;
		}
		while(sd->hp_sub >= 4000) {
			sd->hp_sub -= 4000;
			if(sd->status.hp + bonus <= sd->status.max_hp)
				sd->status.hp += bonus;
			else {
				sd->status.hp = sd->status.max_hp;
				sd->hp_sub = sd->inchealhptick = 0;
			}
		}
	}
	if(bhp!=sd->status.hp)
		clif_updatestatus(sd,SP_HP);

	if((skill=pc_checkskill(sd,SM_RECOVERY)) > 0) {
		if(sd->inchealhptick >= 10000 && sd->status.hp < sd->status.max_hp) {
			bonus = skill*5 + (sd->status.max_hp*skill/500);
			while(sd->inchealhptick >= 10000) {
				sd->inchealhptick -= 10000;
				if(sd->status.hp + bonus <= sd->status.max_hp)
					sd->status.hp += bonus;
				else {
					bonus = sd->status.max_hp - sd->status.hp;
					sd->status.hp = sd->status.max_hp;
					sd->hp_sub = sd->inchealhptick = 0;
				}
				clif_heal(sd->fd,SP_HP,bonus);
			}
		}
	}
	else sd->inchealhptick = 0;

	return 0;

	if(sd->sc_data[SC_APPLEIDUN].timer!=-1) { // Apple of Idun
		if(sd->inchealhptick >= 6000 && sd->status.hp < sd->status.max_hp) {
			bonus = skill*20;
			while(sd->inchealhptick >= 6000) {
				sd->inchealhptick -= 6000;
				if(sd->status.hp + bonus <= sd->status.max_hp)
					sd->status.hp += bonus;
				else {
					bonus = sd->status.max_hp - sd->status.hp;
					sd->status.hp = sd->status.max_hp;
					sd->hp_sub = sd->inchealhptick = 0;
				}
				clif_heal(sd->fd,SP_HP,bonus);
			}
		}
	}
	else sd->inchealhptick = 0;

	return 0;
}

static int pc_natural_heal_sp(struct map_session_data *sd)
{
	int bsp;
	int inc_num,bonus,skill;

	if (sd->sc_data[SC_TRICKDEAD].timer != -1)		// RoVeRT
		return 0;

	if(pc_checkoversp(sd)) {
		sd->sp_sub = sd->inchealsptick = 0;
		return 0;
	}

	bsp=sd->status.sp;

	inc_num = pc_spheal(sd);
	if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer == -1)
		sd->sp_sub += inc_num;
	if(sd->walktimer == -1)
		sd->inchealsptick += inc_num;
	else sd->inchealsptick = 0;

	if(sd->sp_sub >= 8000){
		bonus = 1 + (sd->paramc[3]/6) + (sd->status.max_sp/100);
		while(sd->sp_sub >= 8000){
			sd->sp_sub -= 8000;
			if(sd->status.sp + bonus <= sd->status.max_sp)
				sd->status.sp += bonus;
			else {
				sd->status.sp = sd->status.max_sp;
				sd->sp_sub = sd->inchealsptick = 0;
			}
		}
	}

	if(bsp != sd->status.sp)
		clif_updatestatus(sd,SP_SP);

	if((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0) {
		if(sd->inchealsptick >= 10000 && sd->status.sp < sd->status.max_sp) {
			bonus = skill*3 + (sd->status.max_sp*skill/500);
			while(sd->inchealsptick >= 10000) {
				sd->inchealsptick -= 10000;
				if(sd->status.sp + bonus <= sd->status.max_sp)
					sd->status.sp += bonus;
				else {
					bonus = sd->status.max_sp - sd->status.sp;
					sd->status.sp = sd->status.max_sp;
					sd->sp_sub = sd->inchealsptick = 0;
				}
				clif_heal(sd->fd,SP_SP,bonus);
			}
		}
	}
	else sd->inchealsptick = 0;

	return 0;
}

static int pc_spirit_heal(struct map_session_data *sd,int level)
{
	int bonus_hp,bonus_sp,flag,interval = 10000;

	if(pc_checkoverhp(sd) && pc_checkoversp(sd)) {
		sd->inchealspirittick = 0;
		return 0;
	}

	sd->inchealspirittick += natural_heal_diff_tick;

	if(pc_is50overweight(sd))
		interval <<= 1;

	if(sd->inchealspirittick >= interval) {
		bonus_hp = level*4 + (sd->status.max_hp*level/500);
		bonus_sp = level*2 + (sd->status.max_sp*level/500);
		flag = 0;
		while(sd->inchealspirittick >= interval) {
			sd->inchealspirittick -= interval;
			if(sd->status.hp < sd->status.max_hp) {
				if(sd->status.hp + bonus_hp <= sd->status.max_hp)
					sd->status.hp += bonus_hp;
				else {
					bonus_hp = sd->status.max_hp - sd->status.hp;
					sd->status.hp = sd->status.max_hp;
					flag |= 0x01;
				}
				clif_heal(sd->fd,SP_HP,bonus_hp);
			}
			else
				flag |= 0x01;
			if(sd->status.sp < sd->status.max_sp) {
				if(sd->status.sp + bonus_sp <= sd->status.max_sp)
					sd->status.sp += bonus_sp;
				else {
					bonus_sp = sd->status.max_sp - sd->status.sp;
					sd->status.sp = sd->status.max_sp;
					flag |= 0x02;
				}
				clif_heal(sd->fd,SP_SP,bonus_sp);
			}
			else
				flag |= 0x02;
			if(flag >= 3)
				sd->inchealspirittick = 0;
		}
	}

	return 0;
}

/*==========================================
 * HP/SP 自然回復 各クライアント
 *------------------------------------------
 */

static int pc_natural_heal_sub(struct map_session_data *sd,va_list ap)
{
	int skill;
	if(!pc_is50overweight(sd) && !pc_isdead(sd) && !pc_ishiding(sd) ) {
		pc_natural_heal_hp(sd);
		pc_natural_heal_sp(sd);
	}
	else {
		sd->hp_sub = sd->inchealhptick = 0;
		sd->sp_sub = sd->inchealsptick = 0;
	}
	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0 && pc_issit(sd))
		pc_spirit_heal(sd,skill);
	else
		sd->inchealspirittick = 0;

	return 0;
}

/*==========================================
 * HP/SP自然回復 (interval timer関数)
 *------------------------------------------
 */
int pc_natural_heal(int tid,unsigned int tick,int id,int data)
{
	natural_heal_tick = tick;
	natural_heal_diff_tick = DIFF_TICK(natural_heal_tick,natural_heal_prev_tick);
	clif_foreachclient(pc_natural_heal_sub);

	natural_heal_prev_tick = tick;
	return 0;
}

/*==========================================
 * セーブポイントの保存
 *------------------------------------------
 */
int pc_setsavepoint(struct map_session_data *sd,char *mapname,int x,int y)
{
	strncpy(sd->status.save_point.map,mapname,16);
	sd->status.save_point.x = x;
	sd->status.save_point.y = y;

	return 0;
}


/*==========================================
 * 自動セーブ 各クライアント
 *------------------------------------------
 */
static int last_save_fd,save_flag;
static int pc_autosave_sub(struct map_session_data *sd,va_list ap)
{
	if(save_flag==0 && sd->fd>last_save_fd){
		//printf("autosave %d\n",sd->fd);
		chrif_save(sd);
		// pet
		if(sd->status.pet_id && sd->pet_npcdata)
			intif_save_petdata(sd->status.account_id,&sd->pet);
		save_flag=1;
		last_save_fd = sd->fd;
	}

	return 0;
}


/*==========================================
 * 自動セーブ (timer関数)
 *------------------------------------------
 */
int pc_autosave(int tid,unsigned int tick,int id,int data)
{
	int interval;

	save_flag=0;
	clif_foreachclient(pc_autosave_sub);
	if(save_flag==0)
		last_save_fd=0;

	interval = autosave_interval/(clif_countusers()+1);
	if(interval <= 0)
		interval = 1;
	add_timer(gettick()+interval,pc_autosave,0,0);

	return 0;
}


int pc_read_gm_account()
{
	char line[8192];
	struct gm_account *p;
	FILE *fp;
	int c=0;

	gm_account_db=numdb_init();

	if( (fp=fopen(GM_account_filename,"r"))==NULL )
		return 1;
	while(fgets(line,sizeof(line),fp)){
		if(line[0] == '/' && line[1] == '/')
			continue;
		p=malloc(sizeof(struct gm_account));
		if(p==NULL){
			printf("gm_account: out of memory!\n");
			exit(0);
		}
		if(sscanf(line,"%d %d",&p->account_id,&p->level) != 2 || p->level <= 0) {
			printf("gm_account: broken data [%s] line %d\n",GM_account_filename,c);
		}
		else {
			if(p->level > 99)
				p->level = 99;
			numdb_insert(gm_account_db,p->account_id,p);
		}
		c++;
	}
	fclose(fp);
//	printf("gm_account: %s read done (%d gm account ID)\n",gm_account_txt,c);

	return 0;
}


//
// 初期化物
//
/*==========================================
 * 設定ファイル読み込む
 * exp.txt 必要経験値
 * job_db1.txt 重量,hp,sp,攻撃速度
 * job_db2.txt job能力値ボーナス
 * skill_tree.txt 各職毎のスキルツリー
 * attr_fix.txt 属性修正テーブル
 * size_fix.txt サイズ補正テーブル
 * refine_db.txt 精錬データテーブル
 *------------------------------------------
 */
int pc_readdb(void)
{
	int i,j,k;
	FILE *fp;
	char line[1024],*p;

	// 必要経験値読み込み

	fp=fopen("db/exp.txt","r");
	if(fp==NULL){
		printf("can't read db/exp.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		int bn,b1,b2,jn,j1,j2;
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(sscanf(line,"%d,%d,%d,%d,%d,%d",&bn,&b1,&b2,&jn,&j1,&j2)!=6)
			continue;
		exp_table[0][i]=bn;
		exp_table[1][i]=b1;
		exp_table[2][i]=b2;
		exp_table[3][i]=jn;
		exp_table[4][i]=j1;
		exp_table[5][i]=j2;
		i++;
		if(i >= MAX_LEVEL)
			break;
	}
	fclose(fp);
	printf("read db/exp.txt done\n");

	// JOB補正数値１
	fp=fopen("db/job_db1.txt","r");
	if(fp==NULL){
		printf("can't read db/job_db1.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<20 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j<20)
			continue;
		max_weight_base[i]=atoi(split[0]);
		sscanf(split[1],"%lf",&hp_coefficient[i]);
		sp_coefficient[i]=atoi(split[2]);
		for(j=0;j<17;j++)
			aspd_base[i][j]=atoi(split[j+3]);
		i++;
		if(i==MAX_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db1.txt done\n");

	// JOBボーナス
	fp=fopen("db/job_db2.txt","r");
	if(fp==NULL){
		printf("can't read db/job_db2.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_bonus[i][j]=k;
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if(i==MAX_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db2.txt done\n");

	// スキルツリー
	memset(skill_tree,0,sizeof(skill_tree));
	fp=fopen("db/skill_tree.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_tree.txt\n");
		return 1;
	}
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<13 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j<13)
			continue;
		i=atoi(split[0]);
		for(j=0;skill_tree[i][j].id;j++);
		skill_tree[i][j].id=atoi(split[1]);
		skill_tree[i][j].max=atoi(split[2]);
		for(k=0;k<5;k++){
			skill_tree[i][j].need[k].id=atoi(split[k*2+3]);
			skill_tree[i][j].need[k].lv=atoi(split[k*2+4]);
		}
	}
	fclose(fp);
	printf("read db/skill_tree.txt done\n");

	// 属性修正テーブル
	for(i=0;i<4;i++)
		for(j=0;j<10;j++)
			for(k=0;k<10;k++)
				attr_fix_table[i][j][k]=100;
	fp=fopen("db/attr_fix.txt","r");
	if(fp==NULL){
		printf("can't read db/attr_fix.txt\n");
		return 1;
	}
	while(fgets(line,1020,fp)){
		char *split[10];
		int lv,n;
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<3 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		lv=atoi(split[0]);
		n=atoi(split[1]);
//		printf("%d %d\n",lv,n);
		
		for(i=0;i<n;){
			if( !fgets(line,1024,fp) )
				break;
			if(line[0]=='/' && line[1]=='/')
				continue;
			
			for(j=0,p=line;j<n && p;j++){
				while(*p==32 && *p>0)
					p++;
				attr_fix_table[lv-1][i][j]=atoi(p);
				if(battle_config.attr_recover == 0 && attr_fix_table[lv-1][i][j] < 0)
					attr_fix_table[lv-1][i][j] = 0;
				p=strchr(p,',');
				if(p) *p++=0;
//				printf("%4d ",attr_fix_table[lv-1][i][j]);
			}
//			printf("\n");
			
			i++;
		}
	}
	printf("read db/attr_fix.txt done\n");

	// サイズ補正テーブル
	for(i=0;i<3;i++)
		for(j=0;j<20;j++)
			atkmods[i][j]=100;
	fp=fopen("db/size_fix.txt","r");
	if(fp==NULL){
		printf("can't read db/size_fix.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[20];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<20 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		for(j=0;j<20 && split[j];j++)
			atkmods[i][j]=atoi(split[j]);
		i++;
	}
	printf("read db/size_fix.txt done\n");

	// 精錬データテーブル
	for(i=0;i<5;i++){
		for(j=0;j<10;j++)
			percentrefinery[i][j]=100;
		refinebonus[i][0]=0;
		refinebonus[i][1]=0;
		refinebonus[i][2]=10;
	}
	fp=fopen("db/refine_db.txt","r");
	if(fp==NULL){
		printf("can't read db/refine_db.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[16];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<16 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		refinebonus[i][0]=atoi(split[0]);	// 精錬ボーナス
		refinebonus[i][1]=atoi(split[1]);	// 過剰精錬ボーナス
		refinebonus[i][2]=atoi(split[2]);	// 安全精錬限界
		for(j=0;j<10 && split[j];j++)
			percentrefinery[i][j]=atoi(split[j+3]);
		i++;
	}
	printf("read db/refine_db.txt done\n");

	return 0;
}


/*==========================================
 * pc関 係初期化
 *------------------------------------------
 */
int do_init_pc(void)
{
	add_timer_func_list(pc_walk,"pc_walk");
	add_timer_func_list(pc_attack_timer,"pc_attack_timer");
	add_timer_func_list(pc_natural_heal,"pc_natural_heal");
	add_timer_func_list(pc_ghost_timer,"pc_ghost_timer");
	add_timer_func_list(pc_eventtimer,"pc_eventtimer");
	add_timer_func_list(pc_calc_pvprank_timer,"pc_calc_pvprank_timer");
	add_timer_func_list(pc_autosave,"pc_autosave");
	add_timer_func_list(pc_spiritball_timer,"pc_spiritball_timer");
	add_timer_interval((natural_heal_prev_tick=gettick()+NATURAL_HEAL_INTERVAL),pc_natural_heal,0,0,NATURAL_HEAL_INTERVAL);
	add_timer(gettick()+autosave_interval,pc_autosave,0,0);
	pc_readdb();
	pc_read_gm_account();

	return 0;
}

