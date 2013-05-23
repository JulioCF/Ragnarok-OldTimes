#ifndef _MAP_H_
#define _MAP_H_

#include <stdarg.h>
#include "mmo.h"

#define MAX_PC_CLASS (1+6+6+1+6+1+1+2)
#define MAX_NPC_PER_MAP 512
#define BLOCK_SIZE 8
#define AREA_SIZE 20
#define LOCAL_REG_NUM 16
#define LIFETIME_FLOORITEM 60
#define DAMAGELOG_SIZE 16
#define LOOTITEM_SIZE 20
#define MAX_STATUSCHANGE 128
#define MAX_SKILLUNITGROUP	32
#define MAX_MOBSKILLUNITGROUP	4
#define MAX_SKILLUNITGROUPTICKSET	128
#define MAX_SKILLTIMERSKILL 32
#define MAX_MOBSKILL	24
#define MAX_EVENTQUEUE	2
#define MAX_EVENTTIMER	32
#define NATURAL_HEAL_INTERVAL 500
#define MAX_FLOORITEM 100000
#define MAX_LEVEL 99

#define DEFAULT_AUTOSAVE_INTERVAL 60*1000

#define MAP_CONF_NAME	"conf/map_athena.conf"

enum { BL_NUL, BL_PC, BL_NPC, BL_MOB, BL_ITEM, BL_CHAT, BL_SKILL , BL_PET };
enum { WARP, SHOP, SCRIPT, MONS };
struct block_list {
	struct block_list *next,*prev;
	int id;
	short m,x,y;
	unsigned char type;
	unsigned char subtype;
};

struct walkpath_data {
	unsigned char path_len,path_pos,path_half;
	unsigned char path[32];
};
struct script_reg {
	int index;
	int data;
};
struct status_change {
	int timer;
	int val1,val2,val3,val4;
};
struct vending {
	short index;
	short amount;
	int value;
};

struct skill_unit_group;
struct skill_unit {
	struct block_list bl;
	
	struct skill_unit_group *group;
	
	int limit;
	int val1,val2;
	short alive,range;
};
struct skill_unit_group {
	int src_id;
	int party_id;
	int guild_id;
	int map,range;
	int target_flag;
	unsigned int tick;
	int limit,interval;

	int skill_id,skill_lv;
	int val1,val2;
	int vallist[16];
	char *valstr;
	int unit_id;
	int group_id;
	int unit_count,alive_count;
	struct skill_unit *unit;
};
struct skill_unit_group_tickset {
	unsigned int tick;
	int group_id;
};
struct skill_timerskill {
	int timer;
	int src_id;
	int target_id;
	int map;
	short x,y;
	short skill_id,skill_lv;
	int type;
	int flag;
};


struct npc_data;
struct pet_db;

struct map_session_data {
	struct block_list bl;
	struct {
		unsigned auth : 1 ;
		unsigned change_walk_target : 1 ;
		unsigned attack_continue : 1 ;
		unsigned menu_or_input : 1;
		unsigned dead_sit : 2;
		unsigned skillcastcancel : 1;
		unsigned waitingdisconnect : 1;
	} state;
	int char_id,login_id1,login_id2,sex;
	struct mmo_charstatus status;
	int weight,max_weight;
	int cart_weight,cart_max_weight,cart_num,cart_max_num;
	char mapname[16];
	int fd,new_fd;
	short to_x,to_y;
	short speed,prev_speed;
	short opt1,opt2;
	char dir,head_dir;
	unsigned long client_tick,server_tick;
	struct walkpath_data walkpath;
	int walktimer;
	int npc_id,npc_shopid;
	int npc_pos;
	int npc_menu;
	int npc_amount;
	unsigned long chatID;

	int attacktimer;
	int attacktarget;
	unsigned int attackabletime;

	short attackrange;
	int skilltimer;
	int skilltarget;
	short skillx,skilly;
	short skillid,skilllv;
	short skillitem,skillitemlv;
	struct skill_unit_group skillunit[MAX_SKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	struct skill_timerskill skilltimerskill[MAX_SKILLTIMERSKILL];
	int ghost_timer;

	unsigned long canact_tick;
	unsigned long canmove_tick;
	int hp_sub,sp_sub;
	int inchealhptick,inchealsptick,inchealspirittick;

	int paramb[6],paramc[6],parame[6];
	int hit,flee,flee2,aspd,amotion,dmotion;
	int watk,watk2,atkmods[3],atkmodr[16],atkmoda[16];
	int def,def2,mdef,mdef2,critical,matk1,matk2;
	int atk_ele,def_ele,star,overrefine;
	int wcard[8],dcard[16],wcard_count,dcard_count;
	int castrate,hprate,sprate,dsprate;
	int addele[10],addrace[10],addsize[3],subele[10],subrace[10];
	int addeff[10],reseff[10];
	int watk_,watk_2,atkmods_[3],atkmodr_[16],atkmoda_[16];	//二刀流のために追加
	int atk_ele_,star_,overrefine_;				//二刀流のために追加
	short spiritball, spiritball_old;
	int spirit_timer[10];
	unsigned short combo_flag, skill_old;
	unsigned int combo_delay1, combo_delay2, combo_delay3, triple_delay;

	int reg_num;
	struct script_reg *reg;

	struct status_change sc_data[MAX_STATUSCHANGE];
	short sc_count;

	int trade_partner;
	int deal_item_index[10];
	int deal_item_amount[10];
	int deal_zeny;
	short deal_locked;

	int party_sended,party_invite,party_invite_account;
	int party_hp,party_x,party_y;

	int guild_sended,guild_invite,guild_invite_account;
	int guild_emblem_id,guild_alliance,guild_alliance_account;

	int vender_id;
	int vend_num;
	char message[80];
	struct vending vending[12];

	int catch_target_class;
	struct s_pet pet;
	struct pet_db *petDB;
	struct npc_data *pet_npcdata;
	int pet_hungry_timer;

	int pvp_point,pvp_rank,pvp_timer,pvp_lastusers;

	char eventqueue[MAX_EVENTQUEUE][50];
	int eventtimer[MAX_EVENTTIMER];

	int last_skillid,last_skilllv;		// RoVeRT
	int cast_skillid,cast_skilllv;		// RoVeRT
	int autospell_tick;			// RoVeRT
};

struct npc_item_list {
	int nameid,value;
};
struct npc_data {
	struct block_list bl;
	short n;
	short class,dir;
	short speed;
	char name[24];
	char exname[24];
	struct {
		unsigned state : 8 ;
		unsigned skillstate : 8 ;
		unsigned flag : 8 ;
	} state;
	int timer;
	short to_x,to_y;
	short equip;
	int chat_id;
	struct walkpath_data walkpath;

	union {
		struct {
			char *script;
			short xs,ys;
		} scr;
		struct npc_item_list shop_item[1];
		struct {
			short xs,ys;
			short x,y;
			char name[16];
		} warp;
	} u;
	// ここにメンバを追加してはならない(shop_itemが可変長の為)
};
struct mob_data {
	struct block_list bl;
	short n;
	short class,dir;
	short x0,y0,xs,ys;
	char name[24];
	int spawndelay1,spawndelay2;
	struct {
		unsigned state : 8 ;
		unsigned skillstate : 8 ;
		unsigned targettype : 1 ;
		unsigned steal_flag : 1 ;
		unsigned steal_coin_flag : 1 ;
		unsigned skillcastcancel : 1 ;
		unsigned master_check : 1 ;
	} state;
	int timer;
	short to_x,to_y;
	short speed;
	int hp;
	int target_id,attacked_id;
//	int flag;
	struct walkpath_data walkpath;
	unsigned int next_walktime;
	unsigned int last_deadtime,last_spawntime,last_thinktime;
	short move_fail_count;
	struct {
		int id;
		int dmg;
	} dmglog[DAMAGELOG_SIZE];
	struct item *lootitem;
	short lootitem_count;
	
	struct status_change sc_data[MAX_STATUSCHANGE];
	short sc_count;
	short opt1,opt2,option;
	short min_chase;

	int skilltimer;
	int skilltarget;
	short skillx,skilly;
	short skillid,skilllv,skillidx;
	unsigned int skilldelay[MAX_MOBSKILL];
	int def_ele;
	int master_id,master_dist;
	struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	struct skill_timerskill skilltimerskill[MAX_SKILLTIMERSKILL/2];
	char npc_event[50];
};
enum { MS_IDLE,MS_WALK,MS_ATTACK,MS_DEAD,MS_DELAY };

enum { NONE_ATTACKABLE,ATTACKABLE };

struct map_data {
	char name[16];
	unsigned char *gat;	// NULLなら下のmap_data_other_serverとして扱う
	struct block_list **block,**block_mob;
	int m;
	short xs,ys;
	short bxs,bys;
	int npc_num;
	int users;
	struct {
		unsigned nomemo : 1;
		unsigned noteleport : 1;
		unsigned nosave : 1;
		unsigned nobranch : 1;
		unsigned nopenalty : 1;
		unsigned pvp : 1;
		unsigned pvp_noparty : 1;
		unsigned pvp_noguild : 1;
		unsigned gvg : 1;
		unsigned gvg_noparty : 1;
	} flag;
	struct point save;
	struct npc_data *npc[MAX_NPC_PER_MAP];
};
struct map_data_other_server {
	char name[16];
	unsigned char *gat;	// NULL固定にして判断
	unsigned long ip;
	unsigned int port;
};
#define read_gat(m,x,y) (map[m].gat[(x)+(y)*map[m].xs])
#define read_gatp(m,x,y) (m->gat[(x)+(y)*m->xs])

struct flooritem_data {
	struct block_list bl;
	short subx,suby;
	int cleartimer;
	struct item item_data;
};

enum {
	SP_SPEED,SP_BASEEXP,SP_JOBEXP,SP_KARMA,SP_MANNER,SP_HP,SP_MAXHP,SP_SP,	// 0-7
	SP_MAXSP,SP_STATUSPOINT,SP_0a,SP_BASELEVEL,SP_SKILLPOINT,SP_STR,SP_AGI,SP_VIT,	// 8-15
	SP_INT,SP_DEX,SP_LUK,SP_CLASS,SP_ZENY,SP_SEX,SP_NEXTBASEEXP,SP_NEXTJOBEXP,	// 16-23
	SP_WEIGHT,SP_MAXWEIGHT,SP_1a,SP_1b,SP_1c,SP_1d,SP_1e,SP_1f,	// 24-31
	SP_USTR,SP_UAGI,SP_UVIT,SP_UINT,SP_UDEX,SP_ULUK,SP_26,SP_27,	// 32-39
	SP_28,SP_ATK1,SP_ATK2,SP_MATK1,SP_MATK2,SP_DEF1,SP_DEF2,SP_MDEF1,	// 40-47
	SP_MDEF2,SP_HIT,SP_FLEE1,SP_FLEE2,SP_CRITICAL,SP_ASPD,SP_36,SP_JOBLEVEL,	// 48-55
	// original
	SP_ATTACKRANGE,	SP_ATKELE,SP_DEFELE,	// 56-58
	SP_CASTRATE, SP_MAXHPRATE, SP_MAXSPRATE, SP_SPRATE, // 59-62
	SP_ADDELE, SP_ADDRACE, SP_ADDSIZE, SP_SUBELE, SP_SUBRACE, // 63-67
	SP_ADDEFF, SP_RESEFF,	// 68-69
	SP_CARTINFO=99,	// 99
};

enum {
	LOOK_BASE,LOOK_HAIR,LOOK_WEAPON,LOOK_HEAD_BOTTOM,LOOK_HEAD_TOP,LOOK_HEAD_MID,LOOK_HAIR_COLOR,LOOK_CLOTHES_COLOR,LOOK_SHIELD,LOOK_SHOES
};

struct chat_data {
	struct block_list bl;

	unsigned char pass[8];   /* password */
	unsigned char title[61]; /* room title MAX 60 */
	unsigned char limit;     /* join limit */
	unsigned char users;     /* current users */
	unsigned char pub;       /* room attribute */
	struct map_session_data *usersd[20];
	struct block_list *owner_;
	struct block_list **owner;
	char npc_event[50];
};

struct mons_data {
	int type;
	int max_hp;
	int npc_num;
	int job_exp;
	int base_exp;
	int atk;
	int hit;
	int flee;
	int def;
	struct {
		int nameid,p;
	} dropitem[16];
};


extern struct map_data map[];
extern int map_num;
extern int autosave_interval;

// 鯖全体情報
void map_setusers(int);
int map_getusers(void);
// block削除関連
int map_freeblock( void *bl );
int map_freeblock_lock(void);
int map_freeblock_unlock(void);
// block関連
int map_addblock(struct block_list *);
int map_delblock(struct block_list *);
void map_foreachinarea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,...);
void map_foreachinmovearea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,int,int,...);
int map_countnearpc(int,int,int);
// 一時的object関連
int map_addobject(struct block_list *);
int map_delobject(int);
int map_delobjectnofree(int id);
void map_foreachobject(int (*)(struct block_list*,va_list),int,...);
//
int map_quit(struct map_session_data *);
// npc
int map_addnpc(int,struct npc_data *);


// 床アイテム関連
int map_clearflooritem_timer(int,unsigned int,int,int);
#define map_clearflooritem(id) map_clearflooritem_timer(0,0,id,1)
int map_addflooritem(struct item *,int,int,int,int);

// キャラid＝＞キャラ名 変換関連
void map_addchariddb(int charid,char *name);
int map_reqchariddb(struct map_session_data * sd,int charid);
char * map_charid2nick(int);

struct map_session_data * map_id2sd(int);
struct block_list * map_id2bl(int);
int map_mapname2mapid(char*);
int map_mapname2ipport(char*,int*,int*);
int map_setipport(char *name,unsigned long ip,int port);
void map_addiddb(struct block_list *);
void map_deliddb(struct block_list *bl);
int map_foreachiddb(int (*)(void*,void*,va_list),...);
void map_addnickdb(struct map_session_data *);
struct map_session_data * map_nick2sd(char*);

// gat関連
int map_getcell(int,int,int);
int map_setcell(int,int,int,int);

// その他
int map_calc_dir( struct block_list *src,int x,int y);

// path.cより
int path_search(struct walkpath_data*,int,int,int,int,int,int);
int path_blownpos(int m,int x0,int y0,int dx,int dy,int count);

#endif
