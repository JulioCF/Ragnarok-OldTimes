// Original : mmo.h 2003/03/14 12:07:02 Rev.1.7

#ifndef	_MMO_H_
#define	_MMO_H_

#ifdef CYGWIN
// txtやlogなどの書き出すファイルの改行コード
#define RETCODE	"\r\n"	// (CR/LF：Windows系)
#else
#define RETCODE "\n"	// (LF：Unix系）
#endif

#define MAX_MAP_PER_SERVER 512
#define MAX_INVENTORY 100
#define MAX_AMOUNT 30000
#define MAX_ZENY 1000000000	// 1G zeny
#define MAX_CART 100
#define MAX_SKILL 350
#define GLOBAL_REG_NUM 16
#define DEFAULT_WALK_SPEED 150
#define MAX_STORAGE 100
#define MAX_PARTY 12
#define MAX_GUILD 36
#define MAX_GUILDPOSITION 20
#define MAX_GUILDEXPLUSION 32
#define MAX_GUILDALLIANCE 16
#define MAX_GUILDSKILL	8

#define GRF_PATH_FILENAME "conf/grf-files.txt"

struct item {
	int id;
	short nameid;
	short amount;
	unsigned short equip;
	char identify;
	char refine;
	char attribute;
	short card[4];
};
struct point{
	char map[16];
	short x,y;
};
struct skill {
	unsigned short id,lv,flag;
};
struct global_reg {
	char str[16];
	int value;
};
struct s_pet {
	int account_id;
	int char_id;
	int pet_id;
	short class;
	short level;
	short egg_id;//pet egg id
	short equip;//pet equip name_id
	short intimate;//pet friendly
	short hungry;//pet hungry
	char name[24];
	char rename_flag;
	char incuvate;
};

struct mmo_charstatus {
	int char_id;
	int account_id;
	int base_exp,job_exp,zeny;

	short class;
	short status_point,skill_point;
	short hp,max_hp,sp,max_sp;
	short option,karma,manner;
	short hair,hair_color,clothes_color;
	int party_id,guild_id,pet_id;

	short weapon,shield,shoes,changehand;
	short head_top,head_mid,head_bottom;

	char name[24];
	unsigned char base_level,job_level;
	unsigned char str,agi,vit,int_,dex,luk,char_num,sex;

	struct point last_point,save_point,memo_point[10];
	struct item inventory[MAX_INVENTORY],cart[MAX_CART];
	struct skill skill[MAX_SKILL];
	int global_reg_num;
	struct global_reg global_reg[GLOBAL_REG_NUM];
};

struct storage {
	int account_id;
	short storage_status;
	short storage_amount;
	struct item storage[MAX_STORAGE];
};

struct map_session_data;

struct gm_account {
	int account_id;
	int level;
};

struct party_member {
	int account_id;
	char name[24],map[16];
	int leader,online,lv;
	struct map_session_data *sd;
};
struct party {
	int party_id;
	char name[24];
	int exp;
	int item;
	struct party_member member[MAX_PARTY];
};

struct guild_member {
	int account_id, char_id;
	short hair,hair_color,gender,class,lv;
	int exp,exp_payper;
	short online,position;
	int rsv1,rsv2;
	char name[24];
	struct map_session_data *sd;
};
struct guild_position {
	char name[24];
	int mode;
	int exp_mode;
};
struct guild_alliance {
	int opposition;
	int guild_id;
	char name[24];
};
struct guild_explusion {
	char name[24];
	char mes[40];
	char acc[40];
	int account_id;
	int rsv1,rsv2,rsv3;
};
struct guild_skill {
	int id,lv;
};
struct guild {
	int guild_id;
	short guild_lv, connect_member, max_member, average_lv;
	int exp,next_exp,skill_point,castle_id;
	char name[24],master[24];
	struct guild_member member[MAX_GUILD];
	struct guild_position position[MAX_GUILDPOSITION];
	char mes1[60],mes2[120];
	int emblem_len,emblem_id;
	char emblem_data[2048];
	struct guild_alliance alliance[MAX_GUILDALLIANCE];
	struct guild_explusion explusion[MAX_GUILDEXPLUSION];
	struct guild_skill skill[MAX_GUILDSKILL];
};
struct guild_castle {
	int castle_id;
	int guild_id;
};

enum {
	GBI_EXP			=1,		// ギルドのEXP
	GBI_GUILDLV		=2,		// ギルドのLv
	GBI_SKILLPOINT	=3,		// ギルドのスキルポイント
	GBI_SKILLLV		=4,		// ギルドスキルLv

	GMI_POSITION	=0,		// メンバーの役職変更
	GMI_EXP			=1,		// メンバーのEXP
	
};

#ifndef strcmpi
#define strcmpi strcasecmp
#endif
#ifndef stricmp
#define stricmp strcasecmp
#endif
#ifndef strncmpi
#define strncmpi strncasecmp
#endif
#ifndef strnicmp
#define strnicmp strncasecmp
#endif

#endif	// _MMO_H_
