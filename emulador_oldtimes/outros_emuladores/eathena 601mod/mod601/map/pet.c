#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "timer.h"
#include "pc.h"
#include "map.h"
#include "intif.h"
#include "clif.h"
#include "socket.h"
#include "pet.h"
#include "itemdb.h"
#include "battle.h"
#include "mob.h"
#include "npc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct pet_db pet_db[MAX_PET_DB];

static int dirx[8]={0,-1,-1,-1,0,1,1,1};
static int diry[8]={1,1,0,-1,-1,-1,0,1};

static int pet_timer(int tid,unsigned int tick,int id,int data);

static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

static int calc_next_walk_step(struct npc_data *nd)
{
	if(nd->walkpath.path_pos>=nd->walkpath.path_len)
		return -1;
	if(nd->walkpath.path[nd->walkpath.path_pos]&1)
		return nd->speed*14/10;
	return nd->speed;
}

static int pet_can_reach(struct npc_data *nd,int x,int y)
{
	struct walkpath_data wpd;

	if( nd->bl.x==x && nd->bl.y==y )	// 同じマス
		return 1;

	// 障害物判定
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(&wpd,nd->bl.m,nd->bl.x,nd->bl.y,x,y,0)!=-1)?1:0;
}

int pet_calc_pos(struct npc_data *nd,int dir)
{
	int x,y,dx,dy;
	int i,j=0,k;

	if(dir >= 0 && dir < 8) {
		dx = -dirx[dir]*2;
		dy = -diry[dir]*2;
		x = nd->to_x + dx;
		y = nd->to_y + dy;
		if(!(j=pet_can_reach(nd,x,y))) {
			if(dx > 0) x--;
			else if(dx < 0) x++;
			if(dy > 0) y--;
			else if(dy < 0) y++;
			if(!(j=pet_can_reach(nd,x,y))) {
				for(i=0;i<12;i++) {
					k = rand()%8;
					dx = -dirx[k]*2;
					dy = -diry[k]*2;
					x = nd->to_x + dx;
					y = nd->to_y + dy;
					if((j=pet_can_reach(nd,x,y)))
						break;
					else {
						if(dx > 0) x--;
						else if(dx < 0) x++;
						if(dy > 0) y--;
						else if(dy < 0) y++;
						if((j=pet_can_reach(nd,x,y)))
							break;
					}
				}
				if(!j) {
					x = nd->to_x;
					y = nd->to_y;
					if(!pet_can_reach(nd,x,y))
						return 1;
				}
			}
		}
	}
	else
		return 1;

	nd->to_x = x;
	nd->to_y = y;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pet_walk(struct npc_data *nd,unsigned int tick,int data)
{
	int moveblock;
	int i;
	int x,y,dx,dy;

	nd->state.state=MS_IDLE;
	if(nd->walkpath.path_pos>=nd->walkpath.path_len || nd->walkpath.path_pos!=data)
		return 0;

	if(nd->walkpath.path[nd->walkpath.path_pos]>=8)
		return 1;

	nd->dir=nd->walkpath.path[nd->walkpath.path_pos];
	x = nd->bl.x;
	y = nd->bl.y;
	dx = dirx[nd->dir];
	dy = diry[nd->dir];
	nd->walkpath.path_pos++;
	moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

	nd->state.state=MS_WALK;
	map_foreachinmovearea(clif_petoutsight,nd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,nd);

	x += dx;
	y += dy;

	if(moveblock) map_delblock(&nd->bl);
	nd->bl.x = x;
	nd->bl.y = y;
	if(moveblock) map_addblock(&nd->bl);

	map_foreachinmovearea(clif_petinsight,nd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,nd);
	nd->state.state=MS_IDLE;

	if((i=calc_next_walk_step(nd))>0){
		nd->timer=add_timer(tick+i,pet_timer,nd->bl.id,nd->walkpath.path_pos);
		nd->state.state=MS_WALK;

		if(nd->walkpath.path_pos>=nd->walkpath.path_len)
			clif_fixpetpos(nd);
	}
	return 0;
}

int pet_changestate(struct npc_data *nd,int state)
{
	int i;

	if(nd->timer != -1)
		delete_timer(nd->timer,pet_timer);
	nd->timer=-1;
	nd->state.state=state;

	switch(state) {
		case MS_WALK:
			if((i=calc_next_walk_step(nd))>0){
				i = i/2;
				if(i <= 0)
					i = 1;
				nd->timer=add_timer(gettick()+i,pet_timer,nd->bl.id,0);
			} else
				nd->state.state=MS_IDLE;
			break;
	}

	return 0;
}

static int pet_timer(int tid,unsigned int tick,int id,int data)
{
	struct npc_data *nd;

	nd=(struct npc_data*)map_id2bl(id);
	if(nd==NULL || nd->bl.type!=BL_PET)
		return 1;

	if(nd->timer != tid){
		printf("pet_timer %d != %d\n",nd->timer,tid);
		return 0;
	}
	nd->timer=-1;

	switch(nd->state.state){
		case MS_WALK:
			pet_walk(nd,tick,data);
			break;
		case MS_DELAY:
			pet_changestate(nd,MS_IDLE);
			break;
		default:
			printf("pet_timer : %d ?\n",nd->state.state);
	}

	return 0;
}

int pet_walktoxy(struct npc_data *nd,int x,int y,int easy,int dir)
{
	struct walkpath_data wpd;

	if(dir < 0) {
		if(path_search(&wpd,nd->bl.m,nd->bl.x,nd->bl.y,x,y,easy))
			return 1;
		nd->to_x=x;
		nd->to_y=y;

		memcpy(&nd->walkpath,&wpd,sizeof(wpd));
	}
	else {
		nd->to_x = x;
		nd->to_y = y;
		pet_calc_pos(nd,dir);
		if(path_search(&wpd,nd->bl.m,nd->bl.x,nd->bl.y,nd->to_x,nd->to_y,easy))
			return 1;
		memcpy(&nd->walkpath,&wpd,sizeof(wpd));
	}

	pet_changestate(nd,MS_WALK);
	clif_movepet(nd);
//	printf("walkstart\n");

	return 0;
}

int pet_stop_walking(struct map_session_data *sd,int dir)
{
	struct npc_data *nd;
	int dist;
//	printf("stop walking\n");
	nd = sd->pet_npcdata;
	dist=distance(sd->bl.x,sd->bl.y,nd->bl.x,nd->bl.y);
	if(dir >= 0 && sd->pet.intimate > 0) {
		if(dist < 3) {
			nd->walkpath.path_len=0;
			nd->to_x=nd->bl.x;
			nd->to_y=nd->bl.y;
			clif_fixpos(&nd->bl);
			pet_changestate(nd,MS_IDLE);
		}
		else {
			if(pet_walktoxy(nd,sd->bl.x,sd->bl.y,0,dir)) {
				nd->walkpath.path_len=0;
				nd->to_x=nd->bl.x;
				nd->to_y=nd->bl.y;
				clif_fixpos(&nd->bl);
				pet_changestate(nd,MS_IDLE);
			}
		}
	}
	else {
		nd->walkpath.path_len=0;
		nd->to_x=nd->bl.x;
		nd->to_y=nd->bl.y;
		clif_fixpos(&nd->bl);
		pet_changestate(nd,MS_IDLE);
	}

	return 0;
}

static int pet_hungry(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	int interval;

	sd=map_id2sd(id);
	if(sd==NULL)
		return 1;

	if(sd->pet_hungry_timer != tid){
		printf("pet_hungry_timer %d != %d\n",sd->pet_hungry_timer,tid);
		return 0;
	}
	sd->pet_hungry_timer = -1;
	if(!sd->status.pet_id || !sd->pet_npcdata || !sd->petDB)
		return 1;

	sd->pet.hungry--;
	if(sd->pet.hungry < 0) {
		sd->pet.hungry = 0;
		sd->pet.intimate--;
		if(sd->pet.intimate < 0)
			sd->pet.intimate = 0;
		clif_send_petdata(sd,1,sd->pet.intimate);
	}
	clif_send_petdata(sd,2,sd->pet.hungry);

	if(battle_config.pet_hungry_delay_rate != 100)
		interval = (sd->petDB->hungry_delay*battle_config.pet_hungry_delay_rate)/100;
	else
		interval = sd->petDB->hungry_delay;
	if(interval <= 0)
		interval = 1;
	sd->pet_hungry_timer = add_timer(tick+interval,pet_hungry,sd->bl.id,0);

	return 0;
}

int search_petDB_index(int key,int type)
{
	int i;

	for(i=0;i<MAX_PET_DB;i++) {
		if(pet_db[i].class <= 0)
			continue;
		switch(type) {
			case PET_CLASS:
				if(pet_db[i].class == key)
					return i;
				break;
			case PET_CATCH:
				if(pet_db[i].itemID == key)
					return i;
				break;
			case PET_EGG:
				if(pet_db[i].EggID == key)
					return i;
				break;
			case PET_EQUIP:
				if(pet_db[i].AcceID == key)
					return i;
				break;
			case PET_FOOD:
				if(pet_db[i].FoodID == key)
					return i;
				break;
			default:
				return -1;
		}
	}
	return -1;
}

int pet_hungry_timer_delete(struct map_session_data *sd)
{
	if(sd->pet_hungry_timer != -1) {
		delete_timer(sd->pet_hungry_timer,pet_hungry);
		sd->pet_hungry_timer = -1;
	}

	return 0;
}

int pet_remove_map(struct map_session_data *sd)
{
	if(sd->status.pet_id && sd->pet_npcdata) {
		pet_changestate(sd->pet_npcdata,MS_IDLE);
		if(sd->pet_hungry_timer != -1)
			pet_hungry_timer_delete(sd);
		clif_clearchar_area(&sd->pet_npcdata->bl,0);
		map_delblock(&sd->pet_npcdata->bl);
		map_deliddb(&sd->pet_npcdata->bl);
		map_freeblock(sd->pet_npcdata);
	}
	return 0;
}

int pet_performance(struct map_session_data *sd)
{
	pet_stop_walking(sd,-1);
	clif_pet_performance(sd->pet_npcdata,rand()%3 + 1);

	return 0;
}

int pet_return_egg(struct map_session_data *sd)
{
	struct item tmp_item;
	int flag;

	if(sd->status.pet_id && sd->pet_npcdata) {
		pet_remove_map(sd);
		sd->status.pet_id = 0;
		sd->pet_npcdata = NULL;

		sd->pet.incuvate = 1;
		intif_save_petdata(sd->status.account_id,&sd->pet);

		if(sd->petDB == NULL)
			return 1;
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.nameid = sd->petDB->EggID;
		tmp_item.identify = 1;
		tmp_item.card[0] = 0xff00;
		*((long *)(&tmp_item.card[2])) = sd->pet.pet_id;
		if((flag = pc_additem(sd,&tmp_item,1))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y);
		}

		sd->petDB = NULL;
	}

	return 0;
}

int pet_npc_init(struct map_session_data *sd)
{
	struct npc_data *nd;
	int i,interval;

	if(sd->status.account_id != sd->pet.account_id || sd->status.char_id != sd->pet.char_id ||
		sd->status.pet_id != sd->pet.pet_id) {
		sd->status.pet_id = 0;
		return 1;
	}

	i = search_petDB_index(sd->pet.class,PET_CLASS);
	if(i < 0) {
		sd->status.pet_id = 0;
		return 1;
	}
	sd->petDB = &pet_db[i];
	sd->pet_npcdata = nd = malloc(sizeof(struct npc_data));
	if(nd==NULL){
		printf("out of memory : pet_npc_init\n");
		return 1;
	}

	nd->n = 0;
	nd->bl.m = sd->bl.m;
	nd->bl.x = nd->to_x = sd->bl.x;
	nd->bl.y = nd->to_y = sd->bl.y;
	pet_calc_pos(sd->pet_npcdata,sd->dir);
	nd->bl.x = nd->to_x;
	nd->bl.y = nd->to_y;
	nd->bl.id = npc_get_new_npc_id();
	memcpy(nd->name,sd->pet.name,24);
	nd->class = sd->pet.class;
	nd->equip = sd->pet.equip;
	nd->dir = sd->dir;
	nd->speed = sd->petDB->speed;
	nd->bl.subtype = MONS;
	nd->bl.type = BL_PET;
	nd->state.state = MS_IDLE;
	nd->timer = -1;

	map_addiddb(&nd->bl);

	if(sd->pet_hungry_timer != -1)
		pet_hungry_timer_delete(sd);
	if(battle_config.pet_hungry_delay_rate != 100)
		interval = (sd->petDB->hungry_delay*battle_config.pet_hungry_delay_rate)/100;
	else
		interval = sd->petDB->hungry_delay;
	if(interval <= 0)
		interval = 1;
	sd->pet_hungry_timer = add_timer(gettick()+interval,pet_hungry,sd->bl.id,0);

	return 0;
}

int pet_birth_process(struct map_session_data *sd)
{
	if(sd->status.pet_id && sd->pet.incuvate == 1) {
		sd->status.pet_id = 0;
		return 1;
	}

	sd->pet.incuvate = 0;
	sd->pet.account_id = sd->status.account_id;
	sd->pet.char_id = sd->status.char_id;
	intif_save_petdata(sd->status.account_id,&sd->pet);
	sd->status.pet_id = sd->pet.pet_id;
	if(pet_npc_init(sd)) {
		sd->status.pet_id = 0;
		return 1;
	}

	map_addblock(&sd->pet_npcdata->bl);
	clif_spawnpet(sd->pet_npcdata);
	clif_send_petdata(sd,0,0);
	clif_send_petdata(sd,5,0x14);
	clif_pet_equip(sd->pet_npcdata,sd->pet.equip);
	clif_send_petstatus(sd);

	return 0;
}

int pet_recv_petdata(int account_id,struct s_pet *p,int flag)
{
	struct map_session_data *sd;

	sd = map_id2sd(account_id);
	if(sd == NULL)
		return 1;
	if(flag == 1) {
		sd->status.pet_id = 0;
		return 1;
	}
	memcpy(&sd->pet,p,sizeof(struct s_pet));
	if(sd->pet.incuvate == 1)
		pet_birth_process(sd);
	else {
		pet_npc_init(sd);
		if(sd->bl.prev != NULL) {
			map_addblock(&sd->pet_npcdata->bl);
			clif_spawnpet(sd->pet_npcdata);
			clif_send_petdata(sd,0,0);
			clif_send_petdata(sd,5,0x14);
			clif_pet_equip(sd->pet_npcdata,sd->pet.equip);
			clif_send_petstatus(sd);
		}
	}

	return 0;
}

int pet_select_egg(struct map_session_data *sd,short egg_index)
{
	if(sd->status.inventory[egg_index].card[0] == (short)0xff00)
		intif_request_petdata(sd->status.account_id,sd->status.char_id,*((long *)&sd->status.inventory[egg_index].card[2]));
	else
		printf("wrong egg item inventory %d\n",egg_index);
	pc_delitem(sd,egg_index,1,0);

	return 0;
}

int pet_catch_process1(struct map_session_data *sd,int target_class)
{
	sd->catch_target_class = target_class;
	clif_catch_process(sd);

	return 0;
}

int pet_catch_process2(struct map_session_data *sd,int target_id)
{
	struct mob_data *md;
	int i,pet_catch_rate;

	md=(struct mob_data*)map_id2bl(target_id);

	i = search_petDB_index(md->class,PET_CLASS);
	if(md == NULL || md->bl.type != BL_MOB || md->bl.prev == NULL || i < 0 || sd->catch_target_class != md->class) {
		clif_pet_rulet(sd,0);
		return 1;
	}

	//target_idによる敵→卵判定
//		printf("mob_id = %d, mob_class = %d\n",md->bl.id,md->class);
		//成功の場合
	pet_catch_rate = (pet_db[i].capture + (sd->status.base_level - mob_db[md->class].lv)*30 + sd->paramc[5]*20)*(200 - md->hp*100/mob_db[md->class].max_hp)/100;
	if(pet_catch_rate < 1) pet_catch_rate = 1;
	if(battle_config.pet_catch_rate != 100)
		pet_catch_rate = (pet_catch_rate*battle_config.pet_catch_rate)/100;
// debug code
//		printf("catch percent = %.2f\n",(double)pet_catch_rate/100.);

	if(rand()%10000 < pet_catch_rate) {
		mob_catch_delete(md);
		clif_pet_rulet(sd,1);
//			printf("rulet success %d\n",target_id);
		intif_create_pet(sd->status.account_id,sd->status.char_id,pet_db[i].class,mob_db[pet_db[i].class].lv,
			pet_db[i].EggID,0,pet_db[i].intimate,100,0,1,pet_db[i].jname);
	}
	else
		clif_pet_rulet(sd,0);

	return 0;
}

int pet_get_egg(int account_id,int pet_id,int flag)
{
	struct map_session_data *sd;
	struct item tmp_item;
	int i,ret;

	if(!flag) {
		sd = map_id2sd(account_id);
		if(sd == NULL)
			return 1;

		i = search_petDB_index(sd->catch_target_class,PET_CLASS);
		if(i >= 0) {
			memset(&tmp_item,0,sizeof(tmp_item));
			tmp_item.nameid = pet_db[i].EggID;
			tmp_item.identify = 1;
			tmp_item.card[0] = 0xff00;
			*((long *)(&tmp_item.card[2])) = pet_id;
			if((ret = pc_additem(sd,&tmp_item,1))) {
				clif_additem(sd,0,0,ret);
				map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y);
			}
		}
		else
			intif_delete_petdata(pet_id);
	}

	return 0;
}

int pet_menu(struct map_session_data *sd,int menunum)
{
	switch(menunum) {
		case 0:
			clif_send_petstatus(sd);
			break;
		case 1:
			pet_food(sd);
			break;
		case 2:
			pet_performance(sd);
			break;
		case 3:
			pet_return_egg(sd);
			break;
		case 4:
			pet_unequipitem(sd);
			break;
	}
	return 0;
}

int pet_change_name(struct map_session_data *sd,char *name)
{
	if(sd->pet.rename_flag == 1 && battle_config.pet_rename == 0)
		return 1;

	pet_stop_walking(sd,-1);
	memcpy(sd->pet.name,name,24);
	memcpy(sd->pet_npcdata->name,name,24);
	clif_clearchar_area(&sd->pet_npcdata->bl,0);
	clif_spawnpet(sd->pet_npcdata);
	clif_send_petdata(sd,0,0);
	clif_send_petdata(sd,5,0x14);
	sd->pet.rename_flag = 1;
	intif_save_petdata(sd->status.account_id,&sd->pet);
	clif_pet_equip(sd->pet_npcdata,sd->pet.equip);
	clif_send_petstatus(sd);

	return 0;
}

int pet_equipitem(struct map_session_data *sd,int index)
{
	int nameid;

	nameid = sd->status.inventory[index].nameid;
	if(sd->petDB == NULL)
		return 1;
	if(sd->petDB->AcceID == 0 || nameid != sd->petDB->AcceID || sd->pet.equip != 0) {
		clif_equipitemack(sd,0,0,0);
		return 1;
	}
	else {
		pc_delitem(sd,index,1,0);
		sd->pet.equip = sd->pet_npcdata->equip = nameid;
		clif_pet_equip(sd->pet_npcdata,nameid);
		intif_save_petdata(sd->status.account_id,&sd->pet);
	}

	return 0;
}

int pet_unequipitem(struct map_session_data *sd)
{
	struct item tmp_item;
	int nameid,flag;

	if(sd->petDB == NULL)
		return 1;
	if(sd->pet.equip == 0)
		return 1;

	nameid = sd->pet.equip;
	sd->pet.equip = sd->pet_npcdata->equip = 0;
	clif_pet_equip(sd->pet_npcdata,0);
	intif_save_petdata(sd->status.account_id,&sd->pet);
	memset(&tmp_item,0,sizeof(tmp_item));
	tmp_item.nameid = nameid;
	tmp_item.identify = 1;
	if((flag = pc_additem(sd,&tmp_item,1))) {
		clif_additem(sd,0,0,flag);
		map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y);
	}

	return 0;
}

int pet_food(struct map_session_data *sd)
{
	int i,k;

	if(sd->petDB == NULL)
		return 1;
	i=pc_search_inventory(sd,sd->petDB->FoodID);
	if(i < 0) {
		clif_pet_food(sd,sd->petDB->FoodID,0);
		return 1;
	}
	pc_delitem(sd,i,1,0);
	if(sd->pet.hungry > 90)
		sd->pet.intimate -= sd->petDB->r_full;
	else if(sd->pet.hungry > 75) {
		if(battle_config.pet_friendly_rate != 100)
			k = (sd->petDB->r_hungry * battle_config.pet_friendly_rate)/100;
		else
			k = sd->petDB->r_hungry;
		k = (k * 25)/100;
		if(k <= 0)
			k = 1;
		sd->pet.intimate += k;
	}
	else {
		if(battle_config.pet_friendly_rate != 100)
			k = (sd->petDB->r_hungry * battle_config.pet_friendly_rate)/100;
		else
			k = sd->petDB->r_hungry;
		sd->pet.intimate += k;
	}
	if(sd->pet.intimate < 0)
		sd->pet.intimate = 0;
	else if(sd->pet.intimate > 1000)
		sd->pet.intimate = 1000;
	sd->pet.hungry += sd->petDB->fullness;
	if(sd->pet.hungry > 100)
		sd->pet.hungry = 100;

	clif_send_petdata(sd,2,sd->pet.hungry);
	clif_send_petdata(sd,1,sd->pet.intimate);
	clif_pet_food(sd,sd->petDB->FoodID,1);

	return 0;
}

/*==========================================
 *ペットデータ読み込み
 *------------------------------------------
 */ 
int read_petdb()
{
	FILE *fp;
	char line[1024];
	int j=0;
	
	memset(pet_db,0,sizeof(pet_db));
	fp=fopen("db/pet_db.txt","r");
	if(fp==NULL){
		printf("can't read db/pet_db.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		int nameid,i;
		char *str[55],*p,*np;

		if(line[0] == '/' && line[1] == '/')
			continue;

		for(i=0,p=line;i<17;i++){
			if((np=strchr(p,','))!=NULL){
				str[i]=p;
				*np=0;
				p=np+1;
			} else {
				str[i]=p;
				p+=strlen(p);
			}
		}
		
		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=2000)
			continue;
		
		//MobID,Name,JName,ItemID,EggID,AcceID,FoodID,"Fullness (1回の餌での満腹度増加率%)","HungryDeray (/min)","R_Hungry (空腹時餌やり親密度増加率%)","R_Full (とても満腹時餌やり親密度減少率%)","Intimate (捕獲時親密度%)","Die (死亡時親密度減少率%)","Capture (捕獲率%)",(Name)
		pet_db[j].class = nameid;
		memcpy(pet_db[j].name,str[1],24);
		memcpy(pet_db[j].jname,str[2],24);
		pet_db[j].itemID=atoi(str[3]);
		pet_db[j].EggID=atoi(str[4]);
		pet_db[j].AcceID=atoi(str[5]);
		pet_db[j].FoodID=atoi(str[6]);
		pet_db[j].fullness=atoi(str[7]);
		pet_db[j].hungry_delay=atoi(str[8])*1000;
		pet_db[j].r_hungry=atoi(str[9]);
		if(pet_db[j].r_hungry <= 0)
			pet_db[j].r_hungry=1;
		pet_db[j].r_full=atoi(str[10]);
		pet_db[j].intimate=atoi(str[11]);
		pet_db[j].die=atoi(str[12]);
		pet_db[j].capture=atoi(str[13]);
		pet_db[j].speed=atoi(str[14]);
/*		printf("%d %s %s %d %d %d %d %d %d %d %d %d %d %d\n",
				pet_db[j].mob_ID,pet_db[j].name,pet_db[j].jname,
				pet_db[j].itemID,
				pet_db[j].EggID,
				pet_db[j].AcceID,
				pet_db[j].FoodID,
				pet_db[j].fullness,
				pet_db[j].hungryderay,
				pet_db[j].r_hungry,
				pet_db[j].r_full,
				pet_db[j].intimate,
				pet_db[j].die
				,pet_db[j].capture);*/
		j++;
	}
	fclose(fp);
	printf("read db/pet_db.txt done (count=%d)\n",j);
	return 0;
}

/*==========================================
 * スキル関係初期化処理
 *------------------------------------------
 */
int do_init_pet(void)
{
	add_timer_func_list(pet_timer,"pet_timer");
	add_timer_func_list(pet_hungry,"pet_hungry");

	read_petdb();

	return 0;
}
