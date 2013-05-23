#ifndef _MOB_H_
#define _MOB_H_

struct mob_skill {
	short state;
	short skill_id,skill_lv;
	short permillage;
	int casttime,delay;
	short cancel;
	short cond1,cond2;
	short target;
	short val1;
};

struct mob_db {
	char name[24],jname[24];
	int lv;
	int max_hp,max_sp;
	int base_exp,job_exp;
	int atk1,atk2;
	int def,mdef;
	int str,agi,vit,int_,dex,luk;
	int range,range2,range3;
	int size,race,element,mode;
	int speed,adelay,amotion,dmotion;
	int mexp,mexpper;
	struct { int nameid,p; } dropitem[8];
	struct { int nameid,p; } mvpitem[3];
	int summonflag;
	int maxskill;
	struct mob_skill skill[MAX_MOBSKILL];
};
extern struct mob_db mob_db[];

enum {
	MST_TARGET			=	0,
	MST_SELF			=	1,

	MSC_ALWAYS			=	0x0000,
	MSC_MYHPLTMAXRATE	=	0x0001,
	MSC_FRIENDHPLTMAXRATE=	0x0010,
	MSC_MYSTATUSEQ		=	0x0020,
	MSC_MYSTATUSNE		=	0x0021,
	MSC_FRIENDSTATUSEQ	=	0x0030,
	MSC_FRIENDSTATUSNE	=	0x0030,
	
	MSC_ATTACKPCGT		=	0x0100,
	MSC_ATTACKPCGE		=	0x0101,
	MSC_SLAVELT			=	0x0110,
	MSC_SLAVELE			=	0x0111,
	MSC_CLOSEDATTACKED	=	0x1000,
	MSC_LONGRANGEATTACKED=	0x1001,
	MSC_SKILLUSED		=	0x1010,
	MSC_CASTTARGETED	=	0x1011,
};

enum {
	MSS_IDLE,	// ë“ã@
	MSS_WALK,	// à⁄ìÆ
	MSS_ATTACK,	// çUåÇ
	MSS_DEAD,	// éÄñS
	MSS_LOOT,	// ÉãÅ[Ég
	MSS_CHASE,	// ìÀåÇ
};

int mobdb_searchname(const char *str);
int mob_once_spawn(struct map_session_data *sd,char *mapname,
	int x,int y,const char *mobname,int class,int amount,const char *event);
int mob_once_spawn_area(struct map_session_data *sd,char *mapname,
	int x0,int y0,int x1,int y1,
	const char *mobname,int class,int amount,const char *event);

int mob_walktoxy(struct mob_data *md,int x,int y,int easy);

int mob_target(struct mob_data *md,struct block_list *bl,int dist);
int mob_stop_walking(struct mob_data *md,int type);
int mob_stopattack(struct mob_data *);
int mob_spawn(int);
int mob_damage(struct map_session_data *,struct mob_data*,int);
int mob_heal(struct mob_data*,int);
int do_init_mob(void);

int mob_delete(struct mob_data *md);
int mob_catch_delete(struct mob_data *md);
int mob_deleteslave(struct mob_data *md);

int mob_warp(struct mob_data *md,int x,int y,int type);

int mobskill_use(struct mob_data *md,unsigned int tick,int event);
int mobskill_event(struct mob_data *md,int flag);
int mobskill_castend_id( int tid, unsigned int tick, int id,int data );
int mobskill_castend_pos( int tid, unsigned int tick, int id,int data );
int mob_summonslave(struct mob_data *md2,int class,int amount,int flag);

#endif
