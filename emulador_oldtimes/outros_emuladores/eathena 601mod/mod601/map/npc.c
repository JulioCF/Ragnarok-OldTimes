// $Id: npc.c,v 1.12 2003/07/01 00:29:54 lemit Exp $
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "map.h"
#include "npc.h"
#include "clif.h"
#include "pc.h"
#include "itemdb.h"
#include "script.h"
#include "mob.h"
#include "db.h"
#include "timer.h"
#include "pet.h"
#include "battle.h"
#include "skill.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct npc_src_list {
	struct npc_src_list * next;
	char name[4];
} ;

static struct npc_src_list *npc_src_first,*npc_src_last;
static int npc_id=START_NPC_NUM;
static int npc_warp,npc_shop,npc_script,npc_mob;

int npc_get_new_npc_id(void){ return npc_id++; }

static struct dbt *ev_db;
static struct dbt *npcname_db;

struct event_data {
	struct npc_data *nd;
	int pos;
};


/*==========================================
 * NPCの無効化/有効化
 *------------------------------------------
 */
int npc_enable(const char *name,int flag)
{
	struct npc_data *nd=strdb_search(npcname_db,name);
	if(nd==NULL)
		return 0;
	
	if(flag){	// 有効化
		nd->state.flag&=~1;
		clif_spawnnpc(nd);
	}else{		// 無効化
		nd->state.flag|=1;
		clif_clearchar(&nd->bl,0);
	}
	return 0;
}

/*==========================================
 * イベントキューのイベント処理
 *------------------------------------------
 */
int npc_event_dequeue(struct map_session_data *sd)
{
	sd->npc_id=0;
	if(sd->eventqueue[0][0]){	// キューのイベント処理
		char *name=malloc(50);
		int i;
		if(name==NULL){
			printf("run_script: out of memory !\n");exit(1);
		}
		memcpy(name,sd->eventqueue[0],50);
		for(i=MAX_EVENTQUEUE-2;i>=0;i--)
			memcpy(sd->eventqueue[i],sd->eventqueue[i+1],50);
		add_timer(gettick()+100,npc_event_timer,sd->bl.id,(int)name);
	}
	return 0;
}

/*==========================================
 * イベントの遅延実行
 *------------------------------------------
 */
int npc_event_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	if(sd==NULL)
		return 0;
	
	npc_event(sd,(const char *)data);
	free((void*)data);
	return 0;
}
/*==========================================
 * イベント用ラベルのエクスポート
 * npc_parse_script->strdb_foreachから呼ばれる
 *------------------------------------------
 */
int npc_event_export(void *key,void *data,va_list ap)
{
	char *lname=(char *)key;
	int pos=(int)data;
	struct npc_data *nd=va_arg(ap,struct npc_data *);
	
	if((lname[0]=='O' || lname[0]=='o')&&(lname[1]=='N' || lname[1]=='n')){
		struct event_data *ev;
		char *buf;
		char *p=strchr(lname,':');
		// エクスポートされる
		ev=malloc(sizeof(struct event_data));
		buf=malloc(50);
		if(ev==NULL || buf==NULL){
			printf("npc_event_export: out of memory !\n");
		}else if(p==NULL || (p-lname)>24){
			printf("npc_event_export: label name error !\n");
		}else{
			ev->nd=nd;
			ev->pos=pos;
			*p='\0';
			sprintf(buf,"%s::%s",nd->exname,lname);
			*p=':';
			strdb_insert(ev_db,buf,ev);
//			printf("npc_event_export: export [%s]\n",buf);
		}
	}
	return 0;
}
/*==========================================
 * OnInitイベント実行
 *------------------------------------------
 */
int npc_event_do_oninit_sub(void *key,void *data,va_list ap)
{
	char *p=(char *)key;
	struct event_data *ev=(struct event_data *)data;
	int *c=va_arg(ap,int *);

	if( (p=strchr(p,':')) && p && strcasecmp("::OnInit",p)==0 ){
		run_script(ev->nd->u.scr.script,ev->pos,0,ev->nd->bl.id);
		(*c)++;
	}
	
	return 0;
}
int npc_event_do_oninit(void)
{
	int c=0;
	strdb_foreach(ev_db,npc_event_do_oninit_sub,&c);
	printf("npc: OnInit Event done. (%d npc)\n",c);
	return 0;
}

/*==========================================
 * OnTimer NPC event - by RoVeRT
 *------------------------------------------
 */int npc_do_ontimer_sub(void *key,void *data,va_list ap)
{
	char *p=(char *)key;
	struct event_data *ev=(struct event_data *)data;
	int *c=va_arg(ap,int *);
	struct map_session_data *sd=va_arg(ap,struct map_session_data *);
	int option=va_arg(ap,int);
	int tick=0;
	char temp[10];
	char event[100];

	if(ev->nd->bl.id==(int)*c && (p=strchr(p,':')) && p && strncasecmp("::OnTimer",p,8)==0 ){
		sscanf(&p[9],"%s",temp);
		tick=atoi(temp);

		strcpy( event, ev->nd->name);
		strcat( event, p);

		if (option!=0) {
			pc_addeventtimer(sd,tick,event);
		} else {
			pc_deleventtimer(sd,event);
		}
	}
	return 0;
}
int npc_do_ontimer(int npc_id, struct map_session_data *sd, int option)
{
	strdb_foreach(ev_db,npc_do_ontimer_sub,&npc_id,sd,option);
	return 0;
}

/*==========================================
 * イベント型のNPC処理
 *------------------------------------------
 */
int npc_event(struct map_session_data *sd,const char *eventname)
{
	struct event_data *ev=strdb_search(ev_db,eventname);
	struct npc_data *nd;
	int xs,ys;
	
	if(ev==NULL || (nd=ev->nd)==NULL){
		printf("npc_event: event not found [%s]\n",eventname);
		return 0;
	}

	xs=nd->u.scr.xs;
	ys=nd->u.scr.ys;
	if(xs>=0 && ys>=0 ){
		if(nd->bl.m != sd->bl.m )
			return 0;
		if( xs>0 && (sd->bl.x<nd->bl.x-xs/2 || nd->bl.x+xs/2<sd->bl.x) )
			return 0;
		if( ys>0 && (sd->bl.y<nd->bl.y-ys/2 || nd->bl.y+ys/2<sd->bl.y) )
			return 0;
	}

	if( sd->npc_id!=0){
		//printf("npc_event: npc_id != 0\n");
		int i;
		for(i=0;i<MAX_EVENTQUEUE;i++)
			if(!sd->eventqueue[i][0])
				break;
		if(i==MAX_EVENTQUEUE){
			printf("npc_event: event queue is full !\n");
		}else{
//			printf("npc_event: enqueue\n");
			memcpy(sd->eventqueue[i],eventname,50);
		}
		return 1;
	}
	if(nd->state.flag&1){	// 無効化されている
		npc_event_dequeue(sd);
		return 0;
	}
	
	sd->npc_id=nd->bl.id;
	sd->npc_pos=run_script(nd->u.scr.script,ev->pos,sd->bl.id,nd->bl.id);
	return 0;
}

/*==========================================
 * 接触型のNPC処理
 *------------------------------------------
 */
int npc_touch_areanpc(struct map_session_data *sd,int m,int x,int y)
{
	int i,f=1;
	int xs,ys;

	for(i=0;i<map[m].npc_num;i++){
		if(map[m].npc[i]->state.flag&1){	// 無効化されている
			f=0;
			continue;
		}
	
		switch(map[m].npc[i]->bl.subtype){
		case WARP:
			xs=map[m].npc[i]->u.warp.xs;
			ys=map[m].npc[i]->u.warp.ys;
			break;
		case SCRIPT:
			xs=map[m].npc[i]->u.scr.xs;
			ys=map[m].npc[i]->u.scr.ys;
			break;
		default:
			continue;
		}
		if(x >= map[m].npc[i]->bl.x-xs/2 && x < map[m].npc[i]->bl.x-xs/2+xs &&
		   y >= map[m].npc[i]->bl.y-ys/2 && y < map[m].npc[i]->bl.y-ys/2+ys)
			break;
	}
	if(i==map[m].npc_num){
		if(f)
			printf("npc_touch_areanpc : some bug \n");
		return 1;
	}
	switch(map[m].npc[i]->bl.subtype){
	case WARP:
		pc_setpos(sd,map[m].npc[i]->u.warp.name,map[m].npc[i]->u.warp.x,map[m].npc[i]->u.warp.y,0);
		break;
	case SCRIPT:
		npc_click(sd,map[m].npc[i]->bl.id);
		break;
	}
	return 0;
}

/*==========================================
 * 近くかどうかの判定
 *------------------------------------------
 */
int npc_checknear(struct map_session_data *sd,int id)
{
	struct npc_data *nd;

	nd=(struct npc_data *)map_id2bl(id);
	if(nd==NULL || nd->bl.type!=BL_NPC){
		printf("no such npc : %d\n",id);
		return 1;
	}
	
	if(nd->class<0)	// イベント系は常にOK
		return 1;

	// エリア判定
	if(nd->bl.m!=sd->bl.m ||
	   nd->bl.x<sd->bl.x-AREA_SIZE-1 || nd->bl.x>sd->bl.x+AREA_SIZE+1 ||
	   nd->bl.y<sd->bl.y-AREA_SIZE-1 || nd->bl.y>sd->bl.y+AREA_SIZE+1)
		return 1;

	return 0;
}

/*==========================================
 * クリック時のNPC処理
 *------------------------------------------
 */
int npc_click(struct map_session_data *sd,int id)
{
	struct npc_data *nd;

	if(sd->npc_id != 0){
		printf("npc_click: npc_id != 0\n");
		return 1;
	}

	if(npc_checknear(sd,id))
		return 1;

	nd=(struct npc_data *)map_id2bl(id);

	if(nd->state.flag&1)	// 無効化されている
		return 1;

	sd->npc_id=id;
	switch(nd->bl.subtype){
	case SHOP:
		clif_npcbuysell(sd,id);
		npc_event_dequeue(sd);
		break;
	case SCRIPT:
		sd->npc_pos=run_script(nd->u.scr.script,0,sd->bl.id,id);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_scriptcont(struct map_session_data *sd,int id)
{
	struct npc_data *nd;

	if(id!=sd->npc_id)
		return 1;
	if(npc_checknear(sd,id))
		return 1;

	nd=(struct npc_data *)map_id2bl(id);

	sd->npc_pos=run_script(nd->u.scr.script,sd->npc_pos,sd->bl.id,id);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_buysellsel(struct map_session_data *sd,int id,int type)
{
	struct npc_data *nd;

	if(npc_checknear(sd,id))
		return 1;

	nd=(struct npc_data *)map_id2bl(id);
	if(nd->bl.subtype!=SHOP){
		printf("no such shop npc : %d\n",id);
		sd->npc_id=0;
		return 1;
	}
	if(nd->state.flag&1)	// 無効化されている
		return 1;

	sd->npc_shopid=id;
	if(type==0){
		clif_buylist(sd,nd);
	} else {
		clif_selllist(sd);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_buylist(struct map_session_data *sd,int n,unsigned short *item_list)
{
	struct npc_data *nd;
	int i,j,w,z,skill,itemamount=0,new=0;

	if(npc_checknear(sd,sd->npc_shopid))
		return 3;

	nd=(struct npc_data*)map_id2bl(sd->npc_shopid);
	if(nd->bl.subtype!=SHOP)
		return 3;

	for(i=0,w=z=0;i<n;i++){
		for(j=0;nd->u.shop_item[j].nameid;j++){
			if(nd->u.shop_item[j].nameid==item_list[i*2+1])
				break;
		}
		if(nd->u.shop_item[j].nameid==0)
			return 3;

		z+=pc_modifybuyvalue(sd,nd->u.shop_item[j].value) * item_list[i*2];
		itemamount+=item_list[i*2];

		switch(pc_checkadditem(sd,item_list[i*2+1],item_list[i*2])){
		case ADDITEM_EXIST:
			break;
		case ADDITEM_NEW:
			new++;
			break;
		case ADDITEM_OVERAMOUNT:
			return 2;
		}

		w+=itemdb_weight(item_list[i*2+1]) * item_list[i*2];
	}
	if(z > sd->status.zeny)
		return 1;	// zeny不足
	if(w+sd->weight > sd->max_weight)
		return 2;	// 重量超過
	if(pc_inventoryblank(sd)<new)
		return 3;	// 種類数超過


	pc_payzeny(sd,z);
	for(i=0;i<n;i++){
		struct item item_tmp;

		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid = item_list[i*2+1];
		item_tmp.identify = 1;	// npc販売アイテムは鑑定済み

		pc_additem(sd,&item_tmp,item_list[i*2]);
	}

	//商人経験値
/*	if((sd->status.class == 5) || (sd->status.class == 10) || (sd->status.class == 18)){
		z = z * pc_checkskill(sd,MC_DISCOUNT) / ((1 + 300 / itemamount) * 4000) * battle_config.shop_exp;
		pc_gainexp(sd,0,z);
	}*/
	if(battle_config.shop_exp > 0 && z > 0 && (skill = pc_checkskill(sd,MC_DISCOUNT)) > 0) {
		z = z/battle_config.shop_exp;
		if(z <= 0)
			z = 1;
		pc_gainexp(sd,0,z);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_selllist(struct map_session_data *sd,int n,unsigned short *item_list)
{
	int i,z,skill,itemamount=0;

	if(npc_checknear(sd,sd->npc_shopid))
		return 1;
	for(i=0,z=0;i<n;i++){
		int nameid;
		if(item_list[i*2]-2 <0 || item_list[i*2]-2 >=MAX_INVENTORY)
			return 1;
		nameid=sd->status.inventory[item_list[i*2]-2].nameid;
		if(nameid == 0 ||
		   sd->status.inventory[item_list[i*2]-2].amount < item_list[i*2+1])
			return 1;
		z+=pc_modifysellvalue(sd,itemdb_sellvalue(nameid)) * item_list[i*2+1];
		itemamount+=item_list[i*2+1];
	}

	pc_getzeny(sd,z);
	for(i=0;i<n;i++){
		pc_delitem(sd,item_list[i*2]-2,item_list[i*2+1],0);
	}
	

	//商人経験値
/*	if((sd->status.class == 5) || (sd->status.class == 10) || (sd->status.class == 18)){
		z = z * pc_checkskill(sd,MC_OVERCHARGE) / ((1 + 500 / itemamount) * 4000) * battle_config.shop_exp ;
		pc_gainexp(sd,0,z);
	}*/
	if(battle_config.shop_exp > 0 && z > 0 && (skill = pc_checkskill(sd,MC_OVERCHARGE)) > 0) {
		z = (int)(log((double)z * (double)skill) * (double)battle_config.shop_exp/100.);
		if(z <= 0)
			z = 1;
		pc_gainexp(sd,0,z);
	}

	return 0;

}

//
// 初期化関係
//

/*==========================================
 *
 *------------------------------------------
 */
// 読み込むnpcファイルの設定
void npc_addsrcfile(char *name)
{
	struct npc_src_list *new;

	new=malloc(sizeof(*new)+strlen(name));
	if(new==NULL){
		printf("out of memory : npc_addsrcfile\n");
		exit(1);
	}
	new->next = NULL;
	strcpy(new->name,name);
	if(npc_src_first==NULL)
		npc_src_first = new;
	if(npc_src_last)
		npc_src_last->next = new;

	npc_src_last=new;
}

/*==========================================
 *
 *------------------------------------------
 */
// warp行読み込み
#define WARP_CLASS 45
#define WARP_DEBUG_CLASS 722
static int npc_parse_warp(char *w1,char *w2,char *w3,char *w4)
{
	int x,y,xs,ys,to_x,to_y,m;
	int i,j;
	char mapname[24],to_mapname[24];
	struct npc_data *nd;

	// 引数の個数チェック
	if(sscanf(w1,"%[^,],%d,%d",mapname,&x,&y) != 3 ||
	   sscanf(w4,"%d,%d,%[^,],%d,%d",&xs,&ys,to_mapname,&to_x,&to_y) != 5){
		printf("bad warp line : %s\n",w3);
		return 1;
	}

	m=map_mapname2mapid(mapname);

	nd=malloc(sizeof(struct npc_data));
	if(nd==NULL){
		printf("out of memory : npc_parse_warp\n");
		exit(1);
	}
	memset(nd,0,sizeof(struct npc_data));
	nd->bl.id=npc_id++;
	nd->n=map_addnpc(m,nd);

	nd->bl.m=m;
	nd->bl.x=x;
	nd->bl.y=y;
	nd->dir=0;
	nd->state.flag=0;
	memcpy(nd->name,w3,24);
	memcpy(nd->exname,w3,24);

	nd->chat_id=0;
	if(!battle_config.warp_point_debug)
		nd->class=WARP_CLASS;
	else
		nd->class=WARP_DEBUG_CLASS;
	nd->speed=200;
	memcpy(nd->u.warp.name,to_mapname,16);
	xs+=2; ys+=2;
	nd->u.warp.x=to_x;
	nd->u.warp.y=to_y;
	nd->u.warp.xs=xs;
	nd->u.warp.ys=ys;

	for(i=0;i<ys;i++){
		for(j=0;j<xs;j++){
			int t;
			t=map_getcell(m,x-xs/2+j,y-ys/2+i);
			if(t==1 || t==5)
				continue;
			map_setcell(m,x-xs/2+j,y-ys/2+i,t|0x80);
		}
	}

	//printf("warp npc %s %d read done\n",mapname,nd->bl.id);
	npc_warp++;
	nd->bl.type=BL_NPC;
	nd->bl.subtype=WARP;
	map_addblock(&nd->bl);
	clif_spawnnpc(nd);
	strdb_insert(npcname_db,nd->name,nd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// shop行読み込み
static int npc_parse_shop(char *w1,char *w2,char *w3,char *w4)
{
	char *p;
	int x,y,dir,m;
	int max=64,pos=0;
	char mapname[24];
	struct npc_data *nd;

	// 引数の個数チェック
	if(sscanf(w1,"%[^,],%d,%d,%d",mapname,&x,&y,&dir) != 4 ||
	   strchr(w4,',')==NULL){
		printf("bad shop line : %s\n",w3);
		return 1;
	}
	m = map_mapname2mapid(mapname);

	nd=malloc(sizeof(struct npc_data)+sizeof(nd->u.shop_item[0])*(max+1));
	if(nd==NULL){
		printf("out of memory : npc_parse_shop\n");
		exit(1);
	}
	memset(nd,0,sizeof(struct npc_data)+sizeof(nd->u.shop_item[0])*(max+1));
	p=strchr(w4,',');

	while(p && pos<max){
		int nameid,value;
		p++;
		if(sscanf(p,"%d:%d",&nameid,&value)!=2)
			break;
		nd->u.shop_item[pos].nameid=nameid;
		nd->u.shop_item[pos].value=value;
		pos++;
		p=strchr(p,',');
	}
	if(pos==0){
		free(nd);
		return 1;
	}
	nd->u.shop_item[pos++].nameid=0;

	nd->bl.m = m;
	nd->bl.x = x;
	nd->bl.y = y;
	nd->bl.id=npc_id++;
	nd->dir = dir;
	nd->state.flag=0;
	memcpy(nd->name,w3,24);
	nd->class=atoi(w4);
	nd->speed=200;
	nd->chat_id=0;

	nd=realloc(nd,sizeof(struct npc_data)+sizeof(nd->u.shop_item[0])*pos);
	if(nd==NULL){
		printf("out of memory : npc_parse_shop realloc\n");
		exit(1);
	}

	//printf("shop npc %s %d read done\n",mapname,nd->bl.id);
	npc_shop++;
	nd->bl.type=BL_NPC;
	nd->bl.subtype=SHOP;
	nd->n=map_addnpc(m,nd);
	map_addblock(&nd->bl);
	clif_spawnnpc(nd);
	strdb_insert(npcname_db,nd->name,nd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// script行読み込み
static int npc_parse_script(char *w1,char *w2,char *w3,char *w4,char *first_line,FILE *fp,int *lines)
{
	int x,y,dir,m,xs,ys,class;
	char mapname[24];
	char *srcbuf,*script;
	int srcsize=65536;
	int startline=0;
	char line[1024];
	int i;
	struct npc_data *nd;
	int evflag=0;
	struct dbt *label_db;
	char *p;

	// 引数の個数チェック
	if(sscanf(w1,"%[^,],%d,%d,%d",mapname,&x,&y,&dir) != 4 ||
	   strchr(w4,',')==NULL){
		printf("bad script line : %s\n",w3);
		return 1;
	}
	m = map_mapname2mapid(mapname);

	srcbuf=malloc(srcsize);
	if(srcbuf==NULL){
		printf("out of memory : npc_parse_script srcbuf\n");
		exit(1);
	}
	if(strchr(first_line,'{')){
		strcpy(srcbuf,strchr(first_line,'{'));
		startline=*lines;
	} else
		srcbuf[0]=0;
	while(1){
		for(i=strlen(srcbuf)-1;i>=0 && isspace(srcbuf[i]);i--);
		if(i>=0 && srcbuf[i]=='}')
			break;
		fgets(line,1020,fp);
		(*lines)++;
		if(feof(fp))
			break;
		if(strlen(srcbuf)+strlen(line)+1>=srcsize){
			srcsize+=65536;
			srcbuf=realloc(srcbuf,srcsize);
			if(srcbuf==NULL){
				printf("out of memory : npc_parse_script srcbuf realloc\n");
				exit(1);
			}
		}
		if(srcbuf[0]!='{'){
			if(strchr(line,'{')){
				strcpy(srcbuf,strchr(line,'{'));
				startline=*lines;
			}
		} else
			strcat(srcbuf,line);
	}
	script=parse_script(srcbuf,startline);
	free(srcbuf);
	if(script==NULL){
		// script parse error?
		return 1;
	}

	nd=malloc(sizeof(struct npc_data));
	if(nd==NULL){
		printf("out of memory : npc_parse_script nd\n");
		exit(1);
	}
	memset(nd,0,sizeof(struct npc_data));

	if(sscanf(w4,"%d,%d,%d",&class,&xs,&ys)==3){
		int i,j;
		
		if(xs>0)xs=xs*2+1;
		if(ys>0)ys=ys*2+1;
		
		if(class>=0){	// 接触型NPC

			for(i=0;i<ys;i++){
				for(j=0;j<xs;j++){
					int t;
					t=map_getcell(m,x-xs/2+j,y-ys/2+i);
					if(t==1 || t==5)
						continue;
					map_setcell(m,x-xs/2+j,y-ys/2+i,t|0x80);
				}
			}
		}
		
		nd->u.scr.xs=xs;
		nd->u.scr.ys=ys;
	} else {	// クリック型NPC
		class=atoi(w4);
		nd->u.scr.xs=0;
		nd->u.scr.ys=0;
	}
	
	if(class<0){	// イベント型NPC
		evflag=1;
	}

	while((p=strchr(w3,':'))){
		if(p[1]==':') break;
	}
	if(p){
		*p=0;
		memcpy(nd->name,w3,24);
		memcpy(nd->exname,p+2,24);
	}else{
		memcpy(nd->name,w3,24);
		memcpy(nd->exname,w3,24);
	}
	
	nd->bl.m = m;
	nd->bl.x = x;
	nd->bl.y = y;
	nd->bl.id=npc_id++;
	nd->dir = dir;
	nd->state.flag=0;
	nd->class=class;
	nd->speed=200;
	nd->u.scr.script=script;
	nd->chat_id=0;

	//printf("script npc %s %d %d read done\n",mapname,nd->bl.id,nd->class);
	npc_script++;
	nd->bl.type=BL_NPC;
	nd->bl.subtype=SCRIPT;
	nd->n=map_addnpc(m,nd);
	map_addblock(&nd->bl);

	if(evflag){	// イベント型
		struct event_data *ev=malloc(sizeof(struct event_data));
		if(ev==NULL){
			printf("npc_parse_script: out of memory !(event_data)\n");
		}else{
			ev->nd=nd;
			ev->pos=0;
			strdb_insert(ev_db,nd->exname,ev);
		}
	}else{
		clif_spawnnpc(nd);
	}
	strdb_insert(npcname_db,nd->exname,nd);

	// イベント用ラベルデータのエクスポート
	label_db=script_get_label_db();
	strdb_foreach(label_db,npc_event_export,nd);
	

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_parse_mob(char *w1,char *w2,char *w3,char *w4)
{
	int m,x,y,xs,ys,class,num,delay1,delay2;
	int i;
	char mapname[24];
	char eventname[24]="";
	struct mob_data *md,*base;

	xs=ys=0;
	delay1=delay2=0;
	// 引数の個数チェック
	if(sscanf(w1,"%[^,],%d,%d,%d,%d",mapname,&x,&y,&xs,&ys) < 3 ||
	   sscanf(w4,"%d,%d,%d,%d,%s",&class,&num,&delay1,&delay2,eventname) < 2 ){
		printf("bad monster line : %s\n",w3);
		return 1;
	}

	m=map_mapname2mapid(mapname);

	if( num>1 && battle_config.mob_count_rate!=100){
		if( (num=num*battle_config.mob_count_rate/100)<1 )
			num=1;
	}


	base=malloc(sizeof(struct mob_data)*num);
	if(base==NULL){
		printf("out of memory : npc_parse_mob\n");
		exit(1);
	}
	for(i=0;i<num;i++){
		md=&base[i];

		md->bl.prev=NULL;
		md->bl.next=NULL;
		md->bl.m=m;
		md->bl.x=x;
		md->bl.y=y;
		memcpy(md->name,w3,24);

		md->n = i;
		md->class=class;
		md->bl.id=npc_id++;
		md->x0=x;
		md->y0=y;
		md->xs=xs;
		md->ys=ys;
		md->spawndelay1=delay1;
		md->spawndelay2=delay2;

		memset(&md->state,0,sizeof(md->state));
		md->timer = -1;
		md->target_id=0;
		md->attacked_id=0;
		md->speed=mob_db[class].speed;

		if(mob_db[class].mode&0x02) {
			md->lootitem=malloc(sizeof(struct item)*LOOTITEM_SIZE);
			if(md->lootitem==NULL){
				printf("out of memory : npc_parse_mob\n");
				exit(1);
			}
		}
		else
			md->lootitem=NULL;

		if(strlen(eventname)>=4){
			memcpy(md->npc_event,eventname,24);
		}else
			memset(md->npc_event,0,24);

		md->bl.type=BL_MOB;
		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);

		npc_mob++;
	}
	//printf("warp npc %s %d read done\n",mapname,nd->bl.id);

	return 0;
}

/*==========================================
 * マップフラグの読み込み
 *------------------------------------------
 */
static int npc_parse_mapflag(char *w1,char *w2,char *w3,char *w4)
{
	int m;
	char mapname[24],savemap[16];
	int savex,savey;


	// 引数の個数チェック
//	if(	sscanf(w1,"%[^,],%d,%d,%d",mapname,&x,&y,&dir) != 4 )
	if(	sscanf(w1,"%[^,]",mapname) != 1 )
		return 1;

	m=map_mapname2mapid(mapname);
	if(m<0)
		return 1;

//マップフラグ
	if( strcmp(w3,"nosave")==0 && sscanf(w4,"%[^,],%d,%d",savemap,&savex,&savey)==3){
		map[m].flag.nosave=1;
		memcpy(map[m].save.map,savemap,16);
		map[m].save.x=savex;
		map[m].save.y=savey;
	}
	else if(strcmp(w3,"nomemo")==0) {
		map[m].flag.nomemo=1;
	}
	else if(strcmp(w3,"noteleport")==0) {
		map[m].flag.noteleport=1;
	}
	else if(strcmp(w3,"nobranch")==0) {
		map[m].flag.nobranch=1;
	}
	else if(strcmp(w3,"nopenalty")==0) {
		map[m].flag.nopenalty=1;
	}
	else if(strcmp(w3,"pvp")==0) {
		map[m].flag.pvp=1;
	}
	else if(strcmp(w3,"pvp_noparty")==0) {
		map[m].flag.pvp_noparty=1;
	}
	else if(strcmp(w3,"pvp_noguild")==0) {
		map[m].flag.pvp_noguild=1;
	}
	else if(strcmp(w3,"gvg")==0) {
		map[m].flag.gvg=1;
	}
	else if(strcmp(w3,"gvg_noparty")==0) {
		map[m].flag.gvg_noparty=1;
	}

	return 0;
}

/*==========================================
 * npc初期化
 *------------------------------------------
 */
int do_init_npc(void)
{
	struct npc_src_list *nsl;
	FILE *fp;
	char line[1024];
	int m,lines;

	ev_db=strdb_init(24);
	npcname_db=strdb_init(24);

	for(nsl=npc_src_first;nsl;nsl=nsl->next){
		fp=fopen(nsl->name,"r");
		if(fp==NULL){
			printf("file not found : %s\n",nsl->name);
			exit(1);
		}
		lines=0;
		while(fgets(line,1020,fp)){
			char w1[1024],w2[1024],w3[1024],w4[1024],mapname[1024];
			int i,j,w4pos,count;
			if(line[0] == '/' && line[1] == '/')
				continue;
			// 不要なスペースやタブの連続は詰める
			lines++;
			for(i=j=0;line[i];i++){
				if(line[i]==' '){
					if(!((line[i+1] && (isspace(line[i+1]) || line[i+1]==',')) ||
						 (j && line[j-1]==',')))
						line[j++]=' ';
				} else if(line[i]=='\t'){
					if(!(j && line[j-1]=='\t'))
						line[j++]='\t';
				} else
 					line[j++]=line[i];
			}
			// 最初はタブ区切りでチェックしてみて、ダメならスペース区切りで確認
			if((count=sscanf(line,"%[^\t]\t%[^\t]\t%[^\t\r\n]\t%n%[^\t\r\n]",w1,w2,w3,&w4pos,w4)) < 3 &&
			   (count=sscanf(line,"%s%s%s%n%s",w1,w2,w3,&w4pos,w4)) < 3){
				continue;
			}
			sscanf(w1,"%[^,]",mapname);
			m = map_mapname2mapid(mapname);
			if(strlen(mapname)>16 || m<0){
				// "mapname" is not assigned to this server
				continue;
			}
			if(strcmpi(w2,"warp")==0 && count > 3){
				npc_parse_warp(w1,w2,w3,w4);
			} else if(strcmpi(w2,"shop")==0 && count > 3){
				npc_parse_shop(w1,w2,w3,w4);
			} else if(strcmpi(w2,"script")==0 && count > 3){
				npc_parse_script(w1,w2,w3,w4,line+w4pos,fp,&lines);
			} else if(strcmpi(w2,"monster")==0 && count > 3){
				npc_parse_mob(w1,w2,w3,w4);
			} else if(strcmpi(w2,"mapflag")==0 && count >= 3){
				npc_parse_mapflag(w1,w2,w3,w4);
			}
		}
		fclose(fp);
		printf("read npc %s done\n",nsl->name);
	}
	printf("total %d npcs (%d warp, %d shop, %d script, %d mob)\n",
		   npc_id-START_NPC_NUM,npc_warp,npc_shop,npc_script,npc_mob);

	add_timer_func_list(npc_event_timer,"npc_event_timer");

	//exit(1);

	return 0;
}
