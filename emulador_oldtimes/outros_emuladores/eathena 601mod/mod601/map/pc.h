// $Id: pc.h,v 1.12 2003/06/26 05:31:23 lemit Exp $

#ifndef _PC_H_
#define _PC_H_

#include "map.h"

#define pc_setdead(sd) ((sd)->state.dead_sit = 1)
#define pc_setsit(sd) ((sd)->state.dead_sit = 2)
#define pc_setstand(sd) ((sd)->state.dead_sit = 0)
#define pc_isdead(sd) ((sd)->state.dead_sit == 1)
#define pc_issit(sd) ((sd)->state.dead_sit == 2)
#define pc_setdir(sd,b,h) ((sd)->dir = (b) ,(sd)->head_dir = (h) )
#define pc_setchatid(sd,n) ((sd)->chatID = n)
#define pc_ishiding(sd) ((sd)->status.option&0x0006)
#define pc_iscarton(sd) (((sd)->status.option&0x0008)||(0x0080<=(sd)->status.option&&(sd)->status.option<0x0800))
#define pc_isfalcon(sd) ((sd)->status.option&0x0010)
#define pc_isriding(sd) ((sd)->status.option&0x0020)
#define pc_is50overweight(sd) (sd->weight*2 >= sd->max_weight) 
#define pc_is90overweight(sd) (sd->weight*10 >= sd->max_weight*9)

void pc_set_gm_account_fname(char *str);
int pc_isGM(struct map_session_data *sd);

int pc_setrestartvalue(struct map_session_data *sd,int type);
int pc_makesavestatus(struct map_session_data *);
int pc_setnewpc(struct map_session_data*,int,int,int,int,int,int);
int pc_authok(int,struct mmo_charstatus *);
int pc_authfail(int);

int pc_checkskill(struct map_session_data *sd,int skill_id);
int pc_checkallowskill(struct map_session_data *sd,int nameid);
int pc_checkequip(struct map_session_data *sd,int pos);

int pc_checkoverhp(struct map_session_data*);
int pc_checkoversp(struct map_session_data*);

int pc_can_reach(struct map_session_data*,int,int);
int pc_walktoxy(struct map_session_data*,int,int);
int pc_stop_walking(struct map_session_data*,int);
int pc_movepos(struct map_session_data*,int,int);
int pc_setpos(struct map_session_data*,char*,int,int,int);
int pc_setsavepoint(struct map_session_data*,char*,int,int);
int pc_randomwarp(struct map_session_data *sd,int type);
int pc_memo(struct map_session_data *sd,int i);

int pc_checkadditem(struct map_session_data*,int,int);
int pc_inventoryblank(struct map_session_data*);
int pc_search_inventory(struct map_session_data *sd,int item_id);
int pc_payzeny(struct map_session_data*,int);
int pc_additem(struct map_session_data*,struct item*,int);
int pc_getzeny(struct map_session_data*,int);
int pc_delitem(struct map_session_data*,int,int,int);

int pc_cart_additem(struct map_session_data *sd,struct item *item_data,int amount);
int pc_cart_delitem(struct map_session_data *sd,int n,int amount);
int pc_putitemtocart(struct map_session_data *sd,int idx,int amount);
int pc_getitemfromcart(struct map_session_data *sd,int idx,int amount);

int pc_takeitem(struct map_session_data*,struct flooritem_data*);
int pc_dropitem(struct map_session_data*,int,int);

int pc_checkweighticon(struct map_session_data *sd);

int pc_calcstatus(struct map_session_data*,int);
int pc_bonus(struct map_session_data*,int,int);
int pc_bonus2(struct map_session_data *sd,int,int,int);
int pc_skill(struct map_session_data*,int,int,int);

int pc_insert_card(struct map_session_data *sd,int idx_card,int idx_equip);
int pc_check_equip_card(struct map_session_data *sd,int cardid);
int pc_check_equip_wcard(struct map_session_data *sd,int cardid);
int pc_check_equip_dcard(struct map_session_data *sd,int cardid);

int pc_item_identify(struct map_session_data *sd,int idx);
int pc_steal_item(struct map_session_data *sd,struct block_list *bl);
int pc_steal_coin(struct map_session_data *sd,struct block_list *bl);

int pc_modifybuyvalue(struct map_session_data*,int);
int pc_modifysellvalue(struct map_session_data*,int);

int pc_attack(struct map_session_data*,int,int);
int pc_stopattack(struct map_session_data*);

int pc_checkbaselevelup(struct map_session_data *sd);
int pc_checkjoblevelup(struct map_session_data *sd);
int pc_gainexp(struct map_session_data*,int,int);
int pc_nextbaseexp(struct map_session_data *);
int pc_nextjobexp(struct map_session_data *);
int pc_need_status_point(struct map_session_data *,int);
int pc_statusup(struct map_session_data*,int);
int pc_skillup(struct map_session_data*,int);
int pc_resetstate(struct map_session_data*);
int pc_resetskill(struct map_session_data*);
int pc_equipitem(struct map_session_data*,int,int);
int pc_unequipitem(struct map_session_data*,int);
int pc_checkitem(struct map_session_data*);
int pc_useitem(struct map_session_data*,int);

int pc_damage(struct block_list *,struct map_session_data*,int);
int pc_heal(struct map_session_data *,int,int);
int pc_itemheal(struct map_session_data *sd,int hp,int sp);
int pc_percentheal(struct map_session_data *sd,int,int);
int pc_jobchange(struct map_session_data *,int);
int pc_setoption(struct map_session_data *,int);
int pc_setcart(struct map_session_data *sd,int type);
int pc_setfalcon(struct map_session_data *sd);
int pc_setriding(struct map_session_data *sd);
int pc_changelook(struct map_session_data *,int,int);
int pc_equiplookall(struct map_session_data *sd);

int pc_readparam(struct map_session_data*,int);
int pc_setparam(struct map_session_data*,int,int);
int pc_readreg(struct map_session_data*,int);
int pc_setreg(struct map_session_data*,int,int);
int pc_readglobalreg(struct map_session_data*,char*);
int pc_setglobalreg(struct map_session_data*,char*,int);
int pc_percentrefinery(struct map_session_data *sd,struct item *item);
int pc_equipitemindex(struct map_session_data *sd,int pos);


int pc_addeventtimer(struct map_session_data *sd,int tick,const char *name);
int pc_deleventtimer(struct map_session_data *sd,const char *name);
int pc_cleareventtimer(struct map_session_data *sd);
int pc_addeventtimercount(struct map_session_data *sd,const char *name,int tick);

int pc_calc_pvprank(struct map_session_data *sd);
int pc_calc_pvprank_timer(int tid,unsigned int tick,int id,int data);


int pc_read_gm_account(void);
int pc_setghosttimer(struct map_session_data *sd,int);
int pc_delghosttimer(struct map_session_data *sd);
int pc_addspiritball(struct map_session_data *sd,int,int);
int pc_delspiritball(struct map_session_data *sd,int,int);

int do_init_pc(void);

enum {ADDITEM_EXIST,ADDITEM_NEW,ADDITEM_OVERAMOUNT};

#endif

