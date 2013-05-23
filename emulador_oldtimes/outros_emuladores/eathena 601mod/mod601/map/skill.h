#ifndef _SKILL_H_
#define _SKILL_H_

#define MAX_SKILL_DB			350
#define MAX_SKILL_PRODUCE_DB	 100
#define MAX_SKILL_ARROW_DB	 100

// スキルデータベース
struct skill_db {
	int range,hit,inf,pl,nk,max;
	int sp[10],num[10];
	int cast[10],delay[10];
	int inf2;
};
extern struct skill_db skill_db[MAX_SKILL_DB];

// アイテム作成データベース
struct skill_produce_db {
	int nameid, trigger;
	int req_skill,itemlv;
	int mat_id[5],mat_amount[5];
};
extern struct skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

// 矢作成データベース
struct skill_arrow_db {
	int nameid, trigger;
	int cre_id[5],cre_amount[5];
};
extern struct skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];

struct block_list;
struct map_session_data;
struct skill_unit;
struct skill_unit_group;

int do_init_skill(void);

// スキルデータベースへのアクセサ
int skill_get_range( int id );
int	skill_get_hit( int id );
int	skill_get_inf( int id );
int	skill_get_pl( int id );
int	skill_get_nk( int id );
int	skill_get_max( int id );
int	skill_get_sp( int id ,int lv );
int	skill_get_num( int id ,int lv );
int	skill_get_cast( int id ,int lv );
int	skill_get_delay( int id ,int lv );
int skill_get_unit_id(int id,int flag);
int	skill_get_inf2( int id );

// スキルの使用
int skill_use_id( struct map_session_data *sd, int target_id,
	int skill_num,int skill_lv);
int skill_use_pos( struct map_session_data *sd,
	int skill_x, int skill_y, int skill_num, int skill_lv);

int skill_castend_map( struct map_session_data *sd,int skill_num, const char *map);

int skill_cleartimerskill(struct block_list *src);
int skill_addtimerskill(struct block_list *src,int tick,int target,int x,int y,int skill_id,int skill_lv,int type,int flag);

// 追加効果
int skill_additional_effect( struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick);

// ユニットスキル
struct skill_unit *skill_initunit(struct skill_unit_group *group,int idx,int x,int y);
int skill_delunit(struct skill_unit *unit);
struct skill_unit_group *skill_initunitgroup(struct block_list *src,
	int count,int skillid,int skilllv,int unit_id);
int skill_delunitgroup(struct skill_unit_group *group);
struct skill_unit_group_tickset *skill_unitgrouptickset_search(
	struct block_list *bl,int group_id);
int skill_unitgrouptickset_delete(struct block_list *bl,int group_id);
int skill_clear_unitgroup(struct block_list *src);

int skill_unit_ondamaged(struct skill_unit *src,struct block_list *bl,
	int damage,unsigned int tick);

int skill_unit_move( struct block_list *bl,unsigned int tick,int range);

// 詠唱キャンセル
int skill_castcancel( struct block_list *sd );

#define skill_calc_heal(bl,skill_lv)	\
	(( battle_get_lv(bl)+battle_get_int(bl) )/8 *(4+ skill_lv*8))

// その他
int skill_check_cloaking(struct block_list *bl);

// ステータス異常
int skill_status_change_start(struct block_list *bl,int type,int val1,int val2);
int skill_status_change_timer(int tid, unsigned int tick, int id, int data);
int skill_status_change_end( struct block_list* bl , int type,int tid );
int skill_status_change_clear(struct block_list *bl);


// アイテム作成
int skill_can_produce_mix( struct map_session_data *sd, int nameid, int trigger );
int skill_produce_mix( struct map_session_data *sd,
	int nameid, int slot1, int slot2, int slot3 );

// mobスキルのため
int skill_castend_nodamage_id( struct block_list *src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_castend_damage_id( struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_castend_pos2( struct block_list *src, int x,int y,int skillid,int skilllv,unsigned int tick,int flag);

// スキル攻撃一括処理
int skill_attack( int attack_type, struct block_list* src, struct block_list *dsrc,
	 struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );


enum {	// struct map_session_data の status_changeの番号テーブル
// 64未満クライアントへの通知あり、64以上通知無し。番号の変更可能性あり
// 2-2次職の値はなんかめちゃくちゃっぽいので暫定。たぶん変更されます。

	SC_STONE				=64,
	SC_FREEZE				=65,
	SC_STAN					=66,
	SC_SLEEP				=67,
	SC_POISON				=68,
	SC_CURSE				=69,
	SC_SILENCE				=70,
	SC_CONFUSION			=71,
	SC_BLIND				=72,
	
	SC_DIVINA				= SC_SILENCE,
	SC_PROVOKE				= 0,
	SC_ENDURE				= 1,
	SC_TWOHANDQUICKEN		= 2,
	SC_CONCENTRATE			= 3,
	SC_HIDDING				= 4,
	SC_CLOAKING				= 5,
	SC_ENCPOISON			= 6,
	SC_POISONREACT			= 7,
	SC_QUAGMIRE				= 8,
	SC_ANGELUS				= 9,
	SC_BLESSING				=10,
	SC_SIGNUMCRUCIS			=11,
	SC_INCREASEAGI			=12,
	SC_DECREASEAGI			=13,
	SC_IMPOSITIO			=15,
	SC_SUFFRAGIUM			=16,
	SC_ASPERSIO				=17,
	SC_BENEDICTIO			=18,
	SC_KYRIE				=19,
	SC_MAGNIFICAT			=20,
	SC_GLORIA				=21,
	SC_AETERNA				=22,
	SC_ADRENALINE			=23,
	SC_WEAPONPERFECTION		=24,
	SC_OVERTHRUST			=25,
	SC_MAXIMIZEPOWER		=26,
	SC_TRICKDEAD			=29,
	SC_LOUD					=30,
	SC_ENERGYCOAT			=31,
	SC_STRIPWEAPON			=50,
	SC_STRIPSHIELD			=51,
	SC_STRIPARMOR			=52,
	SC_STRIPHELM			=53,
	SC_CP_WEAPON			=54,
	SC_CP_SHIELD			=55,
	SC_CP_ARMOR				=56,
	SC_CP_HELM				=57,
	SC_AUTOGUARD			=58,
	SC_REFLECTSHIELD		=59,
	SC_DEVOTION				=60,
	SC_PROVIDENCE			=61,
	SC_DEFENDER				=62,
	SC_CALLSPIRITS			=63,
	SC_SIGHTTRASHER			=73,
	SC_SIGHT				=75,
	SC_RUWACH				=76,
	SC_AUTOCOUNTER			=77,
	SC_DRUMBATTLE			=78,
	SC_SIEGFRIED			=79,
	SC_WHISTLE				=80,
	SC_APPLEIDUN			=81,
	SC_FORTUNE				=82,
	SC_SERVICE4U			=83,
	SC_CASTCANCEL			=84,
	SC_VOLCANO				=85,
	SC_EXPLOSIONSPIRITS		=86,
	SC_STEELBODY			=87,
	SC_SPELLBREAKER			=88,
	SC_DELUGE				=89,
	SC_FLAMELAUNCHER		=95,
	SC_FROSTWEAPON			=96,
	SC_LIGHTNINGLOADER		=97,
	SC_SEISMICWEAPON		=98,
	SC_SPEARSQUICKEN		=99,
	SC_ASSNCROS				=100,
	SC_POEMBRAGI			=101,
	SC_FREECAST				=102,
	SC_NIBELUNGEN			=103,
	SC_HUMMING				=104,
	SC_ETERNALCHAOS			=105,
	SC_DONTFORGETME			=106,
	SC_ABSORBSPIRIT			=107,
	SC_LOKIWAIL				=108,
	SC_INTOABYSS			=109,
	SC_BLADESTOP			=110,
	SC_VIOLENTGALE			=111,
	SC_LANDPROTECTOR		=112,
	SC_ADAPTATION			=113,
	SC_KIMARICHMAN			=114,
	
	SC_RIDING				=27,
	SC_FALCON				=28,
	SC_WEIGHT50				=35,
	SC_WEIGHT90				=36,
	SC_SPEEDPOTION0			=37,
	SC_SPEEDPOTION1			=38,
	SC_SPEEDPOTION2			=39,
	
	SC_SAFETYWALL			=90,
	SC_PNEUMA				=91,
	SC_WATERBALL			=92,
	SC_METEOSTORM			=93,
	SC_ANKLE				=94,


// by RoVeRT
	SC_SLOWPOISON = 14,			
	SC_BROKNARMOR = 32,
	SC_BROKNWEAPON = 33,

// for skill memory
	SC_CANNIBALIZE = 115,
	SC_SPHEREMINE = 116,

	SC_AUTOSPELL = 117,

// unused 34, 40-49, 74
};
extern int SkillStatusChangeTable[];

enum {
	NV_BASIC = 1,

	SM_SWORD = 2,
	SM_TWOHAND = 3,
	SM_RECOVERY = 4,
	SM_BASH = 5,
	SM_PROVOKE = 6,
	SM_MAGNUM = 7,
	SM_ENDURE = 8,

	MG_SRECOVERY = 9,
	MG_SIGHT = 10,
	MG_NAPALMBEAT = 11,
	MG_SAFETYWALL = 12,
	MG_SOULSTRIKE = 13,
	MG_COLDBOLT= 14,
	MG_FROSTDIVER = 15,
	MG_STONECURSE = 16,
	MG_FIREBALL = 17,
	MG_FIREWALL = 18,
	MG_FIREBOLT = 19,
	MG_LIGHTNINGBOLT = 20,
	MG_THUNDERSTORM = 21,

	AL_DP = 22,
	AL_DEMONBANE = 23,
	AL_RUWACH = 24,
	AL_PNEUMA = 25,
	AL_TELEPORT = 26,
	AL_WARP = 27,
	AL_HEAL = 28,
	AL_INCAGI = 29,
	AL_DECAGI = 30,
	AL_HOLYWATER = 31,
	AL_CRUCIS = 32,
	AL_ANGELUS = 33,
	AL_BLESSING = 34,
	AL_CURE = 35,

	MC_INCCARRY = 36,
	MC_DISCOUNT = 37,
	MC_OVERCHARGE = 38,
	MC_PUSHCART = 39,
	MC_IDENTIFY = 40,
	MC_VENDING = 41,
	MC_MAMMONITE = 42,

	AC_OWL = 43,
	AC_VULTURE = 44,
	AC_CONCENTRATION = 45,
	AC_DOUBLE = 46,
	AC_SHOWER = 47,

	TF_DOUBLE = 48,
	TF_MISS = 49,
	TF_STEAL = 50,
	TF_HIDING = 51,
	TF_POISON = 52,
	TF_DETOXIFY = 53,

	ALL_RESURRECTION = 54,

	KN_SPEARMASTERY = 55,
	KN_PIERCE = 56,
	KN_BRANDISHSPEAR = 57,
	KN_SPEARSTAB = 58,
	KN_SPEARBOOMERANG = 59,
	KN_TWOHANDQUICKEN = 60,
	KN_AUTOCOUNTER = 61,
	KN_BOWLINGBASH = 62,
	KN_RIDING = 63,
	KN_CAVALIERMASTERY = 64,

	PR_MACEMASTERY = 65,
	PR_IMPOSITIO = 66,
	PR_SUFFRAGIUM = 67,
	PR_ASPERSIO = 68,
	PR_BENEDICTIO = 69,
	PR_SANCTUARY = 70,
	PR_SLOWPOISON = 71,
	PR_STRECOVERY = 72,
	PR_KYRIE = 73,
	PR_MAGNIFICAT = 74,
	PR_GLORIA = 75,
	PR_LEXDIVINA = 76,
	PR_TURNUNDEAD = 77,
	PR_LEXAETERNA = 78,
	PR_MAGNUS = 79,

	WZ_FIREPILLAR = 80,
	WZ_SIGHTRASHER = 81,
	WZ_FIREIVY = 82,
	WZ_METEOR = 83,
	WZ_JUPITEL = 84,
	WZ_VERMILION = 85,
	WZ_WATERBALL = 86,
	WZ_ICEWALL = 87,
	WZ_FROSTNOVA = 88,
	WZ_STORMGUST = 89,
	WZ_EARTHSPIKE = 90,
	WZ_HEAVENDRIVE = 91,
	WZ_QUAGMIRE = 92,
	WZ_ESTIMATION = 93,

	BS_IRON = 94,
	BS_STEEL = 95,
	BS_ENCHANTEDSTONE = 96,
	BS_ORIDEOCON = 97,
	BS_DAGGER = 98,
	BS_SWORD = 99,
	BS_TWOHANDSWORD = 100,
	BS_AXE = 101,
	BS_MACE = 102,
	BS_KNUCKLE = 103,
	BS_SPEAR = 104,
	BS_HILTBINDING = 105,
	BS_FINDINGORE = 106,
	BS_WEAPONRESEARCH = 107,
	BS_REPAIRWEAPON = 108,
	BS_SKINTEMPER = 109,
	BS_HAMMERFALL = 110,
	BS_ADRENALINE = 111,
	BS_WEAPONPERFECT = 112,
	BS_OVERTHRUST = 113,
	BS_MAXIMIZE = 114,

	HT_SKIDTRAP = 115,
	HT_LANDMINE = 116,
	HT_ANKLESNARE = 117,
	HT_SHOCKWAVE = 118,
	HT_SANDMAN = 119,
	HT_FLASHER = 120,
	HT_FREEZINGTRAP = 121,
	HT_BLASTMINE = 122,
	HT_CLAYMORETRAP = 123,
	HT_REMOVETRAP = 124,
	HT_TALKIEBOX = 125,
	HT_BEASTBANE = 126,
	HT_FALCON = 127,
	HT_STEELCROW = 128,
	HT_BLITZBEAT = 129,
	HT_DETECTING = 130,
	HT_SPRINGTRAP = 131,

	AS_RIGHT = 132,
	AS_LEFT = 133,
	AS_KATAR = 134,
	AS_CLOAKING = 135,
	AS_SONICBLOW = 136,
	AS_GRIMTOOTH = 137,
	AS_ENCHANTPOISON = 138,
	AS_POISONREACT = 139,
	AS_VENOMDUST = 140,
	AS_SPLASHER = 141,

	NV_FIRSTAID = 142,
	NV_TRICKDEAD = 143,
	SM_MOVINGRECOVERY = 144,
	SM_FATALBLOW = 145,
	SM_AUTOBERSERK = 146,
	AC_MAKINGARROW = 147,
	AC_CHARGEARROW = 148,
	TF_SPRINKLESAND = 149,
	TF_BACKSLIDING = 150,
	TF_PICKSTONE = 151,
	TF_THROWSTONE = 152,
	MC_CARTREVOLUTION = 153,
	MC_CHANGECART = 154,
	MC_LOUD = 155,
	AL_HOLYLIGHT = 156,
	MG_ENERGYCOAT = 157,

	NPC_PIERCINGATT = 158,
	NPC_MENTALBREAKER = 159,
	NPC_RANGEATTACK = 160,
	NPC_ATTRICHANGE,
	NPC_CHANGEWATER,
	NPC_CHANGEGROUND,
	NPC_CHANGEFIRE,
	NPC_CHANGEWIND,
	NPC_CHANGEPOISON,
	NPC_CHANGEHOLY,
	NPC_CHANGEDARKNESS,
	NPC_CHANGETELEKINESIS,
	NPC_CRITICALSLASH = 170,
	NPC_COMBOATTACK,
	NPC_GUIDEDATTACK,
	NPC_SELFDESTRUCTION,
	NPC_SPLASHATTACK,
	NPC_SUICIDE,
	NPC_POISON,
	NPC_BLINDATTACK,
	NPC_SILENCEATTACK,
	NPC_STUNATTACK,
	NPC_PETRIFYATTACK = 180,
	NPC_CURSEATTACK,
	NPC_SLEEPATTACK,
	NPC_RANDOMATTACK,
	NPC_WATERATTACK,
	NPC_GROUNDATTACK,
	NPC_FIREATTACK,
	NPC_WINDATTACK,
	NPC_POISONATTACK,
	NPC_HOLYATTACK,
	NPC_DARKNESSATTACK = 190,
	NPC_TELEKINESISATTACK,
	NPC_MAGICALATTACK,
	NPC_METAMORPHOSIS,
	NPC_PROVOCATION,
	NPC_SMOKING,
	NPC_SUMMONSLAVE,
	NPC_EMOTION,
	NPC_TRANSFORMATION,
	NPC_BLOODDRAIN,
	NPC_ENERGYDRAIN = 200,
	NPC_KEEPING,
	NPC_DARKBREATH,
	NPC_DARKBLESSING,
	NPC_BARRIER,
	NPC_DEFENDER,
	NPC_LICK,
	NPC_HALLUCINATION,
	NPC_REBIRTH,
	NPC_SUMMONMONSTER = 209,

	RG_SNATCHER = 210,
	RG_STEALCOIN = 211,
	RG_BACKSTAP = 212,
	RG_TUNNELDRIVE = 213,
	RG_RAID = 214,
	RG_STRIPWEAPON = 215,
	RG_STRIPSHIELD = 216,
	RG_STRIPARMOR = 217,
	RG_STRIPHELM = 218,
	RG_INTIMIDATE = 219,
	RG_GRAFFITI = 220,
	RG_FLAGGRAFFITI = 221,
	RG_CLEANER = 222,
	RG_GANGSTER = 223,
	RG_COMPULSION = 224,
	RG_PLAGIARISM = 225,

	AM_AXEMASTERY = 226,
	AM_LEARNINGPOTION = 227,
	AM_PHARMACY = 228,
	AM_DEMONSTRATION = 229,
	AM_ACIDTERROR = 230,
	AM_POTIONPITCHER = 231,
	AM_CANNIBALIZE = 232,
	AM_SPHEREMINE = 233,
	AM_CP_WEAPON = 234,
	AM_CP_SHIELD = 235,
	AM_CP_ARMOR = 236,
	AM_CP_HELM = 237,
	AM_BIOETHICS = 238,
	AM_BIOTECHNOLOGY = 239,
	AM_CREATECREATURE = 240,
	AM_CULTIVATION = 241,
	AM_FLAMECONTROL = 242,
	AM_CALLHOMUN = 243,
	AM_REST = 244,
	AM_DRILLMASTER = 245,
	AM_HEALHOMUN = 246,
	AM_RESURRECTHOMUN = 247,

	CR_TRUST = 248,
	CR_AUTOGUARD = 249,
	CR_SHIELDCHARGE = 250,
	CR_SHIELDBOOMERANG = 251,
	CR_REFLECTSHIELD = 252,
	CR_HOLYCROSS = 253,
	CR_GRANDCROSS = 254,
	CR_DEVOTION = 255,
	CR_PROVIDENCE = 256,
	CR_DEFENDER = 257,
	CR_SPEARQUICKEN = 258,

	MO_IRONHAND = 259,
	MO_SPIRITSRECOVERY = 260,
	MO_CALLSPIRITS = 261,
	MO_ABSORBSPIRITS = 262,
	MO_TRIPLEATTACK = 263,
	MO_BODYRELOCATION = 264,
	MO_DODGE = 265,
	MO_INVESTIGATE = 266,
	MO_FINGEROFFENSIVE = 267,
	MO_STEELBODY = 268,
	MO_BLADESTOP = 269,
	MO_EXPLOSIONSPIRITS = 270,
	MO_EXTREMITYFIST = 271,
	MO_CHAINCOMBO = 272,
	MO_COMBOFINISH = 273,

	SA_ADVANCEDBOOK = 274,
	SA_CASTCANCEL = 275,
	SA_MAGICROD = 276,
	SA_SPELLBREAKER = 277,
	SA_FREECAST = 278,
	SA_AUTOSPELL = 279,
	SA_FLAMELAUNCHER = 280,
	SA_FROSTWEAPON = 281,
	SA_LIGHTNINGLOADER = 282,
	SA_SEISMICWEAPON = 283,
	SA_DRAGONOLOGY = 284,
	SA_VOLCANO = 285,
	SA_DELUGE = 286,
	SA_VIOLENTGALE = 287,
	SA_LANDPROTECTOR = 288,
	SA_DISPELL = 289,
	SA_ABRACADABRA = 290,
	SA_MONOCELL = 291,
	SA_CLASSCHANGE = 292,
	SA_SUMMONMONSTER = 293,
	SA_REVERSEORCISH = 294,
	SA_DEATH = 295,
	SA_FORTUNE = 296,
	SA_TAMINGMONSTER = 297,
	SA_QUESTION = 298,
	SA_GRAVITY = 299,
	SA_LEVELUP = 300,
	SA_INSTANTDEATH = 301,
	SA_FULLRECOVERY = 302,
	SA_COMA = 303,

	BD_ADAPTATION = 304,
	BD_ENCORE = 305,
	BD_LULLABY = 306,
	BD_RICHMANKIM = 307,
	BD_ETERNALCHAOS = 308,
	BD_DRUMBATTLEFIELD = 309,
	BD_RINGNIBELUNGEN = 310,
	BD_ROKISWEIL = 311,
	BD_INTOABYSS = 312,
	BD_SIEGFRIED = 313,
	BD_RAGNAROK = 314,

	BA_MUSICALLESSON = 315,
	BA_MUSICALSTRIKE = 316,
	BA_DISSONANCE = 317,
	BA_FROSTJOKE = 318,
	BA_WHISTLE = 319,
	BA_ASSASSINCROSS = 320,
	BA_POEMBRAGI = 321,
	BA_APPLEIDUN = 322,

	DC_DANCINGLESSON = 323,
	DC_THROWARROW = 324,
	DC_UGLYDANCE = 325,
	DC_SCREAM = 326,
	DC_HUMMING = 327,
	DC_DONTFORGETME = 328,
	DC_FORTUNEKISS = 329,
	DC_SERVICEFORYOU = 330,

	GD_APPROVAL=10000,
	GD_KAFRACONTACT,
	GD_GUARDIANRESEARCH,
	GD_CHARISMA,
	GD_EXTENSION,
};

#endif

