#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "battle.h"

#include "timer.h"

#include "map.h"
#include "pc.h"
#include "skill.h"
#include "mob.h"
#include "itemdb.h"
#include "clif.h"


int attr_fix_table[4][10][10];

struct Battle_Config battle_config;

// 各種パラメータ所得

int battle_get_lv(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].lv;
	else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->status.base_level;
	else
		return 0;
}
int battle_get_hp(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return ((struct mob_data *)bl)->hp;
	else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->status.hp;
	else
		return 1;
}
int battle_get_max_hp(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].max_hp;
	else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->status.max_hp;
	else
		return 1;
}
int battle_get_str(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].str;
	else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->paramc[0];
	else
		return 0;
}
int battle_get_agi(struct block_list *bl)
{
	int agi=0;
	struct status_change *sc_data=battle_get_sc_data(bl);
	
	if(bl->type==BL_MOB){
	
		agi=mob_db[((struct mob_data *)bl)->class].agi;
				
		if( sc_data[SC_INCREASEAGI].timer!=-1)	// 速度増加(PCはpc.cで)
			agi += 2+sc_data[SC_INCREASEAGI].val1;

	}else if(bl->type==BL_PC)
		agi=((struct map_session_data *)bl)->paramc[1];
	else
		return 0;

	if( sc_data[SC_DECREASEAGI].timer!=-1)	// 速度減少
		agi -= 2+sc_data[SC_DECREASEAGI].val1;
	
	if( sc_data[SC_QUAGMIRE].timer!=-1 )	// クァグマイア
		agi/=2;

	return agi;
}
int battle_get_vit(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].vit;
	else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->paramc[2];
	else
		return 0;
}
int battle_get_int(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		struct status_change *sc_data=battle_get_sc_data(bl);
		int int_=mob_db[((struct mob_data *)bl)->class].int_;

		if( sc_data[SC_BLESSING].timer!=-1){	// ブレッシング
			int race=battle_get_race(bl);
			if( race==1 || race==6 )	int_/=2;	// 悪 魔/不死
			else int_ += sc_data[SC_BLESSING].val1;	// その他
		}
		return int_;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->paramc[3];
	else
		return 0;
}
int battle_get_dex(struct block_list *bl)
{
	int dex=0;
	if(bl->type==BL_MOB){
		struct status_change *sc_data=battle_get_sc_data(bl);

		dex= mob_db[((struct mob_data *)bl)->class].dex;

		if( sc_data[SC_BLESSING].timer!=-1){	// ブレッシング
			int race=battle_get_race(bl);
			if( race==1 || race==6 )	dex/=2;		// 悪 魔/不死
			else dex += sc_data[SC_BLESSING].val1;	// その他
		}
	}else if(bl->type==BL_PC)
		dex= ((struct map_session_data *)bl)->paramc[4];
	else
		return 0;
		
	if( battle_get_sc_data(bl)[SC_QUAGMIRE].timer!=-1 )	// クァグマイア
		dex/=2;

	return dex;
}
int battle_get_luk(struct block_list *bl)
{
	int luk=0;
	if(bl->type==BL_MOB){
		luk=mob_db[((struct mob_data*)bl)->class].luk;
		// MOBのみ追加計算（PCはpc_calcstatusで計算する）
		if( battle_get_sc_data(bl)[SC_GLORIA].timer!=-1 )	// グロリア(PCはpc.cで)
			luk+=30;
	
	}else if(bl->type==BL_PC)
		luk=((struct map_session_data*)bl)->paramc[5];
	else
		return 0;

	if( battle_get_sc_data(bl)[SC_CURSE].timer!=-1 )		// 呪い
		luk=0;
		
	return luk;
}

int battle_get_flee(struct block_list *bl)
{
	int flee;
	if(bl->type==BL_MOB){
		flee=battle_get_agi(bl) + battle_get_lv(bl);
		// MOBのみ追加計算（PCはpc_calcstatusで計算する）
		if( battle_get_sc_data(bl)[SC_BLIND].timer!=-1 )	// 暗黒による低下
			flee -= flee*25/100;

	}else if(bl->type==BL_PC){
		flee=((struct map_session_data *)bl)->flee;
	}else
		return 0;
	
	return flee;
}
int battle_get_hit(struct block_list *bl)
{
	int hit;
	if(bl->type==BL_MOB){
		hit=battle_get_dex(bl) + battle_get_lv(bl);
		// MOBのみ追加計算（PCはpc_calcstatusで計算する）
		if( battle_get_sc_data(bl)[SC_BLIND].timer!=-1 )	// 暗黒による低下
			hit -= hit*25/100;

	}else if(bl->type==BL_PC)
		hit=((struct map_session_data *)bl)->hit;
	else
		return 0;

	return hit;
}
int battle_get_flee2(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return battle_get_luk(bl)/10+1;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->flee2;
	else
		return 0;
	
}
int battle_get_critical(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return battle_get_luk(bl)/3+1;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->critical;
	else
		return 0;
}


int battle_get_atk(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data*)bl)->class].atk1;
	}else if(bl->type==BL_PC){
		int atk=((struct map_session_data*)bl)->watk;

		// PCのみ追加計算（atk1,atk2の定義がmobと異なる。mobはatk2で計算）
		if( battle_get_sc_data(bl)[SC_CURSE].timer!=-1 )		// 呪い
			atk-= atk*25/100;
		return atk;
	}else
		return 0;
}
int battle_get_atk2(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		struct status_change *sc_data=battle_get_sc_data(bl);
		int atk2=mob_db[((struct mob_data*)bl)->class].atk2;
		
		// MOBのみ追加計算（PCはpc_calcstatusで計算する）
		if( sc_data[SC_IMPOSITIO].timer!=-1)	// インポシティオマヌス
			atk2 += sc_data[SC_IMPOSITIO].val1*5;
		if( sc_data[SC_PROVOKE].timer!=-1 )		// プロボック
			atk2 = atk2*(100+2*sc_data[SC_PROVOKE].val1)/100;
		if( sc_data[SC_CURSE].timer!=-1 )		// 呪い
			atk2 -= atk2*25/100;
		return atk2;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data*)bl)->watk2;
	else
		return 0;
}
int battle_get_def(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data *)bl)->class].def;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->def;
	else
		return 0;
}
int battle_get_mdef(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data *)bl)->class].mdef;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->mdef;
	else
		return 0;
}
int battle_get_def2(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		struct status_change *sc_data=battle_get_sc_data(bl);
		int def2= mob_db[((struct mob_data *)bl)->class].vit;

		// MOBのみ追加計算（PCはpc_calcstatusで計算する）
		if( sc_data[SC_ANGELUS].timer!=-1)	// エンジェラス
			def2 = def2*(110+5*sc_data[SC_ANGELUS].val1)/100;
		
		if( sc_data[SC_PROVOKE].timer!=-1 )		// プロボック
			def2= def2*(100-6*sc_data[SC_PROVOKE].val1)/100;


		return def2;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->def2;
	else
		return 0;
}
int battle_get_mdef2(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data *)bl)->class].int_;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->mdef2;
	else
		return 0;
}
int battle_get_amotion(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data *)bl)->class].amotion;
	}else if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->amotion;
	else
		return 200;
}
int battle_get_dmotion(struct block_list *bl)
{
	int ret;
	if(bl->type==BL_MOB){
		ret=mob_db[((struct mob_data *)bl)->class].dmotion;
	}else if(bl->type==BL_PC)
		ret=((struct map_session_data *)bl)->dmotion;
	else
		return 200;
	
	if( (battle_get_sc_data(bl))[SC_PROVOKE].timer!=-1 )	// プロボック
		ret=0;
	
	return ret;
}
int battle_get_element(struct block_list *bl)
{
	int ret;
	struct status_change *sc_data=battle_get_sc_data(bl);
	
	if(bl->type==BL_MOB)	// 10の位＝Lv*2、１の位＝属性
		ret=((struct mob_data *)bl)->def_ele;
	else if(bl->type==BL_PC)
		ret=20+((struct map_session_data *)bl)->def_ele;	// 防御属性Lv1
	else
		return 20;
	
	if( sc_data[SC_BENEDICTIO].timer!=-1 )	// 聖体降福
		ret=26;
	if( sc_data[SC_FREEZE].timer!=-1 )	// 凍結
		ret=21;
	
	return ret;
}
int battle_get_attack_element(struct block_list *bl)
{
	int ret;
	struct status_change *sc_data=battle_get_sc_data(bl);
	
	if(bl->type==BL_MOB)
		ret=0;
	else if(bl->type==BL_PC)
		ret=((struct map_session_data *)bl)->atk_ele;
	else
		return 0;
	
	if( sc_data[SC_FROSTWEAPON].timer!=-1)	// フロストウェポン
		ret=1;
	if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// サイズミックウェポン
		ret=2;
	if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// フレームランチャー
		ret=3;
	if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// ライトニングローダー
		ret=4;
	if( sc_data[SC_ENCPOISON].timer!=-1)	// エンチャントポイズン
		ret=5;
	if( sc_data[SC_ASPERSIO].timer!=-1)		// アスペルシオ
		ret=6;
	
	return ret;
}
int battle_get_attack_element2(struct block_list *bl)
{
	int ret;
	struct status_change *sc_data=battle_get_sc_data(bl);
	
	if(bl->type==BL_MOB)
		ret=0;
	else if(bl->type==BL_PC)
		ret=((struct map_session_data *)bl)->atk_ele_;
	else
		return 0;
	
	if( sc_data[SC_FROSTWEAPON].timer!=-1)	// フロストウェポン
		ret=1;
	if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// サイズミックウェポン
		ret=2;
	if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// フレームランチャー
		ret=3;
	if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// ライトニングローダー
		ret=4;
	if( sc_data[SC_ENCPOISON].timer!=-1)	// エンチャントポイズン
		ret=5;
	if( sc_data[SC_ASPERSIO].timer!=-1)		// アスペルシオ
		ret=6;
	
	return ret;
}
int battle_get_party_id(struct block_list *bl)
{
	if( bl->type == BL_PC )
		return ((struct map_session_data *)bl)->status.party_id;
	else if( bl->type==BL_MOB ){
		struct mob_data *md=(struct mob_data *)bl;
		if( md->master_id>0 )
			return md->master_id;
		return md->bl.id;
	}else if( bl->type==BL_SKILL )
		return ((struct skill_unit *)bl)->group->party_id;
	else
		return -1;
}
int battle_get_guild_id(struct block_list *bl)
{
	if( bl->type == BL_PC )
		return ((struct map_session_data *)bl)->status.guild_id;
	else if( bl->type==BL_MOB )
		return ((struct mob_data *)bl)->class;
	else if( bl->type==BL_SKILL )
		return ((struct skill_unit *)bl)->group->guild_id;
	else
		return -1;
}
int battle_get_race(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data *)bl)->class].race;
	}else if(bl->type==BL_PC)
		return 7;
	else
		return 0;
}
int battle_get_size(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data *)bl)->class].size;
	}else if(bl->type==BL_PC)
		return 1;
	else
		return 0;
}
int battle_get_mode(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data *)bl)->class].mode;
	}else
		return 0x01;	// とりあえず動くということで1
}

int battle_get_mexp(struct block_list *bl)
{
	if(bl->type==BL_MOB){
		return mob_db[((struct mob_data *)bl)->class].mexp;
	}else
		return 0;
}

// StatusChange系の所得
struct status_change *battle_get_sc_data(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return ((struct mob_data*)bl)->sc_data;
	else if(bl->type==BL_PC)
		return ((struct map_session_data*)bl)->sc_data;
	return NULL;
}
short *battle_get_sc_count(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->sc_count;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->sc_count;
	return NULL;
}
short *battle_get_opt1(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->opt1;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->opt1;
	return NULL;
}
short *battle_get_opt2(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->opt2;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->opt2;
	return NULL;
}
short *battle_get_option(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->option;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->status.option;
	return NULL;
}

//-------------------------------------------------------------------

// ダメージの遅延
struct battle_delay_damage_ {
	struct block_list *src,*target;
	int damage;
};
int battle_delay_damage_sub(int tid,unsigned int tick,int id,int data)
{
	struct battle_delay_damage_ *dat=(struct battle_delay_damage_ *)data;
	if( map_id2bl(id)==dat->src && dat->target->prev!=NULL)
		battle_damage(dat->src,dat->target,dat->damage);
	free(dat);
	return 0;
}
int battle_delay_damage(unsigned int tick,struct block_list *src,struct block_list *target,int damage)
{
	struct battle_delay_damage_ *dat=malloc(sizeof(struct battle_delay_damage_));
	dat->src=src;
	dat->target=target;
	dat->damage=damage;
	add_timer(tick,battle_delay_damage_sub,src->id,(int)dat);
	return 0;
}

// 実際にHPを操作
int battle_damage(struct block_list *bl,struct block_list *target,int damage)
{
	struct map_session_data *sd=NULL;
	struct status_change *sc_data=battle_get_sc_data(target);
	short *sc_count;
	if(damage==0)
		return 0;

	if(target->prev == NULL)
		return 0;

	if(bl) {
		if(bl->prev==NULL)
			return 0;

		if(bl->type==BL_PC)
			sd=(struct map_session_data *)bl;
	}
		
	if(damage<0)
		return battle_heal(bl,target,-damage,0);
	
	if( (sc_count=battle_get_sc_count(target))!=NULL && *sc_count>0){
		// 凍結、石化、睡眠を消去
		if(sc_data[SC_FREEZE].timer!=-1)
			skill_status_change_end(target,SC_FREEZE,-1);
		if(sc_data[SC_STONE].timer!=-1)
			skill_status_change_end(target,SC_STONE,-1);
		if(sc_data[SC_SLEEP].timer!=-1)
			skill_status_change_end(target,SC_SLEEP,-1);
	}


	if(target->type==BL_MOB){	// MOB

		struct mob_data *md=(struct mob_data *)target;
		if(md->skilltimer!=-1 && md->state.skillcastcancel)	// 詠唱妨害
			skill_castcancel(target);
		return mob_damage(sd,md,damage);

	}else if(target->type==BL_PC){	// PC

		struct map_session_data *tsd=(struct map_session_data *)target;
		if(tsd->skilltimer!=-1){	// 詠唱妨害
				// フェンカードや妨害されないスキルかの検査
			if( !pc_check_equip_dcard(tsd,4077) && tsd->state.skillcastcancel)
				skill_castcancel(target);
		}
		return pc_damage(bl,tsd,damage);

	}else if(target->type==BL_SKILL)
		return skill_unit_ondamaged((struct skill_unit *)target,bl,damage,gettick());
	return 0;
}
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp)
{
	if(hp<0)
		return battle_damage(bl,target,-hp);
	
	if(target->type==BL_MOB)
		return mob_heal((struct mob_data *)target,hp);
	else if(target->type==BL_PC)
		return pc_heal((struct map_session_data *)target,hp,sp);
	return 0;
}

// 攻撃停止
int battle_stopattack(struct block_list *bl)
{
	if(bl->type==BL_MOB)
		return mob_stopattack((struct mob_data*)bl);
	else if(bl->type==BL_PC)
		return pc_stopattack((struct map_session_data*)bl);
	return 0;
}
// 移動停止
int battle_stopwalking(struct block_list *bl,int type)
{
	if(bl->type==BL_MOB)
		return mob_stop_walking((struct mob_data*)bl,type);
	else if(bl->type==BL_PC)
		return pc_stop_walking((struct map_session_data*)bl,type);
	return 0;
}


/*==========================================
 * ダメージの属性修正
 *------------------------------------------
 */
int battle_attr_fix(int damage,int atk_elem,int def_elem)
{
	int def_type= def_elem%10, def_lv=def_elem/10/2;

	if(	atk_elem<0 || atk_elem>9 || def_type<0 || def_type>9 ||
		def_lv<1 || def_lv>4){	// 属 性値がおかしいのでとりあえずそのまま返す
		printf("battle_attr_fix: unknown attr type: atk=%d def_type=%d def_lv=%d\n",atk_elem,def_type,def_lv);
		return damage;
	}

	return damage*attr_fix_table[def_lv-1][atk_elem][def_type]/100;
}


/*==========================================
 * ダメージ最終計算
 *------------------------------------------
 */
int battle_calc_damage(struct block_list *bl,int damage,int flag)
{
	struct map_session_data *sd=NULL;
	struct mob_data *md=NULL;
	struct status_change *sc_data,*sc;
	short *sc_count;
	int i;
	

	if(bl->type==BL_MOB) md=(struct mob_data *)bl;
	else				 sd=(struct map_session_data *)bl;
	
	sc_data=battle_get_sc_data(bl);
	sc_count=battle_get_sc_count(bl);

	if(sc_count!=NULL && *sc_count>0){
	
		if(sc_data[SC_SAFETYWALL].timer!=-1 && damage>0 &&
			flag&BF_WEAPON && flag&BF_SHORT ){
			// セーフティウォール
			struct skill_unit *unit=(struct skill_unit*)sc_data[SC_SAFETYWALL].val2;
			if( unit->alive && (--unit->group->val2)<=0 )
				skill_delunit(unit);
			skill_unit_move(bl,gettick(),1);	// 重ね掛けチェック
			damage=0;
		}
		if(sc_data[SC_PNEUMA].timer!=-1 && damage>0 &&
			flag&BF_WEAPON && flag&BF_LONG ){
			// ニューマ
			damage=0;
		}
		
		if(sc_data[SC_AETERNA].timer!=-1 && damage>0){	// レックスエーテルナ
			damage<<=1;
			skill_status_change_end( bl,SC_AETERNA,-1 );
		}
	
		if(sc_data[SC_ENERGYCOAT].timer!=-1 && damage>0){	// エナジーコート
			if(sd){
				if(sd->status.sp>0 && flag&BF_WEAPON){
					int per = sd->status.sp * 5 / (sd->status.max_sp + 1);
					sd->status.sp -= damage * (per * 5 + 10) / 1000;
					if( sd->status.sp <0 ) sd->status.sp=0;
					damage -= damage * (per * 6) / 100;
					clif_updatestatus(sd,SP_SP);
				}
				if(sd->status.sp<=0)
					skill_status_change_end( bl,SC_ENERGYCOAT,-1 );
			}
		}
	
		if(sc_data[SC_KYRIE].timer!=-1){	// キリエエレイソン
			sc=&sc_data[SC_KYRIE];
			sc->val2-=damage;
			if(flag&BF_WEAPON){
				if(sc->val2>=0)	damage=0;
				else			damage=-sc->val2;
			}
			if((--sc->val1)<=0 || (sc->val2<=0))
				skill_status_change_end(bl, SC_KYRIE, -1);
		}
	}
	
	if(	damage>0 && sc_data!=NULL && (
		sc_data[i=SC_STONE].timer!=-1	||	// 石化
		sc_data[i=SC_FREEZE].timer!=-1	||	// 凍結
		sc_data[i=SC_SLEEP].timer!=-1	)){	// 睡眠
		skill_status_change_end( bl, i, -1 );	// ダメージ受けたら解ける
	}

	if( md!=NULL && md->hp>0 )	// 反撃などのMOBスキル判定
		mobskill_event(md,flag);

	return damage;
}

/*==========================================
 * 修練ダメージ
 *------------------------------------------
 */
int battle_addmastery(struct map_session_data *sd,struct block_list *target,int dmg)
{
	int damage,skill,item_id;
	int race=battle_get_race(target);
	damage = 0;

	// デーモンベイン(+3 ～ +30) vs 不死 or 悪魔 (死人は含めない？)
	if((skill = pc_checkskill(sd,AL_DEMONBANE)) > 0 && (race==1 || race==6) ) 
		damage += (skill * 3);

	// ビーストベイン(+4 ～ +40) vs 動物 or 昆虫
	if((skill = pc_checkskill(sd,HT_BEASTBANE)) > 0 && (race==2 || race==4) ) 
		damage += (skill * 4);

	
	item_id = pc_checkequip(sd,2);	// 右手or両手
	if(item_id == -1) return (dmg);
	switch(itemdb_look(item_id))
	{
		case 0x00:	// 素手
		{
			// 鉄 拳(+3 ～ +30) 素手,ナックル
			if((skill = pc_checkskill(sd,MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x01:	// 短剣 (Updated By AppleGirl)
		case 0x02:	// 1HS
		{
			// 剣修練(+4 ～ +40) 片手剣 短剣含む
			if((skill = pc_checkskill(sd,SM_SWORD)) > 0) {
				damage += (skill * 4);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x03:	// 2HS
		{
			// 両手剣修練(+4 ～ +40) 両手剣
			if((skill = pc_checkskill(sd,SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x04:	// 1HL
		{
			// 槍修練(+4 ～ +40,+5 ～ +50) 槍
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// ペコに乗ってない
				else
					damage += (skill * 5);	// ペコに乗ってる
			}
			break;
		}
		case 0x05:	// 2HL
		{
			// 槍修練(+4 ～ +40,+5 ～ +50) 槍
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// ペコに乗ってない
				else
					damage += (skill * 5);	// ペコに乗ってる
			}
			break;
		}
		case 0x06:	// 片手斧
		{
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x07: // Axe by Tato
		{
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x08:	// メイス
		{
			// メイス修練(+3 ～ +30) メイス
			if((skill = pc_checkskill(sd,PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x09:	// なし?
			break;
		case 0x0a:	// 杖
			break;
		case 0x0b:	// 弓
			break;
		case 0x0c:	// Knuckles
		{
			// 鉄拳(+3 ～ +30) 素手,ナックル
			if((skill = pc_checkskill(sd,MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x0d:	// Musical Instrument
		{
			// 楽器の練習(+3 ～ +30) 楽器
			if((skill = pc_checkskill(sd,BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x0e:	// Dance Mastery
		{
			// Dance Lesson Skill Effect(+3 damage for every lvl = +30) 鞭
			if((skill = pc_checkskill(sd,DC_DANCINGLESSON)) > 0) {
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x0f:	// Book
		{
			// Advance Book Skill Effect(+3 damage for every lvl = +30) {
			if((skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0) {
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
		case 0x10:	// Katars
		{
			// カタール修練(+3 ～ +30) カタール
			if((skill = pc_checkskill(sd,AS_KATAR)) > 0) {
				//ソニックブロー時は別処理（1撃に付き1/8適応)
				damage += (skill * 3);
			clif_updatestatus(sd,SP_ATK1);
			clif_updatestatus(sd,SP_ATK2);
			}
			break;
		}
	}
	damage = dmg + damage;
	return (damage);
}

/*==========================================
 * 武器ダメージ計算
 *------------------------------------------
 */
struct Damage battle_calc_weapon_attack(
	struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct map_session_data *sd=NULL,*tsd=NULL;
	struct mob_data* md=NULL,*tmd=NULL;
	int hitrate,cri,atkmin,atkmax;
	int str,dex,luk,item_id;
	int def1 = battle_get_def(target);
	int def2 = battle_get_def2(target);
	struct Damage wd;
	int damage,damage2,type,div_,blewcount=0;
	int flag,skill;
	int t_mode=0,t_race=7,t_size=1,s_race=7,s_ele=0;
	struct status_change *sc_data,*t_sc_data;
	short *sc_count;
	short *option, *opt1, *opt2;
	int atkmax_=0, atkmin_=0, s_ele_;	//二刀流用
	int da=0;

	// アタッカー
	if(src->type==BL_PC)
		sd=(struct map_session_data *)src;
	else
		md=(struct mob_data *)src;
	s_race=battle_get_race(src);
	s_ele=battle_get_attack_element(src);
	s_ele_=battle_get_attack_element2(src);
	sc_data=battle_get_sc_data(src);
	sc_count=battle_get_sc_count(src);
	option=battle_get_option(src);
	opt1=battle_get_opt1(src);
	opt2=battle_get_opt2(src);
	
	// ターゲット

	if(target->type==BL_PC)
		tsd=(struct map_session_data *)target;
	else
		tmd=(struct mob_data *)target;
	t_race=battle_get_race( target );
	t_size=battle_get_size( target );
	t_mode=battle_get_mode( target );
	t_sc_data=battle_get_sc_data( target );

	if( *opt1>0 )	// 状 態異常による妨害
		battle_stopattack(src);

/*	if( !(t_mode&0x20) &&
		(sc_data[SC_TRICKDEAD].timer != -1 ||
		 (*option&0x06 && t_race!=4 && t_race!=6)
		) )	// スキルによる妨害
		battle_stopattack(src);*/
	
	flag=BF_SHORT|BF_WEAPON|BF_NORMAL;	// 攻撃の種類の設定
	
	// 回避率計算、回避判定は後で
	hitrate=battle_get_hit(src) -battle_get_flee(target) + 80;

	// 敵回避率増加スキルによる無条件命中率減少
	if( tsd && (skill = pc_checkskill(tsd,49))>0)
		if(hitrate>=100-3*skill)
			hitrate=100-3*skill;

	type=0;	// normal
	div_ = 1; // single attack

	str=battle_get_str(src);
	dex=battle_get_dex(src);
	luk=battle_get_luk(src);
	if( src->type==BL_PC ){	// PCの処理(基本攻撃力/サイズ修正)
		// 基本攻撃力の算出
		// カード・ATK増加修正(スケルトン、ウルフ、アンドレ)
		int item_look=0;
		damage = damage2 = (str/10)*(str/10) + str + dex/5 + luk/5;
		atkmin = atkmin_ = dex;
		if((item_id = pc_checkequip(sd,2)) != -1 && (item_look=itemdb_look(item_id)==11)) {
			// 弓の場合
			damage = (dex/10)*(dex/10) + dex + str/5 + luk/5;
			// 矢のダメージを追加
	//		damage += rand()%矢の攻撃力;
			atkmin = sd->watk * (dex<sd->watk)?dex:sd->watk;
			flag=(flag&~BF_RANGEMASK)|BF_LONG;
			//矢を減らす
			pc_delitem(sd,pc_search_inventory(sd,pc_checkequip(sd,0x8000)),1,0);
		}

		// instrument dex fix - Sara
		if((item_id = pc_checkequip(sd,2)) != -1 && (item_look=itemdb_look(item_id)==13)) {
			damage = (dex/10)*(dex/10) + dex + str/5 + luk/5;
			atkmin = sd->watk * (dex<sd->watk)?dex:sd->watk;
		}
		// whip dex fix - Sara
		if((item_id = pc_checkequip(sd,2)) != -1 && (item_look=itemdb_look(item_id)==14)) {
			damage = (dex/10)*(dex/10) + dex + str/5 + luk/5;
			atkmin = sd->watk * (dex<sd->watk)?dex:sd->watk;
		}

		// サイズ修正
		// ペコ騎乗していて、槍で攻撃した場合は中型のサイズ修正を100にする
		// ウェポンパーフェクション,ドレイクC
		if( target->type==BL_MOB ){
			if(pc_isriding(sd) && (item_look==4 || item_look==5) && t_size==1)	//ペコ騎乗していて、槍で中型を攻撃
				atkmax = sd->watk;
			else{
				atkmax = ((sd->watk - sd->watk_) * sd->atkmods[ t_size ]) / 100;
				atkmax_ = (sd->watk_ * sd->atkmods_[ t_size ]) / 100;
			}
		}else
			atkmax = sd->watk;
		if( pc_check_equip_wcard(sd,4137) ){	// ドレイクカード
			atkmax = sd->watk - sd->watk_;
			atkmax_ = sd->watk_;
		}
	
		if( sc_data[SC_WEAPONPERFECTION].timer!=-1 )	// ウェポンパーフェクション
			atkmax = sd->watk;

	}else{
		// MOBの処理（基本攻撃力/ サイズ修正なし）
		damage2=0;
		damage = (str/10)*(str/10)+str;
		atkmin = battle_get_atk(src);
		atkmax = battle_get_atk2(src);
		if( mob_db[md->class].range>2 )
			flag=(flag&~BF_RANGEMASK)|BF_LONG;
	}
	if(atkmin > atkmax) atkmin = atkmax;
	if(atkmin_ > atkmax_) atkmin_ = atkmax_;

	if( sc_data[SC_MAXIMIZEPOWER].timer!=-1 ){	// マキシマイズパワー
		atkmin=atkmax;
		atkmin_=atkmax_;
	}


	//ダブルアタック判定
	if( src->type == BL_PC ){
		item_id = pc_checkequip(sd,2);		// 右手用か?
		if(itemdb_look(item_id) == 0x01) {
			if( skill_num==0 && (skill = pc_checkskill(sd,TF_DOUBLE)) > 0)
				da = (rand()%100 < (skill*5)) ? 1:0;
		}
	}

	//三段掌
	if( src->type == BL_PC ){
		if( skill_num==0 && (skill = pc_checkskill(sd,MO_TRIPLEATTACK)) > 0)
			da = (rand()%100 < (30 - skill)) ? 2:0;
	}

	if(da == 0){ //ダブルアタックが発動していない
		// クリティカル計算
		cri = 1+(luk * 30)/10;

		if( src->type == BL_PC ){
			// カード等の追加効果の再計算(百分率を千分率に直す)
			cri += 10*(sd->critical - ((sd->paramc[5]*3/10)+1));

			if((item_id = pc_checkequip(sd,34)) != -1)
				if(itemdb_look(item_id) == 16)
					// カタールの場合、クリティカルを倍に
					cri <<=1;
		}
		cri -= (battle_get_luk(target) * 30)/10;
		
		if( t_sc_data[SC_SLEEP].timer!=-1 )	// 睡眠中はクリティカルが倍に
			cri <<=1;
	}
	if(da == 0 && //ダブルアタックが発動していない
		(rand() % 1000) < cri && skill_num==0 &&	// 判定（スキルの場合は無視）
		(src->type==BL_PC || battle_config.enemy_critical ) &&	// 敵の判定
		!(tsd!=NULL && pc_check_equip_dcard(tsd,4143)))	// オークヒーローカード
	{
		damage += atkmax;
		damage2 += atkmax_;
		type = 0x0a;
	}
	else
	{
		int vitbonusmax, t_vit=0;
		int rnd;
	
		rnd = rand();
		if(atkmax > atkmin)
			damage += atkmin + rnd % (atkmax-atkmin);
		else
			damage += atkmin ;
		if(atkmax_ > atkmin_)
			damage2 += atkmin_ + rnd % (atkmax_-atkmin_);
		else
			damage2 += atkmin_ ;
		// スキル修正１（攻撃力倍化系）
		// オーバートラスト(+5% ～ +25%),他攻撃系スキルの場合ここで補正
		// バッシュ,マグナムブレイク,
		// ボーリングバッシュ,スピアブーメラン,ブランディッシュスピア,スピアスタッブ,
		// メマーナイト,カートレボリューション
		// ダブルストレイフィング,アローシャワー,チャージアロー,
		// ソニックブロー
		if( sc_data!=NULL && sc_data[SC_OVERTHRUST].timer!=-1)	// オーバートラスト
			damage += damage*(5*sc_data[SC_OVERTHRUST].val1)/100;
		if(skill_num>0){
			int i;
			if( (i=skill_get_pl(skill_num))>0 )
				s_ele=s_ele_=i;
			
			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// バッシュ
				damage = damage*(100+ 30*skill_lv)/100;
				hitrate+=(skill_lv<6)?10:20;
				break;
			case SM_MAGNUM:		// マグナムブレイク
				damage = damage*(5*skill_lv +(wflag)?65:115 )/100;
				blewcount=2;
				break;
			case MC_MAMMONITE:	// メマーナイト
				damage = damage*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// ダブルストレイフィング
				damage = damage*(180+ 20*skill_lv)/100;
				div_=2;
				break;
			case AC_SHOWER:	// アローシャワー
				damage = damage*(75 + 5*skill_lv)/100;
				break;
			case KN_PIERCE:	// ピアース
				damage = damage*(100+ 10*skill_lv)/100;
				hitrate=hitrate*(100+5*skill_lv)/100;
				div_=t_size+1;
				damage*=div_;
				break;
			case KN_SPEARSTAB:	// スピアスタブ
				damage = damage*(100+ 15*skill_lv)/100;
				blewcount=4;
				break;
			case KN_SPEARBOOMERANG:	// スピアブーメラン
				damage = damage*(100+ 50*skill_lv)/100;
				break;
			case KN_BOWLINGBASH:	// ボウリングバッシュ
				damage = damage*(100+ 50*skill_lv)/100;
				//blewcount=4;skill.cで吹き飛ばしやってみた
				break;
			case AS_SONICBLOW:	// ソニックブロウ
				damage = damage*(300+ 50*skill_lv)/100;
				div_=8;
				break;
			case AC_CHARGEARROW:	// チャージアロー
				blewcount=6;
				break;
			case TF_SPRINKLESAND:	// 砂まき
				damage = damage*125/100;
				break;
			case MC_CARTREVOLUTION:	// カートレボリューション
				if(sd->cart_max_weight > 0 && sd->cart_weight > 0)
						damage = (damage*(150 + pc_checkskill(sd,BS_WEAPONRESEARCH) + (sd->cart_weight*100/sd->cart_max_weight) ) )/100;
					else
						damage = (damage*150)/100;
				blewcount=2;
				break;
			// 以下MOB
			case NPC_COMBOATTACK:	// 多段攻撃
				div_=skill_get_num(171,skill_lv);
				damage *= div_;
				break;
			case NPC_RANDOMATTACK:	// ランダムATK攻撃
				damage = damage*(50+rand()%150)/100;
				break;
			// 属性攻撃（適当）
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_TELEKINESISATTACK:
				damage = damage*(100+25*skill_lv)/100;
				break;
			case RG_BACKSTAP:	// バックスタブ
				damage = damage*(300+ 40*skill_lv)/100;
				break;
			case RG_RAID:	// サプライズアタック
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// インティミデイト
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// シールドチャージ
				damage = damage*(100+ 20*skill_lv)/100;
				blewcount=4+skill_lv;
				break;
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case CR_GRANDCROSS:	// グランドクロス
				damage = damage*(100+ 40*skill_lv)/100;
//				md->hp = mob_db[md->class].max_hp/75;
				break;
			case MO_FINGEROFFENSIVE:	//指弾
				if(!battle_config.finger_offensive_type) {
					if(src->type == BL_PC) {
						damage = damage * (100 + 50 * skill_lv) / 100 * sd->spiritball_old;
						div_ = sd->spiritball_old;
					}
					else {
						damage = damage * (100 + 50 * skill_lv) / 100;
						div_ = 1;
					}
				}
				else {
					damage = damage * (100 + 50 * skill_lv) / 100;
					div_ = 1;
				}
				break;
			case MO_INVESTIGATE:	// 発 勁
				damage = damage*(100+ 75*skill_lv)/100 * (def1 + def2)/100;
				hitrate = 100000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
				damage = damage * (8 + ((sd->status.sp)/10)) + 250 + (skill_lv * 150);
				sd->status.sp = 0;
				clif_updatestatus(sd,SP_SP);
				hitrate = 100000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_CHAINCOMBO:	// 連打掌
				damage = damage*(150+ 50*skill_lv)/100;
				div_=4;
				break;
			case MO_COMBOFINISH:	// 猛龍拳
				blewcount=3;
				damage = damage*(240+ 60*skill_lv)/100;
				break;
			case BA_MUSICALSTRIKE:	// ミュージカルストライク
				damage = damage*(100+ 50 * skill_lv)/100;
				break;
			case BA_DISSONANCE:	// 不協和音
				damage = skill_lv*20+30;
				break;
			case DC_THROWARROW:	// 矢撃ち
				damage = damage*(100+ 50 * skill_lv)/100;
				break;
			}
		}
		
		if( skill_num!=NPC_CRITICALSLASH ){
			// 対 象の防御力によるダメージの減少
			// ディバインプロテクション（ここでいいのかな？）
			if ( !(skill_num == MO_INVESTIGATE || skill_num == MO_EXTREMITYFIST)) {	//DEF, VIT無視
				t_vit = def2;
				if( src->type==BL_MOB && (s_race==1 || s_race==6) )
					if(target->type==BL_PC && (skill=pc_checkskill(tsd,AL_DP)) > 0 )
						t_vit+=skill*3;
			
				vitbonusmax = (t_vit/20)*(t_vit/20)-1;
				damage = damage * (100 - def1) /100
					- t_vit - ((vitbonusmax < 1)?0: rand()%vitbonusmax );
				damage2 = damage2 * (100 - def1) /100
					- t_vit - ((vitbonusmax < 1)?0: rand()%vitbonusmax );
			}
		}
	}
	// 精錬ダメージの追加
	if( !(skill_num == MO_INVESTIGATE || skill_num == MO_EXTREMITYFIST)) {			//DEF, VIT無視
		if( src->type==BL_PC && (item_id = pc_checkequip(sd,2)) != -1)
			damage += battle_get_atk2(src) - sd->watk_2;
		if( src->type==BL_PC)
			damage2 += sd->watk_2;
	}
	if(da == 2) { //三段掌が発動しているか
		type = 0x08;
		div_ = 255;	//三段掌用に…
		damage = damage * (100 + 20 * pc_checkskill(sd, MO_TRIPLEATTACK)) / 100;
	}
 	// 過剰精錬ボーナス
	if( src->type==BL_PC){
		if(sd->overrefine - sd->overrefine_>0 )
			damage+=(rand()%(sd->overrefine - sd->overrefine_))+1;
		if(sd->overrefine_>0 )
			damage2+=(rand()%sd->overrefine_)+1;
	}
	// 0未満だった場合1に補正
	if(damage<1) damage=1;
	if(damage2<1) damage2=0;

	// スキル修正２（修練系）
	// 修練ダメージ(右手のみ) ソニックブロー時は別処理（1撃に付き1/8適応)
	if( !(skill_num == MO_INVESTIGATE || skill_num == MO_EXTREMITYFIST)) {			//修練ダメージ無視
		if( src->type==BL_PC )
			damage = battle_addmastery(sd,target,damage);
	}

	// 回避修正
	if( src->type==BL_PC && hitrate < 100000)
		hitrate = ((hitrate<5)?5:hitrate);
	else if(hitrate < 100000)
		hitrate = ((hitrate>95)?95: ((hitrate<5)?5:hitrate) );
	if( skill_num==NPC_GUIDEDATTACK ||
		skill_num==RG_BACKSTAP ||		// バックスタブ
		hitrate >= 100000 ||			// 必中攻撃
		t_sc_data[SC_SLEEP].timer!=-1 ||	// 睡眠は必中
		t_sc_data[SC_STAN].timer!=-1 ||		// スタンは必中
		t_sc_data[SC_FREEZE].timer!=-1 )	// 凍結は必中
		hitrate = 100;
	if(!(rand()%100<hitrate) && type==0)
		damage = damage2 = 0;
	
	// スキル修正３（武器研究）
	if( src->type==BL_PC && (skill=pc_checkskill(sd,BS_WEAPONRESEARCH)) > 0)
		damage+= skill*2;
	// カード・ダメージUP修正 両手持ちの場合左手のカードも右手に適用,左手はカードによる補正はなし
	if( src->type==BL_PC){
		int cardfix=100;
		int t_ele=battle_get_elem_type(target);
		cardfix=cardfix*(100+sd->addrace[t_race])/100;	// 種族によるダメージ修正
		cardfix=cardfix*(100+sd->addele[t_ele])/100;	// 属 性によるダメージ修正
		cardfix=cardfix*(100+sd->addsize[t_size])/100;	// サイズによるダメージ修正
		if( pc_check_equip_dcard(sd,4140) && (t_mode&0x20) )
			cardfix=cardfix*125/100;			// 深淵の騎士カード（ボスに25％追加）	
		damage=damage*cardfix/100;
	}
	if( target->type==BL_PC ){
		int cardfix=100;
		cardfix=cardfix*(100-tsd->subrace[s_race])/100;	// 種族によるダメージ耐性
		cardfix=cardfix*(100-tsd->subele[s_ele])/100;	// 属 性によるダメージ耐性
		if( pc_check_equip_dcard(tsd,4045) && (flag&BF_LONG) )
			cardfix=cardfix*75/100;			// ホルンカード（遠距離は25％減少）
		damage=damage*cardfix/100;
	}
	// 属 性の適用
	damage=battle_attr_fix(damage,
		s_ele, battle_get_element(target) );
	damage2=battle_attr_fix(damage2,
		s_ele_, battle_get_element(target) );

	if( target->type==BL_PC && battle_get_attack_element(src)==3 &&
		(skill=pc_checkskill(tsd,BS_SKINTEMPER))>0 )
		damage=damage*(100-skill*5)/100;		// スキンテンパリング

	// 星のかけらの適用
	if( src->type==BL_PC){
		damage += sd->star - sd->star_;
		damage2 += sd->star_;
		damage += sd->spiritball*3;
		damage2 += sd->spiritball*3;
	}

	if( src->type == BL_PC ){
		// >二刀流の左右ダメージ計算誰かやってくれぇぇぇぇえええ！
		// >map_session_data に左手ダメージ(atk,atk2)追加して
		// >pc_calcstatus()でやるべきかな？
		// map_session_data に左手武器(atk,atk2,ele,star,atkmods)追加して
		// pc_calcstatus()でデータを入力しています

		// 右手、左手修練の適用
		if(((item_id = pc_checkequip(sd,32)) !=-1)	// 左手用か?
		&&(itemdb_type(item_id)==4)&&(itemdb_look(item_id)!=16)
		&&(sd->status.class==12)&&(pc_checkskill(sd,AS_LEFT)!=-1)) {// 二刀流か?
			// 右手修練(60% ～ 100%) 右手全般
			skill = pc_checkskill(sd,AS_RIGHT);
			damage = damage * (50 + (skill * 10))/100;
			// 左手修練(40% ～ 80%) 左手全般
			skill = pc_checkskill(sd,AS_LEFT);
			damage2 = damage2 * (30 + (skill * 10))/100;
			if(damage2 < 1)damage2 = -1;
		}else{
			damage2 = 0;
		}
		// 右手,短剣のみ
		if(da == 1) { //ダブルアタックが発動しているか
 			div_ = 2;
			damage <<= 1;
			type = 0x08;
		}

		item_id = pc_checkequip(sd,34);		// 両 手用か？
		if(item_id != -1) {
			if(itemdb_look(item_id) == 16) {
				// カタール追撃ダメージ
				skill = pc_checkskill(sd,TF_MISS);
				damage2 = damage * (1 + (skill * 2))/100;
				if(damage2 < 1)damage2 = -1;
			}
		}
	}

	// インベナム修正
	if(skill_num==TF_POISON){
		damage+=damage*15*skill_lv/100;
	}

#if 0
	// ダブルストレイフィング
	if(sd->status.weapon == 0x0b) {
		if((skill = pc_checkskill(sd,AR_DOUBLESTRAFE)) > 0) {
			div_ = 2;
			damage = damage * (3+(skill*0.5));
		}
	}
	// ソニックブロー
	if(sd->status.weapon == 0x10) {
		if((skill = pc_checkskill(sd,AS_SONICBLOW)) > 0) {
			div_ = skill;
			damage = damage * (3+(skill*0.5));
		}
	}
#endif

	if(src->type==BL_PC) {			// RoVeRT
		if (pc_check_equip_dcard(sd,4115) && (rand()%100 < 3)) {
			int hp = damage * 15/100;
			pc_heal(sd, hp,0);	// Hunter Fly - 3% chance to recover 15% of damage as HP
		}


		if (pc_check_equip_dcard(sd,4134) && (rand()%100 < 3)) {
			int sp = damage * 5/100;
			pc_heal(sd,0, sp);	// Dracula_Card	- 3% chance to recover 5% of damage as SP
		}

		if ((item_id = pc_checkequip(sd,2)) != -1) {
			if ((item_id==1132 || item_id==1526) && !mob_db[tmd->class].mexp && rand()%100 < 3) {	// Edge & Slash - Give a Sudden Death
				if (target->type==BL_PC)
					damage = sd->status.hp;
				else
					damage = tmd->hp;
			}

			if (item_id==1140 && target->type==BL_MOB && mob_db[tmd->class].mexp)	// Byeollungum - Gives double damage to Boss
				damage *=2;
		}
	}
	if(target->type==BL_PC) {		// RoVeRT
		if (pc_check_equip_dcard(tsd,4135) && (rand()%100 < 30)) {
			damage = 0;	// Orc_Lord_Card - 30% of Physical attacks are reflected
			damage2= 0;
		}

		if (pc_check_equip_dcard(tsd,4163) && (rand()%100 < 4)) {
			damage *=3;	// Executioner Card - 4% chance of +300% damage
		}

		if (pc_check_equip_dcard(tsd,4164) && (rand()%100 < 10)) {
			damage = 0;	// Anolian Card - AddMeleeAttackReflect 10
			damage2= 0;
		}
		if ((item_id=pc_checkequip(tsd,256)) != -1 )
			if ( itemdb_look(item_id) ==  30) {
				damage = 0;	// Golden Gear - 100% of Physical attacks are reflected
				damage2= 0;
			}
	}

	// 完全回避の判定
	if( skill_num==0 && tsd!=NULL && rand()%100 < tsd->flee2){
		damage=0;
		type=0x0b;
	}

#if 0	//お遊びで追加したコードなので目障りなら消してください
	// モンスターの完全回避
	if( skill_num==0 && tmd!=NULL && battle_config.enemy_critical 
	 && rand()%1000<battle_get_luk(target)+1 ){
		damage=0;
		type=0x0b;
	}
#endif
	
	if(damage2<1)		// ダメージ最終修正
		damage=battle_calc_damage(target,damage,flag);
	else if(damage==0)	// 右手がミス？
		damage2=battle_calc_damage(target,damage2,flag);
	else{	// 両 手/カタールの場合はちょっと計算ややこしい
		int d1=damage+damage2,d2=damage2;
		damage=battle_calc_damage(target,damage+damage2,flag);
		damage2=damage*d2/d1;
		if(damage2<=0)damage2=-1;
		damage-=damage2;
	}

	if(def1 >= 10000) {
		if(damage > 0)
			damage = 1;
		if(damage2 > 0)
			damage2 = 1;
	}
	if(battle_config.skill_min_damage) {
		if(div_ < 255) {
			if(damage > 0 && damage < div_)
				damage = div_;
		}
		else if(damage > 0 && damage < 3)
			damage = 3;
	}

	wd.damage=damage;
	wd.damage2=damage2;
	wd.type=type;
	wd.div_=div_;
	wd.amotion=battle_get_amotion(src);
	wd.dmotion=battle_get_dmotion(target);
	wd.blewcount=blewcount;

	return wd;
}

/*==========================================
 * 魔法ダメージ計算
 *------------------------------------------
 */
struct Damage  battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	int int_=battle_get_int(bl);
//	int t_int=battle_get_int(target);
	int mdef1=battle_get_mdef(target);
	int mdef2=battle_get_mdef2(target);
	int matk1,matk2,damage=0,div_=1,blewcount=0;
	struct Damage md;
	int aflag;
	int normalmagic_flag=1;
	struct map_session_data *sd=NULL;

	matk1=int_+(int_/5)*(int_/5);
	matk2=int_+(int_/7)*(int_/7);

#define MATK_FIX( a,b ) { matk1=matk1*(a)/(b); matk2=matk2*(a)/(b); }
	
	if( bl->type==BL_PC ){
		sd=(struct map_session_data *)bl;
		matk1=sd->matk1;
		matk2=sd->matk2;
	
		if(	itemdb_look(pc_checkequip(sd,2))==0x0a)
			// 杖によるMATK15％アップ
			MATK_FIX(115,100);
	}
	
	aflag=BF_MAGIC|BF_LONG|BF_SKILL;
	
	switch(skill_num){	// 基本ダメージ計算(スキルごとに処理)
				// ヒールor聖体
	case AL_HEAL:
	case PR_BENEDICTIO:
		damage = skill_calc_heal(bl,skill_lv)/2;
		normalmagic_flag=0;
		break;
	case PR_SANCTUARY:	// サンクチュアリ
		damage = (skill_lv>6)?388:skill_lv*50;
		normalmagic_flag=0;
		blewcount=3|0x10000;
		break;
	case ALL_RESURRECTION:
	case PR_TURNUNDEAD:	// 攻撃リザレクションとターンアンデッド
		if( battle_get_elem_type(target)==9){
			int hp = 0, mhp = 0, thres = 0;
			hp = battle_get_hp(target);
			mhp = battle_get_max_hp(target);
			thres = (skill_lv * 20) + battle_get_luk(bl)+
					battle_get_int(bl) + battle_get_lv(bl)+
					((200 - hp * 200 / mhp));
//			printf("ターンアンデッド！ 確率%d ‰(千分率)\n", thres);
			if(rand()%1000 < thres && !(battle_get_mexp(target)))	// 成功
				damage = hp;
			else					// 失敗
				damage = battle_get_lv(bl) + battle_get_int(bl) + skill_lv * 10;
		}
		normalmagic_flag=0;
		break;

	case MG_NAPALMBEAT:	// ナパームビート（分散計算込み）
		MATK_FIX(70+ skill_lv*10,100);
		if(flag>0){
			MATK_FIX(1,flag);
		}else
			printf("battle_calc_magic_attack(): napam enemy count=0 !\n");
		break;
	case MG_FIREBALL:	// ファイヤーボール
		{
			const int drate[]={100,90,70};
			if(flag>2)
				matk1=matk2=0;
			else
				MATK_FIX( (95+skill_lv*5)*drate[flag] ,10000 );
		}
		break;
	case MG_FIREWALL:	// ファイヤーウォール
		{
			int ele=battle_get_elem_type(target);
			if( ele!=3 && ele!=9 )
				blewcount=2|0x10000;
			MATK_FIX( 1,2 );
		}
		break;
	case MG_THUNDERSTORM:	// サンダーストーム
		MATK_FIX( 80,100 );
		break;
	case MG_FROSTDIVER:	// フロストダイバ
		MATK_FIX( 100+skill_lv*10, 100);
		break;
	case WZ_FIREPILLAR:	// ファイヤーピラー
		mdef1=mdef2=0;	// MDEF無視
		MATK_FIX( 1,5 );
		matk1+=50;
		matk2+=50;
		break;
	case WZ_METEOR:
		MATK_FIX( skill_lv*30+80, 100 );
		break;
	case WZ_JUPITEL:	// ユピテルサンダー
		blewcount=(skill_lv>6)?6:skill_lv;
		break;
	case WZ_VERMILION:	// ロードオブバーミリオン
		MATK_FIX( skill_lv*20+80, 100 );
		break;
	case WZ_WATERBALL:	// ウォーターボール
		matk1+= skill_lv*30;
		matk2+= skill_lv*30;
		break;
	case WZ_STORMGUST:	// ストームガスト
		MATK_FIX( skill_lv*40+100 ,100 );
		blewcount=2|0x10000;
		break;
	case AL_HOLYLIGHT:	// ホーリーライト
		MATK_FIX( 125,100 );
		break;
	
	}
	
	if(normalmagic_flag){	// 一般魔法ダメージ計算
		if(matk1>matk2)
			damage= matk2+rand()%(matk1-matk2);
		else
			damage= matk2;
		damage= (damage*(100-mdef1))/100 - mdef2;

		if(damage<1)
			damage=1;
	}
	

	damage=battle_attr_fix(damage,		// 属 性修正
		 skill_get_pl(skill_num), battle_get_element(target) );

	div_=skill_get_num( skill_num,skill_lv );
	
	if(div_>1 && skill_num != WZ_VERMILION)
		damage*=div_;
	

	if( target->type==BL_PC){
		if (pc_check_equip_dcard((struct map_session_data *)target,4128) )
			damage=0;	// 黄 金蟲カード（魔法ダメージ０）

		if (pc_check_equip_dcard((struct map_session_data *)target,4146) && (rand()%100 < 30))		// RoVeRT
			damage=0;	// Maya Card 30% chance refelect magic
	}

	damage=battle_calc_damage(target,damage,aflag);	// 最終修正

	if(mdef1 >= 10000 && damage > 0) {
		damage = 1;
	}
	if(battle_config.skill_min_damage) {
		if(damage > 0 && damage < div_)
			damage = div_;
	}

	md.damage=damage;
	md.div_=div_;
	md.amotion=battle_get_amotion(bl);
	md.dmotion=battle_get_dmotion(target);
	md.damage2=0;
	md.type=0;
	md.blewcount=blewcount;

	return md;
}

/*==========================================
 * その他ダメージ計算
 *------------------------------------------
 */
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	int int_=battle_get_int(bl);
//	int luk=battle_get_luk(bl);
	int dex=battle_get_dex(bl);
	int skill;
	struct map_session_data *sd=NULL;
	int damage=0,div_=1,blewcount=0;
	struct Damage md;
	int damagefix=1;
	
	int aflag=BF_MISC|BF_LONG|BF_SKILL;

	if( bl->type==BL_PC )
		sd=(struct map_session_data *)bl;

	switch(skill_num){

	case HT_LANDMINE:	// ランドマイン
		damage=skill_lv*(dex+75)*(100+int_)/100;
		break;

	case HT_BLASTMINE:	// ブラストマイン
		damage=skill_lv*(dex/2+50)*(100+int_)/100;
		break;
	
	case HT_CLAYMORETRAP:	// クレイモアートラップ
		damage=skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// ブリッツビート
		if( sd==NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill=0;
		damage=(dex/10+int_/2+skill*3+40)*2;
		if(flag > 1)
			damage /= flag;
		break;

	case TF_THROWSTONE:	// 石投げ
		damage=30;
		damagefix=0;
		break;
	
	case NPC_SELFDESTRUCTION:	// 自爆
		damage=battle_get_hp(bl)-((bl==target)?1:0);
		damagefix=0;
		break;
	
	case NPC_SMOKING:	// タバコを吸う
		damage=3;
		damagefix=0;
		break;
	}

	if(damagefix){
		if(damage<1)
			damage=1;
	
		damage=battle_attr_fix(damage,		// 属 性修正
			 skill_get_pl(skill_num), battle_get_element(target) );
	}
	
	div_=skill_get_num( skill_num,skill_lv );
	if(div_>1)
		damage*=div_;
	
	damage=battle_calc_damage(target,damage,aflag);	// 最終修正

	if((damage > 0 && damage < div_) || (battle_get_def(target) >= 10000 && battle_get_mdef(target) >= 10000 && damage > 0)) {
		damage = div_;
	}

	md.damage=damage;
	md.div_=div_;
	md.amotion=battle_get_amotion(bl);
	md.dmotion=battle_get_dmotion(target);
	md.damage2=0;
	md.type=0;
	md.blewcount=blewcount;
	return md;

}
/*==========================================
 * ダメージ計算一括処理用
 *------------------------------------------
 */
struct Damage battle_calc_attack(	int attack_type,
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	struct Damage d;
	switch(attack_type){
	case BF_WEAPON:
		return battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
	case BF_MAGIC:
		return battle_calc_magic_attack(bl,target,skill_num,skill_lv,flag);
	case BF_MISC:
		return battle_calc_misc_attack(bl,target,skill_num,skill_lv,flag);
	default:
		printf("battle_calc_attack: unknwon attack type ! %d\n",attack_type);
		break;
	}
	return d;
}
/*==========================================
 * 通常攻撃処理まとめ
 *------------------------------------------
 */
int battle_weapon_attack( struct block_list *src,struct block_list *target,
	 unsigned int tick,int flag)
{
	struct map_session_data *sd=NULL;
	sd = (struct map_session_data *)src;

	if(src->prev == NULL || target->prev == NULL)
		return 0;
	if(src->type == BL_PC && pc_isdead((struct map_session_data *)src))
		return 0;
	if(target->type == BL_PC && pc_isdead((struct map_session_data *)target))
		return 0;

	// 弓の場合
	if(itemdb_look(pc_checkequip(sd,2)) == 11) {
//		// 矢が装備されていない
		if(pc_checkequip(sd,0x8000) == -1){
			clif_arrow_fail(sd,0);
			pc_stopattack(sd);			// RoVeRT
			return 0;
		}
	}

	if(battle_check_target(src,target,BCT_ENEMY) > 0 &&
		battle_check_range(src,target->x,target->y,0)){
		// 攻撃対象となりうるので攻撃
		struct Damage wd;
		wd=battle_calc_weapon_attack(src,target,0,0,0);
		if (wd.div_ == 255 && src->type == BL_PC)	{ //三段掌
			sd->skill_old = 0;
			sd->triple_delay = 1000 - 4 * battle_get_agi(src) - 2 *  battle_get_dex(src);
			if (sd->triple_delay < sd->aspd*2) sd->triple_delay = sd->aspd*2;
			if (pc_checkskill(sd, MO_COMBOFINISH)) sd->triple_delay += 300;
			clif_status_change(src, 0x59, 1);
			clif_combo_delay(src, sd->triple_delay);
			clif_skill_damage(src , target , tick , wd.amotion , wd.dmotion , 
				wd.damage , 3 , MO_TRIPLEATTACK, pc_checkskill(sd,MO_TRIPLEATTACK) , wd.type );
		}
		else
			clif_damage(src,target,tick, wd.amotion, wd.dmotion, 
				wd.damage, wd.div_ , wd.type, wd.damage2);
		//二刀流左手とカタール追撃のミス表示(無理やり～)
		if(wd.damage2==-1){wd.damage2=0;clif_damage(src,target,tick+200, wd.amotion, wd.dmotion,0, 1, 0, 0);}
		map_freeblock_lock();

		battle_damage(src,target,(wd.damage+wd.damage2));
		if((wd.damage+wd.damage2)>0)
			skill_additional_effect(src,target,0,0,tick);

		map_freeblock_unlock();
	}
	return 0;
}

/*==========================================
 * 敵味方判定(1=肯定,0=否定,-1=エラー)
 * flag&0xf0000 = 0x00000:敵じゃないか判定（ret:1＝敵ではない）
 *				= 0x10000:パーティー判定（ret:1=パーティーメンバ)
 *				= 0x20000:全て(ret:1=敵味方両方)
 *				= 0x40000:敵か判定(ret:1=敵)
 *				= 0x50000:パーティーじゃないか判定(ret:1=パーティでない)
 *------------------------------------------
 */
int battle_check_target( struct block_list *src, struct block_list *target,int flag)
{
	int s_p,s_g,t_p,t_g;
	struct block_list *ss=src;

	if( flag&0x40000 ){	// 反転フラグ
		int ret=battle_check_target(src,target,flag&0x30000);
		if(ret!=-1)
			return !ret;
		return -1;
	}

	if( flag&0x20000 ){
		if( target->type==BL_MOB || target->type==BL_PC )
			return 1;
		else
			return -1;
	}
	
//	if( target->type==BL_SKILL )	// 対 象がスキルユニットなら無条件肯定
//		return 0;

				// スキルユニットの場合、親を求める
	if( src->type==BL_SKILL)
		if( (ss=map_id2bl( ((struct skill_unit *)src)->group->src_id))==NULL )
			return -1;

	if( src==target || ss==target )	// 同じなら肯定
		return 1;

	if( src->prev==NULL ||	// 死んでるならエラー
		(src->type==BL_PC && pc_isdead((struct map_session_data *)src) ) )
		return -1;

	if( (ss->type == BL_PC && target->type==BL_MOB) ||
		(ss->type == BL_MOB && target->type==BL_PC) )
		return 0;	// PCvsMOBなら否定

	s_p=battle_get_party_id(src);
	s_g=battle_get_guild_id(src);

	t_p=battle_get_party_id(target);
	t_g=battle_get_guild_id(target);

	if( s_p<0 || t_p<0 )	// PCとMOBじゃない
		return -1;

	if(flag&0x10000 && s_p>0 && s_p == t_p )	// 同じパーティなら肯定（味方）
		return 1;

	if(flag&0x10000 && s_p>0 && s_p != t_p )		// パーティ検索なら同じパーティじゃない時点で否定
		return 0;

	if(ss->type == BL_MOB && s_g>0 && s_g == t_g )	// 同じギルド/mobクラスなら肯定（味方）
		return 1;

	if( ss->type==BL_PC && target->type==BL_PC) { // 両 方PVPモードなら否定（敵）
		if(map[src->m].flag.pvp) {
			if(map[src->m].flag.pvp_noparty && s_p > 0 && s_p == t_p)
				return 1;
			else if(map[src->m].flag.pvp_noguild && s_g > 0 && s_g == t_g)
				return 1;
			return 0;
		}
		if(map[src->m].flag.gvg) {
			if(s_g > 0 && s_g == t_g)
				return 1;
			else if(map[src->m].flag.gvg_noparty && s_p > 0 && s_p == t_p)
				return 1;
			return 0;
		}
	}

	return 1;	// 該当しないので無関係人物（まあ敵じゃないので味方）
}
/*==========================================
 * 射程判定
 *------------------------------------------
 */
int battle_check_range(struct block_list *src,int x,int y,int range)
{
	int dx=abs(x-src->x),dy=abs(y-src->y);
	struct walkpath_data wpd;
	if( range>0 && range < ((dx>dy)?dx:dy) )	// 遠すぎる
		return 0;

	if( src->x==x && src->y==y )	// 同じマス
		return 1;

	// 障害物判定
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(&wpd,src->m,src->x,src->y,x,y,0x10001)!=-1)?1:0;
}

/*==========================================
 * 設定ファイル読み込み用（フラグ）
 *------------------------------------------
 */
int battle_config_switch(const char *str)
{
	if(strcmpi(str,"on")==0 || strcmpi(str,"yes")==0)
		return 1;
	if(strcmpi(str,"off")==0 || strcmpi(str,"no")==0)
		return 0;
	return atoi(str);
}
/*==========================================
 * 設定ファイルを読み込む
 *------------------------------------------
 */
int battle_config_read(const char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;

	battle_config.warp_point_debug=0;
	battle_config.enemy_critical=1;
	battle_config.cast_rate=100;
	battle_config.delay_rate=100;
	battle_config.delay_dependon_dex=0;
	battle_config.sdelay_attack_enable=0;
	battle_config.pc_damage_delay=1;
	battle_config.defnotenemy=1;
	battle_config.attr_recover=1;
	battle_config.flooritem_lifetime=LIFETIME_FLOORITEM*1000;
	battle_config.item_rate=100;
	battle_config.base_exp_rate=100;
	battle_config.job_exp_rate=100;
	battle_config.death_penalty_type=0;
	battle_config.death_penalty_base=0;
	battle_config.death_penalty_job=0;
	battle_config.restart_hp_rate=0;
	battle_config.restart_sp_rate=0;
	battle_config.mvp_item_rate=100;
	battle_config.mvp_exp_rate=100;
	battle_config.mvp_hp_rate=100;
	battle_config.atc_gmonly=0;
	battle_config.gm_allskill=0;
	battle_config.wp_rate=100;
	battle_config.monster_active_enable=1;
	battle_config.monster_loot_type=0;
	battle_config.mob_skill_use=1;
	battle_config.mob_count_rate=100;
	battle_config.quest_skill_learn=0;
	battle_config.quest_skill_reset=1;
	battle_config.basic_skill_check=1;
	battle_config.guild_emperium_check=1;
	battle_config.ghost_time=5000;
	battle_config.pet_catch_rate=100;
	battle_config.pet_rename=0;
	battle_config.pet_friendly_rate=100;
	battle_config.pet_hungry_delay_rate=100;
	battle_config.skill_min_damage=0;
	battle_config.sanctuary_type=0;
	battle_config.finger_offensive_type=0;
	battle_config.heal_exp=0;
	battle_config.shop_exp=0;
	battle_config.asuradelay=300;
	battle_config.equip_modifydisplay=0;
	fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)){
		const struct {
			char str[32];
			int *val;
		} data[] ={
			{ "warp_point_debug", &battle_config.warp_point_debug },
			{	"enemy_critical",		&battle_config.enemy_critical		},
			{	"casting_rate",			&battle_config.cast_rate			},
			{	"delay_rate",			&battle_config.delay_rate			},
			{	"delay_dependon_dex",	&battle_config.delay_dependon_dex	},
			{ "skill_delay_attack_enable", &battle_config.sdelay_attack_enable },
			{ "player_damage_delay", &battle_config.pc_damage_delay },
			{	"defunit_not_enemy",	&battle_config.defnotenemy			},
			{	"attribute_recover",	&battle_config.attr_recover	},
			{	"flooritem_lifetime",			&battle_config.flooritem_lifetime			},
			{	"item_rate",			&battle_config.item_rate			},
			{	"base_exp_rate",		&battle_config.base_exp_rate		},
			{	"job_exp_rate",			&battle_config.job_exp_rate			},
			{	"death_penalty_type",			&battle_config.death_penalty_type			},
			{	"death_penalty_base",			&battle_config.death_penalty_base			},
			{	"death_penalty_job",			&battle_config.death_penalty_job			},
			{ "restart_hp_rate", &battle_config.restart_hp_rate },
			{ "restart_sp_rate", &battle_config.restart_sp_rate },
			{	"mvp_hp_rate",			&battle_config.mvp_hp_rate			},
			{	"mvp_item_rate",		&battle_config.mvp_item_rate		},
			{	"mvp_exp_rate",			&battle_config.mvp_exp_rate			},
			{	"atcommand_gm_only",	&battle_config.atc_gmonly			},
			{	"gm_all_skill",			&battle_config.gm_allskill			},
			{	"weapon_produce_rate",	&battle_config.wp_rate				},
			{ "monster_active_enable", &battle_config.monster_active_enable },
			{	"monster_loot_type",	&battle_config.monster_loot_type	},
			{	"mob_skill_use",		&battle_config.mob_skill_use		},
			{	"mob_count_rate",		&battle_config.mob_count_rate		},
			{ "quest_skill_learn", &battle_config.quest_skill_learn },
			{ "quest_skill_reset", &battle_config.quest_skill_reset },
			{ "basic_skill_check", &battle_config.basic_skill_check },
			{ "guild_emperium_check", &battle_config.guild_emperium_check},
			{	"ghost_time",		&battle_config.ghost_time		},
			{	"pet_catch_rate",		&battle_config.pet_catch_rate		},
			{	"pet_rename",			&battle_config.pet_rename			},
			{ "pet_friendly_rate", &battle_config.pet_friendly_rate	},
			{ "pet_hungry_delay_rate", &battle_config.pet_hungry_delay_rate	},
			{ "skill_min_damage", &battle_config.skill_min_damage },
			{ "sanctuary_type", &battle_config.sanctuary_type },
			{ "finger_offensive_type", &battle_config.finger_offensive_type	},
			{ "heal_exp", &battle_config.heal_exp	},
			{ "shop_exp", &battle_config.shop_exp	},
			{ "asuradelay", &battle_config.asuradelay},
			{ "equip_modifydisplay",	&battle_config.equip_modifydisplay	},
		};
		
		if(line[0] == '/' && line[1] == '/')
			continue;
		i=sscanf(line,"%[^:]:%s",w1,w2);
		if(i!=2)
			continue;
		for(i=0;i<sizeof(data)/(sizeof(data[0]));i++)
			if(strcmpi(w1,data[i].str)==0)
				*data[i].val=battle_config_switch(w2);
	}
	fclose(fp);

	if(battle_config.flooritem_lifetime < 1000)
		battle_config.flooritem_lifetime = LIFETIME_FLOORITEM*1000;
	if(battle_config.restart_hp_rate < 0)
		battle_config.restart_hp_rate = 0;
	else if(battle_config.restart_hp_rate > 100)
		battle_config.restart_hp_rate = 100;
	if(battle_config.restart_sp_rate < 0)
		battle_config.restart_sp_rate = 0;
	else if(battle_config.restart_sp_rate > 100)
		battle_config.restart_sp_rate = 100;

	add_timer_func_list(battle_delay_damage_sub,"battle_delay_damage_sub");

	return 0;
}
