// $Id: map.c,v 1.11 2003/06/29 05:56:44 lemit Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <netdb.h>

#include "core.h"
#include "timer.h"
#include "db.h"
#include "grfio.h"
#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "npc.h"
#include "pc.h"
#include "mob.h"
#include "chat.h"
#include "itemdb.h"
#include "storage.h"
#include "skill.h"
#include "trade.h"
#include "party.h"
#include "battle.h"
#include "script.h"
#include "guild.h"
#include "pet.h"
#include "atcommand.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

// 極力 staticでローカルに収める
static struct dbt * id_db;
static struct dbt * map_db;
static struct dbt * nick_db;
static struct dbt * charid_db;

static int users;
static struct block_list *object[MAX_FLOORITEM];
static int first_free_object_id,last_object_id;

#define block_free_max 50000
static void *block_free[block_free_max];
static int block_free_count=0,block_free_lock=0;

#define BL_LIST_MAX 5120

struct map_data map[MAX_MAP_PER_SERVER];
int map_num=0;

int autosave_interval=DEFAULT_AUTOSAVE_INTERVAL;

struct charid2nick {
	char nick[24];
	int req_id;
};

/*==========================================
 * 全map鯖総計での接続数設定
 * (char鯖から送られてくる)
 *------------------------------------------
 */
void map_setusers(int n)
{
	users=n;
}

/*==========================================
 * 全map鯖総計での接続数取得 (/wへの応答用)
 *------------------------------------------
 */
int map_getusers(void)
{
	return users;
}

//
// block削除の安全性確保処理
//

/*==========================================
 * blockをfreeするときfreeの変わりに呼ぶ
 * ロックされているときはバッファにためる
 *------------------------------------------
 */
int map_freeblock( void *bl )
{
	if(block_free_lock==0)
		free(bl);
	else{
		if( block_free_count>=block_free_max )
			printf("map_freeblock: *WARNING* too many free block! %d %d\n",
				block_free_count,block_free_lock);
		else
			block_free[block_free_count++]=bl;
	}
	return block_free_lock;
}
/*==========================================
 * blockのfreeを一時的に禁止する
 *------------------------------------------
 */
int map_freeblock_lock(void)
{
	return ++block_free_lock;
}
/*==========================================
 * blockのfreeのロックを解除する
 * このとき、ロックが完全になくなると
 * バッファにたまっていたblockを全部削除
 *------------------------------------------
 */
int map_freeblock_unlock(void)
{
	if( (--block_free_lock)==0 ){
		int i;
//		if(block_free_count>0)
//			printf("map_freeblock_unlock: free %d object\n",block_free_count);
		for(i=0;i<block_free_count;i++)
			free(block_free[i]);
		block_free_count=0;
	}else if(block_free_lock<0){
		printf("map_freeblock_unlock: lock count < 0 !\n");
	}
	return block_free_lock;
}


//
// block化処理
//
/*==========================================
 * map[]のblock_listから繋がっている場合に
 * bl->prevにbl_headのアドレスを入れておく
 *------------------------------------------
 */
static struct block_list bl_head;

/*==========================================
 * map[]のblock_listに追加
 * mobは数が多いので別リスト
 *
 * 既にlink済みかの確認が無い。危険かも
 *------------------------------------------
 */
int map_addblock(struct block_list *bl)
{
	int m,x,y;

	m=bl->m;
	x=bl->x;
	y=bl->y;
	if(m<0 || m>=map_num ||
	   x<0 || x>=map[m].xs ||
	   y<0 || y>=map[m].ys)
		return 1;

	if(bl->type==BL_MOB){
		bl->next = map[m].block_mob[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs];
		bl->prev = &bl_head;
		if(bl->next) bl->next->prev = bl;
		map[m].block_mob[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs] = bl;
	} else {
		bl->next = map[m].block[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs];
		bl->prev = &bl_head;
		if(bl->next) bl->next->prev = bl;
		map[m].block[x/BLOCK_SIZE+(y/BLOCK_SIZE)*map[m].bxs] = bl;
		if(bl->type==BL_PC)
			map[m].users++;
	}

	return 0;
}

/*==========================================
 * map[]のblock_listから外す
 * prevがNULLの場合listに繋がってない
 *------------------------------------------
 */
int map_delblock(struct block_list *bl)
{
	// 既にblocklistから抜けている
	if(bl->prev==NULL){
		if(bl->next!=NULL){
			// prevがNULLでnextがNULLでないのは有ってはならない
			printf("map_delblock error : bl->next!=NULL\n");
		}
		return 0;
	}

	if(bl->type==BL_PC)
		map[bl->m].users--;
	if(bl->next) bl->next->prev = bl->prev;
	if(bl->prev==&bl_head){
		// リストの頭なので、map[]のblock_listを更新する
		if(bl->type==BL_MOB){
			map[bl->m].block_mob[bl->x/BLOCK_SIZE+(bl->y/BLOCK_SIZE)*map[bl->m].bxs] = bl->next;
		} else {
			map[bl->m].block[bl->x/BLOCK_SIZE+(bl->y/BLOCK_SIZE)*map[bl->m].bxs] = bl->next;
		}
	} else {
		bl->prev->next = bl->next;
	}
	bl->next = NULL;
	bl->prev = NULL;

	return 0;
}

/*==========================================
 * 周囲のPC人数を数える (現在未使用)
 *------------------------------------------
 */
int map_countnearpc(int m,int x,int y)
{
	int bx,by,c=0;
	struct block_list *bl;

	if(map[m].users==0)
		return 0;
	for(by=y/BLOCK_SIZE-AREA_SIZE/BLOCK_SIZE-1;by<=y/BLOCK_SIZE+AREA_SIZE/BLOCK_SIZE+1;by++){
		if(by<0 || by>=map[m].bys)
			continue;
		for(bx=x/BLOCK_SIZE-AREA_SIZE/BLOCK_SIZE-1;bx<=x/BLOCK_SIZE+AREA_SIZE/BLOCK_SIZE+1;bx++){
			if(bx<0 || bx>=map[m].bxs)
				continue;
			bl = map[m].block[bx+by*map[m].bxs];
			for(;bl;bl=bl->next){
				if(bl->type==BL_PC)
					c++;
			}
		}
	}
	return c;
}

/*==========================================
 * map m (x0,y0)-(x1,y1)内の全objに対して
 * funcを呼ぶ
 * type!=0 ならその種類のみ
 *------------------------------------------
 */
void map_foreachinarea(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int type,...)
{
	int bx,by;
	struct block_list *bl;
	va_list ap;
	const size_t block_list_max=BL_LIST_MAX;
	struct block_list *list[block_list_max];
	int blockcount=0,i;

	va_start(ap,type);
	if(x0<0) x0=0;
	if(y0<0) y0=0;
	if(x1>=map[m].xs) x1=map[m].xs-1;
	if(y1>=map[m].ys) y1=map[m].ys-1;
	if(type==0 || type!=BL_MOB)
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && bl->type!=type)
						continue;
					if(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && blockcount<block_list_max)
						list[blockcount++]=bl;
				}
			}
		}
	if(type==0 || type==BL_MOB)
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block_mob[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && blockcount<block_list_max)
						list[blockcount++]=bl;
				}
			}
		}
	
	if(blockcount>=block_list_max)
		printf("map_foreachinarea: *WARNING* block count too many!\n");
	
	map_freeblock_lock();	// メモリからの解放を禁止する
		
	for(i=0;i<blockcount;i++)
		if(list[i]->prev)	// 有効かどうかチェック
			func(list[i],ap);

	map_freeblock_unlock();	// 解放を許可する
	
	va_end(ap);
}

/*==========================================
 * 矩形(x0,y0)-(x1,y1)が(dx,dy)移動した時の
 * 領域外になる領域(矩形かL字形)内のobjに
 * 対してfuncを呼ぶ
 *
 * dx,dyは-1,0,1のみとする（どんな値でもいいっぽい？）
 *------------------------------------------
 */
void map_foreachinmovearea(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int dx,int dy,int type,...)
{
	int bx,by;
	struct block_list *bl;
	va_list ap;
	const size_t block_list_max=BL_LIST_MAX;
	struct block_list *list[block_list_max];
	int blockcount=0,i;

	va_start(ap,type);
	if(dx==0 || dy==0){
		// 矩形領域の場合
		if(dx==0){
			if(dy<0){
				y0=y1+dy+1;
			} else {
				y1=y0+dy-1;
			}
		} else if(dy==0){
			if(dx<0){
				x0=x1+dx+1;
			} else {
				x1=x0+dx-1;
			}
		}
		if(x0<0) x0=0;
		if(y0<0) y0=0;
		if(x1>=map[m].xs) x1=map[m].xs-1;
		if(y1>=map[m].ys) y1=map[m].ys-1;
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && bl->type!=type)
						continue;
					if(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && blockcount<block_list_max)
						list[blockcount++]=bl;
				}
				bl = map[m].block_mob[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && bl->type!=type)
						continue;
					if(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && blockcount<block_list_max)
						list[blockcount++]=bl;
				}
			}
		}
	}else{
		// L字領域の場合
		
		if(x0<0) x0=0;
		if(y0<0) y0=0;
		if(x1>=map[m].xs) x1=map[m].xs-1;
		if(y1>=map[m].ys) y1=map[m].ys-1;
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && bl->type!=type)
						continue;
					if(!(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1))
						continue;
					if(((dx>0 && bl->x<x0+dx) || (dx<0 && bl->x>x1+dx) ||
						(dy>0 && bl->y<y0+dy) || (dy<0 && bl->y>y1+dy)) &&
						blockcount<block_list_max)
							list[blockcount++]=bl;
				}
				bl = map[m].block_mob[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && bl->type!=type)
						continue;
					if(!(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1))
						continue;
					if(((dx>0 && bl->x<x0+dx) || (dx<0 && bl->x>x1+dx) ||
						(dy>0 && bl->y<y0+dy) || (dy<0 && bl->y>y1+dy)) &&
						blockcount<block_list_max)
							list[blockcount++]=bl;
				}
			}
		}

	}

	if(blockcount>=block_list_max)
		printf("map_foreachinarea: *WARNING* block count too many!\n");
	
	map_freeblock_lock();	// メモリからの解放を禁止する
		
	for(i=0;i<blockcount;i++)
		if(list[i]->prev)	// 有効かどうかチェック
			func(list[i],ap);

	map_freeblock_unlock();	// 解放を許可する
	

	va_end(ap);
}

/*==========================================
 * 床アイテムやエフェクト用の一時obj割り当て
 * object[]への保存とid_db登録まで
 *
 * bl->idもこの中で設定して問題無い?
 *------------------------------------------
 */
int map_addobject(struct block_list *bl)
{
	int i;
	if(first_free_object_id<2 || first_free_object_id>=MAX_FLOORITEM)
		first_free_object_id=2;
	for(i=first_free_object_id;i<MAX_FLOORITEM;i++)
		if(object[i]==NULL)
			break;
	if(i>=MAX_FLOORITEM){
		printf("no free object id\n");
		return 0;
	}
	first_free_object_id=i;
	if(last_object_id<i)
		last_object_id=i;
	object[i]=bl;
	numdb_insert(id_db,i,bl);
	return i;
}

/*==========================================
 * 一時objectの解放
 *	map_delobjectのfreeしないバージョン
 *------------------------------------------
 */
int map_delobjectnofree(int id)
{
	if(object[id]==NULL)
		return 0;

	map_delblock(object[id]);
	numdb_erase(id_db,id);
//	map_freeblock(object[id]);
	object[id]=NULL;

	if(first_free_object_id>id)
		first_free_object_id=id;

	while(last_object_id>2 && object[last_object_id]==NULL)
		last_object_id--;

	return 0;
}

/*==========================================
 * 一時objectの解放
 * block_listからの削除、id_dbからの削除
 * object dataのfree、object[]へのNULL代入
 *
 * addとの対称性が無いのが気になる
 *------------------------------------------
 */
int map_delobject(int id)
{
	struct block_list *obj=object[id];

	if(obj==NULL)
		return 0;

	map_delobjectnofree(id);	
	map_freeblock(obj);

	return 0;
}

/*==========================================
 * 全一時obj相手にfuncを呼ぶ
 *
 *------------------------------------------
 */
void map_foreachobject(int (*func)(struct block_list*,va_list),int type,...)
{
	int i;
	int block_list_count=0;
	const size_t block_list_max=BL_LIST_MAX;
	struct block_list *list[block_list_max];
	va_list ap;

	va_start(ap,type);


	for(i=2;i<=last_object_id;i++){
		if(object[i]){
			if(type && object[i]->type!=type)
				continue;
			if(block_list_count>=block_list_max)
				printf("map_foreachobject: too many block !\n");
			else
				list[block_list_count++]=object[i];
		}
	}
	
	map_freeblock_lock();
	
	for(i=0;i<block_list_count;i++)
		if( list[i]->prev || list[i]->next )
			func(list[i],ap);

	map_freeblock_unlock();

	va_end(ap);
}

/*==========================================
 * 床アイテムを消す
 *
 * data==0の時はtimerで消えた時
 * data!=0の時は拾う等で消えた時として動作
 *
 * 後者は、map_clearflooritem(id)へ
 * map.h内で#defineしてある
 *------------------------------------------
 */
int map_clearflooritem_timer(int tid,unsigned int tick,int id,int data)
{
	struct flooritem_data *fitem;

	fitem = (struct flooritem_data *)object[id];
	if(fitem==NULL || fitem->bl.type!=BL_ITEM || (!data && fitem->cleartimer != tid)){
		printf("map_clearflooritem_timer : error\n");
		return 1;
	}
	if(data)
		delete_timer(fitem->cleartimer,map_clearflooritem_timer);
	else if(fitem->item_data.card[0] == (short)0xff00)
		intif_delete_petdata(*((long *)(&fitem->item_data.card[2])));
	clif_clearflooritem(fitem,0);
	map_delobject(fitem->bl.id);

	return 0;
}

/*==========================================
 * (m,x,y)の周囲rangeマス内の空き(=侵入可能)cellの
 * 内から適当なマス目の座標をx+(y<<16)で返す
 *
 * 現状range=1でアイテムドロップ用途のみ
 *------------------------------------------
 */
int map_searchrandfreecell(int m,int x,int y,int range)
{
	int free_cell,i,j,c;

	for(free_cell=0,i=-range;i<=range;i++){
		if(i+y<0 || i+y>=map[m].ys)
			continue;
		for(j=-range;j<=range;j++){
			if(j+x<0 || j+x>=map[m].xs)
				continue;
			if((c=read_gat(m,j+x,i+y))==1 || c==5)
				continue;
			free_cell++;
		}
	}
	if(free_cell==0)
		return -1;
	free_cell=rand()%free_cell;
	for(i=-range;i<=range;i++){
		if(i+y<0 || i+y>=map[m].ys)
			continue;
		for(j=-range;j<=range;j++){
			if(j+x<0 || j+x>=map[m].xs)
				continue;
			if((c=read_gat(m,j+x,i+y))==1 || c==5)
				continue;
			if(free_cell==0){
				x+=j;
				y+=i;
				i=range+1;
				break;
			}
			free_cell--;
		}
	}

	return x+(y<<16);
}

/*==========================================
 * (m,x,y)を中心に3x3以内に床アイテム設置
 *
 * item_dataはamount以外をcopyする
 *------------------------------------------
 */
int map_addflooritem(struct item *item_data,int amount,int m,int x,int y)
{
	int xy,r;
	struct flooritem_data *fitem;

	if((xy=map_searchrandfreecell(m,x,y,1))<0)
		return 0;
	r=rand();

	fitem = malloc(sizeof(*fitem));
	if(fitem==NULL){
		printf("out of memory : map_addflooritem\n");
		exit(1);
	}
	fitem->bl.type=BL_ITEM;
	fitem->bl.m=m;
	fitem->bl.x=xy&0xffff;
	fitem->bl.y=(xy>>16)&0xffff;

	fitem->bl.id = map_addobject(&fitem->bl);
	if(fitem->bl.id==0){
		free(fitem);
		return 0;
	}

	memcpy(&fitem->item_data,item_data,sizeof(*item_data));
	fitem->item_data.amount=amount;
	fitem->subx=(r&3)*3+3;
	fitem->suby=((r>>2)&3)*3+3;
	fitem->cleartimer=add_timer(gettick()+battle_config.flooritem_lifetime,map_clearflooritem_timer,fitem->bl.id,0);

	map_addblock(&fitem->bl);
	clif_dropflooritem(fitem);

	return fitem->bl.id;
}

/*==========================================
 * charid_dbへ追加(返信待ちがあれば返信)
 *------------------------------------------
 */
void map_addchariddb(int charid,char *name)
{
	struct charid2nick *p;
	int req=0;
	p=numdb_search(charid_db,charid);
	if(p==NULL){	// データベースにない
		p = malloc(sizeof(struct charid2nick));
		if(p==NULL){
			printf("out of memory : map_addchariddb\n");
			exit(1);
		}
		p->req_id=0;
	}else
		numdb_erase(charid_db,charid);
	
	req=p->req_id;
	memcpy(p->nick,name,24);
	p->req_id=0;
	numdb_insert(charid_db,charid,p);
	if(req){	// 返信待ちがあれば返信
		struct map_session_data *sd = map_id2sd(req);
		if(sd!=NULL)
			clif_solved_charname(sd,charid);
	}
}
/*==========================================
 * charid_dbへ追加（返信要求のみ）
 *------------------------------------------
 */
int map_reqchariddb(struct map_session_data * sd,int charid)
{
	struct charid2nick *p;
	p=numdb_search(charid_db,charid);
	if(p!=NULL)	// データベースにすでにある
		return 0;
	p = malloc(sizeof(struct charid2nick));
	if(p==NULL){
		printf("out of memory : map_reqchariddb\n");
		exit(1);
	}
	memset(p->nick,0,24);
	p->req_id=sd->bl.id;
	numdb_insert(charid_db,charid,p);
	return 0;
}

/*==========================================
 * id_dbへblを追加
 *------------------------------------------
 */
void map_addiddb(struct block_list *bl)
{
	numdb_insert(id_db,bl->id,bl);
}

/*==========================================
 * id_dbからblを削除
 *------------------------------------------
 */
void map_deliddb(struct block_list *bl)
{
	numdb_erase(id_db,bl->id);
}

/*==========================================
 * nick_dbへsdを追加
 *------------------------------------------
 */
void map_addnickdb(struct map_session_data *sd)
{
	strdb_insert(nick_db,sd->status.name,sd);
}

/*==========================================
 * PCのquit処理 map.c内分
 *
 * quit処理の主体が違うような気もしてきた
 *------------------------------------------
 */
int map_quit(struct map_session_data *sd)
{
	if(sd->chatID)	// チャットから出る
		chat_leavechat(sd);
		
	if(sd->trade_partner)	// 取引を中断する
		trade_tradecancel(sd);

	if(sd->party_invite>0)	// パーティ勧誘を拒否する
		party_reply_invite(sd,sd->party_invite_account,0);

	if(sd->guild_invite>0)	// ギルド勧誘を拒否する
		guild_reply_invite(sd,sd->guild_invite,0);
	if(sd->guild_alliance>0)	// ギルド同盟勧誘を拒否する
		guild_reply_reqalliance(sd,sd->guild_alliance_account,0);

	party_send_logout(sd);	// パーティのログアウトメッセージ送信

	guild_send_memberinfoshort(sd,0);	// ギルドのログアウトメッセージ送信

	pc_cleareventtimer(sd);	// イベントタイマを破棄する

	skill_castcancel(&sd->bl);	// 詠唱を中断する
	skill_status_change_clear(&sd->bl);	// ステータス異常を解除する
	skill_clear_unitgroup(&sd->bl);	// スキルユニットグループの削除
	skill_cleartimerskill(&sd->bl);
	pc_stopattack(sd);
	pc_delghosttimer(sd);
	pc_delspiritball(sd,sd->spiritball,1);

	storage_storage_quitsave(sd);	// 倉庫を開いてるなら保存する

	clif_clearchar_area(&sd->bl,2);
	map_delblock(&sd->bl);

	numdb_erase(id_db,sd->bl.id);
	strdb_erase(nick_db,sd->status.name);

	return 0;
}

/*==========================================
 * id番号のPCを探す。居なければNULL
 *------------------------------------------
 */
struct map_session_data * map_id2sd(int id)
{
	struct block_list *bl;

	bl=numdb_search(id_db,id);
	if(bl && bl->type==BL_PC)
		return (struct map_session_data*)bl;
	return NULL;
}

/*==========================================
 * char_id番号の名前を探す
 *------------------------------------------
 */
char * map_charid2nick(int id)
{
	struct charid2nick *p=numdb_search(charid_db,id);
	if(p==NULL)
		return NULL;
	if(p->req_id!=0)
		return NULL;
	return p->nick;
}

/*==========================================
 * 名前がnickのPCを探す。居なければNULL
 *------------------------------------------
 */
struct map_session_data * map_nick2sd(char *nick)
{
	return strdb_search(nick_db,nick);
}

/*==========================================
 * id番号の物を探す
 * 一時objectの場合は配列を引くのみ
 *------------------------------------------
 */
struct block_list * map_id2bl(int id)
{
	if(id<sizeof(object)/sizeof(object[0]))
		return object[id];
	return numdb_search(id_db,id);
}

/*==========================================
 * id_db内の全てにfuncを実行
 *------------------------------------------
 */
int map_foreachiddb(int (*func)(void*,void*,va_list),...)
{
	va_list ap;

	va_start(ap,func);
	numdb_foreach(id_db,func,ap);
	va_end(ap);
	return 0;
}

/*==========================================
 * map.npcへ追加 (warp等の領域持ちのみ)
 *------------------------------------------
 */
int map_addnpc(int m,struct npc_data *nd)
{
	int i;
	if(m<0 || m>=map_num)
		return -1;
	for(i=0;i<map[m].npc_num && i<MAX_NPC_PER_MAP;i++)
		if(map[m].npc[i]==NULL)
			break;
	if(i==MAX_NPC_PER_MAP){
		printf("too many NPCs in one map %s\n",map[m].name);
		return -1;
	}
	if(i==map[m].npc_num){
		map[m].npc_num++;
	}
	map[m].npc[i]=nd;
	nd->n = i;
	numdb_insert(id_db,nd->bl.id,nd);

	return i;
}

/*==========================================
 * map名からmap番号へ変換
 *------------------------------------------
 */
int map_mapname2mapid(char *name)
{
	struct map_data *md;

	md=strdb_search(map_db,name);
	if(md==NULL || md->gat==NULL)
		return -1;
	return md->m;
}

/*==========================================
 * 他鯖map名からip,port変換
 *------------------------------------------
 */
int map_mapname2ipport(char *name,int *ip,int *port)
{
	struct map_data_other_server *mdos;

	mdos=strdb_search(map_db,name);
	if(mdos==NULL || mdos->gat)
		return -1;
	*ip=mdos->ip;
	*port=mdos->port;
	return 0;
}

/*==========================================
 * 彼我の方向を計算
 *------------------------------------------
 */
int map_calc_dir( struct block_list *src,int x,int y)
{
	int dir=0;
	int dx=x-src->x,dy=y-src->y;
	if( dx==0 && dy==0 ){	// 彼我の場所一致
		dir=0;	// 上
	}else if( dx>=0 && dy>=0 ){	// 方向的に右上
		dir=7;						// 右上
		if( dx*3-1<dy ) dir=0;		// 上
		if( dx>dy*3 ) dir=6;		// 右
	}else if( dx>=0 && dy<=0 ){	// 方向的に右下
		dir=5;						// 右下
		if( dx*3-1<-dy ) dir=4;		// 下
		if( dx>-dy*3 ) dir=6;		// 右
	}else if( dx<=0 && dy<=0 ){ // 方向的に左下
		dir=3;						// 左下
		if( dx*3+1>dy ) dir=4;		// 下
		if( dx<dy*3 ) dir=2;		// 左
	}else{						// 方向的に左上
		dir=1;						// 左上
		if( -dx*3-1<dy ) dir=0;		// 上
		if( -dx>dy*3 ) dir=2;		// 左
	}
	return dir;
}

// gat系
/*==========================================
 * (m,x,y)の状態を調べる
 *------------------------------------------
 */
int map_getcell(int m,int x,int y)
{
	if(x<0 || x>=map[m].xs-1 || y<0 || y>=map[m].ys-1)
		return 1;
	return map[m].gat[x+y*map[m].xs];
}

/*==========================================
 * (m,x,y)の状態をtにする
 *------------------------------------------
 */
int map_setcell(int m,int x,int y,int t)
{
	if(x<0 || x>=map[m].xs || y<0 || y>=map[m].ys)
		return t;
	return map[m].gat[x+y*map[m].xs]=t;
}

/*==========================================
 * 他鯖管理のマップをdbに追加
 *------------------------------------------
 */
int map_setipport(char *name,unsigned long ip,int port)
{
	struct map_data *md;
	struct map_data_other_server *mdos;

	md=strdb_search(map_db,name);
	if(md==NULL){ // not exist -> add new data
		mdos=malloc(sizeof(*mdos));
		if(mdos==NULL){
			printf("out of memory : map_setipport\n");
			exit(1);
		}
		memcpy(mdos->name,name,16);
		mdos->gat  = NULL;
		mdos->ip   = ip;
		mdos->port = port;
		strdb_insert(map_db,mdos->name,mdos);
	} else {
		if(md->gat){ // local -> check data
			if(ip!=clif_getip() || port!=clif_getport()){
				printf("from char server : %s -> %08lx:%d\n",name,ip,port);
				return 1;
			}
		} else { // update
			mdos=(struct map_data_other_server *)md;
			mdos->ip   = ip;
			mdos->port = port;
		}
	}
	return 0;
}

// 初期化周り
/*==========================================
 * 水場ファイルの第4列で水場高さ設定
 *------------------------------------------
 */
static struct {
	char mapname[24];
	int waterheight;
} *waterlist=NULL;

#define NO_WATER 1000000

static int map_waterheight(char *mapname)
{
	int i;
	for(i=0;waterlist[i].mapname[0] && i < MAX_MAP_PER_SERVER;i++)
		if(strcmp(waterlist[i].mapname,mapname)==0)
			return waterlist[i].waterheight;
	return NO_WATER;
}

static void map_readwater(char *watertxt)
{
	char line[1024],w1[1024];
	FILE *fp;
	int n=0;

	fp=fopen(watertxt,"r");
	if(fp==NULL){
		printf("file not found: %s\n",watertxt);
		return;
	}
	if(waterlist==NULL)
		waterlist=calloc(MAX_MAP_PER_SERVER,sizeof(*waterlist));
	while(fgets(line,1020,fp) && n < MAX_MAP_PER_SERVER){
		int wh,count;
		if(line[0] == '/' && line[1] == '/')
			continue;
		if((count=sscanf(line,"%s%d",w1,&wh)) < 1){
			continue;
		}
		strcpy(waterlist[n].mapname,w1);
		if(count >= 2)
			waterlist[n].waterheight = wh;
		else
			waterlist[n].waterheight = 3;
		n++;
	}
	fclose(fp);
}

/*==========================================
 * マップ1枚読み込み
 *------------------------------------------
 */
static int map_readmap(int m,char *fn)
{
	unsigned char *gat;
	int s;
	int x,y,xs,ys;
	struct gat_1cell {float high[4]; int type;} *p;
	int wh;

	// read & convert fn
	gat=grfio_read(fn);
	if(gat==NULL)
		return -1;

	map[m].m=m;
	xs=map[m].xs=*(int*)(gat+6);
	ys=map[m].ys=*(int*)(gat+10);
	map[m].gat=malloc(s=map[m].xs*map[m].ys);
	if(map[m].gat==NULL){
		printf("out of memory : map_readmap gat\n");
		exit(1);
	}
	map[m].npc_num=0;
	map[m].users=0;
	memset(&map[m].flag,0,sizeof(map[m].flag));
	wh=map_waterheight(map[m].name);
	for(y=0;y<ys;y++){
		p=(struct gat_1cell*)(gat+y*xs*20+14);
		for(x=0;x<xs;x++){
			if(wh!=NO_WATER && p->type==0){
				// 水場判定
				map[m].gat[x+y*xs]=(p->high[0]>wh || p->high[1]>wh || p->high[2]>wh || p->high[3]>wh) ? 3 : 0;
			} else {
				map[m].gat[x+y*xs]=p->type;
			}
			p++;
		}
	}
	free(gat);

	map[m].bxs=(xs+BLOCK_SIZE-1)/BLOCK_SIZE;
	map[m].bys=(ys+BLOCK_SIZE-1)/BLOCK_SIZE;

	map[m].block=malloc(map[m].bxs*map[m].bys*sizeof(struct block_list*));
	if(map[m].block==NULL){
		printf("out of memory : map_readmap block\n");
		exit(1);
	}
	memset(map[m].block,0,map[m].bxs*map[m].bys*sizeof(struct block_list*));

	map[m].block_mob=malloc(map[m].bxs*map[m].bys*sizeof(struct block_list*));
	if(map[m].block_mob==NULL){
		printf("out of memory : map_readmap block_mob\n");
		exit(1);
	}
	memset(map[m].block_mob,0,map[m].bxs*map[m].bys*sizeof(struct block_list*));

	strdb_insert(map_db,map[m].name,&map[m]);
	printf("%s read done\n",fn);

	return 0;
}

/*==========================================
 * 全てのmapデータを読み込む
 *------------------------------------------
 */
int map_readallmap(void)
{
	int i;
	char fn[256];

	// 先に全部のマップの存在を確認
	for(i=0;i<map_num;i++){
		if(strstr(map[i].name,".gat")==NULL)
			continue;
		sprintf(fn,"data\\%s",map[i].name);
		grfio_size(fn);
	}
	for(i=0;i<map_num;i++){
		if(strstr(map[i].name,".gat")==NULL)
			continue;
		sprintf(fn,"data\\%s",map[i].name);
		map_readmap(i,fn);
	}
	free(waterlist);
	return 0;
}

/*==========================================
 * 読み込むmapを追加する
 *------------------------------------------
 */
int map_addmap(char *mapname)
{
	if(map_num>=MAX_MAP_PER_SERVER-1){
		printf("too many map\n");
		return 1;
	}
	memcpy(map[map_num].name,mapname,16);
	map_num++;
	return 0;
}

/*==========================================
 * 設定ファイルを読み込む
 *------------------------------------------
 */
int map_config_read(char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;
	struct hostent *h=NULL;

	fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)){
		if(line[0] == '/' && line[1] == '/')
			continue;
		i=sscanf(line,"%[^:]: %[^\r\n\t]",w1,w2);
		if(i!=2)
			continue;
		if(strcmpi(w1,"userid")==0){
			chrif_setuserid(w2);
		} else if(strcmpi(w1,"passwd")==0){
			chrif_setpasswd(w2);
		} else if(strcmpi(w1,"char_ip")==0){
			h = gethostbyname (w2);
			if(h != NULL) { 
				printf("Character sever IP address : %s -> %d.%d.%d.%d\n",w2,(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
				sprintf(w2,"%d.%d.%d.%d",(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
			}
			chrif_setip(w2);
		} else if(strcmpi(w1,"char_port")==0){
			chrif_setport(atoi(w2));
		} else if(strcmpi(w1,"map_ip")==0){
			h = gethostbyname (w2);
			if(h != NULL) { 
				printf("Map server IP address : %s -> %d.%d.%d.%d\n",w2,(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
				sprintf(w2,"%d.%d.%d.%d",(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
			}
			clif_setip(w2);
		} else if(strcmpi(w1,"map_port")==0){
			clif_setport(atoi(w2));
		} else if(strcmpi(w1,"water_height")==0){
			map_readwater(w2);
		} else if(strcmpi(w1,"gm_account_filename")==0){
			pc_set_gm_account_fname(w2);
		} else if(strcmpi(w1,"map")==0){
			map_addmap(w2);
		} else if(strcmpi(w1,"npc")==0){
			npc_addsrcfile(w2);
		} else if(strcmpi(w1,"data_grf")==0){
			grfio_setdatafile(w2);
		} else if(strcmpi(w1,"sdata_grf")==0){
			grfio_setsdatafile(w2);
		} else if(strcmpi(w1,"autosave_time")==0){
			autosave_interval=atoi(w2)*1000;
			if(autosave_interval <= 0)
				autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
		}
	}
	fclose(fp);

	return 0;
}

/*==========================================
 * map鯖終了時処理
 *------------------------------------------
 */
void do_final(void)
{
	do_final_itemdb();
	do_final_storage();
}

/*==========================================
 * map鯖初期化の大元
 *------------------------------------------
 */
int do_init(int argc,char *argv[])
{
	srand(gettick());

	if(map_config_read((argc<2)? MAP_CONF_NAME:argv[1]))
		exit(1);
	battle_config_read((argc>2)? argv[2]:BATTLE_CONF_FILENAME);
	atcommand_config_read((argc>3)? argv[3]:ATCOMMAND_CONF_FILENAME);
	script_config_read((argc>4)? argv[4]:SCRIPT_CONF_NAME);

	atexit(do_final);

	id_db = numdb_init();
	map_db = strdb_init(16);
	nick_db = strdb_init(24);
	charid_db = numdb_init();

	grfio_init((argc>5)? argv[5]:GRF_PATH_FILENAME);
	map_readallmap();

	add_timer_func_list(map_clearflooritem_timer,"map_clearflooritem_timer");

	do_init_chrif();
	do_init_clif();
	do_init_mob();	// npcの初期化時内でmob_spawnして、mob_dbを参照するのでinit_npcより先
	do_init_script();
	do_init_npc();
	do_init_pc();
	do_init_itemdb();
	do_init_storage();
	do_init_party();
	do_init_guild();
	do_init_skill();
	do_init_pet();
	npc_event_do_oninit();	// npcのOnInitイベント実行

	return 0;
}


