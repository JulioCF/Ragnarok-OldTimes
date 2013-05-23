#ifndef _BATTLE_H_
#define _BATTLE_H_

// ダメージ
struct Damage {
	int damage,damage2;
	int type,div_;
	int amotion,dmotion;
	int blewcount;
};

// 属性表（読み込みはpc.c、battle_attr_fixで使用）
extern int attr_fix_table[4][10][10];

struct map_session_data;
struct mob_data;
struct block_list;

// ダメージ計算

struct Damage battle_calc_attack(	int attack_type,
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_weapon_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);

// 属性修正計算
int battle_attr_fix(int damage,int atk_elem,int def_elem);

// ダメージ最終計算
int battle_calc_damage(struct block_list *bl,int damage,int flag);
enum {	// 最終計算のフラグ
	BF_WEAPON	= 0x0001,
	BF_MAGIC	= 0x0002,
	BF_MISC		= 0x0004,
	BF_SHORT	= 0x0010,
	BF_LONG		= 0x0040,
	BF_SKILL	= 0x0100,
	BF_NORMAL	= 0x0200,
	BF_WEAPONMASK=0x000f,
	BF_RANGEMASK= 0x00f0,
	BF_SKILLMASK= 0x0f00,
};

// 実際にHPを増減
int battle_delay_damage(unsigned int tick,struct block_list *src,struct block_list *target,int damage);
int battle_damage(struct block_list *bl,struct block_list *target,int damage);
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp);

// 攻撃や移動を止める
int battle_stopattack(struct block_list *bl);
int battle_stopwalking(struct block_list *bl,int type);

// 通常攻撃処理まとめ
int battle_weapon_attack( struct block_list *bl,struct block_list *target,
	 unsigned int tick,int flag);

// 各種パラメータを得る
int battle_get_lv(struct block_list *bl);
int battle_get_hp(struct block_list *bl);
int battle_get_max_hp(struct block_list *bl);
int battle_get_str(struct block_list *bl);
int battle_get_agi(struct block_list *bl);
int battle_get_vit(struct block_list *bl);
int battle_get_int(struct block_list *bl);
int battle_get_dex(struct block_list *bl);
int battle_get_luk(struct block_list *bl);
int battle_get_hit(struct block_list *bl);
int battle_get_flee(struct block_list *bl);
int battle_get_def(struct block_list *bl);
int battle_get_mdef(struct block_list *bl);
int battle_get_flee2(struct block_list *bl);
int battle_get_def2(struct block_list *bl);
int battle_get_mdef2(struct block_list *bl);
int battle_get_atk(struct block_list *bl);
int battle_get_atk2(struct block_list *bl);
int battle_get_amotion(struct block_list *bl);
int battle_get_dmotion(struct block_list *bl);
int battle_get_element(struct block_list *bl);
int battle_get_attack_element(struct block_list *bl);
int battle_get_attack_element2(struct block_list *bl);  //左手武器属性取得
#define battle_get_elem_type(bl)	(battle_get_element(bl)%10)
#define battle_get_elem_level(bl)	(battle_get_element(bl)/10/2)
int battle_get_party_id(struct block_list *bl);
int battle_get_guild_id(struct block_list *bl);
int battle_get_race(struct block_list *bl);
int battle_get_size(struct block_list *bl);
int battle_get_mode(struct block_list *bl);
int battle_get_mexp(struct block_list *bl);

struct status_change *battle_get_sc_data(struct block_list *bl);
short *battle_get_sc_count(struct block_list *bl);
short *battle_get_opt1(struct block_list *bl);
short *battle_get_opt2(struct block_list *bl);
short *battle_get_option(struct block_list *bl);


enum {
	BCT_NOENEMY	=0x00000,
	BCT_PARTY	=0x10000,
	BCT_ENEMY	=0x40000,
	BCT_NOPARTY	=0x50000,
	BCT_ALL		=0x20000,
	BCT_NOONE	=0x60000,
};
int battle_check_target( struct block_list *src, struct block_list *target,int flag);

int battle_check_range(struct block_list *src,int x,int y,int range);


// 設定
extern struct Battle_Config {
	int warp_point_debug;
	int enemy_critical;
	int cast_rate,delay_rate,delay_dependon_dex;
	int sdelay_attack_enable;
	int pc_damage_delay;
	int defnotenemy;
	int attr_recover;
	int flooritem_lifetime;
	int item_rate,base_exp_rate,job_exp_rate;
	int death_penalty_type;
	int death_penalty_base,death_penalty_job;
	int restart_hp_rate;
	int restart_sp_rate;
	int mvp_item_rate,mvp_exp_rate;
	int mvp_hp_rate;
	int atc_gmonly,gm_allskill;
	int wp_rate;
	int monster_active_enable;
	int monster_loot_type;
	int mob_skill_use;
	int mob_count_rate;
	int quest_skill_learn;
	int quest_skill_reset;
	int basic_skill_check;
	int guild_emperium_check;
	int ghost_time;
	int pet_catch_rate;
	int pet_rename;
	int pet_friendly_rate;
	int pet_hungry_delay_rate;
	int skill_min_damage;
	int sanctuary_type;
	int finger_offensive_type;
	int heal_exp,shop_exp;
	int asuradelay;
	int equip_modifydisplay;
} battle_config;

#define BATTLE_CONF_FILENAME	"conf/battle_athena.conf"
int battle_config_read(const char *cfgName);

#endif
