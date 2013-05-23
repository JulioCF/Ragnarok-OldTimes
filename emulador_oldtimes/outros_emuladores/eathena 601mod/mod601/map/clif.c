// $Id: clif.c,v 1.19 2003/07/04 15:26:33 lemit Exp $

#define DUMP_UNKNOWN_PACKET	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"
#include "timer.h"

#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "pc.h"
#include "npc.h"
#include "itemdb.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "script.h"
#include "skill.h"
#include "atcommand.h"
#include "intif.h"
#include "battle.h"
#include "mob.h"
#include "party.h"
#include "guild.h"
#include "vending.h"
#include "pet.h"
#include "version.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static const int packet_len_table[0x200]={
   10,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0, 55, 17,  3, 37,  46, -1, 23, -1,  3,108,  3,  2,
#if PACKETVER < 2
    3, 28, 19, 11,  3, -1,  9,  5,  52, 51, 56, 58, 41,  2,  6,  6,
#else
    3, 28, 19, 11,  3, -1,  9,  5,  54, 53, 58, 60, 41,  2,  6,  6,
#endif
// 78-7b 亀島以降 lv99エフェクト用

    7,  3,  2,  2,  2,  5, 16, 12,  10,  7, 29, 23, -1, -1, -1,  0,
    7, 22, 28,  2,  6, 30, -1, -1,   3, -1, -1,  5,  9, 17, 17,  6,
   23,  6,  6, -1, -1, -1, -1,  8,   7,  6,  7,  4,  7,  0, -1,  6,
    8,  8,  3,  3, -1,  6,  6, -1,   7,  6,  2,  5,  6, 44,  5,  3,

    7,  2,  6,  8,  6,  7, -1, -1,  -1, -1,  3,  3,  6,  6,  2, 27,
    3,  4,  4,  2, -1, -1,  3, -1,   6, 14,  3, -1, 28, 29, -1, -1,
   30, 30, 26,  2,  6, 26,  3,  3,   8, 19,  5,  2,  3,  2,  2,  2,
    3,  2,  6,  8, 21,  8,  8,  2,   2, 26,  3, -1,  6, 27, 30, 10,


    2,  6,  6, 30, 79, 31, 10, 10,  -1, -1,  4,  6,  6,  2, 11, -1,
   10, 39,  4, 10, 31, 35, 10, 18,   2, 13, 15, 20, 68,  2,  3, 16,
    6, 14, -1, -1, 21,  8,  8,  8,   8,  8,  2,  2,  3,  4,  2, -1,
    6, 86,  6, -1, -1,  7, -1,  6,   3, 16,  4,  4,  4,  6, 24, 26,

   22, 14,  6, 10, 23, 19,  6, 39,   8,  9,  6, 27, -1,  2,  6,  6,
  110,  6, -1, -1, -1, -1, -1,  6,  -1, 54, 66, 54, 90, 42,  6, 42,
   -1, -1, -1, -1, -1, 30, -1,  3,  14,  3, 30, 10, 43, 14,186,182,
   14, 30, 10,  3, -1,  6,106, -1,   4,  5,  4, -1,  6,  7, -1, -1,

    6,  3,106, 10, 10, 34,  0,  6,   8,  4,  4,  4, 29, -1, 10,  6,
#if PACKETVER < 1
   90, 86, 24,  6, 30,102,  8,  4,   8,  4, 14, 10, -1,  6,  2,  6,
#else
   90, 86, 24,  6, 30,102,  9,  4,   8,  4, 14, 10, -1,  6,  2,  6,
#endif
// 196 comodo以降 状態表示アイコン用
    3,  3, 35,  5, 11, 26, -1,  4,   4,  6, 10, 12,  6, -1,  4,  4,
   11,  7, -1, 67, 12, 18,114,  6,   3,  6, 26, 26, 26, 26,  2,  3,

    2, 14, 10, -1, 22, 22,  0,  0,  13, 97,  0,  0,  0,  0,  0,  0,
    8,  0, 10,  0,  0,  0,  0, 11,   0,  0,  0,  0,  0,  0, 33,  6,
    0,  8,  0,  0,  0,  0,  0,  0,  28,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

};

// local define
enum {
	ALL_CLIENT,
	ALL_SAMEMAP,
	AREA,
	AREA_WOS,
	AREA_WOC,
	AREA_WOSC,
	CHAT,
	CHAT_WOS,
	PARTY,
	PARTY_WOS,
	PARTY_SAMEMAP,
	PARTY_SAMEMAP_WOS,
	PARTY_AREA,
	PARTY_AREA_WOS,
	GUILD,
	GUILD_WOS,
	SELF };

#define WBUFPOS(p,pos,x,y) { unsigned char *__p = (p); __p+=(pos); __p[0] = (x)>>2; __p[1] = ((x)<<6) | (((y)>>4)&0x3f); __p[2] = (y)<<4; }
#define WBUFPOS2(p,pos,x0,y0,x1,y1) { unsigned char *__p = (p); __p+=(pos); __p[0] = (x0)>>2; __p[1] = ((x0)<<6) | (((y0)>>4)&0x3f); __p[2] = ((y0)<<4) | (((x1)>>6)&0x0f); __p[3]=((x1)<<2) | (((y1)>>8)&0x03); __p[4]=(y1); }

#define WFIFOPOS(fd,pos,x,y) { WBUFPOS (WFIFOP(fd,pos),0,x,y); }
#define WFIFOPOS2(fd,pos,x0,y0,x1,y1) { WBUFPOS2(WFIFOP(fd,pos),0,x0,y0,x1,y1); }

static char map_ip_str[16];
static in_addr_t map_ip;
static int map_port = 5121;


/*==========================================
 * map鯖のip設定
 *------------------------------------------
 */
void clif_setip(char *ip)
{
	memcpy(map_ip_str,ip,16);
	map_ip=inet_addr(map_ip_str);
}

/*==========================================
 * map鯖のport設定
 *------------------------------------------
 */
void clif_setport(int port)
{
	map_port=port;
}

/*==========================================
 * map鯖のip読み出し
 *------------------------------------------
 */
in_addr_t clif_getip(void)
{
	return map_ip;
}

/*==========================================
 * map鯖のport読み出し
 *------------------------------------------
 */
int clif_getport(void)
{
	return map_port;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_countusers(void)
{
	int users=0,i;
	struct map_session_data *sd;

	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth)
			users++;
	}
	return users;
}

/*==========================================
 * 全てのclientに対してfunc()実行
 *------------------------------------------
 */
int clif_foreachclient(int (*func)(struct map_session_data*,va_list),...)
{
	int i;
	va_list ap;
	struct map_session_data *sd;

	va_start(ap,func);
	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth)
			func(sd,ap);
	}
	va_end(ap);
	return 0;
}

/*==========================================
 * clif_sendでAREA*指定時用
 *------------------------------------------
 */
int clif_send_sub(struct block_list *bl,va_list ap)
{
	unsigned char *buf;
	int len;
	struct block_list *src_bl;
	int type;
	struct map_session_data *sd;

	buf=va_arg(ap,unsigned char*);
	len=va_arg(ap,int);
	src_bl=va_arg(ap,struct block_list*);
	type=va_arg(ap,int);

	sd=(struct map_session_data *)bl;
	switch(type){
	case AREA_WOS:
		if(bl==src_bl)
			return 0;
		break;
	case AREA_WOC:
		if(sd->chatID || bl==src_bl)
			return 0;
		break;
	case AREA_WOSC:
		if(sd->chatID && sd->chatID == ((struct map_session_data*)src_bl)->chatID)
			return 0;
		break;
	}
	memcpy(WFIFOP(sd->fd,0),buf,len);
	WFIFOSET(sd->fd,len);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send(unsigned char *buf,int len,struct block_list *bl,int type)
{
	int i;
	struct map_session_data *sd;
	struct chat_data *cd;
	struct party *p=NULL;
	struct guild *g=NULL;
	int x0=0,x1=0,y0=0,y1=0;

	switch(type){
	case ALL_CLIENT:	// 全クライアントに送信
		for(i=0;i<fd_max;i++){
			if(session[i] && (sd=session[i]->session_data) && sd->state.auth){
				memcpy(WFIFOP(i,0),buf,len);
				WFIFOSET(i,len);
			}
		}
		break;
	case ALL_SAMEMAP:	// 同じマップの全クライアントに送信
		for(i=0;i<fd_max;i++){
			if(session[i] && (sd=session[i]->session_data) && sd->state.auth &&
			   sd->bl.m == bl->m){
				memcpy(WFIFOP(i,0),buf,len);
				WFIFOSET(i,len);
			}
		}
		break;
	case AREA:
	case AREA_WOS:
	case AREA_WOC:
	case AREA_WOSC:
		map_foreachinarea(clif_send_sub,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,BL_PC,buf,len,bl,type);
		break;
	case CHAT:
	case CHAT_WOS:
		cd=(struct chat_data*)bl;
		if(bl->type==BL_PC){
			sd=(struct map_session_data*)bl;
			cd=(struct chat_data*)map_id2bl(sd->chatID);
		} else if(bl->type!=BL_CHAT)
			break;
		if(cd==NULL)
			break;
		for(i=0;i<cd->users;i++){
			if(type==CHAT_WOS && cd->usersd[i]==(struct map_session_data*)bl)
				continue;
			memcpy(WFIFOP(cd->usersd[i]->fd,0),buf,len);
			WFIFOSET(cd->usersd[i]->fd,len);
		}
		break;

	case PARTY_AREA:		// 同じ画面内の全パーティーメンバに送信
	case PARTY_AREA_WOS:	// 自分以外の同じ画面内の全パーティーメンバに送信
		x0=bl->x-AREA_SIZE;
		y0=bl->y-AREA_SIZE;
		x1=bl->x+AREA_SIZE;
		y1=bl->y+AREA_SIZE;
	case PARTY:				// 全パーティーメンバに送信
	case PARTY_WOS:			// 自分以外の全パーティーメンバに送信
	case PARTY_SAMEMAP:		// 同じマップの全パーティーメンバに送信
	case PARTY_SAMEMAP_WOS:	// 自分以外の同じマップの全パーティーメンバに送信
		if(bl->type==BL_PC){
			sd=(struct map_session_data *)bl;
			if(sd->status.party_id>0)
				p=party_search(sd->status.party_id);
		}
		if(p){
			for(i=0;i<MAX_PARTY;i++){
				if((sd=p->member[i].sd)!=NULL){
					if( sd->bl.id==bl->id && (type==PARTY_WOS ||
						type==PARTY_SAMEMAP_WOS || type==PARTY_AREA_WOS))
						continue;
					if(type!=PARTY && type!=PARTY_WOS && bl->m!=sd->bl.m)	// マップチェック
						continue;
					if((type==PARTY_AREA || type==PARTY_AREA_WOS) &&
						(sd->bl.x<x0 || sd->bl.y<y0 ||
						 sd->bl.x>x1 || sd->bl.y>y1 ) )
						continue;
						
					memcpy(WFIFOP(sd->fd,0),buf,len);
					WFIFOSET(sd->fd,len);
//					printf("send party %d %d %d\n",p->party_id,i,flag);
				}
			}
		}
		break;
	case SELF:
		sd=(struct map_session_data *)bl;
		memcpy(WFIFOP(sd->fd,0),buf,len);
		WFIFOSET(sd->fd,len);
		break;

	case GUILD:
	case GUILD_WOS:
		if(bl->type==BL_PC){
			sd=(struct map_session_data *)bl;
			if(sd->status.guild_id>0)
				g=guild_search(sd->status.guild_id);
		}
		if(g){
			for(i=0;i<g->max_member;i++){
				if((sd=g->member[i].sd)!=NULL){
					if(type==GUILD_WOS && sd->bl.id==bl->id)
						continue;
					memcpy(WFIFOP(sd->fd,0),buf,len);
					WFIFOSET(sd->fd,len);
				}
			}
		}
		break;
		
	default:
		printf("clif_send まだ作ってないよー\n");
		return -1;
	}
	return 0;
}

//
// パケット作って送信
//
/*==========================================
 *
 *------------------------------------------
 */
int clif_authok(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x73;
	WFIFOL(fd,2)=gettick();
	WFIFOPOS(fd,6,sd->bl.x,sd->bl.y);
	WFIFOB(fd,9)=5;
	WFIFOB(fd,10)=5;
	WFIFOSET(fd,packet_len_table[0x73]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_authfail_fd(int fd,int type)
{
	WFIFOW(fd,0)=0x81;
	WFIFOL(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x81]);

	clif_setwaitclose(fd);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_charselectok(int id)
{
	struct map_session_data *sd;
	int fd;

	if((sd=map_id2sd(id))==NULL)
		return 1;

	fd=sd->fd;
	WFIFOW(fd,0)=0xb3;
	WFIFOB(fd,2)=1;
	WFIFOSET(fd,packet_len_table[0xb3]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set009e(struct flooritem_data *fitem,unsigned char *buf)
{
	//009e <ID>.l <name ID>.w <identify flag>.B <X>.w <Y>.w <subX>.B <subY>.B <amount>.w
	WBUFW(buf,0)=0x9e;
	WBUFL(buf,2)=fitem->bl.id;
	WBUFW(buf,6)=fitem->item_data.nameid;
	WBUFB(buf,8)=fitem->item_data.identify;
	WBUFW(buf,9)=fitem->bl.x;
	WBUFW(buf,11)=fitem->bl.y;
	WBUFB(buf,13)=fitem->subx;
	WBUFB(buf,14)=fitem->suby;
	WBUFW(buf,15)=fitem->item_data.amount;

	return packet_len_table[0x9e];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dropflooritem(struct flooritem_data *fitem)
{
	char buf[64];

	if(fitem->item_data.nameid==0)
		return 0;
	clif_set009e(fitem,buf);
	clif_send(buf,packet_len_table[0x9e],&fitem->bl,AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearflooritem(struct flooritem_data *fitem,int fd)
{
	unsigned char buf[16];

	WBUFW(buf,0) = 0xa1;
	WBUFL(buf,2) = fitem->bl.id;

	if(fd==0){
		clif_send(buf,packet_len_table[0xa1],&fitem->bl,AREA);
	} else {
		memcpy(WFIFOP(fd,0),buf,6);
		WFIFOSET(fd,packet_len_table[0xa1]);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar(struct block_list *bl,int type)
{
	unsigned char buf[16];

	WBUFW(buf,0) = 0x80;
	WBUFL(buf,2) = bl->id;
	WBUFB(buf,6) = type;
	clif_send(buf,packet_len_table[0x80],bl,type==1 ? AREA : AREA_WOS);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar_id(int id,int type,int fd)
{
	unsigned char buf[16];

	WBUFW(buf,0) = 0x80;
	WBUFL(buf,2) = id;
	WBUFB(buf,6) = type;
	memcpy(WFIFOP(fd,0),buf,7);
	WFIFOSET(fd,packet_len_table[0x80]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0078(struct map_session_data *sd,unsigned char *buf)
{
	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->opt1;
	WBUFW(buf,10)=sd->opt2;
	WBUFW(buf,12)=sd->status.option;
	WBUFW(buf,14)=sd->status.class;
	WBUFW(buf,16)=sd->status.hair;
	WBUFW(buf,18)=sd->status.weapon;
	WBUFW(buf,20)=sd->status.head_bottom;
	WBUFW(buf,22)=sd->status.shield;
	WBUFW(buf,24)=sd->status.head_top;
	WBUFW(buf,26)=sd->status.head_mid;
	WBUFW(buf,28)=sd->status.hair_color;
	WBUFW(buf,30)=sd->status.clothes_color;
	WBUFW(buf,32)=sd->head_dir;
	WBUFL(buf,34)=sd->status.guild_id;
	WBUFL(buf,38)=sd->guild_emblem_id;
	WBUFW(buf,42)=sd->status.manner;
	WBUFB(buf,44)=sd->status.karma;
	WBUFB(buf,45)=sd->sex;
	WBUFPOS(buf,46,sd->bl.x,sd->bl.y);
	WBUFB(buf,48)|=sd->dir&0x0f;
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;
	WBUFB(buf,51)=sd->state.dead_sit;
	WBUFW(buf,52)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_len_table[0x78];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set007b(struct map_session_data *sd,unsigned char *buf)
{
	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->opt1;
	WBUFW(buf,10)=sd->opt2;
	WBUFW(buf,12)=sd->status.option;
	WBUFW(buf,14)=sd->status.class;
	WBUFW(buf,16)=sd->status.hair;
	WBUFW(buf,18)=sd->status.weapon;
	WBUFW(buf,20)=sd->status.head_bottom;
	WBUFL(buf,22)=gettick();
	WBUFW(buf,26)=sd->status.shield;
	WBUFW(buf,28)=sd->status.head_top;
	WBUFW(buf,30)=sd->status.head_mid;
	WBUFW(buf,32)=sd->status.hair_color;
	WBUFW(buf,34)=sd->status.clothes_color;
	WBUFW(buf,36)=sd->head_dir;
	WBUFL(buf,38)=sd->status.guild_id;
	WBUFL(buf,42)=sd->guild_emblem_id;
	WBUFW(buf,46)=sd->status.manner;
	WBUFB(buf,48)=sd->status.karma;
	WBUFB(buf,49)=sd->sex;
	WBUFPOS2(buf,50,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y);
	WBUFB(buf,55)=0;
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;
	WBUFW(buf,58)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set01e1(struct map_session_data *sd,unsigned char *buf)
{
	WBUFW(buf,0)=0x1e1;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->spiritball;

	return packet_len_table[0x1e1];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0192(int fd,int m,int x,int y,int type)
{
	WFIFOW(fd,0) = 0x192;
	WFIFOW(fd,2) = x;
	WFIFOW(fd,4) = y;
	WFIFOW(fd,6) = type;
	memcpy(WFIFOP(fd,8),map[m].name,16);
	WFIFOSET(fd,packet_len_table[0x192]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpc(struct map_session_data *sd)
{
	clif_set0078(sd,WFIFOP(sd->fd,0));
	WFIFOW(sd->fd,0)=0x79;
	WFIFOW(sd->fd,51)=(sd->status.base_level>99)?99:sd->status.base_level;
	clif_send(WFIFOP(sd->fd,0),packet_len_table[0x79],&sd->bl,AREA_WOS);

	if(sd->spiritball > 0)
		clif_spiritball(sd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnnpc(struct npc_data *nd)
{
	unsigned char buf[64];

	memset(buf,0,64);

	WBUFW(buf,0)=0x7c;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,20)=nd->class;
	//change 26 to 36
	WBUFPOS(buf,36,nd->bl.x,nd->bl.y);

	clif_send(buf,packet_len_table[0x7c],&nd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnmob(struct mob_data *md)
{
	unsigned char buf[64];

	memset(buf,0,64);

	WBUFW(buf,0)=0x7c;
	WBUFL(buf,2)=md->bl.id;
	WBUFW(buf,6)=md->speed;
	WBUFW(buf,20)=md->class;
	WBUFPOS(buf,36,md->bl.x,md->bl.y);

	clif_send(buf,packet_len_table[0x7c],&md->bl,AREA);

	return 0;
}

// pet

/*==========================================
 *
 *------------------------------------------
 */
static int clif_pet0078(struct npc_data *nd,unsigned char *buf)
{
	memset(buf,0,packet_len_table[0x78]);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,14)=nd->class;
	WBUFW(buf,16)=0x14;
	WBUFW(buf,20)=nd->equip;
	WBUFPOS(buf,46,nd->bl.x,nd->bl.y);
	WBUFB(buf,48)|=nd->dir&0x0f;
	WBUFB(buf,49)=0;
	WBUFB(buf,50)=0;

	return packet_len_table[0x78];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_pet007b(struct npc_data *nd,unsigned char *buf)
{
	memset(buf,0,packet_len_table[0x7b]);

	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,14)=nd->class;
	WBUFW(buf,16)=0x14;
	WBUFW(buf,20)=nd->equip;
	WBUFL(buf,22)=gettick();
	WBUFPOS2(buf,50,nd->bl.x,nd->bl.y,nd->to_x,nd->to_y);
	WBUFB(buf,56)=0;
	WBUFB(buf,57)=0;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpet(struct npc_data *nd)
{
	unsigned char buf[64];

	memset(buf,0,64);

	WBUFW(buf,0)=0x7c;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,20)=nd->class;
	WBUFPOS(buf,36,nd->bl.x,nd->bl.y);

	clif_send(buf,packet_len_table[0x7c],&nd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movepet(struct npc_data *nd)
{
	unsigned char buf[256];

	clif_pet007b(nd,buf);
	clif_send(buf,packet_len_table[0x7b],&nd->bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_servertick(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x7f;
	WFIFOL(fd,2)=sd->server_tick;
	WFIFOSET(fd,packet_len_table[0x7f]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_walkok(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x87;
	WFIFOL(fd,2)=gettick();;
	WFIFOPOS2(fd,6,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y);
	WFIFOB(fd,11)=0;
	WFIFOSET(fd,packet_len_table[0x87]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movechar(struct map_session_data *sd)
{
	int fd=sd->fd;

	clif_set007b(sd,WFIFOP(fd,0));
	clif_send(WFIFOP(fd,0),packet_len_table[0x7b],&sd->bl,AREA_WOS);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_quitsave(int fd,struct map_session_data *sd)
{
	if(sd->status.pet_id && sd->pet_npcdata) {
		pet_remove_map(sd);
		if(sd->pet.intimate <= 0) {
			intif_delete_petdata(sd->status.pet_id);
			sd->status.pet_id = 0;
			sd->pet_npcdata = NULL;
			sd->petDB = NULL;
		}
		else
			intif_save_petdata(sd->status.account_id,&sd->pet);
	}
	chrif_save(sd);
	map_quit(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_waitclose(int tid,unsigned int tick,int id,int data)
{
	if(session[id])
		session[id]->eof=1;
	close(id);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_setwaitclose(int fd)
{
	add_timer(gettick()+5000,clif_waitclose,fd,0);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemap(struct map_session_data *sd,char *mapname,int x,int y)
{
	int fd=sd->fd;
	
	WFIFOW(fd,0)=0x91;
	memcpy(WFIFOP(fd,2),mapname,16);
	WFIFOW(fd,18)=x;
	WFIFOW(fd,20)=y;
	WFIFOSET(fd,packet_len_table[0x91]);
	
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapserver(struct map_session_data *sd,char *mapname,int x,int y,int ip,int port)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x92;
	memcpy(WFIFOP(fd,2),mapname,16);
	WFIFOW(fd,18)=x;
	WFIFOW(fd,20)=y;
	WFIFOL(fd,22)=ip;
	WFIFOW(fd,26)=port;
	WFIFOSET(fd,packet_len_table[0x92]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpos(struct block_list *bl)
{
	char buf[16];

	WBUFW(buf,0)=0x88;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=bl->x;
	WBUFW(buf,8)=bl->y;

	clif_send(buf,packet_len_table[0x88],bl,AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_npcbuysell(struct map_session_data* sd,int id)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xc4;
	WFIFOL(fd,2)=id;
	WFIFOSET(fd,packet_len_table[0xc4]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_buylist(struct map_session_data *sd,struct npc_data *nd)
{
	int fd=sd->fd,i;

	WFIFOW(fd,0)=0xc6;
	for(i=0;nd->u.shop_item[i].nameid;i++){
		WFIFOL(fd,4+i*11)=nd->u.shop_item[i].value;
		WFIFOL(fd,8+i*11)=pc_modifybuyvalue(sd,nd->u.shop_item[i].value);
		WFIFOB(fd,12+i*11)=itemdb_type(nd->u.shop_item[i].nameid);
		WFIFOW(fd,13+i*11)=nd->u.shop_item[i].nameid;
	}
	WFIFOW(fd,2)=i*11+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_selllist(struct map_session_data *sd)
{
	int fd=sd->fd,i,c=0,val;

	WFIFOW(fd,0)=0xc7;
	for(i=0;i<MAX_INVENTORY;i++) {
		if(sd->status.inventory[i].nameid) {
			val=itemdb_sellvalue(sd->status.inventory[i].nameid);
			if(val<=0)
				continue;
			WFIFOW(fd,4+c*10)=i+2;
			WFIFOL(fd,6+c*10)=val;
			WFIFOL(fd,10+c*10)=pc_modifysellvalue(sd,val);
			c++;
		}
	}
	WFIFOW(fd,2)=c*10+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmes(struct map_session_data *sd,int npcid,char *mes)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xb4;
	WFIFOW(fd,2)=strlen(mes)+9;
	WFIFOL(fd,4)=npcid;
	strcpy(WFIFOP(fd,8),mes);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptnext(struct map_session_data *sd,int npcid)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xb5;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0xb5]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptclose(struct map_session_data *sd,int npcid)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xb6;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0xb6]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmenu(struct map_session_data *sd,int npcid,char *mes)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xb7;
	WFIFOW(fd,2)=strlen(mes)+8;
	WFIFOL(fd,4)=npcid;
	strcpy(WFIFOP(fd,8),mes);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinput(struct map_session_data *sd,int npcid)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x142;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_len_table[0x142]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_viewpoint(struct map_session_data *sd,int npc_id,int type,int x,int y,int id,int color)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x144;
	WFIFOL(fd,2)=npc_id;
	WFIFOL(fd,6)=type;
	WFIFOL(fd,10)=x;
	WFIFOL(fd,14)=y;
	WFIFOB(fd,18)=id;
	WFIFOL(fd,19)=color;
	WFIFOSET(fd,packet_len_table[0x144]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_cutin(struct map_session_data *sd,char *image,int type)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x145;
	memcpy(WFIFOP(fd,2),image,16);
	WFIFOB(fd,18)=type;
	WFIFOSET(fd,packet_len_table[0x145]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_additem(struct map_session_data *sd,int n,int amount,int fail)
{
	int fd=sd->fd;
	unsigned char *buf=WFIFOP(fd,0);

	if(n<0 || n>=MAX_INVENTORY || sd->status.inventory[n].nameid==0)
		return 1;

	WBUFW(buf,0)=0xa0;
	WBUFW(buf,2)=n+2;
	WBUFW(buf,4)=amount;
	WBUFW(buf,6)=sd->status.inventory[n].nameid;
	WBUFB(buf,8)=sd->status.inventory[n].identify;
	WBUFB(buf,9)=sd->status.inventory[n].attribute;
	WBUFB(buf,10)=sd->status.inventory[n].refine;
	WBUFW(buf,11)=sd->status.inventory[n].card[0];
	WBUFW(buf,13)=sd->status.inventory[n].card[1];
	WBUFW(buf,15)=sd->status.inventory[n].card[2];
	WBUFW(buf,17)=sd->status.inventory[n].card[3];
	WBUFW(buf,19)=itemdb_equippoint(sd,sd->status.inventory[n].nameid);
	WBUFB(buf,21)=itemdb_type(sd->status.inventory[n].nameid);
	WBUFB(buf,22)=fail;

	WFIFOSET(fd,packet_len_table[0xa0]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_delitem(struct map_session_data *sd,int n,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xaf;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=amount;

	WFIFOSET(fd,packet_len_table[0xaf]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_itemlist(struct map_session_data *sd)
{
	int i,n,fd=sd->fd,arrow=-1;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0xa3;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==0 ||
		   itemdb_isequip(sd->status.inventory[i].nameid))
			continue;
		WBUFW(buf,n*10+4)=i+2;
		WBUFW(buf,n*10+6)=sd->status.inventory[i].nameid;
		WBUFB(buf,n*10+8)=itemdb_type(sd->status.inventory[i].nameid);
		WBUFB(buf,n*10+9)=sd->status.inventory[i].identify;
		WBUFW(buf,n*10+10)=sd->status.inventory[i].amount;
		if(itemdb_equip(sd->status.inventory[i].nameid) == 0x8000) {
			WBUFW(buf,n*10+12)=0x8000;
			if(sd->status.inventory[i].equip) arrow=i;	// ついでに矢装備チェック
		}
		else
			WBUFW(buf,n*10+12)=0;
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	if(arrow!=-1)
		clif_arrowequip(sd,arrow);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equiplist(struct map_session_data *sd)
{
	int i,n,id,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0xa4;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if((id=sd->status.inventory[i].nameid)==0 ||
		   !itemdb_isequip(sd->status.inventory[i].nameid))
			continue;
		WBUFW(buf,n*20+4)=i+2;
		WBUFW(buf,n*20+6)=id;
		WBUFB(buf,n*20+8)=itemdb_type(id);
		WBUFB(buf,n*20+9)=sd->status.inventory[i].identify;
		WBUFW(buf,n*20+10)=itemdb_equippoint(sd,id);
		WBUFW(buf,n*20+12)=sd->status.inventory[i].equip;
		WBUFB(buf,n*20+14)=sd->status.inventory[i].attribute;
		WBUFB(buf,n*20+15)=sd->status.inventory[i].refine;
		WBUFW(buf,n*20+16)=sd->status.inventory[i].card[0];
		WBUFW(buf,n*20+18)=sd->status.inventory[i].card[1];
		WBUFW(buf,n*20+20)=sd->status.inventory[i].card[2];
		WBUFW(buf,n*20+22)=sd->status.inventory[i].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;	
}

/*==========================================
 * カプラさんに預けてある消耗品&収集品リスト
 *------------------------------------------
 */
int clif_storageitemlist(struct map_session_data *sd,struct storage *stor)
{
	int i,n,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0xa5;
	for(i=0,n=0;i<MAX_STORAGE;i++){
		if(stor->storage[i].nameid==0 ||
		   itemdb_isequip(stor->storage[i].nameid))
			continue;
		WBUFW(buf,n*10+4)=i+1;
		WBUFW(buf,n*10+6)=stor->storage[i].nameid;
		WBUFB(buf,n*10+8)=itemdb_type(stor->storage[i].nameid);
		WBUFB(buf,n*10+9)=stor->storage[i].identify;
		WBUFW(buf,n*10+10)=stor->storage[i].amount;
		WBUFW(buf,n*10+12)=0;
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * カプラさんに預けてある装備リスト
 *------------------------------------------
 */
int clif_storageequiplist(struct map_session_data *sd,struct storage *stor)
{
	int i,n,id,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0xa6;
	for(i=0,n=0;i<MAX_STORAGE;i++){
		if((id=stor->storage[i].nameid)==0 ||
		   !itemdb_isequip(stor->storage[i].nameid))
			continue;
		WBUFW(buf,n*20+4)=i+1;
		WBUFW(buf,n*20+6)=id;
		WBUFB(buf,n*20+8)=itemdb_type(id);
		WBUFB(buf,n*20+9)=stor->storage[i].identify;
		WBUFW(buf,n*20+10)=itemdb_equip(id);
		WBUFW(buf,n*20+12)=stor->storage[i].equip;
		WBUFB(buf,n*20+14)=stor->storage[i].attribute;
		WBUFB(buf,n*20+15)=stor->storage[i].refine;
		WBUFW(buf,n*20+16)=stor->storage[i].card[0];
		WBUFW(buf,n*20+18)=stor->storage[i].card[1];
		WBUFW(buf,n*20+20)=stor->storage[i].card[2];
		WBUFW(buf,n*20+22)=stor->storage[i].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * ステータスを送りつける
 * 表示専用数字はこの中で計算して送る
 *------------------------------------------
 */
int clif_updatestatus(struct map_session_data *sd,int type)
{
	int fd=sd->fd,len=8;

	WFIFOW(fd,0)=0xb0;
	WFIFOW(fd,2)=type;
	switch(type){
		// 00b0
	case SP_WEIGHT:
		pc_checkweighticon(sd);
		WFIFOW(fd,0)=0xb0;
		WFIFOW(fd,2)=type;
		WFIFOL(fd,4)=sd->weight;
		break;
	case SP_MAXWEIGHT:
		WFIFOL(fd,4)=sd->max_weight;
		break;
	case SP_SPEED:
		WFIFOL(fd,4)=sd->speed;
		break;
	case SP_BASELEVEL:
		WFIFOL(fd,4)=sd->status.base_level;
		break;
	case SP_JOBLEVEL:
		WFIFOL(fd,4)=sd->status.job_level;
		break;
	case SP_STATUSPOINT:
		WFIFOL(fd,4)=sd->status.status_point;
		break;
	case SP_SKILLPOINT:
		WFIFOL(fd,4)=sd->status.skill_point;
		break;
	case SP_HIT:
		WFIFOL(fd,4)=sd->hit;
		break;
	case SP_FLEE1:
		WFIFOL(fd,4)=sd->flee;
		break;
	case SP_FLEE2:
		WFIFOL(fd,4)=sd->flee2;
		break;
	case SP_MAXHP:
		WFIFOL(fd,4)=sd->status.max_hp;
		break;
	case SP_MAXSP:
		WFIFOL(fd,4)=sd->status.max_sp;
		break;
	case SP_HP:
		WFIFOL(fd,4)=sd->status.hp;
		break;
	case SP_SP:
		WFIFOL(fd,4)=sd->status.sp;
		break;
	case SP_ASPD:
		WFIFOL(fd,4)=sd->aspd;
		break;
	case SP_ATK1:
		WFIFOL(fd,4)=sd->paramc[0]+sd->watk;
		break;
	case SP_DEF1:
		WFIFOL(fd,4)=sd->def;
		break;
	case SP_MDEF1:
		WFIFOL(fd,4)=sd->mdef;
		break;
	case SP_ATK2:
		WFIFOL(fd,4)=sd->watk2;
		break;
	case SP_DEF2:
		WFIFOL(fd,4)=sd->def2;
		break;
	case SP_MDEF2:
		WFIFOL(fd,4)=sd->mdef2;
		break;
	case SP_CRITICAL:
		WFIFOL(fd,4)=sd->critical;
		break;
	case SP_MATK1:
		WFIFOL(fd,4)=sd->matk1;
		break;
	case SP_MATK2:
		WFIFOL(fd,4)=sd->matk2;
		break;

		// 00b1 終了
	case SP_ZENY:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=sd->status.zeny;
		break;
	case SP_BASEEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=sd->status.base_exp;
		break;
	case SP_JOBEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=sd->status.job_exp;
		break;
	case SP_NEXTBASEEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=pc_nextbaseexp(sd);
		break;
	case SP_NEXTJOBEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=pc_nextjobexp(sd);
		break;

		// 00be 終了
	case SP_USTR:
	case SP_UAGI:
	case SP_UVIT:
	case SP_UINT:
	case SP_UDEX:
	case SP_ULUK:
		WFIFOW(fd,0)=0xbe;
		WFIFOB(fd,4)=pc_need_status_point(sd,type-SP_USTR+SP_STR);
		len=5;
		break;

		// 013a 終了
	case SP_ATTACKRANGE:
		WFIFOW(fd,0)=0x13a;
		WFIFOW(fd,2)=sd->attackrange;
		len=4;
		break;

		// 0141 終了
	case SP_STR:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.str;
		WFIFOL(fd,10)=sd->paramb[0];
		len=14;
		break;
	case SP_AGI:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.agi;
		WFIFOL(fd,10)=sd->paramb[1];
		len=14;
		break;
	case SP_VIT:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.vit;
		WFIFOL(fd,10)=sd->paramb[2];
		len=14;
		break;
	case SP_INT:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.int_;
		WFIFOL(fd,10)=sd->paramb[3];
		len=14;
		break;
	case SP_DEX:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.dex;
		WFIFOL(fd,10)=sd->paramb[4];
		len=14;
		break;
	case SP_LUK:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.luk;
		WFIFOL(fd,10)=sd->paramb[5];
		len=14;
		break;
	
	case SP_CARTINFO:
		WFIFOW(fd,0)=0x121;
		WFIFOW(fd,2)=sd->cart_num;
		WFIFOW(fd,4)=sd->cart_max_num;
		WFIFOL(fd,6)=sd->cart_weight;
		WFIFOL(fd,10)=sd->cart_max_weight;
		len=14;
		break;
		
	default:
		printf("clif_updatestatus : make %d routine\n",type);
		return 1;
	}
	WFIFOSET(fd,len);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changelook(struct block_list *bl,int type,int val)
{
	unsigned char buf[32];
	int equip_l=0,equip_r=0;
	struct map_session_data *sd=NULL;
	sd = (struct map_session_data *)bl;

	if (battle_config.equip_modifydisplay &&
		(type==LOOK_WEAPON || type==LOOK_SHIELD || type==LOOK_SHOES)){
		WBUFW(buf,0)=0x1d7;
		WBUFL(buf,2)=bl->id;
		switch(type) {
		case LOOK_WEAPON:
		case LOOK_SHIELD:
			WBUFB(buf,6)=0x02;  // 武器＆盾、というか、手装備？
			if ((equip_l=pc_checkequip(sd,34)) <= 0){
				equip_l=pc_checkequip(sd,2);
				equip_r=pc_checkequip(sd,32);
			}
			break;
		case LOOK_SHOES:
			WBUFB(buf,6)=0x09;  // 靴
			equip_l=pc_checkequip(sd,64);
			break;
		}
		WBUFW(buf,7)=(equip_l>0)?equip_l:0;
		WBUFW(buf,9)=(equip_r>0)?equip_r:0;
		clif_send(buf,packet_len_table[0x1d7],bl,AREA); // packet length is 11 bytes
	} else if (type!=LOOK_SHOES){
		WBUFW(buf,0)=0xc3;
		WBUFL(buf,2)=bl->id;
		WBUFB(buf,6)=type;
		WBUFB(buf,7)=val;
		clif_send(buf,packet_len_table[0xc3],bl,AREA);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_initialstatus(struct map_session_data *sd)
{
	int fd=sd->fd,i;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0xbd;
	WBUFW(buf,2)=sd->status.status_point;
	WBUFB(buf,4)=sd->status.str;
	WBUFB(buf,5)=pc_need_status_point(sd,SP_STR);
	WBUFB(buf,6)=sd->status.agi;
	WBUFB(buf,7)=pc_need_status_point(sd,SP_AGI);
	WBUFB(buf,8)=sd->status.vit;
	WBUFB(buf,9)=pc_need_status_point(sd,SP_VIT);
	WBUFB(buf,10)=sd->status.int_;
	WBUFB(buf,11)=pc_need_status_point(sd,SP_INT);
	WBUFB(buf,12)=sd->status.dex;
	WBUFB(buf,13)=pc_need_status_point(sd,SP_DEX);
	WBUFB(buf,14)=sd->status.luk;
	WBUFB(buf,15)=pc_need_status_point(sd,SP_LUK);

	WBUFW(buf,16) = sd->status.str + sd->watk;
	WBUFW(buf,18) = sd->watk2; //atk bonus
	i=sd->paramc[3];
	WBUFW(buf,20) = sd->matk1;
	WBUFW(buf,22) = sd->matk2;
	WBUFW(buf,24) = sd->def; // def
	WBUFW(buf,26) = sd->def2;
	WBUFW(buf,28) = sd->mdef; // mdef
	WBUFW(buf,30) = sd->mdef2;
	WBUFW(buf,32) = sd->hit;
	WBUFW(buf,34) = sd->flee;
	WBUFW(buf,36) = sd->flee2;
	WBUFW(buf,38) = sd->critical;
	WBUFW(buf,40) = sd->status.karma;
	WBUFW(buf,42) = sd->status.manner;

	WFIFOSET(fd,packet_len_table[0xbd]);

	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);

	clif_updatestatus(sd,SP_ATTACKRANGE);
	clif_updatestatus(sd,SP_ASPD);

	return 0;
}
/*==========================================
 *矢装備
 *------------------------------------------
 */
int clif_arrowequip(struct map_session_data *sd,int val)
{
	int fd=sd->fd;
	
	WFIFOW(fd,0)=0x013c;
	WFIFOW(fd,2)=val+2;//矢のアイテムID

	WFIFOSET(fd,packet_len_table[0x013c]);
	
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
 int clif_arrow_fail(struct map_session_data *sd,int type)
 {
 	int fd=sd->fd;

 	WFIFOW(fd,0)=0x013b;
  	WFIFOW(fd,2)=type;

	WFIFOSET(fd,packet_len_table[0x013b]);
 	
 	return 0;

 }
/*==========================================
 * 作成可能 矢リスト送信
 *------------------------------------------
 */
int clif_arrow_create_list(struct map_session_data *sd)
{
	int i,c;
	int fd=sd->fd;
	
	WFIFOW(fd,0)=0x1ad;

	for(i=1,c=1;i<MAX_SKILL_ARROW_DB;i++){
		if(pc_search_inventory(sd,skill_arrow_db[i].nameid)!=-1&&pc_search_inventory(sd,skill_arrow_db[i].nameid)!=pc_search_inventory(sd,0)){
			WFIFOW(fd,c*2+2) = skill_arrow_db[i].nameid;
			c++;
		}
	}
	WFIFOW(fd,2)=c*2+2;

	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_statusupack(struct map_session_data *sd,int type,int ok,int val)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xbc;
	WFIFOW(fd,2)=type;
	WFIFOB(fd,4)=ok;
	WFIFOB(fd,5)=val;
	WFIFOSET(fd,packet_len_table[0xbc]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equipitemack(struct map_session_data *sd,int n,int pos,int ok)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xaa;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=pos;
	WFIFOB(fd,6)=ok;
	WFIFOSET(fd,packet_len_table[0xaa]);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_unequipitemack(struct map_session_data *sd,int n,int pos,int ok)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xac;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=pos;
	WFIFOB(fd,6)=ok;
	WFIFOSET(fd,packet_len_table[0xac]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_misceffect(struct block_list* bl,int type)
{
	char buf[32];

	WBUFW(buf,0) = 0x19b;
	WBUFL(buf,2) = bl->id;
	WBUFL(buf,6) = type;

	clif_send(buf,packet_len_table[0x19b],bl,AREA);
	return 0;
}

/*==========================================
 * 表示オプション変更
 *------------------------------------------
 */
int clif_changeoption(struct block_list* bl)
{
	char buf[32];
	short option = *battle_get_option(bl);
	struct status_change *sc_data = battle_get_sc_data(bl);
	static const int omask[]={ 0x10,0x20 };
	static const int scnum[]={ SC_FALCON, SC_RIDING };
	int i;

	WBUFW(buf,0) = 0x119;
	WBUFL(buf,2) = bl->id;
	WBUFW(buf,6) = *battle_get_opt1(bl);
	WBUFW(buf,8) = *battle_get_opt2(bl);
	WBUFW(buf,10) = option;
	WBUFB(buf,12) = 0;	// ??

	clif_send(buf,packet_len_table[0x119],bl,AREA);
	
	// アイコンの表示
	for(i=0;i<sizeof(omask)/sizeof(omask[0]);i++){
		if( option&omask[i] ){
			if( sc_data[scnum[i]].timer==-1)
				skill_status_change_start(bl,scnum[i],0,0);
		}else{
			skill_status_change_end(bl,scnum[i],-1);
		}
	}
	
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_useitemack(struct map_session_data *sd,int index,int amount,int ok)
{
#if PACKETVER < 3
	int fd=sd->fd;

	WFIFOW(fd,0)=0xa8;
	WFIFOW(fd,2)=index+2;
	WFIFOW(fd,4)=amount;
	WFIFOB(fd,6)=ok;
	WFIFOSET(fd,packet_len_table[0xa8]);
	return 0;
#else
	char buf[32];

	WBUFW(buf,0)=0x1c8;
	WBUFW(buf,2)=index+2;
	WBUFW(buf,4)=sd->status.inventory[index].nameid;
	WBUFL(buf,6)=sd->bl.id;
	WBUFW(buf,10)=amount;
	WBUFB(buf,12)=ok;

	clif_send(buf,packet_len_table[0x1c8],&sd->bl,AREA);
#endif

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_createchat(struct map_session_data *sd,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xd6;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xd6]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dispchat(struct chat_data *cd,int fd)
{
	char buf[128];	// 最大title(60バイト)+17

	if(cd==NULL || *cd->owner==NULL)
		return 1;

	WBUFW(buf,0)=0xd7;
	WBUFW(buf,2)=strlen(cd->title)+17;
	WBUFL(buf,4)=(*cd->owner)->id;
	WBUFL(buf,8)=cd->bl.id;
	WBUFW(buf,12)=cd->limit;
	WBUFW(buf,14)=cd->users;
	WBUFB(buf,16)=cd->pub;
	strcpy(WBUFP(buf,17),cd->title);
	if(fd){
		memcpy(WFIFOP(fd,0),buf,WBUFW(buf,2));
		WFIFOSET(fd,WBUFW(buf,2));
	} else {
		clif_send(buf,WBUFW(buf,2),*cd->owner,AREA_WOSC);
	}		
	return 0;
}

/*==========================================
 * chatの状態変更成功
 * 外部の人用と命令コード(d7->df)が違うだけ
 *------------------------------------------
 */
int clif_changechatstatus(struct chat_data *cd)
{
	char buf[128];	// 最大title(60バイト)+17

	if(cd==NULL || cd->usersd[0]==NULL)
		return 1;

	WBUFW(buf,0)=0xdf;
	WBUFW(buf,2)=strlen(cd->title)+17;
	WBUFL(buf,4)=cd->usersd[0]->bl.id;
	WBUFL(buf,8)=cd->bl.id;
	WBUFW(buf,12)=cd->limit;
	WBUFW(buf,14)=cd->users;
	WBUFB(buf,16)=cd->pub;
	strcpy(WBUFP(buf,17),cd->title);
	clif_send(buf,WBUFW(buf,2),&cd->usersd[0]->bl,CHAT);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchat(struct chat_data *cd,int fd)
{
	char buf[32];

	WBUFW(buf,0)=0xd8;
	WBUFL(buf,2)=cd->bl.id;
	if(fd){
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0xd8]);
		WFIFOSET(fd,packet_len_table[0xd8]);
	} else {
		clif_send(buf,packet_len_table[0xd8],&cd->usersd[0]->bl,AREA_WOSC);
	}		
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatfail(struct map_session_data *sd,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xda;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xda]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatok(struct map_session_data *sd,struct chat_data* cd)
{
	int fd=sd->fd;
	int i;

	WFIFOW(fd,0)=0xdb;
	WFIFOW(fd,2)=8+(28*cd->users);
	WFIFOL(fd,4)=cd->bl.id;
	for(i = 0;i < cd->users;i++){
		WFIFOL(fd,8+i*28) = (i!=0)||((*cd->owner)->type==BL_NPC);
		memcpy(WFIFOP(fd,8+i*28+4),cd->usersd[i]->status.name,24);
	}
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_addchat(struct chat_data* cd,struct map_session_data *sd)
{
	char buf[32];

	WBUFW(buf, 0) = 0x0dc;
	WBUFW(buf, 2) = cd->users;
	memcpy(WBUFP(buf, 4),sd->status.name,24);
	clif_send(buf,packet_len_table[0xdc],&sd->bl,CHAT_WOS);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changechatowner(struct chat_data* cd,struct map_session_data *sd)
{
	char buf[64];

	WBUFW(buf, 0) = 0xe1;
	WBUFL(buf, 2) = 1;
	memcpy(WBUFP(buf,6),cd->usersd[0]->status.name,24);
	WBUFW(buf,30) = 0xe1;
	WBUFL(buf,32) = 0;
	memcpy(WBUFP(buf,36),sd->status.name,24);

	clif_send(buf,packet_len_table[0xe1]*2,&sd->bl,CHAT);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_leavechat(struct chat_data* cd,struct map_session_data *sd)
{
	char buf[32];

	WBUFW(buf, 0) = 0xdd;
	WBUFW(buf, 2) = cd->users-1;
	memcpy(WBUFP(buf,4),sd->status.name,24);
	WBUFB(buf,28) = 0;

	clif_send(buf,packet_len_table[0xdd],&sd->bl,CHAT);
	return 0;
}

/*==========================================
 * 取り引き要請受け
 *------------------------------------------
 */
int clif_traderequest(struct map_session_data *sd,char *name)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xe5;
	strcpy(WFIFOP(fd,2),name);
	WFIFOSET(fd,packet_len_table[0xe5]);

	return 0;
}

/*==========================================
 * 取り引き要求応答
 *------------------------------------------
 */
int clif_tradestart(struct map_session_data *sd,int type)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xe7;
	WFIFOB(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0xe7]);

	return 0;
}

/*==========================================
 * 相手方からのアイテム追加
 *------------------------------------------
 */
int clif_tradeadditem(struct map_session_data *sd,struct map_session_data *tsd,int index,int amount)
{
	int fd=tsd->fd;

	WFIFOW(fd,0)=0xe9;
	WFIFOL(fd,2)=amount;
	if(index==0){
		WFIFOW(fd,6) =0; // type id
		WFIFOB(fd,8) =0; //identify flag
		WFIFOB(fd,9) =0; // attribute
		WFIFOB(fd,10)=0; //refine
		WFIFOW(fd,11)=0; //card (4w)
		WFIFOW(fd,13)=0; //card (4w)
		WFIFOW(fd,15)=0; //card (4w)
		WFIFOW(fd,17)=0; //card (4w)
	}else{
		WFIFOW(fd,6) =sd->status.inventory[index-2].nameid; // type id
		WFIFOB(fd,8) =sd->status.inventory[index-2].identify; //identify flag
		WFIFOB(fd,9) =sd->status.inventory[index-2].attribute; // attribute
		WFIFOB(fd,10)=sd->status.inventory[index-2].refine; //refine
		WFIFOW(fd,11)=sd->status.inventory[index-2].card[0]; //card (4w)
		WFIFOW(fd,13)=sd->status.inventory[index-2].card[1]; //card (4w)
		WFIFOW(fd,15)=sd->status.inventory[index-2].card[2]; //card (4w)
		WFIFOW(fd,17)=sd->status.inventory[index-2].card[3]; //card (4w)
	}
	WFIFOSET(fd,packet_len_table[0xe9]);

	return 0;
}

/*==========================================
 * アイテム追加成功/失敗
 *------------------------------------------
 */
int clif_tradeitemok(struct map_session_data *sd,int index,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xea;
	WFIFOW(fd,2)=index;
	WFIFOB(fd,4)=fail;
	WFIFOSET(fd,packet_len_table[0xea]);

	return 0;
}

/*==========================================
 * 取り引きok押し
 *------------------------------------------
 */
int clif_tradedeal_lock(struct map_session_data *sd,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xec;
	WFIFOB(fd,2)=fail; // 0=you 1=the other person
	WFIFOSET(fd,packet_len_table[0xec]);

	return 0;
}

/*==========================================
 * 取り引きがキャンセルされました
 *------------------------------------------
 */
int clif_tradecancelled(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xee;
	WFIFOSET(fd,packet_len_table[0xee]);

	return 0;
}

/*==========================================
 * 取り引き完了
 *------------------------------------------
 */
int clif_tradecompleted(struct map_session_data *sd,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xf0;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xf0]);

	return 0;
}

/*==========================================
 * カプラ倉庫のアイテム数を更新
 *------------------------------------------
 */
int clif_updatestorageamount(struct map_session_data *sd,struct storage *stor)
{
	int fd=sd->fd;

	WFIFOW(fd,0) = 0xf2;  // update storage amount
	WFIFOW(fd,2) = stor->storage_amount;  //items
	WFIFOW(fd,4) = MAX_STORAGE; //items max
	WFIFOSET(fd,packet_len_table[0xf2]);

	return 0;
}

/*==========================================
 * カプラ倉庫にアイテムを追加する
 *------------------------------------------
 */
int clif_storageitemadded(struct map_session_data *sd,struct storage *stor,int index,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0) =0xf4; // Storage item added
	WFIFOW(fd,2) =index+1; // index
	WFIFOL(fd,4) =amount; // amount
	WFIFOW(fd,8) =stor->storage[index].nameid; // id
	WFIFOB(fd,10)=stor->storage[index].identify; //identify flag
	WFIFOB(fd,11)=stor->storage[index].attribute; // attribute
	WFIFOB(fd,12)=stor->storage[index].refine; //refine
	WFIFOW(fd,13)=stor->storage[index].card[0]; //card (4w)
	WFIFOW(fd,15)=stor->storage[index].card[1]; //card (4w)
	WFIFOW(fd,17)=stor->storage[index].card[2]; //card (4w)
	WFIFOW(fd,19)=stor->storage[index].card[3]; //card (4w)
	WFIFOSET(fd,packet_len_table[0xf4]);

	return 0;
}

/*==========================================
 * カプラ倉庫からアイテムを取り去る
 *------------------------------------------
 */
int clif_storageitemremoved(struct map_session_data *sd,int index,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xf6; // Storage item removed
	WFIFOW(fd,2)=index+1;
	WFIFOL(fd,4)=amount;
	WFIFOSET(fd,packet_len_table[0xf6]);

	return 0;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
int clif_storageclose(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xf8; // Storage Closed
	WFIFOSET(fd,packet_len_table[0xf8]);

	return 0;
}

//
// callback系 ?
//
/*==========================================
 * PC表示
 *------------------------------------------
 */
void clif_getareachar_pc(struct map_session_data* sd,struct map_session_data* dstsd)
{
	if(dstsd->walktimer != -1){
		clif_set007b(dstsd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,packet_len_table[0x7b]);
	} else {
		clif_set0078(dstsd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,packet_len_table[0x78]);
	}
	if(dstsd->chatID){
		struct chat_data *cd;
		cd=(struct chat_data*)map_id2bl(dstsd->chatID);
		if(cd->usersd[0]==dstsd)
			clif_dispchat(cd,sd->fd);
	}
	if(dstsd->vender_id){
		clif_showvendingboard(&dstsd->bl,dstsd->message,sd->fd);
	}
	if(dstsd->spiritball > 0) {
		clif_set01e1(dstsd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,packet_len_table[0x1e1]);
	}
}

/*==========================================
 * NPC表示
 *------------------------------------------
 */
void clif_getareachar_npc(struct map_session_data* sd,struct npc_data* nd)
{
	int fd=sd->fd;

	if(nd->class<0 || nd->state.flag&1 )
		return;

	memset(WFIFOP(fd,0),0,packet_len_table[0x78]);
	WFIFOW(fd,0)=0x78;
	WFIFOL(fd,2)=nd->bl.id;
	WFIFOW(fd,6)=nd->speed;
	WFIFOW(fd,14)=nd->class;
	WFIFOPOS(fd,46,nd->bl.x,nd->bl.y);
	WFIFOB(fd,48)|=nd->dir&0x0f;
	WFIFOB(fd,49)=5;
	WFIFOB(fd,50)=5;
	WFIFOSET(fd,packet_len_table[0x78]);
	
	if(nd->chat_id){
		clif_dispchat((struct chat_data*)map_id2bl(nd->chat_id),sd->fd);
	}

}

/*==========================================
 * MOB表示1
 *------------------------------------------
 */
static int clif_mob0078(struct mob_data *md,unsigned char *buf)
{
	memset(buf,0,packet_len_table[0x78]);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=md->bl.id;
	WBUFW(buf,6)=md->speed;
	WBUFW(buf,8)=md->opt1;
	WBUFW(buf,10)=md->opt2;
	WBUFW(buf,12)=md->option;
	WBUFW(buf,14)=md->class;
	WBUFPOS(buf,46,md->bl.x,md->bl.y);
	WBUFB(buf,48)|=md->dir&0x0f;
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;

	return packet_len_table[0x78];
}

/*==========================================
 * MOB表示2
 *------------------------------------------
 */
static int clif_mob007b(struct mob_data *md,unsigned char *buf)
{
	memset(buf,0,packet_len_table[0x7b]);

	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=md->bl.id;
	WBUFW(buf,6)=md->speed;
	WBUFW(buf,8)=md->opt1;
	WBUFW(buf,10)=md->opt2;
	WBUFW(buf,12)=md->option;
	WBUFW(buf,14)=md->class;
	WBUFL(buf,22)=gettick();
	WBUFPOS2(buf,50,md->bl.x,md->bl.y,md->to_x,md->to_y);
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;

	return packet_len_table[0x7b];
}

/*==========================================
 * 移動停止
 *------------------------------------------
 */
int clif_movemob(struct mob_data *md)
{
	unsigned char buf[256];

	clif_mob007b(md,buf);
	clif_send(buf,packet_len_table[0x7b],&md->bl,AREA);

	return 0;
}

/*==========================================
 * モンスターの位置修正
 *------------------------------------------
 */
int clif_fixmobpos(struct mob_data *md)
{
	unsigned char buf[256];

	if(md->state.state == MS_WALK){
		clif_mob007b(md,buf);
		clif_send(buf,packet_len_table[0x7b],&md->bl,AREA);
	} else {
		clif_mob0078(md,buf);
		clif_send(buf,packet_len_table[0x78],&md->bl,AREA);
	}
	return 0;
}
/*==========================================
 * PCの位置修正
 *------------------------------------------
 */
int clif_fixpcpos(struct map_session_data *sd)
{
	unsigned char buf[256];

	if(sd->walktimer != -1){
		clif_set007b(sd,buf);
		clif_send(buf,packet_len_table[0x7b],&sd->bl,AREA);
	} else {
		clif_set0078(sd,buf);
		clif_send(buf,packet_len_table[0x78],&sd->bl,AREA);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpetpos(struct npc_data *nd)
{
	unsigned char buf[256];

	if(nd->state.state == MS_WALK){
		clif_pet007b(nd,buf);
		clif_send(buf,packet_len_table[0x7b],&nd->bl,AREA);
	} else {
		clif_pet0078(nd,buf);
		clif_send(buf,packet_len_table[0x78],&nd->bl,AREA);
	}
	return 0;
}

/*==========================================
 * 通常攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_damage(struct block_list *src,struct block_list *dst,unsigned int tick,int sdelay,int ddelay,int damage,int div,int type,int damage2)
{
	unsigned char buf[256];
	struct map_session_data *sd;

	if(dst->type==BL_PC) {
		sd=(struct map_session_data *)dst;
		if(sd->sc_data[SC_ENDURE].timer != -1)
			type = 9;
	}
	WBUFW(buf,0)=0x8a;
	WBUFL(buf,2)=src->id;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=tick;
	WBUFL(buf,14)=sdelay;
	WBUFL(buf,18)=ddelay;
	WBUFW(buf,22)=damage;
	WBUFW(buf,24)=div;
	WBUFB(buf,26)=type;
	WBUFW(buf,27)=damage2;
	clif_send(buf,packet_len_table[0x8a],src,AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_mob(struct map_session_data* sd,struct mob_data* md)
{
	if(md->state.state == MS_WALK){
		clif_mob007b(md,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,packet_len_table[0x7b]);
	} else {
		clif_mob0078(md,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,packet_len_table[0x78]);
	}
}
/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_pet(struct map_session_data* sd,struct npc_data* nd)
{
	if(nd->state.state == MS_WALK){
		clif_pet007b(nd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,packet_len_table[0x7b]);
	} else {
		clif_pet0078(nd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,packet_len_table[0x78]);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_item(struct map_session_data* sd,struct flooritem_data* fitem)
{
	int fd=sd->fd;

	//009d <ID>.l <item ID>.w <identify flag>.B <X>.w <Y>.w <amount>.w <subX>.B <subY>.B
	WFIFOW(fd,0)=0x9d;
	WFIFOL(fd,2)=fitem->bl.id;
	WFIFOW(fd,6)=fitem->item_data.nameid;
	WFIFOB(fd,8)=fitem->item_data.identify;
	WFIFOW(fd,9)=fitem->bl.x;
	WFIFOW(fd,11)=fitem->bl.y;
	WFIFOW(fd,13)=fitem->item_data.amount;
	WFIFOB(fd,15)=fitem->subx;
	WFIFOB(fd,16)=fitem->suby;

	WFIFOSET(fd,packet_len_table[0x9d]);
}
/*==========================================
 * 場所スキルエフェクトが視界に入る
 *------------------------------------------
 */
int clif_getareachar_skillunit(struct map_session_data *sd,struct skill_unit *unit)
{
	int fd=sd->fd;
#if PACKETVER < 3
	WFIFOW(fd, 0)=0x11f;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOL(fd, 6)=unit->group->src_id;
	WFIFOW(fd,10)=unit->bl.x;
	WFIFOW(fd,12)=unit->bl.y;
	WFIFOB(fd,14)=unit->group->unit_id;
	WFIFOB(fd,15)=0;
	WFIFOSET(fd,packet_len_table[0x11f]);
#else
	WFIFOW(fd, 0)=0x1c9;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOL(fd, 6)=unit->group->src_id;
	WFIFOW(fd,10)=unit->bl.x;
	WFIFOW(fd,12)=unit->bl.y;
	WFIFOB(fd,14)=unit->group->unit_id;
	WFIFOB(fd,15)=0;
	memset(WFIFOP(fd,16),packet_len_table[0x1c9]-16,0);
	WFIFOSET(fd,packet_len_table[0x1c9]);
#endif
	if(unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,5);

	return 0;
}
/*==========================================
 * 場所スキルエフェクトが視界から消える
 *------------------------------------------
 */
int clif_clearchar_skillunit(struct skill_unit *unit,int fd)
{
	WFIFOW(fd, 0)=0x120;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOSET(fd,packet_len_table[0x120]);
	if(unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,unit->val2);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
 int clif_getareachar(struct block_list* bl,va_list ap)
{
	struct map_session_data *sd;

	sd=va_arg(ap,struct map_session_data*);

	switch(bl->type){
	case BL_PC:
		if(sd==(struct map_session_data*)bl)
			break;
		clif_getareachar_pc(sd,(struct map_session_data*) bl);
		break;
	case BL_NPC:
		clif_getareachar_npc(sd,(struct npc_data*) bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd,(struct mob_data*) bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd,(struct npc_data*) bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd,(struct flooritem_data*) bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd,(struct skill_unit *)bl);
		break;
	default:
		printf("get area char ??? %d\n",bl->type);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd,*dstsd;

	sd=va_arg(ap,struct map_session_data*);
	switch(bl->type){
	case BL_PC:
		dstsd = (struct map_session_data*) bl;
		if(sd != dstsd) {
			clif_clearchar_id(dstsd->bl.id,0,sd->fd);
			clif_clearchar_id(sd->bl.id,0,dstsd->fd);
			if(dstsd->chatID){
				struct chat_data *cd;
				cd=(struct chat_data*)map_id2bl(dstsd->chatID);
				if(cd->usersd[0]==dstsd)
					clif_dispchat(cd,sd->fd);
			}
			if(dstsd->vender_id){
				clif_closevendingboard(&dstsd->bl,sd->fd);
			}
		}
		break;
	case BL_NPC:
	case BL_MOB:
	case BL_PET:
		clif_clearchar_id(bl->id,0,sd->fd);
		break;
	case BL_ITEM:
		clif_clearflooritem((struct flooritem_data*)bl,sd->fd);
		break;
	case BL_SKILL:
		clif_clearchar_skillunit((struct skill_unit *)bl,sd->fd);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd,*dstsd;

	sd=va_arg(ap,struct map_session_data*);
	switch(bl->type){
	case BL_PC:
		dstsd = (struct map_session_data *)bl;
		if(sd != dstsd) {
			clif_getareachar_pc(sd,dstsd);
			clif_getareachar_pc(dstsd,sd);
		}
		break;
	case BL_NPC:
		clif_getareachar_npc(sd,(struct npc_data*)bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd,(struct mob_data*)bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd,(struct npc_data*)bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd,(struct flooritem_data*)bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd,(struct skill_unit *)bl);
		break;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_moboutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	md=va_arg(ap,struct mob_data*);
	if(bl->type==BL_PC){
		sd = (struct map_session_data*) bl;
		clif_clearchar_id(md->bl.id,0,sd->fd);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mobinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	md=va_arg(ap,struct mob_data*);
	if(bl->type==BL_PC){
		sd = (struct map_session_data *)bl;
		clif_getareachar_mob(sd,md);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	nd=va_arg(ap,struct npc_data*);
	if(bl->type==BL_PC){
		sd = (struct map_session_data*) bl;
		clif_clearchar_id(nd->bl.id,0,sd->fd);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	nd=va_arg(ap,struct npc_data*);
	if(bl->type==BL_PC){
		sd = (struct map_session_data *)bl;
		clif_getareachar_pet(sd,nd);
	}
	return 0;
}

/*==========================================
 * スキルリストを送信する
 *------------------------------------------
 */
int clif_skillinfoblock(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i,c,len=4,id;
	WFIFOW(fd,0)=0x10f;
	for ( i = c = 0; i < MAX_SKILL; i++){
		if( (id=sd->status.skill[i].id)!=0 ){
			WFIFOW(fd,len  ) = id;
			WFIFOW(fd,len+2) = skill_get_inf(id);
			WFIFOW(fd,len+4) = 0;
			WFIFOW(fd,len+6) = sd->status.skill[i].lv;
			WFIFOW(fd,len+8) = skill_get_sp(id,sd->status.skill[i].lv);
			WFIFOW(fd,len+10)= skill_get_range(id);
			memset(WFIFOP(fd,len+12),0,24);
			if(!(skill_get_inf2(id)&0x01) || battle_config.quest_skill_learn == 1 || (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill) ) 
				WFIFOB(fd,len+36)= 
					(sd->status.skill[i].lv < skill_get_max(id) && sd->status.skill[i].flag ==0 )? 1:0;
			else
				WFIFOB(fd,len+36) = 0;
/*			printf("skill id=%d inf=%d lv=%d sp=%d range=%d up=%d\n",
				WFIFOW(fd,len),WFIFOW(fd,len+2),WFIFOW(fd,len+6),
				WFIFOW(fd,len+8),WFIFOW(fd,len+10),WFIFOB(fd,len+36) );*/
			len+=37;
			c++;
		}
	}
	WFIFOW(fd,2)=len;
	WFIFOSET(fd,len);
//	printf("skill len=%d %d\n",len,c);
	return 0;
}

/*==========================================
 * スキル割り振り通知
 *------------------------------------------
 */
int clif_skillup(struct map_session_data *sd,int skill_num)
{
	int fd=sd->fd;
	WFIFOW(fd,0) = 0x10e;
	WFIFOW(fd,2) = skill_num;
	WFIFOW(fd,4) = sd->status.skill[skill_num].lv;
	WFIFOW(fd,6) = skill_get_sp(skill_num,sd->status.skill[skill_num].lv);
	WFIFOW(fd,8) = skill_get_range(skill_num);
	WFIFOB(fd,10) = 1;
	WFIFOSET(fd,packet_len_table[0x10e]);
	return 0;
}

/*==========================================
 * スキル詠唱エフェクトを送信する
 *------------------------------------------
 */
int clif_skillcasting(struct block_list* bl,
	int src_id,int dst_id,int dst_x,int dst_y,int skill_num,int casttime)
{
	unsigned char buf[32];
	WBUFW(buf,0) = 0x13e;
	WBUFL(buf,2) = src_id;
	WBUFL(buf,6) = dst_id;
	WBUFW(buf,10) = dst_x;
	WBUFW(buf,12) = dst_y;
	WBUFW(buf,14) = skill_num;//魔法詠唱スキル
	WBUFL(buf,16) = skill_get_pl(skill_num);//属性
	WBUFL(buf,20) = casttime;//skill詠唱時間
	clif_send(buf,packet_len_table[0x13e], bl, AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillcastcancel(struct block_list* bl)
{
	unsigned char buf[16];
	WBUFW(buf,0) = 0x1b9;
	WBUFL(buf,2) = bl->id;
	clif_send(buf,packet_len_table[0x1b9], bl, AREA);
	return 0;
}

/*==========================================
 * スキル詠唱失敗
 *------------------------------------------
 */
int clif_skill_fail(struct map_session_data *sd,int skill_id,int type,int btype)
{

	int fd=sd->fd;
	WFIFOW(fd,0) = 0x110;
	WFIFOW(fd,2) = skill_id;
	WFIFOW(fd,4) = btype;
	WFIFOW(fd,6) = 0;
	WFIFOB(fd,8) = 0;
	WFIFOB(fd,9) = type;
	WFIFOSET(fd,packet_len_table[0x110]);
	return 0;
}

/*==========================================
 * スキル攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_skill_damage(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,int skill_id,int skill_lv,int type)
{
	unsigned char buf[64];
	struct map_session_data *sd;

	if(dst->type==BL_PC) {
		sd=(struct map_session_data *)dst;
		if(sd->sc_data[SC_ENDURE].timer != -1)
			type = 9;
	}

#if PACKETVER < 3
	WBUFW(buf,0)=0x114;
	WBUFW(buf,2)=skill_id;
/*	
	static int a=157;
	printf("%d\n",a);
	WBUFW(buf,2)=a++;
*/	
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFW(buf,24)=damage;
	WBUFW(buf,26)=skill_lv;
	WBUFW(buf,28)=div;
	WBUFB(buf,30)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_len_table[0x114],src,AREA);
#else
	WBUFW(buf,0)=0x1de;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFL(buf,24)=damage;
	WBUFW(buf,28)=skill_lv;
	WBUFW(buf,30)=div;
	WBUFB(buf,32)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_len_table[0x1de],src,AREA);
#endif
	return 0;
}
/*==========================================
 * 吹き飛ばしスキル攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_skill_damage2(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,int skill_id,int skill_lv,int type)
{
	unsigned char buf[64];
	struct map_session_data *sd;

	if(dst->type==BL_PC) {
		sd=(struct map_session_data *)dst;
		if(sd->sc_data[SC_ENDURE].timer != -1)
			type = 9;
	}

	WBUFW(buf,0)=0x115;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFW(buf,24)=dst->x;
	WBUFW(buf,26)=dst->y;
	WBUFW(buf,28)=damage;
	WBUFW(buf,30)=skill_lv;
	WBUFW(buf,32)=div;
	WBUFB(buf,34)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_len_table[0x115],src,AREA);

	return 0;
}
/*==========================================
 * 支援/回復スキルエフェクト
 *------------------------------------------
 */
int clif_skill_nodamage(struct block_list *src,struct block_list *dst,
	int skill_id,int heal,int fail)
{
	unsigned char buf[32];

	WBUFW(buf,0)=0x11a;
	WBUFW(buf,2)=skill_id;
	WBUFW(buf,4)=heal;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=src->id;
	WBUFB(buf,14)=fail;
	clif_send(buf,packet_len_table[0x11a],src,AREA);
	return 0;
}
/*==========================================
 * 場所スキルエフェクト
 *------------------------------------------
 */
int clif_skill_poseffect(struct block_list *src,int skill_id,int val,int x,int y,int tick)
{
	unsigned char buf[32];
	WBUFW(buf,0)=0x117;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFW(buf,8)=val;
	WBUFW(buf,10)=x;
	WBUFW(buf,12)=y;
	WBUFL(buf,14)=tick;
	clif_send(buf,packet_len_table[0x117],src,AREA);
	return 0;
}
/*==========================================
 * 場所スキルエフェクト表示
 *------------------------------------------
 */
int clif_skill_setunit(struct skill_unit *unit)
{
	unsigned char buf[128];
#if PACKETVER < 3
	WBUFW(buf, 0)=0x11f;
	WBUFL(buf, 2)=unit->bl.id;
	WBUFL(buf, 6)=unit->group->src_id;
	WBUFW(buf,10)=unit->bl.x;
	WBUFW(buf,12)=unit->bl.y;
	WBUFB(buf,14)=unit->group->unit_id;
	WBUFB(buf,15)=0;
	clif_send(buf,packet_len_table[0x11f],&unit->bl,AREA);
#else
	WBUFW(buf, 0)=0x1c9;
	WBUFL(buf, 2)=unit->bl.id;
	WBUFL(buf, 6)=unit->group->src_id;
	WBUFW(buf,10)=unit->bl.x;
	WBUFW(buf,12)=unit->bl.y;
	WBUFB(buf,14)=unit->group->unit_id;
	WBUFB(buf,15)=0;
	memset(&buf[16],packet_len_table[0x1c9]-16,0);
	clif_send(buf,packet_len_table[0x1c9],&unit->bl,AREA);
#endif
	return 0;
}
/*==========================================
 * 場所スキルエフェクト削除
 *------------------------------------------
 */
int clif_skill_delunit(struct skill_unit *unit)
{
	unsigned char buf[16];
	WBUFW(buf, 0)=0x120;
	WBUFL(buf, 2)=unit->bl.id;
	clif_send(buf,packet_len_table[0x120],&unit->bl,AREA);
	return 0;
}
/*==========================================
 * ワープ場所選択
 *------------------------------------------
 */
int clif_skill_warppoint(struct map_session_data *sd,int skill_num,
	const char *map1,const char *map2,const char *map3,const char *map4)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x11c;
	WFIFOW(fd,2)=skill_num;
	memcpy(WFIFOP(fd, 4),map1,16);
	memcpy(WFIFOP(fd,20),map2,16);
	memcpy(WFIFOP(fd,36),map3,16);
	memcpy(WFIFOP(fd,52),map4,16);
	WFIFOSET(fd,packet_len_table[0x11c]);
	return 0;
}
/*==========================================
 * メモ応答
 *------------------------------------------
 */
int clif_skill_memo(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x11e;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x11e]);
	return 0;
}

/*==========================================
 * モンスター情報
 *------------------------------------------
 */
int clif_skill_estimation(struct map_session_data *sd,struct block_list *dst)
{
	struct mob_data *md;
	unsigned char buf[64];
	int i;
	
	if(dst->type!=BL_MOB)
		return 0;
	md=(struct mob_data *)dst;

	WBUFW(buf, 0)=0x18c;
	WBUFW(buf, 2)=md->class;
	WBUFW(buf, 4)=mob_db[md->class].lv;
	WBUFW(buf, 6)=mob_db[md->class].size;
	WBUFL(buf, 8)=md->hp;
	WBUFW(buf,12)=(mob_db[md->class].def < 10000)? mob_db[md->class].def:100;
	WBUFW(buf,14)=mob_db[md->class].race;
	WBUFW(buf,16)=(mob_db[md->class].mdef < 10000)? mob_db[md->class].mdef:99;
	WBUFW(buf,18)=mob_db[md->class].element%10;
	for(i=0;i<9;i++)
		WBUFB(buf,20+i)= battle_attr_fix(100,i+1,mob_db[md->class].element);
	
	if(sd->status.party_id>0)
		clif_send(buf,packet_len_table[0x18c],&sd->bl,PARTY_AREA);
	else{
		memcpy(WFIFOP(sd->fd,0),buf,packet_len_table[0x18c]);
		WFIFOSET(sd->fd,packet_len_table[0x18c]);
	}
	return 0;
}
/*==========================================
 * アイテム合成可能リスト
 *------------------------------------------
 */
int clif_skill_produce_mix_list(struct map_session_data *sd,int trigger)
{
	int i,c,fd=sd->fd;
	WFIFOW(fd, 0)=0x18d;
	
	for(i=0,c=0;i<MAX_SKILL_PRODUCE_DB;i++){
		if( skill_can_produce_mix(sd,skill_produce_db[i].nameid,trigger) ){
			WFIFOW(fd,c*8+ 4)= skill_produce_db[i].nameid;
			WFIFOW(fd,c*8+ 6)= 0x0012;
			WFIFOL(fd,c*8+ 8)= sd->status.char_id;
			c++;
		}
	}
	WFIFOW(fd, 2)=c*8+8;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}


/*==========================================
 * 状態異常アイコン/メッセージ表示
 *------------------------------------------
 */
int clif_status_change(struct block_list *bl,int type,int flag)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x0196;
	WBUFW(buf,2)=type;
	WBUFL(buf,4)=bl->id;
	WBUFB(buf,8)=flag;
	clif_send(buf,packet_len_table[0x196],bl,AREA);
	return 0;
}

/*==========================================
 * メッセージ表示
 *------------------------------------------
 */
int clif_displaymessage(int fd,char* mes)
{
	WFIFOW(fd,0) = 0x8e;
	WFIFOW(fd,2) = 4+1+strlen(mes);
	memcpy(WFIFOP(fd,4), mes, WFIFOW(fd,2)-4);
	WFIFOSET(fd, WFIFOW(fd,2));

	return 0;
}

/*==========================================
 * 天の声を送信する
 *------------------------------------------
 */
int clif_GMmessage(struct block_list *bl,char* mes,int len,int flag)
{
	unsigned char buf[len+16];
	int lp=(flag&0x10)?8:4;
	WBUFW(buf,0) = 0x9a;
	WBUFW(buf,2) = len+lp;
	WBUFL(buf,4) = 0x65756c62;
	memcpy(WBUFP(buf,lp), mes, len);
	flag&=0x07;
	clif_send(buf, WBUFW(buf,2), bl,
		(flag==1)? ALL_SAMEMAP:
		(flag==2)? AREA:
		(flag==3)? SELF:
		ALL_CLIENT);
	return 0;
}


/*==========================================
 * HPSP回復エフェクトを送信する
 *------------------------------------------
 */
int clif_heal(int fd,int type,int val)
{
	WFIFOW(fd,0)=0x13d;
	WFIFOW(fd,2)=type;
	WFIFOW(fd,4)=val;
	WFIFOSET(fd,packet_len_table[0x13d]);

	return 0;
}

/*==========================================
 * 復活する
 *------------------------------------------
 */
int clif_resurrection(struct block_list *bl,int type)
{
	unsigned char buf[16];

	WBUFW(buf,0)=0x148;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=type;
	clif_send(buf,packet_len_table[0x148],bl,type==1 ? AREA : AREA_WOS);

	return 0;
}

/*==========================================
 * PVP実装？（仮）
 *------------------------------------------
 */
int clif_set0199(int fd,int type)
{
	WFIFOW(fd,0)=0x199;
	WFIFOW(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x199]);

	return 0;
}

/*==========================================
 * PVP実装？(仮)
 *------------------------------------------
 */
int clif_pvpset(struct map_session_data *sd,int pvprank,int pvpnum,int type)
{
	if(type == 2) {
		WFIFOW(sd->fd,0) = 0x19a;
		WFIFOL(sd->fd,2) = sd->bl.id;
		WFIFOL(sd->fd,6) = pvprank;
		WFIFOL(sd->fd,10) = pvpnum;
		WFIFOSET(sd->fd,packet_len_table[0x19a]);
	}
	else {
		char buf[32];

		WBUFW(buf,0) = 0x19a;
		WBUFL(buf,2) = sd->bl.id;
		WBUFL(buf,6) = pvprank;
		WBUFL(buf,10) = pvpnum;
		if(!type)
			clif_send(buf,packet_len_table[0x19a],&sd->bl,AREA);
		else
			clif_send(buf,packet_len_table[0x19a],&sd->bl,ALL_SAMEMAP);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send0199(int map,int type)
{
	struct block_list bl;
	char buf[16];

	bl.m = map;
	WBUFW(buf,0)=0x199;
	WBUFW(buf,2)=type;
	clif_send(buf,packet_len_table[0x199],&bl,ALL_SAMEMAP);

	return 0;
}

/*==========================================
 * 精錬エフェクトを送信する
 *------------------------------------------
 */
int clif_refine(int fd,struct map_session_data *sd,int fail,int index,int val)
{
	WFIFOW(fd,0)=0x188;
	WFIFOW(fd,2)=fail;
	WFIFOW(fd,4)=index;
	WFIFOW(fd,6)=val;
	WFIFOSET(fd,packet_len_table[0x188]);

	return 0;
}

/*==========================================
 * Wisを送信する
 *------------------------------------------
 */
int clif_wis_message(int fd,char *nick,char *mes,int mes_len)
{
	WFIFOW(fd,0)=0x97;
	WFIFOW(fd,2)=mes_len +24+ 4;
	memcpy(WFIFOP(fd,4),nick,24);
	memcpy(WFIFOP(fd,28),mes,mes_len);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * Wisの送信結果を送信する
 *------------------------------------------
 */
int clif_wis_end(int fd,int flag)
{
	WFIFOW(fd,0)=0x98;
	WFIFOW(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x98]);
	return 0;
}

/*==========================================
 * キャラID名前引き結果を送信する
 *------------------------------------------
 */
int clif_solved_charname(struct map_session_data *sd,int char_id)
{
	char *p= map_charid2nick(char_id);
	int fd=sd->fd;
	if(p!=NULL){
		WFIFOW(fd,0)=0x194;
		WFIFOL(fd,2)=char_id;
		memcpy(WFIFOP(fd,6), p,24 );
		WFIFOSET(fd,packet_len_table[0x194]);
	}else{
		map_reqchariddb(sd,char_id);
		chrif_searchcharid(char_id);
	}
	return 0;
}

/*==========================================
 * カードの挿入可能リストを返す
 *------------------------------------------
 */
int clif_use_card(struct map_session_data *sd,int idx)
{
	int i,c;
	int ep=itemdb_equip(sd->status.inventory[idx].nameid);
	int fd=sd->fd;
	WFIFOW(fd,0)=0x017b;
	
	for(i=c=0;i<MAX_INVENTORY;i++){
		int nameid = sd->status.inventory[i].nameid;
		int j;

		if(itemdb_type(nameid)!=4 && itemdb_type(nameid)!=5)	// 武器防具じゃない
			continue;
		if( sd->status.inventory[i].card[0]==0x00ff)	// 製造武器
			continue;
		if( sd->status.inventory[i].card[0]==(short)0xff00)
			continue;
		if( sd->status.inventory[i].identify==0 )	// 未鑑定
			continue;

		if( (itemdb_equip(nameid)&ep)==0)	// 装備個所が違う
			continue;
		if( itemdb_type(nameid)==4 && ep==32)	// 盾カードと両手武器
			continue;

		for(j=0;j<itemdb_slot(nameid);j++){
			if( sd->status.inventory[i].card[j]==0 )
				break;
		}
		if(j==itemdb_slot(nameid))	// すでにカードが一杯
			continue;

		WFIFOW(fd,4+c*2)=i+2;
		c++;
	}
	WFIFOW(fd,2)=4+c*2;
	WFIFOSET(fd,WFIFOW(fd,2));
	
	return 0;
}
/*==========================================
 * カードの挿入終了
 *------------------------------------------
 */
int clif_insert_card(struct map_session_data *sd,int idx_equip,int idx_card,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x17d;
	WFIFOW(fd,2)=idx_equip+2;
	WFIFOW(fd,4)=idx_card+2;
	WFIFOB(fd,6)=flag;
	WFIFOSET(fd,packet_len_table[0x17d]);
	return 0;
}

/*==========================================
 * 鑑定可能アイテムリスト送信
 *------------------------------------------
 */
int clif_item_identify_list(struct map_session_data *sd)
{
	int i,c;
	int fd=sd->fd;
	
	WFIFOW(fd,0)=0x177;
	for(i=c=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].identify!=1){
			WFIFOW(fd,c*2+4)=i+2;
			c++;
		}
	}
	WFIFOW(fd,2)=c*2+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * 鑑定結果
 *------------------------------------------
 */
int clif_item_identified(struct map_session_data *sd,int idx,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd, 0)=0x179;
	WFIFOW(fd, 2)=idx+2;
	WFIFOB(fd, 4)=flag;
	WFIFOSET(fd,packet_len_table[0x179]);
	return 0;
}

/*==========================================
 * アイテムによる一時的なスキル効果
 *------------------------------------------
 */
int clif_item_skill(struct map_session_data *sd,int skillid,int skilllv,const char *name)
{
	int fd=sd->fd;
	WFIFOW(fd, 0)=0x147;
	WFIFOW(fd, 2)=skillid;
	WFIFOW(fd, 4)=skill_get_inf(skillid);
	WFIFOW(fd, 6)=0;
	WFIFOW(fd, 8)=skilllv;
	WFIFOW(fd,10)=skill_get_sp(skillid,skilllv);
	WFIFOW(fd,12)=skill_get_range(skillid);
	memcpy(WFIFOP(fd,14),name,24);
	WFIFOB(fd,38)=0;
	WFIFOSET(fd,packet_len_table[0x147]);
	return 0;
}

/*==========================================
 * カートにアイテム追加
 *------------------------------------------
 */
int clif_cart_additem(struct map_session_data *sd,int n,int amount,int fail)
{
	int fd=sd->fd;
	unsigned char *buf=WFIFOP(fd,0);

	if(n<0 || n>=MAX_CART || sd->status.cart[n].nameid==0)
		return 1;

	WBUFW(buf,0)=0x124;
	WBUFW(buf,2)=n+2;
	WBUFL(buf,4)=amount;
	WBUFW(buf,8)=sd->status.cart[n].nameid;
	WBUFB(buf,10)=sd->status.cart[n].identify;
	WBUFB(buf,11)=sd->status.cart[n].attribute;
	WBUFB(buf,12)=sd->status.cart[n].refine;
	WBUFW(buf,13)=sd->status.cart[n].card[0];
	WBUFW(buf,15)=sd->status.cart[n].card[1];
	WBUFW(buf,17)=sd->status.cart[n].card[2];
	WBUFW(buf,19)=sd->status.cart[n].card[3];

	WFIFOSET(fd,packet_len_table[0x124]);
	return 0;
}

/*==========================================
 * カートからアイテム削除
 *------------------------------------------
 */
int clif_cart_delitem(struct map_session_data *sd,int n,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x125;
	WFIFOW(fd,2)=n+2;
	WFIFOL(fd,4)=amount;

	WFIFOSET(fd,packet_len_table[0x125]);

	return 0;
}

/*==========================================
 * カートのアイテムリスト
 *------------------------------------------
 */
int clif_cart_itemlist(struct map_session_data *sd)
{
	int i,n,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x123;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid==0 ||
		   itemdb_isequip(sd->status.cart[i].nameid))
			continue;
		WBUFW(buf,n*10+4)=i+2;
		WBUFW(buf,n*10+6)=sd->status.cart[i].nameid;
		WBUFB(buf,n*10+8)=itemdb_type(sd->status.cart[i].nameid);
		WBUFB(buf,n*10+9)=sd->status.cart[i].identify;
		WBUFW(buf,n*10+10)=sd->status.cart[i].amount;
		WBUFW(buf,n*10+12)=0;
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * カートの装備品リスト
 *------------------------------------------
 */
int clif_cart_equiplist(struct map_session_data *sd)
{
	int i,n,id,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x122;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if((id=sd->status.cart[i].nameid)==0 ||
		   !itemdb_isequip(sd->status.cart[i].nameid))
			continue;
		WBUFW(buf,n*20+4)=i+2;
		WBUFW(buf,n*20+6)=id;
		WBUFB(buf,n*20+8)=itemdb_type(id);
		WBUFB(buf,n*20+9)=sd->status.cart[i].identify;
		WBUFW(buf,n*20+10)=itemdb_equip(id);
		WBUFW(buf,n*20+12)=sd->status.cart[i].equip;
		WBUFB(buf,n*20+14)=sd->status.cart[i].attribute;
		WBUFB(buf,n*20+15)=sd->status.cart[i].refine;
		WBUFW(buf,n*20+16)=sd->status.cart[i].card[0];
		WBUFW(buf,n*20+18)=sd->status.cart[i].card[1];
		WBUFW(buf,n*20+20)=sd->status.cart[i].card[2];
		WBUFW(buf,n*20+22)=sd->status.cart[i].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
	return 0;
}

/*==========================================
 * 露店開設
 *------------------------------------------
 */
int clif_openvendingreq(struct map_session_data *sd,int num)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x12d;
	WFIFOW(fd,2)=num;
	WFIFOSET(fd,packet_len_table[0x12d]);

	return 0;
}

/*==========================================
 * 露店看板表示
 *------------------------------------------
 */
int clif_showvendingboard(struct block_list* bl,char *message,int fd)
{
	unsigned char buf[128];

	WBUFW(buf,0)=0x131;
	WBUFL(buf,2)=bl->id;
	strcpy(WBUFP(buf,6),message);
	if(fd){
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0x131]);
		WFIFOSET(fd,packet_len_table[0x131]);
	}else{
		clif_send(buf,packet_len_table[0x131],bl,AREA_WOS);
	}
	return 0;
}

/*==========================================
 * 露店看板消去
 *------------------------------------------
 */
int clif_closevendingboard(struct block_list* bl,int fd)
{
	unsigned char buf[16];

	WBUFW(buf,0)=0x132;
	WBUFL(buf,2)=bl->id;
	if(fd){
		memcpy(WFIFOP(fd,0),buf,packet_len_table[0x132]);
		WFIFOSET(fd,packet_len_table[0x132]);
	}else{
		clif_send(buf,packet_len_table[0x132],bl,AREA_WOS);
	}

	return 0;
}
/*==========================================
 * 露店アイテムリスト
 *------------------------------------------
 */
int clif_vendinglist(struct map_session_data *sd,int id,struct vending *vending)
{
	int i,n,index,fd=sd->fd;
	struct map_session_data *vsd=map_id2sd(id);
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x133;
	WBUFL(buf,4)=id;
	for(i=0,n=0;i<vsd->vend_num;i++){
		if(vending[i].amount==0)
			continue;
		WBUFL(buf,8+n*22)=vending[i].value;
		WBUFW(buf,12+n*22)=vending[i].amount;
		WBUFW(buf,14+n*22)=(index=vending[i].index)+2;
		WBUFB(buf,16+n*22)=itemdb_type(vsd->status.cart[index].nameid);
		WBUFW(buf,17+n*22)=vsd->status.cart[index].nameid;
		WBUFB(buf,19+n*22)=vsd->status.cart[index].identify;
		WBUFB(buf,20+n*22)=vsd->status.cart[index].attribute;
		WBUFB(buf,21+n*22)=vsd->status.cart[index].refine;
		WBUFW(buf,22+n*22)=vsd->status.cart[index].card[0];
		WBUFW(buf,24+n*22)=vsd->status.cart[index].card[1];
		WBUFW(buf,26+n*22)=vsd->status.cart[index].card[2];
		WBUFW(buf,28+n*22)=vsd->status.cart[index].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=8+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return 0;
}

/*==========================================
 * 露店アイテム購入失敗
 *------------------------------------------
*/
int clif_buyvending(struct map_session_data *sd,int index,int amount,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x135;
	WFIFOW(fd,2)=index+2;
	WFIFOW(fd,4)=amount;
	WFIFOB(fd,6)=fail;
	WFIFOSET(fd,packet_len_table[0x135]);

	return 0;
}

/*==========================================
 * 露店開設成功
 *------------------------------------------
*/
int clif_openvending(struct map_session_data *sd,int id,struct vending *vending)
{
	int i,n,index,fd=sd->fd;
	unsigned char *buf = WFIFOP(fd,0);

	WBUFW(buf,0)=0x136;
	WBUFL(buf,4)=id;
	for(i=0,n=0;i<sd->vend_num;i++){
		WBUFL(buf,8+n*22)=vending[i].value;
		WBUFW(buf,12+n*22)=(index=vending[i].index)+2;
		WBUFW(buf,14+n*22)=vending[i].amount;
		WBUFB(buf,16+n*22)=itemdb_type(sd->status.cart[index].nameid);
		WBUFW(buf,17+n*22)=sd->status.cart[index].nameid;
		WBUFB(buf,19+n*22)=sd->status.cart[index].identify;
		WBUFB(buf,20+n*22)=sd->status.cart[index].attribute;
		WBUFB(buf,21+n*22)=sd->status.cart[index].refine;
		WBUFW(buf,22+n*22)=sd->status.cart[index].card[0];
		WBUFW(buf,24+n*22)=sd->status.cart[index].card[1];
		WBUFW(buf,26+n*22)=sd->status.cart[index].card[2];
		WBUFW(buf,28+n*22)=sd->status.cart[index].card[3];
		n++;
	}
	if(n){
		WBUFW(buf,2)=8+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return 0;
}

/*==========================================
 * 露店アイテム販売報告
 *------------------------------------------
*/
int clif_vendingreport(struct map_session_data *sd,int index,int amount)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x137;
	WFIFOW(fd,2)=index+2;
	WFIFOW(fd,4)=amount;
	WFIFOSET(fd,packet_len_table[0x137]);

	return 0;
}

/*==========================================
 * パーティ作成完了
 *------------------------------------------
 */
int clif_party_created(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0xfa;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0xfa]);
	return 0;
}
/*==========================================
 * パーティ情報送信
 *------------------------------------------
 */
int clif_party_info(struct party *p,int fd)
{
	unsigned char buf[1024];
	int i,c;
	struct map_session_data *sd=NULL;
	
	WBUFW(buf,0)=0xfb;
	memcpy(WBUFP(buf,4),p->name,24);
	for(i=c=0;i<MAX_PARTY;i++){
		struct party_member *m=&p->member[i];
		if(m->account_id>0){
			if(sd==NULL) sd=m->sd;
			WBUFL(buf,28+c*46)=m->account_id;
			memcpy(WBUFP(buf,28+c*46+ 4),m->name,24);
			memcpy(WBUFP(buf,28+c*46+28),m->map,16);
			WBUFB(buf,28+c*46+44)=(m->leader)?0:1;
			WBUFB(buf,28+c*46+45)=(m->online)?0:1;
			c++;
		}
	}
	WBUFW(buf,2)=28+c*46;
	if(fd>=0){	// fdが設定されてるならそれに送る
		memcpy(WFIFOP(fd,0),buf,WBUFW(buf,2));
		WFIFOSET(fd,WFIFOW(fd,2));
		return 9;
	}
	if(sd!=NULL)
		clif_send(buf,WBUFW(buf,2),&sd->bl,PARTY);
	return 0;
}
/*==========================================
 * パーティ勧誘
 *------------------------------------------
 */
int clif_party_invite(struct map_session_data *sd,struct map_session_data *tsd)
{
	int fd=tsd->fd;
	struct party *p;
	if( (p=party_search(sd->status.party_id))==NULL )
		return 0;

	WFIFOW(fd,0)=0xfe;
	WFIFOL(fd,2)=sd->status.account_id;
	memcpy(WFIFOP(fd,6),p->name,24);
	WFIFOSET(fd,packet_len_table[0xfe]);
	return 0;
}

/*==========================================
 * パーティ勧誘結果
 *------------------------------------------
 */
int clif_party_inviteack(struct map_session_data *sd,char *nick,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0xfd;
	memcpy(WFIFOP(fd,2),nick,24);
	WFIFOB(fd,26)=flag;
	WFIFOSET(fd,packet_len_table[0xfd]);
	return 0;
}

/*==========================================
 * パーティ設定送信
 * flag & 0x001=exp変更ミス
 *        0x010=item変更ミス
 *        0x100=一人にのみ送信
 *------------------------------------------
 */
int clif_party_option(struct party *p,struct map_session_data *sd,int flag)
{
	unsigned char buf[16];

//	printf("clif_party_option: %d %d %d\n",p->exp,p->item,flag);
	if(sd==NULL && flag==0){
		int i;
		for(i=0;i<MAX_PARTY;i++)
			if((sd=map_id2sd(p->member[i].account_id))!=NULL)
				break;
	}
	if(sd==NULL)
		return 0;
	WBUFW(buf,0)=0x101;
	WBUFW(buf,2)=((flag&0x01)?2:p->exp);
	WBUFW(buf,4)=((flag&0x10)?2:p->item);
	if(flag==0)
		clif_send(buf,packet_len_table[0x101],&sd->bl,PARTY);
	else{
		memcpy(WFIFOP(sd->fd,0),buf,packet_len_table[0x101]);
		WFIFOSET(sd->fd,packet_len_table[0x101]);
	}
	return 0;
}
/*==========================================
 * パーティ脱退（脱退前に呼ぶこと）
 *------------------------------------------
 */
int clif_party_leaved(struct party *p,struct map_session_data *sd,int account_id,char *name,int flag)
{
	unsigned char buf[64];
	int i;
	
	WBUFW(buf,0)=0x105;
	WBUFL(buf,2)=account_id;
	memcpy(WBUFP(buf,6),name,24);
	WBUFB(buf,30)=flag&0x0f;

	if((flag&0xf0)==0){
		if(sd==NULL)
			for(i=0;i<MAX_PARTY;i++)
				if((sd=p->member[i].sd)!=NULL)
					break;
		if(sd!=NULL)
			clif_send(buf,packet_len_table[0x105],&sd->bl,PARTY);
	}else if(sd!=NULL){
		memcpy(WFIFOP(sd->fd,0),buf,packet_len_table[0x105]);
		WFIFOSET(sd->fd,packet_len_table[0x105]);
	}
	return 0;
}
/*==========================================
 * パーティメッセージ送信
 *------------------------------------------
 */
int clif_party_message(struct party *p,int account_id,char *mes,int len)
{
	struct map_session_data *sd;
	int i;
	for(i=0;i<MAX_PARTY;i++){
		if((sd=p->member[i].sd)!=NULL)
			break;
	}
	if(sd!=NULL){
		unsigned char buf[1024];
		WBUFW(buf,0)=0x109;
		WBUFW(buf,2)=len+8;
		WBUFL(buf,4)=account_id;
		memcpy(WBUFP(buf,8),mes,len);
		clif_send(buf,len+8,&sd->bl,PARTY);
	}
	return 0;
}
/*==========================================
 * パーティ座標通知
 *------------------------------------------
 */
int clif_party_xy(struct party *p,struct map_session_data *sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x107;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=sd->bl.x;
	WBUFW(buf,8)=sd->bl.y;
	clif_send(buf,packet_len_table[0x107],&sd->bl,PARTY_SAMEMAP_WOS);
//	printf("clif_party_xy %d\n",sd->status.account_id);
	return 0;
}
/*==========================================
 * パーティHP通知
 *------------------------------------------
 */
int clif_party_hp(struct party *p,struct map_session_data *sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x106;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=sd->status.hp;
	WBUFW(buf,8)=sd->status.max_hp;
	clif_send(buf,packet_len_table[0x106],&sd->bl,PARTY_AREA_WOS);
//	printf("clif_party_hp %d\n",sd->status.account_id);
	return 0;
}
/*==========================================
 * パーティ場所移動（未使用）
 *------------------------------------------
 */
int clif_party_move(struct party *p,struct map_session_data *sd,int online)
{
	unsigned char buf[128];
	WBUFW(buf, 0)=0x104;
	WBUFL(buf, 2)=sd->status.account_id;
	WBUFL(buf, 6)=0;
	WBUFW(buf,10)=sd->bl.x;
	WBUFW(buf,12)=sd->bl.y;
	WBUFB(buf,14)=!online;
	memcpy(WBUFP(buf,15),p->name,24);
	memcpy(WBUFP(buf,39),sd->status.name,24);
	memcpy(WBUFP(buf,63),map[sd->bl.m].name,16);
	clif_send(buf,packet_len_table[0x104],&sd->bl,PARTY);
	return 0;
}
/*==========================================
 * 攻撃するために移動が必要
 *------------------------------------------
 */
int clif_movetoattack(struct map_session_data *sd,struct block_list *bl)
{
	int fd=sd->fd;
	WFIFOW(fd, 0)=0x139;
	WFIFOL(fd, 2)=bl->id;
	WFIFOW(fd, 6)=bl->x;
	WFIFOW(fd, 8)=bl->y;
	WFIFOW(fd,10)=sd->bl.x;
	WFIFOW(fd,12)=sd->bl.y;
	WFIFOW(fd,14)=sd->attackrange;
	WFIFOSET(fd,packet_len_table[0x139]);
	return 0;
}
/*==========================================
 * 製造エフェクト
 *------------------------------------------
 */
int clif_produceeffect(struct map_session_data *sd,int flag,int nameid)
{
	int fd=sd->fd;
	
	// 名前の登録と送信を先にしておく
	if( map_charid2nick(sd->status.char_id)==NULL )
		map_addchariddb(sd->status.char_id,sd->status.name);
	clif_solved_charname(sd,sd->status.char_id);
	
	WFIFOW(fd, 0)=0x18f;
	WFIFOW(fd, 2)=flag;
	WFIFOW(fd, 4)=nameid;
	WFIFOSET(fd,packet_len_table[0x18f]);
	return 0;
}

// pet
int clif_catch_process(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x19e;
	WFIFOSET(fd,packet_len_table[0x19e]);

	return 0;
}

int clif_pet_rulet(struct map_session_data *sd,int data)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1a0;
	WFIFOB(fd,2)=data;
	WFIFOSET(fd,packet_len_table[0x1a0]);

	return 0;
}

/*==========================================
 * pet卵リスト作成
 *------------------------------------------
 */
int clif_sendegg(struct map_session_data *sd)
{
	//R 01a6 <len>.w <index>.w*
	int i,n=0,fd=sd->fd;

	WFIFOW(fd,0)=0x1a6;
	if(!sd->status.pet_id) {
		for(i=0,n=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid==0 ||
			   itemdb_type(sd->status.inventory[i].nameid)!=7 ||
		  	 sd->status.inventory[i].amount<=0)
				continue;
			WFIFOW(fd,n*2+4)=i+2;
			n++;
		}
	}
	WFIFOW(fd,2)=4+n*2;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

int clif_send_petdata(struct map_session_data *sd,int type,int param)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1a4;
	WFIFOB(fd,2)=type;
	WFIFOL(fd,3)=sd->pet_npcdata->bl.id;
	WFIFOL(fd,7)=param;
	WFIFOSET(fd,packet_len_table[0x1a4]);

	return 0;
}

int clif_send_petstatus(struct map_session_data *sd)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1a2;
	memcpy(WFIFOP(fd,2),sd->pet.name,24);
	WFIFOB(fd,26)=(battle_config.pet_rename == 1)? 0:sd->pet.rename_flag;
	WFIFOW(fd,27)=sd->pet.level;
	WFIFOW(fd,29)=sd->pet.hungry;
	WFIFOW(fd,31)=sd->pet.intimate;
	WFIFOW(fd,33)=sd->pet.equip;
	WFIFOSET(fd,packet_len_table[0x1a2]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pet_emotion(struct npc_data *nd,int param)
{
	unsigned char buf[16];

	memset(buf,0,packet_len_table[0x1aa]);

	WBUFW(buf,0)=0x1aa;
	WBUFL(buf,2)=nd->bl.id;
	WBUFL(buf,6)=param;

	clif_send(buf,packet_len_table[0x1aa],&nd->bl,AREA);

	return 0;
}

int clif_pet_performance(struct npc_data *nd,int param)
{
	unsigned char buf[16];

	memset(buf,0,packet_len_table[0x1a4]);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=4;
	WBUFL(buf,3)=nd->bl.id;
	WBUFL(buf,7)=param;

	clif_send(buf,packet_len_table[0x1a4],&nd->bl,AREA);

	return 0;
}

int clif_pet_equip(struct npc_data *nd,int nameid)
{
	unsigned char buf[16];

	memset(buf,0,packet_len_table[0x1a4]);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=3;
	WBUFL(buf,3)=nd->bl.id;
	WBUFL(buf,7)=nameid;

	clif_send(buf,packet_len_table[0x1a4],&nd->bl,AREA);

	return 0;
}

int clif_pet_food(struct map_session_data *sd,int foodid,int fail)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0x1a3;
	WFIFOB(fd,2)=fail;
	WFIFOW(fd,3)=foodid;
	WFIFOSET(fd,packet_len_table[0x1a3]);

	return 0;
}

/*==========================================
 * 氣球 
 *------------------------------------------
 */
int clif_spiritball(struct map_session_data *sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x1d0;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->spiritball;
	clif_send(buf,packet_len_table[0x1d0],&sd->bl,AREA);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_combo_delay(struct block_list *bl,int wait)
{
	unsigned char buf[32];

	WBUFW(buf,0)=0x1d2;
	WBUFL(buf,2)=bl->id;
	WBUFL(buf,6)=wait;
	clif_send(buf,packet_len_table[0x1d2],bl,AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapcell(int m,int x,int y,int cell_type,int type)
{
	struct block_list bl;
	char buf[32];

	bl.m = m;
	bl.x = x;
	bl.y = y;
	WBUFW(buf,0) = 0x192;
	WBUFW(buf,2) = x;
	WBUFW(buf,4) = y;
	WBUFW(buf,6) = cell_type;
	memcpy(WBUFP(buf,8),map[m].name,16);
	if(!type)
		clif_send(buf,packet_len_table[0x192],&bl,AREA);
	else
		clif_send(buf,packet_len_table[0x192],&bl,ALL_SAMEMAP);

	return 0;
}

/*==========================================
 * MVPエフェクト
 *------------------------------------------
 */
int clif_mvp_effect(struct map_session_data *sd)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x10c;
	WBUFL(buf,2)=sd->bl.id;
	clif_send(buf,packet_len_table[0x10c],&sd->bl,AREA);
	return 0;
}
/*==========================================
 * MVPアイテム所得
 *------------------------------------------
 */
int clif_mvp_item(struct map_session_data *sd,int nameid)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x10a;
	WFIFOW(fd,2)=nameid;
	WFIFOSET(fd,packet_len_table[0x10a]);
	return 0;
}
/*==========================================
 * MVP経験値所得
 *------------------------------------------
 */
int clif_mvp_exp(struct map_session_data *sd,int exp)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x10b;
	WFIFOL(fd,2)=exp;
	WFIFOSET(fd,packet_len_table[0x10b]);
	return 0;
}

/*==========================================
 * ギルド作成可否通知
 *------------------------------------------
 */
int clif_guild_created(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x167;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x167]);
	return 0;
}
/*==========================================
 * ギルド所属通知
 *------------------------------------------
 */
int clif_guild_belonginfo(struct map_session_data *sd,struct guild *g)
{
	int fd=sd->fd;
	int ps=guild_getposition(sd,g);

	memset(WFIFOP(fd,0),0,13);
	WFIFOW(fd,0)=0x16c;
	WFIFOL(fd,2)=g->guild_id;
	WFIFOL(fd,6)=g->emblem_id;
	WFIFOL(fd,10)=g->position[ps].mode;
	memcpy(WFIFOP(fd,19),g->name,24);
	WFIFOSET(fd,packet_len_table[0x16c]);
	return 0;
}
/*==========================================
 * ギルドメンバログイン通知
 *------------------------------------------
 */
int clif_guild_memberlogin_notice(struct guild *g,int idx,int flag)
{
	unsigned char buf[64];
	WBUFW(buf, 0)=0x16d;
	WBUFL(buf, 2)=g->member[idx].account_id;
	WBUFL(buf, 6)=g->member[idx].char_id;
	WBUFL(buf,10)=flag;
	if(g->member[idx].sd==NULL){
		struct map_session_data *sd=guild_getavailablesd(g);
		if(sd!=NULL)
			clif_send(buf,packet_len_table[0x16d],&sd->bl,GUILD);
	}else
		clif_send(buf,packet_len_table[0x16d],&g->member[idx].sd->bl,GUILD_WOS);
	return 0;
}
/*==========================================
 * ギルドマスター通知(14dへの応答)
 *------------------------------------------
 */
int clif_guild_masterormember(struct map_session_data *sd)
{
	int type=0x57,fd=sd->fd;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g!=NULL && strcmp(g->master,sd->status.name)==0)
		type=0xd7;
	WFIFOW(fd,0)=0x14e;
	WFIFOL(fd,2)=type;
	WFIFOSET(fd,packet_len_table[0x14e]);
	return 0;
}
/*==========================================
 * ギルド基本情報
 *------------------------------------------
 */
int clif_guild_basicinfo(struct map_session_data *sd)
{
	int fd=sd->fd;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	
	WFIFOW(fd, 0)=0x1b6;//0x150;
	WFIFOL(fd, 2)=g->guild_id;
	WFIFOL(fd, 6)=g->guild_lv;
	WFIFOL(fd,10)=g->connect_member;
	WFIFOL(fd,14)=g->max_member;
	WFIFOL(fd,18)=g->average_lv;
	WFIFOL(fd,22)=g->exp;
	WFIFOL(fd,26)=g->next_exp;
	WFIFOL(fd,30)=0;	// 上納
	WFIFOL(fd,34)=0;	// VW（性格の悪さ？：性向グラフ左右）
	WFIFOL(fd,38)=0;	// RF（正義の度合い？：性向グラフ上下）
	WFIFOL(fd,42)=0;	// 人数？
	memcpy(WFIFOP(fd,46),g->name,24);
	memcpy(WFIFOP(fd,70),g->master,24);
	memcpy(WFIFOP(fd,94),"",20);	// 本拠地
	WFIFOSET(fd,packet_len_table[WFIFOW(fd,0)]);
	return 0;
}
/*==========================================
 * ギルド同盟/敵対情報
 *------------------------------------------
 */
int clif_guild_allianceinfo(struct map_session_data *sd)
{
	int fd=sd->fd,i,c;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x14c;
	for(i=c=0;i<MAX_GUILDALLIANCE;i++){
		struct guild_alliance *a=&g->alliance[i];
		if(a->guild_id>0){
			WFIFOL(fd,c*32+4)=a->opposition;
			WFIFOL(fd,c*32+8)=a->guild_id;
			memcpy(WFIFOP(fd,c*32+12),a->name,24);
			c++;
		}
	}
	WFIFOW(fd, 2)=c*32+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * ギルドメンバーリスト
 *------------------------------------------
 */
int clif_guild_memberlist(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i,c;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;

	WFIFOW(fd, 0)=0x154;
	for(i=0,c=0;i<g->max_member;i++){
		struct guild_member *m=&g->member[i];
		if(m->account_id==0)
			continue;
		WFIFOL(fd,c*104+ 4)=m->account_id;
		WFIFOL(fd,c*104+ 8)=m->char_id;
		WFIFOW(fd,c*104+12)=m->hair;
		WFIFOW(fd,c*104+14)=m->hair_color;
		WFIFOW(fd,c*104+16)=m->gender;
		WFIFOW(fd,c*104+18)=m->class;
		WFIFOW(fd,c*104+20)=m->lv;
		WFIFOL(fd,c*104+22)=m->exp;
		WFIFOL(fd,c*104+26)=m->online;
		WFIFOL(fd,c*104+30)=m->position;
		memset(WFIFOP(fd,c*104+34),0,50);	// メモ？
		memcpy(WFIFOP(fd,c*104+84),m->name,24);
		c++;
	}
	WFIFOW(fd, 2)=c*104+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルド役職名リスト
 *------------------------------------------
 */
int clif_guild_positionnamelist(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x166;
	for(i=0;i<MAX_GUILDPOSITION;i++){
		WFIFOL(fd,i*28+4)=i;
		memcpy(WFIFOP(fd,i*28+8),g->position[i].name,24);
	}
	WFIFOW(fd,2)=i*28+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルド役職情報リスト
 *------------------------------------------
 */
int clif_guild_positioninfolist(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd, 0)=0x160;
	for(i=0;i<MAX_GUILDPOSITION;i++){
		struct guild_position *p=&g->position[i];
		WFIFOL(fd,i*16+ 4)=i;
		WFIFOL(fd,i*16+ 8)=p->mode;
		WFIFOL(fd,i*16+12)=i;
		WFIFOL(fd,i*16+16)=p->exp_mode;
	}
	WFIFOW(fd, 2)=i*16+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルド役職変更通知
 *------------------------------------------
 */
int clif_guild_positionchanged(struct guild *g,int idx)
{
	struct map_session_data *sd;
	unsigned char buf[128];
	WBUFW(buf, 0)=0x174;
	WBUFW(buf, 2)=44;
	WBUFL(buf, 4)=idx;
	WBUFL(buf, 8)=g->position[idx].mode;
	WBUFL(buf,12)=idx;
	WBUFL(buf,16)=g->position[idx].exp_mode;
	memcpy(WBUFP(buf,20),g->position[idx].name,24);
	if( (sd=guild_getavailablesd(g))!=NULL )
		clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルドメンバ変更通知
 *------------------------------------------
 */
int clif_guild_memberpositionchanged(struct guild *g,int idx)
{
	struct map_session_data *sd;
	unsigned char buf[64];
	WBUFW(buf, 0)=0x156;
	WBUFW(buf, 2)=16;
	WBUFL(buf, 4)=g->member[idx].account_id;
	WBUFL(buf, 8)=g->member[idx].char_id;
	WBUFL(buf,12)=g->member[idx].position;
	if( (sd=guild_getavailablesd(g))!=NULL )
		clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルドエンブレム送信
 *------------------------------------------
 */
int clif_guild_emblem(struct map_session_data *sd,struct guild *g)
{
	int fd=sd->fd;
	if(g->emblem_len<=0)
		return 0;
	WFIFOW(fd,0)=0x152;
	WFIFOW(fd,2)=g->emblem_len+12;
	WFIFOL(fd,4)=g->guild_id;
	WFIFOL(fd,8)=g->emblem_id;
	memcpy(WFIFOP(fd,12),g->emblem_data,g->emblem_len);
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルドスキル送信
 *------------------------------------------
 */
int clif_guild_skillinfo(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i,id,c;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd,0)=0x0162;
	WFIFOW(fd,4)=g->skill_point;
	for(i=c=0;i<MAX_GUILDSKILL;i++){
		if(g->skill[i].id>0){
			WFIFOW(fd,c*37+ 6) = id = g->skill[i].id;
			WFIFOW(fd,c*37+ 8) = guild_skill_get_inf(id);
			WFIFOW(fd,c*37+10) = 0;
			WFIFOW(fd,c*37+12) = g->skill[i].lv;
			WFIFOW(fd,c*37+14) = guild_skill_get_sp(id,g->skill[i].lv);
			WFIFOW(fd,c*37+16) = guild_skill_get_range(id);
			memset(WFIFOP(fd,c*37+18),0,24);
			WFIFOB(fd,c*37+42)= //up;
				(g->skill[i].lv < guild_skill_get_max(id))? 1:0;
			c++;
		}
	}
	WFIFOW(fd,2)=c*37+6;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}
/*==========================================
 * ギルド告知送信
 *------------------------------------------
 */
int clif_guild_notice(struct map_session_data *sd,struct guild *g)
{
	int fd=sd->fd;
	if(*g->mes1==0 && *g->mes2==0)
		return 0;
	WFIFOW(fd,0)=0x16f;
	memcpy(WFIFOP(fd,2),g->mes1,60);
	memcpy(WFIFOP(fd,62),g->mes2,120);
	WFIFOSET(fd,packet_len_table[0x16f]);
	return 0;
}


/*==========================================
 * ギルドメンバ勧誘
 *------------------------------------------
 */
int clif_guild_invite(struct map_session_data *sd,struct guild *g)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x16a;
	WFIFOL(fd,2)=g->guild_id;
	memcpy(WFIFOP(fd,6),g->name,24);
	WFIFOSET(fd,packet_len_table[0x16a]);
	return 0;
}
/*==========================================
 * ギルドメンバ勧誘結果
 *------------------------------------------
 */
int clif_guild_inviteack(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x169;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x169]);
	return 0;
}
/*==========================================
 * ギルドメンバ脱退通知
 *------------------------------------------
 */
int clif_guild_leave(struct map_session_data *sd,const char *name,const char *mes)
{
	unsigned char buf[128];
	WBUFW(buf, 0)=0x15a;
	memcpy(WBUFP(buf, 2),name,24);
	memcpy(WBUFP(buf,26),mes,40);
	clif_send(buf,packet_len_table[0x15a],&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルドメンバ追放通知
 *------------------------------------------
 */
int clif_guild_explusion(struct map_session_data *sd,const char *name,const char *mes,
	int account_id)
{
	unsigned char buf[128];
	WBUFW(buf, 0)=0x15c;
	memcpy(WBUFP(buf, 2),name,24);
	memcpy(WBUFP(buf,26),mes,40);
	memcpy(WBUFP(buf,66),"dummy",24);
	clif_send(buf,packet_len_table[0x15c],&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルド追放メンバリスト
 *------------------------------------------
 */
int clif_guild_explusionlist(struct map_session_data *sd)
{
	int fd=sd->fd;
	int i,c;
	struct guild *g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;
	WFIFOW(fd,0)=0x163;
	for(i=c=0;i<MAX_GUILDEXPLUSION;i++){
		struct guild_explusion *e=&g->explusion[i];
		if(e->account_id>0){
			memcpy(WFIFOP(fd,c*88+ 4),e->name,24);
			memcpy(WFIFOP(fd,c*88+28),e->acc,24);
			memcpy(WFIFOP(fd,c*88+52),e->mes,44);
			c++;
		}
	}
	WFIFOW(fd,2)=c*88+4;
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

/*==========================================
 * ギルド会話
 *------------------------------------------
 */
int clif_guild_message(struct guild *g,int account_id,const char *mes,int len)
{
	struct map_session_data *sd;
	unsigned char buf[len+32];
	WBUFW(buf, 0)=0x17f;
	WBUFW(buf, 2)=len+4;
	memcpy(WBUFP(buf,4),mes,len);
	
	if( (sd=guild_getavailablesd(g))!=NULL )
		clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);
	return 0;
}
/*==========================================
 * ギルドスキル割り振り通知
 *------------------------------------------
 */
int clif_guild_skillup(struct map_session_data *sd,int skill_num,int lv)
{
	int fd=sd->fd;
	WFIFOW(fd,0) = 0x10e;
	WFIFOW(fd,2) = skill_num;
	WFIFOW(fd,4) = lv;
	WFIFOW(fd,6) = guild_skill_get_sp(skill_num,lv);
	WFIFOW(fd,8) = guild_skill_get_range(skill_num);
	WFIFOB(fd,10) = 1;
	WFIFOSET(fd,11);
	return 0;
}
/*==========================================
 * ギルド同盟要請
 *------------------------------------------
 */
int clif_guild_reqalliance(struct map_session_data *sd,int account_id,const char *name)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x171;
	WFIFOL(fd,2)=account_id;
	memcpy(WFIFOP(fd,6),name,24);
	WFIFOSET(fd,packet_len_table[0x171]);
	return 0;
}
/*==========================================
 * ギルド同盟結果
 *------------------------------------------
 */
int clif_guild_allianceack(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x173;
	WFIFOL(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x173]);
	return 0;
}
/*==========================================
 * ギルド関係解消通知
 *------------------------------------------
 */
int clif_guild_delalliance(struct map_session_data *sd,int guild_id,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x184;
	WFIFOL(fd,2)=guild_id;
	WFIFOL(fd,6)=flag;
	WFIFOSET(fd,packet_len_table[0x184]);
	return 0;
}
/*==========================================
 * ギルド敵対結果
 *------------------------------------------
 */
int clif_guild_oppositionack(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x181;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x181]);
	return 0;
}
/*==========================================
 * ギルド関係追加
 *------------------------------------------
 */
/*int clif_guild_allianceadded(struct guild *g,int idx)
{
	unsigned char buf[64];
	WBUFW(fd,0)=0x185;
	WBUFL(fd,2)=g->alliance[idx].opposition;
	WBUFL(fd,6)=g->alliance[idx].guild_id;
	memcpy(WBUFP(fd,10),g->alliance[idx].name,24);
	clif_send(buf,packet_len_table[0x185],guild_getavailablesd(g),GUILD);
	return 0;
}*/
/*==========================================
 * ギルド解散通知
 *------------------------------------------
 */
int clif_guild_broken(struct map_session_data *sd,int flag)
{
	int fd=sd->fd;
	WFIFOW(fd,0)=0x15e;
	WFIFOL(fd,2)=flag;
	WFIFOSET(fd,packet_len_table[0x15e]);
	return 0;
}

/*==========================================
 * エモーション
 *------------------------------------------
 */
void clif_emotion(struct block_list *bl,int type)
{
	unsigned char buf[8];
	WBUFW(buf,0)=0xc0;
	WBUFL(buf,2)=bl->id;
	WBUFB(buf,6)=type;
	clif_send(buf,packet_len_table[0xc0],bl,AREA);
}

/*==========================================
 *
 *------------------------------------------
 */

int clif_GM_kickack(struct map_session_data *sd,int id)
{
	int fd=sd->fd;

	WFIFOW(fd,0)=0xcd;
	WFIFOL(fd,2)=id;
	WFIFOSET(fd,packet_len_table[0xcd]);
	return 0;
}

void clif_parse_QuitGame(int fd,struct map_session_data *sd);

int clif_GM_kick(struct map_session_data *sd,struct map_session_data *tsd,int type)
{
	if(type)
		clif_GM_kickack(sd,tsd->status.account_id);
	clif_parse_QuitGame(tsd->fd,tsd);

	return 0;
}

// ------------
// clif_parse_*
// ------------
// パケット読み取って色々操作
/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_WantToConnection(int fd,struct map_session_data *sd)
{
	struct map_session_data *old_sd;

	if(sd){
		printf("clif_parse_WantToConnection : invalid request?\n");
		return;
	}

	sd=session[fd]->session_data=malloc(sizeof(*sd));
	if(sd==NULL){
		printf("out of memory : clif_parse_WantToConnection\n");
		exit(1);
	}
	memset(sd,0,sizeof(*sd));
	sd->fd = fd;

	pc_setnewpc(sd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOL(fd,14),RFIFOB(fd,18),fd);

	if((old_sd=map_id2sd(RFIFOL(fd,2))) != NULL){
		// 2重loginなので切断用のデータを保存する
		old_sd->new_fd=fd;
	} else {
		map_addiddb(&sd->bl);
	}

	chrif_authreq(sd);

	WFIFOL(fd,0)=sd->bl.id;
	WFIFOSET(fd,4);
}

/*==========================================
 * 007d クライアント側マップ読み込み完了
 * map侵入時に必要なデータを全て送りつける
 *------------------------------------------
 */
void clif_parse_LoadEndAck(int fd,struct map_session_data *sd)
{
	if(sd->bl.prev != NULL)
		return;

	// 接続ok時
	//clif_authok();
	clif_skillinfoblock(sd);
	pc_checkitem(sd);
	//guild_info();

	// loadendack時
	// next exp
	clif_updatestatus(sd,SP_NEXTBASEEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);
	// skill point
	clif_updatestatus(sd,SP_SKILLPOINT);
	// item
	clif_itemlist(sd);
	clif_equiplist(sd);
	// cart
	if(pc_iscarton(sd)){
		clif_cart_itemlist(sd);
		clif_cart_equiplist(sd);
		clif_updatestatus(sd,SP_CARTINFO);
	}
	// param all
	clif_initialstatus(sd);
	// party
	party_send_movemap(sd);
	// guild
	guild_send_memberinfoshort(sd,1);
	// 119
	// 78

	
	if(battle_config.ghost_time > 0)
		pc_setghosttimer(sd,battle_config.ghost_time);
	map_addblock(&sd->bl);	// ブロック登録
	clif_spawnpc(sd);	// spawn

	// weight max , now
	clif_updatestatus(sd,SP_MAXWEIGHT);
	clif_updatestatus(sd,SP_WEIGHT);
	
	// pvp
	if(sd->pvp_timer!=-1)
		delete_timer(sd->pvp_timer,pc_calc_pvprank_timer);
	if(map[sd->bl.m].flag.pvp){
		sd->pvp_timer=add_timer(
			gettick()+200,pc_calc_pvprank_timer,sd->bl.id,0);
		sd->pvp_rank=0;
		sd->pvp_lastusers=0;
		sd->pvp_point=5;
		clif_set0199(sd->fd,1);
	}
	else {
		sd->pvp_timer=-1;
	}
	if(map[sd->bl.m].flag.gvg) {
		clif_set0199(sd->fd,3);
	}
	
	// pet
	if(sd->status.pet_id && sd->pet_npcdata && sd->pet.intimate > 0) {
		map_addblock(&sd->pet_npcdata->bl);
		clif_spawnpet(sd->pet_npcdata);
		clif_send_petdata(sd,0,0);
		clif_send_petdata(sd,5,0x14);
		clif_pet_equip(sd->pet_npcdata,sd->pet.equip);
		clif_send_petstatus(sd);
	}

	// view equipment item
	clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);

	if(sd->status.hp<sd->status.max_hp/4 && pc_checkskill(sd,SM_AUTOBERSERK)>0 &&
		(sd->sc_data[SC_PROVOKE].timer==-1 || sd->sc_data[SC_PROVOKE].val2==0 ))
		// オートバーサーク発動
		skill_status_change_start(&sd->bl,SC_PROVOKE,10,1);
	// option
	clif_changeoption(&sd->bl);
	if(sd->sc_data[SC_TRICKDEAD].timer != -1)
		skill_status_change_end(&sd->bl,SC_TRICKDEAD,-1);

	map_foreachinarea(clif_getareachar,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,0,sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_TickSend(int fd,struct map_session_data *sd)
{
	sd->client_tick=RFIFOL(fd,2);
	sd->server_tick=gettick();
	clif_servertick(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_WalkToXY(int fd,struct map_session_data *sd)
{
	int x,y;

	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}

	if(sd->skilltimer!=-1 && pc_checkskill(sd,SA_FREECAST) <= 0) // フリーキャスト
		return;

	if(sd->chatID)
		return;

	if(sd->canmove_tick > gettick())
		return;

	// ステータス異常やハイディング中(トンネルドライブ無)で動けない
	if(sd->opt1 > 0 || sd->sc_data[SC_ANKLE].timer!=-1)
		return;
	if( (sd->status.option&2) && pc_checkskill(sd,RG_TUNNELDRIVE) <= 0)
		return;

	pc_stopattack(sd);

	x = RFIFOB(fd,2)*4+(RFIFOB(fd,3)>>6);
	y = ((RFIFOB(fd,3)&0x3f)<<4)+(RFIFOB(fd,4)>>4);

	pc_walktoxy(sd,x,y);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_QuitGame(int fd,struct map_session_data *sd)
{
	WFIFOW(fd,0)=0x18b;
	WFIFOW(fd,2)=0;
	WFIFOSET(fd,packet_len_table[0x18b]);
	clif_setwaitclose(fd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GetCharNameRequest(int fd,struct map_session_data *sd)
{
	struct block_list *bl;

	bl=map_id2bl(RFIFOL(fd,2));
	if(bl==NULL)
		return;

	WFIFOW(fd,0)=0x95;
	WFIFOL(fd,2)=RFIFOL(fd,2);

	switch(bl->type){
	case BL_PC: {
			struct map_session_data *ssd=(struct map_session_data *)bl;
			struct party *p=NULL;
			struct guild *g=NULL;
			memcpy(WFIFOP(fd,6),((struct map_session_data*)bl)->status.name,24);
			if( ssd->status.guild_id>0 &&(g=guild_search(ssd->status.guild_id))!=NULL &&
				(ssd->status.party_id==0 ||(p=party_search(ssd->status.party_id))!=NULL) ){
				// ギルド所属ならパケット0195を返す
				int i,ps=-1;
				for(i=0;i<g->max_member;i++){
					if( g->member[i].account_id==ssd->status.account_id &&
						g->member[i].char_id==ssd->status.char_id )
						ps=g->member[i].position;
				}
				if(ps>=0 && ps<MAX_GUILDPOSITION){
				
					WFIFOW(fd, 0)=0x195;
					if(p)
						memcpy(WFIFOP(fd,30),p->name,24);
					else
						WFIFOB(fd,30)=0;
					memcpy(WFIFOP(fd,54),g->name,24);
					memcpy(WFIFOP(fd,78),g->position[ps].name,24);
					WFIFOSET(fd,packet_len_table[0x195]);
					break;
				}
			}
			WFIFOSET(fd,packet_len_table[0x95]);
		} break;
	case BL_PET:
	case BL_NPC:
		memcpy(WFIFOP(fd,6),((struct npc_data*)bl)->name,24);
		WFIFOSET(fd,packet_len_table[0x95]);
		break;
	case BL_MOB:
		memcpy(WFIFOP(fd,6),((struct mob_data*)bl)->name,24);
		WFIFOSET(fd,packet_len_table[0x95]);
		break;
	default:
		printf("clif_parse_GetCharNameRequest : bad type %d(%d)\n",bl->type,RFIFOL(fd,2));
		break;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GlobalMessage(int fd,struct map_session_data *sd)
{
	if(atcommand(fd,sd,RFIFOP(fd,4))) return;
	WFIFOW(fd,0)=0x8d;
	WFIFOW(fd,2)=RFIFOW(fd,2)+4;
	WFIFOL(fd,4)=sd->bl.id;
	memcpy(WFIFOP(fd,8),RFIFOP(fd,4),RFIFOW(fd,2)-4);
	clif_send(WFIFOP(fd,0),WFIFOW(fd,2),&sd->bl,sd->chatID ? CHAT_WOS : AREA_WOC);

	memcpy(WFIFOP(fd,0),RFIFOP(fd,0),RFIFOW(fd,2));
	WFIFOW(fd,0)=0x8e;
	WFIFOSET(fd,WFIFOW(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_MapMove(int fd,struct map_session_data *sd)
{
	char mapname[32];

	if (battle_config.atc_gmonly == 0 || pc_isGM(sd) >= atcommand_config.mapmove) {
		memcpy(mapname,RFIFOP(fd,2),16);
		mapname[16]=0;
		pc_setpos(sd,mapname,RFIFOW(fd,18),RFIFOW(fd,20),2);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChangeDir(int fd,struct map_session_data *sd)
{
	pc_setdir(sd,RFIFOB(fd,4),RFIFOW(fd,2));

	WFIFOW(fd,0)=0x9c;
	WFIFOL(fd,2)=sd->bl.id;
	WFIFOW(fd,6)=RFIFOW(fd,2);
	WFIFOB(fd,8)=RFIFOB(fd,4);
	clif_send(WFIFOP(fd,0),packet_len_table[0x9c],&sd->bl,AREA_WOS);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_Emotion(int fd,struct map_session_data *sd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 2){
		WFIFOW(fd,0)=0xc0;
		WFIFOL(fd,2)=sd->bl.id;
		WFIFOB(fd,6)=RFIFOB(fd,2);
		clif_send(WFIFOP(fd,0),packet_len_table[0xc0],&sd->bl,AREA);
	}
	else
		clif_skill_fail(sd,1,0,1);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_HowManyConnections(int fd,struct map_session_data *sd)
{
	WFIFOW(fd,0)=0xc2;
	WFIFOL(fd,2)=map_getusers();
	WFIFOSET(fd,packet_len_table[0xc2]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ActionRequest(int fd,struct map_session_data *sd)
{
	unsigned int tick;

	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}

	tick=gettick();

	pc_stop_walking(sd,0);
	pc_stopattack(sd);
	switch(RFIFOB(fd,6)){
	case 0x00:	// once attack
	case 0x07:	// continuous attack
		if(!battle_config.sdelay_attack_enable && (sd->skilltimer == -1 || pc_checkskill(sd,SA_FREECAST) <= 0) ) {
			if(DIFF_TICK(tick , sd->canact_tick) < 0) {
				clif_skill_fail(sd,1,4,0);
				return;
			}
		}
		if(sd->ghost_timer != -1)
			pc_delghosttimer(sd);
		pc_attack(sd,RFIFOL(fd,2),RFIFOB(fd,6)!=0);
		break;
	case 0x02:	// sitdown
		if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 3) {
			pc_setsit(sd);
			WFIFOW(fd,0)=0x8a;
			WFIFOL(fd,2)=sd->bl.id;
			WFIFOB(fd,26)=2;
			clif_send(WFIFOP(fd,0),packet_len_table[0x8a],&sd->bl,AREA);
		}
		else
			clif_skill_fail(sd,1,0,2);
		break;
	case 0x03:	// standup
		pc_setstand(sd);
		WFIFOW(fd,0)=0x8a;
		WFIFOL(fd,2)=sd->bl.id;
		WFIFOB(fd,26)=3;
		clif_send(WFIFOP(fd,0),packet_len_table[0x8a],&sd->bl,AREA);
		break;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_Restart(int fd,struct map_session_data *sd)
{
	switch(RFIFOB(fd,2)){
	case 0x00:
		if(pc_isdead(sd)){
			pc_setstand(sd);
			pc_setrestartvalue(sd,0);
			pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,2);
		}
		break;
	case 0x01:
		chrif_charselectreq(sd);
		break;
	}
}

/*==========================================
 * Wisの送信
 *------------------------------------------
 */
void clif_parse_Wis(int fd,struct map_session_data *sd)
{
	intif_wis_message(sd,RFIFOP(fd,4),RFIFOP(fd,28),RFIFOW(fd,2)-28);

/*	struct map_session_data *dstsd;
	int dstfd;

	dstsd=map_nick2sd(RFIFOP(fd,4));

	if(dstsd==NULL){
		WFIFOW(fd,0)=0x98;
		WFIFOB(fd,2)=1;
		WFIFOSET(fd,packet_len_table[0x98]);
	} else {
		dstfd = dstsd->fd;

		WFIFOW(dstfd,0)=0x97;
		WFIFOW(dstfd,2)=RFIFOW(fd,2);
		memcpy(WFIFOP(dstfd,4),sd->status.name,24);
		memcpy(WFIFOP(dstfd,28),RFIFOP(fd,28),RFIFOW(fd,2)-28);
		WFIFOSET(dstfd,WFIFOW(dstfd,2));

		WFIFOW(fd,0)=0x98;
		WFIFOB(fd,2)=0;
		WFIFOSET(fd,packet_len_table[0x98]);
	}*/
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GMmessage(int fd,struct map_session_data *sd)
{
	if (battle_config.atc_gmonly == 0 || pc_isGM(sd) >= atcommand_config.broadcast)
		intif_GMmessage(RFIFOP(fd,4),RFIFOW(fd,2)-4,0);
/*	WFIFOW(fd,0)=0x9a;
	WFIFOW(fd,2)=RFIFOW(fd,2);
	memcpy(WFIFOP(fd,4),RFIFOP(fd,4),RFIFOW(fd,2)-4);
	clif_send(WFIFOP(fd,0),RFIFOW(fd,2),&sd->bl,ALL_CLIENT);*/
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_TakeItem(int fd,struct map_session_data *sd)
{
	struct flooritem_data *fitem=(struct flooritem_data*)map_id2bl(RFIFOL(fd,2));

	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}

	if(fitem==NULL || fitem->bl.m != sd->bl.m)
		return;

	if(sd->npc_id!=0)return;
	pc_takeitem(sd,fitem);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_DropItem(int fd,struct map_session_data *sd)
{
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id!=0)return;
	pc_dropitem(sd,RFIFOW(fd,2)-2,RFIFOW(fd,4));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_UseItem(int fd,struct map_session_data *sd)
{
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id!=0)return;

	if(sd->ghost_timer != -1)
		pc_delghosttimer(sd);
	pc_useitem(sd,RFIFOW(fd,2)-2);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_EquipItem(int fd,struct map_session_data *sd)
{
	int index;
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	index = RFIFOW(fd,2)-2;
	if(sd->npc_id!=0) return;

	if(sd->status.inventory[index].identify != 1) {		// 未鑑定
		clif_equipitemack(sd,index,0,0);	// fail
		return;
	}
	//ペット用装備であるかないか
	if(itemdb_type(sd->status.inventory[index].nameid) != 8){
		if(itemdb_type(sd->status.inventory[index].nameid) == 10)
			RFIFOW(fd,4)=0x8000;	// 矢を無理やり装備できるように（－－；
		pc_equipitem(sd,index,RFIFOW(fd,4));
	}else
		pet_equipitem(sd,index);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_UnequipItem(int fd,struct map_session_data *sd)
{
	if(pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id!=0)return;
	pc_unequipitem(sd,RFIFOW(fd,2)-2);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcClicked(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	npc_click(sd,RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcBuySellSelected(int fd,struct map_session_data *sd)
{
	npc_buysellsel(sd,RFIFOL(fd,2),RFIFOB(fd,6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcBuyListSend(int fd,struct map_session_data *sd)
{
	int fail=0,n;
	unsigned short *item_list;

	n = (RFIFOW(fd,2)-4) /4;
	item_list = (unsigned short*)RFIFOP(fd,4);

	fail = npc_buylist(sd,n,item_list);

	WFIFOW(fd,0)=0xca;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xca]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcSellListSend(int fd,struct map_session_data *sd)
{
	int fail=0,n;
	unsigned short *item_list;

	n = (RFIFOW(fd,2)-4) /4;
	item_list = (unsigned short*)RFIFOP(fd,4);

	fail = npc_selllist(sd,n,item_list);

	WFIFOW(fd,0)=0xcb;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_len_table[0xcb]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_CreateChatRoom(int fd,struct map_session_data *sd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 4){
		chat_createchat(sd,RFIFOW(fd,4),RFIFOB(fd,6),RFIFOP(fd,7),RFIFOP(fd,15),RFIFOW(fd,2)-15);
	}
	else
		clif_skill_fail(sd,1,0,3);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatAddMember(int fd,struct map_session_data *sd)
{
	chat_joinchat(sd,RFIFOL(fd,2),RFIFOP(fd,6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatRoomStatusChange(int fd,struct map_session_data *sd)
{
	chat_changechatstatus(sd,RFIFOW(fd,4),RFIFOB(fd,6),RFIFOP(fd,7),RFIFOP(fd,15),RFIFOW(fd,2)-15);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChangeChatOwner(int fd,struct map_session_data *sd)
{
	chat_changechatowner(sd,RFIFOP(fd,6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_KickFromChat(int fd,struct map_session_data *sd)
{
	chat_kickchat(sd,RFIFOP(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatLeave(int fd,struct map_session_data *sd)
{
	chat_leavechat(sd);
}

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
void clif_parse_TradeRequest(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 1){
		trade_traderequest(sd,RFIFOL(sd->fd,2));
	}
	else
		clif_skill_fail(sd,1,0,0);
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
void clif_parse_TradeAck(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	trade_tradeack(sd,RFIFOB(sd->fd,2));
}

/*==========================================
 * アイテム追加
 *------------------------------------------
 */
void clif_parse_TradeAddItem(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	trade_tradeadditem(sd,RFIFOW(sd->fd,2),RFIFOL(sd->fd,4));
}

/*==========================================
 * アイテム追加完了(ok押し)
 *------------------------------------------
 */
void clif_parse_TradeOk(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	trade_tradeok(sd);
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
void clif_parse_TradeCansel(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	trade_tradecancel(sd);
}

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
void clif_parse_TradeCommit(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	trade_tradecommit(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_StopAttack(int fd,struct map_session_data *sd)
{
	pc_stopattack(sd);
}

/*==========================================
 * カートへアイテムを移す
 *------------------------------------------
 */
void clif_parse_PutItemToCart(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	pc_putitemtocart(sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
}
/*==========================================
 * カートからアイテムを出す
 *------------------------------------------
 */
void clif_parse_GetItemFromCart(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	pc_getitemfromcart(sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
}

/*==========================================
 * 付属品(鷹,ペコ,カート)をはずす
 *------------------------------------------
 */
void clif_parse_RemoveOption(int fd,struct map_session_data *sd)
{
	pc_setoption(sd,0);
}

/*==========================================
 * チェンジカート
 *------------------------------------------
 */
void clif_parse_ChangeCart(int fd,struct map_session_data *sd)
{
	pc_setcart(sd,RFIFOW(fd,2));
}

/*==========================================
 * ステータスアップ
 *------------------------------------------
 */
void clif_parse_StatusUp(int fd,struct map_session_data *sd)
{
	pc_statusup(sd,RFIFOW(fd,2));
}

/*==========================================
 * スキルレベルアップ
 *------------------------------------------
 */
void clif_parse_SkillUp(int fd,struct map_session_data *sd)
{
	pc_skillup(sd,RFIFOW(fd,2));
}

/*==========================================
 * スキル使用（ID指定）
 *------------------------------------------
 */
void clif_parse_UseSkillToId(int fd,struct map_session_data *sd)
{
	int skillnum,skilllv,lv;
	unsigned int tick=gettick();
	if(DIFF_TICK(tick , sd->canact_tick) < 0) {
		clif_skill_fail(sd,RFIFOW(fd,4),4,0);
		return;
	}

	if(sd->npc_id!=0)return;
	if(sd->ghost_timer != -1)
		pc_delghosttimer(sd);
	skillnum = RFIFOW(fd,4);
	skilllv = RFIFOW(fd,2);
	if(sd->skillitem == -1) {
		if( (lv = pc_checkskill(sd,skillnum)) > 0) {
			if(skilllv > lv)
				skilllv = lv;
			skill_use_id(sd,RFIFOL(fd,6),skillnum,skilllv);
		}
	}
	else {
		if(skilllv > sd->skillitemlv)
			skilllv = sd->skillitemlv;
		skill_use_id(sd,RFIFOL(fd,6),skillnum,skilllv);
	}
}

/*==========================================
 * スキル使用（場所指定）
 *------------------------------------------
 */
void clif_parse_UseSkillToPos(int fd,struct map_session_data *sd)
{
	int skillnum,skilllv,lv;
	unsigned int tick=gettick();
	if(DIFF_TICK(tick , sd->canact_tick) < 0) {
		clif_skill_fail(sd,RFIFOW(fd,4),4,0);
		return;
	}

	if(sd->npc_id!=0)return;
	if(sd->ghost_timer != -1)
		pc_delghosttimer(sd);
	skillnum = RFIFOW(fd,4);
	skilllv = RFIFOW(fd,2);
	if(sd->skillitem == -1) {
		if( (lv = pc_checkskill(sd,skillnum)) > 0) {
			if(skilllv > lv)
				skilllv = lv;
			skill_use_pos(sd,RFIFOW(fd,6),RFIFOW(fd,8),skillnum,skilllv);
		}
	}
	else {
		if(skilllv > sd->skillitemlv)
			skilllv = sd->skillitemlv;
		skill_use_pos(sd,RFIFOW(fd,6),RFIFOW(fd,8),skillnum,skilllv);
	}
}

/*==========================================
 * スキル使用（map指定）
 *------------------------------------------
 */
void clif_parse_UseSkillMap(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;

	if(sd->ghost_timer != -1)
		pc_delghosttimer(sd);
	skill_castend_map(sd,RFIFOW(fd,2),RFIFOP(fd,4));
}
/*==========================================
 * メモ要求
 *------------------------------------------
 */
void clif_parse_RequestMemo(int fd,struct map_session_data *sd)
{
	pc_memo(sd,-1);
}
/*==========================================
 * アイテム合成
 *------------------------------------------
 */
void clif_parse_ProduceMix(int fd,struct map_session_data *sd)
{
	skill_produce_mix(sd,RFIFOW(fd,2),RFIFOW(fd,4),RFIFOW(fd,6),RFIFOW(fd,8));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcSelectMenu(int fd,struct map_session_data *sd)
{
	sd->npc_menu=RFIFOB(fd,6);
	npc_scriptcont(sd,RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcNextClicked(int fd,struct map_session_data *sd)
{
	npc_scriptcont(sd,RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcAmountInput(int fd,struct map_session_data *sd)
{
	sd->npc_amount=RFIFOL(fd,6);
	npc_scriptcont(sd,RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcCloseClicked(int fd,struct map_session_data *sd)
{
	// nop
}

/*==========================================
 * アイテム鑑定
 *------------------------------------------
 */
void clif_parse_ItemIdentify(int fd,struct map_session_data *sd)
{
	pc_item_identify(sd,RFIFOW(fd,2)-2);
}
/*==========================================
 * 矢作成
 *------------------------------------------
 */
void clif_arrow_created(int fd,struct map_session_data *sd)
{
	int i,mate=0;
	struct item tmp_item;
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.identify=1;

	for(i=1;i<MAX_SKILL_ARROW_DB;i++){
		if(RFIFOW(fd,2) == skill_arrow_db[i].nameid){
			mate = i;
			break;
		}
	}
	for(i=0;i<3;i++){
		tmp_item.nameid=skill_arrow_db[mate].cre_id[i];
		tmp_item.amount=skill_arrow_db[mate].cre_amount[i];
		if(!tmp_item.nameid||!tmp_item.amount)break;
		pc_additem(sd,&tmp_item,tmp_item.amount);

		printf("arrow create %d -> %d:%d\n",mate,tmp_item.nameid,tmp_item.amount);
	}
	pc_delitem(sd,pc_search_inventory(sd,RFIFOW(fd,2)),1,0);
}
/*==========================================
 * カード使用
 *------------------------------------------
 */
void clif_parse_UseCard(int fd,struct map_session_data *sd)
{
	clif_use_card(sd,RFIFOW(fd,2)-2);
}
/*==========================================
 * カード挿入装備選択
 *------------------------------------------
 */
void clif_parse_InsertCard(int fd,struct map_session_data *sd)
{
	pc_insert_card(sd,RFIFOW(fd,2)-2,RFIFOW(fd,4)-2);
}


/*==========================================
 * 0193 キャラID名前引き
 *------------------------------------------
 */
void clif_parse_SolveCharName(int fd,struct map_session_data *sd)
{
	clif_solved_charname(sd,RFIFOL(fd,2));
}

/*==========================================
 * 0197 /resetskill /resetstate
 *------------------------------------------
 */
void clif_parse_ResetChar(int fd,struct map_session_data *sd)
{
	if (battle_config.atc_gmonly == 0 || pc_isGM(sd) >= atcommand_config.resetstate) {
		switch(RFIFOW(fd,2)){
		case 0:
			pc_resetstate(sd);
			break;
		case 1:
			pc_resetskill(sd);
			break;
		}
	}
}

/*==========================================
 * 019c /lb等
 *------------------------------------------
 */
void clif_parse_LGMmessage(int fd,struct map_session_data *sd)
{
	if (battle_config.atc_gmonly == 0 || pc_isGM(sd) >= atcommand_config.local_broadcast) {
		WFIFOW(fd,0)=0x9a;
		WFIFOW(fd,2)=RFIFOW(fd,2);
		memcpy(WFIFOP(fd,4),RFIFOP(fd,4),RFIFOW(fd,2)-4);
		clif_send(WFIFOP(fd,0),RFIFOW(fd,2),&sd->bl,ALL_SAMEMAP);
	}
}

/*==========================================
 * カプラ倉庫へ入れる
 *------------------------------------------
 */
void clif_parse_MoveToKafra(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	storage_storageadd(sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void clif_parse_MoveFromKafra(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	storage_storageget(sd,RFIFOW(fd,2)-1,RFIFOL(fd,4));
}

/*==========================================
 * カプラ倉庫へカートから入れる
 *------------------------------------------
 */
void clif_parse_MoveToKafraFromCart(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	storage_storageaddfromcart(sd,RFIFOW(fd,2)-2,RFIFOL(fd,4));
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void clif_parse_MoveFromKafraToCart(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	storage_storagegettocart(sd,RFIFOW(fd,2)-1,RFIFOL(fd,4));
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
void clif_parse_CloseKafra(int fd,struct map_session_data *sd)
{
	if(sd->npc_id!=0)return;
	storage_storageclose(sd);
}

/*==========================================
 * パーティを作る
 *------------------------------------------
 */
void clif_parse_CreateParty(int fd,struct map_session_data *sd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 7){
		party_create(sd,RFIFOP(fd,2));
	}
	else
		clif_skill_fail(sd,1,0,4);
}

/*==========================================
 * パーティを作る
 *------------------------------------------
 */
void clif_parse_CreateParty2(int fd,struct map_session_data *sd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 7){
		party_create(sd,RFIFOP(fd,2));
	}
	else
		clif_skill_fail(sd,1,0,4);
}


/*==========================================
 * パーティに勧誘
 *------------------------------------------
 */
void clif_parse_PartyInvite(int fd,struct map_session_data *sd)
{
	party_invite(sd,RFIFOL(fd,2));
}
/*==========================================
 * パーティ勧誘返答
 *------------------------------------------
 */
void clif_parse_ReplyPartyInvite(int fd,struct map_session_data *sd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 5){
		party_reply_invite(sd,RFIFOL(fd,2),RFIFOL(fd,6));
	}
	else {
		party_reply_invite(sd,RFIFOL(fd,2),-1);
		clif_skill_fail(sd,1,0,4);
	}
}
/*==========================================
 * パーティ脱退要求
 *------------------------------------------
 */
void clif_parse_LeaveParty(int fd,struct map_session_data *sd)
{
	party_leave(sd);
}
/*==========================================
 * パーティ除名要求
 *------------------------------------------
 */
void clif_parse_RemovePartyMember(int fd,struct map_session_data *sd)
{
	party_removemember(sd,RFIFOL(fd,2),RFIFOP(fd,6));
}
/*==========================================
 * パーティ設定変更要求
 *------------------------------------------
 */
void clif_parse_PartyChangeOption(int fd,struct map_session_data *sd)
{
	party_changeoption(sd,RFIFOW(fd,2),RFIFOW(fd,4));
}
/*==========================================
 * パーティメッセージ送信要求
 *------------------------------------------
 */
void clif_parse_PartyMessage(int fd,struct map_session_data *sd)
{
	if(atcommand(fd,sd,RFIFOP(fd,4))) return;
	party_send_message(sd,RFIFOP(fd,4),RFIFOW(fd,2)-4);
}

/*==========================================
 * 露店閉鎖
 *------------------------------------------
 */
void clif_parse_CloseVending(int fd,struct map_session_data *sd)
{
	vending_closevending(sd);
}

/*==========================================
 * 露店アイテムリスト要求
 *------------------------------------------
 */
void clif_parse_VendingListReq(int fd,struct map_session_data *sd)
{
	vending_vendinglistreq(sd,RFIFOL(fd,2));
}

/*==========================================
 * 露店アイテム購入
 *------------------------------------------
 */
void clif_parse_PurchaseReq(int fd,struct map_session_data *sd)
{
	vending_purchasereq(sd,RFIFOW(fd,2),RFIFOL(fd,4),RFIFOP(fd,8));
}

/*==========================================
 * 露店開設
 *------------------------------------------
 */
void clif_parse_OpenVending(int fd,struct map_session_data *sd)
{
	vending_openvending(sd,RFIFOW(fd,2),RFIFOP(fd,4),RFIFOB(fd,84),RFIFOP(fd,85));
}

/*==========================================
 * ギルドを作る
 *------------------------------------------
 */
void clif_parse_CreateGuild(int fd,struct map_session_data *sd)
{
	guild_create(sd,RFIFOP(fd,6));
}
/*==========================================
 * ギルドマスターかどうか確認
 *------------------------------------------
 */
void clif_parse_GuildCheckMaster(int fd,struct map_session_data *sd)
{
	clif_guild_masterormember(sd);
}
/*==========================================
 * ギルド情報要求
 *------------------------------------------
 */
void clif_parse_GuildReqeustInfo(int fd,struct map_session_data *sd)
{
	switch(RFIFOL(fd,2)){
	case 0:	// ギルド基本情報、同盟敵対情報
		clif_guild_basicinfo(sd);
		clif_guild_allianceinfo(sd);
		break;
	case 1:	// メンバーリスト、役職名リスト
		clif_guild_positionnamelist(sd);
		clif_guild_memberlist(sd);
		break;
	case 2:	// 役職名リスト、役職情報リスト
		clif_guild_positionnamelist(sd);
		clif_guild_positioninfolist(sd);
		break;
	case 3:	// スキルリスト
		clif_guild_skillinfo(sd);
		break;
	case 4:	// 追放リスト
		clif_guild_explusionlist(sd);
		break;
	default:
		printf("clif: guild request info: unknown type %d\n",RFIFOL(fd,2));
	}
}
/*==========================================
 * ギルド役職変更
 *------------------------------------------
 */
void clif_parse_GuildChangePositionInfo(int fd,struct map_session_data *sd)
{
	int i;
	for(i=4;i<RFIFOW(fd,2);i+=40){
		guild_change_position(sd,RFIFOL(fd,i),
			RFIFOL(fd,i+4),RFIFOL(fd,i+12),RFIFOP(fd,i+16));
	}
}
/*==========================================
 * ギルドメンバ役職変更
 *------------------------------------------
 */
void clif_parse_GuildChangeMemberPosition(int fd,struct map_session_data *sd)
{
	int i;
	for(i=4;i<RFIFOW(fd,2);i+=12){
		guild_change_memberposition(sd->status.guild_id,
			RFIFOL(fd,i),RFIFOL(fd,i+4),RFIFOL(fd,i+8));
	}
}

/*==========================================
 * ギルドエンブレム要求
 *------------------------------------------
 */
void clif_parse_GuildRequestEmblem(int fd,struct map_session_data *sd)
{
	struct guild *g=guild_search(RFIFOL(fd,2));
	if(g!=NULL)
		clif_guild_emblem(sd,g);
}
/*==========================================
 * ギルドエンブレム変更
 *------------------------------------------
 */
void clif_parse_GuildChangeEmblem(int fd,struct map_session_data *sd)
{
	guild_change_emblem(sd,RFIFOW(fd,2)-4,RFIFOP(fd,4));
}
/*==========================================
 * ギルド告知変更
 *------------------------------------------
 */
void clif_parse_GuildChangeNotice(int fd,struct map_session_data *sd)
{
	guild_change_notice(sd,RFIFOL(fd,2),RFIFOP(fd,6),RFIFOP(fd,66));
}

/*==========================================
 * ギルド勧誘
 *------------------------------------------
 */
void clif_parse_GuildInvite(int fd,struct map_session_data *sd)
{
	guild_invite(sd,RFIFOL(fd,2));
}
/*==========================================
 * ギルド勧誘返信
 *------------------------------------------
 */
void clif_parse_GuildReplyInvite(int fd,struct map_session_data *sd)
{
	guild_reply_invite(sd,RFIFOL(fd,2),RFIFOB(fd,6));
}
/*==========================================
 * ギルド脱退
 *------------------------------------------
 */
void clif_parse_GuildLeave(int fd,struct map_session_data *sd)
{
	guild_leave(sd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOP(fd,14));
}
/*==========================================
 * ギルド追放
 *------------------------------------------
 */
void clif_parse_GuildExplusion(int fd,struct map_session_data *sd)
{
	guild_explusion(sd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOP(fd,14));
}
/*==========================================
 * ギルド会話
 *------------------------------------------
 */
void clif_parse_GuildMessage(int fd,struct map_session_data *sd)
{
	if(atcommand(fd,sd,RFIFOP(fd,4))) return;
	guild_send_message(sd,RFIFOP(fd,4),RFIFOW(fd,2)-4);
}
/*==========================================
 * ギルド同盟要求
 *------------------------------------------
 */
void clif_parse_GuildRequestAlliance(int fd,struct map_session_data *sd)
{
	guild_reqalliance(sd,RFIFOL(fd,2));
}
/*==========================================
 * ギルド同盟要求返信
 *------------------------------------------
 */
void clif_parse_GuildReplyAlliance(int fd,struct map_session_data *sd)
{
	guild_reply_reqalliance(sd,RFIFOL(fd,2),RFIFOL(fd,6));
}
/*==========================================
 * ギルド関係解消
 *------------------------------------------
 */
void clif_parse_GuildDelAlliance(int fd,struct map_session_data *sd)
{
	guild_delalliance(sd,RFIFOL(fd,2),RFIFOL(fd,6));
}
/*==========================================
 * ギルド敵対
 *------------------------------------------
 */
void clif_parse_GuildOpposition(int fd,struct map_session_data *sd)
{
	guild_opposition(sd,RFIFOL(fd,2));
}
/*==========================================
 * ギルド解散
 *------------------------------------------
 */
void clif_parse_GuildBreak(int fd,struct map_session_data *sd)
{
	guild_break(sd,RFIFOP(fd,2));
}

// pet
void clif_parse_PetMenu(int fd,struct map_session_data *sd)
{
	pet_menu(sd,RFIFOB(fd,2));
}
void clif_parse_CatchPet(int fd,struct map_session_data *sd)
{
	pet_catch_process2(sd,RFIFOL(fd,2));
}

void clif_parse_SelectEgg(int fd,struct map_session_data *sd)
{
	pet_select_egg(sd,RFIFOW(fd,2)-2);
}

void clif_parse_SendEmotion(int fd,struct map_session_data *sd)
{
	clif_pet_emotion(sd->pet_npcdata,RFIFOL(fd,2));
}

void clif_parse_ChangePetName(int fd,struct map_session_data *sd)
{
	pet_change_name(sd,RFIFOP(fd,2));
}

// Kick
void clif_parse_GMKick(int fd,struct map_session_data *sd)
{
	struct block_list *target;
	int tid = RFIFOL(fd,2);

	if(pc_isGM(sd) >= atcommand_config.kick) {
		target = map_id2bl(tid);
		if(target) {
			if(target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if(pc_isGM(sd) > pc_isGM(tsd))
					clif_GM_kick(sd,tsd,1);
				else
					clif_GM_kickack(sd,0);
			}
			else if(target->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)target;
				mob_damage(NULL,md,md->hp);
			}
			else
				clif_GM_kickack(sd,0);
		}
		else
			clif_GM_kickack(sd,0);
	}
}

/*==========================================
 * GMによるチャット禁止時間参照（？）
 *------------------------------------------
 */
void clif_parse_GMReqNoChatCount(int fd,struct map_session_data *sd)
{
	/*	返信パケットが不明だが、放置でもエラーにはならない。
		ただ、何度も呼ばれるので鯖の負荷がほんの少し上がるかも知れない。
		0xb0のtype4ではない模様(0xb0は禁止される側に送られると予想)。 */
	return;
}

/*==========================================
 * GM funtions
 *------------------------------------------
 */
void clif_parse_Shift(int fd,struct map_session_data *sd)	// RoVeRT
{
	struct map_session_data *pl_sd;

	if ((pl_sd=map_nick2sd(RFIFOP(fd,2)))!=NULL && pc_isGM(sd) >= atcommand_config.jumpto)
		pc_setpos(sd, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y, 3);
}

void clif_parse_Recall(int fd,struct map_session_data *sd)	// RoVeRT
{
	struct map_session_data *pl_sd;

	if ((pl_sd=map_nick2sd(RFIFOP(fd,2)))!=NULL && pc_isGM(sd) >= atcommand_config.recall) {
		if(pc_isGM(sd) > pc_isGM(pl_sd))
			pc_setpos(pl_sd, sd->mapname, sd->bl.x, sd->bl.y, 2);
	}
}

void clif_parse_Hide(int fd,struct map_session_data *sd)	// RoVeRT
{
	if (pc_isGM(sd) >= atcommand_config.hide) {
		if(sd->status.option&64){
			sd->status.option -= 64;
		}else{
			sd->status.option += 64;
		}
		clif_changeoption(&sd->bl);
	}
}

/*==========================================
 * クライアントからのパケット解析
 * socket.cのdo_parsepacketから呼び出される
 *------------------------------------------
 */
static int clif_parse(int fd)
{
	int packet_len,cmd;
	struct map_session_data *sd;
	static void (*clif_parse_func_table[0x200])() = {
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		// 70
		NULL,NULL,
		clif_parse_WantToConnection,
		NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,
		clif_parse_LoadEndAck,
		clif_parse_TickSend,
		NULL,
		// 80
		NULL,NULL,NULL,NULL,NULL,
		clif_parse_WalkToXY,
		NULL,NULL, NULL,
		clif_parse_ActionRequest,
		NULL,NULL,
		clif_parse_GlobalMessage,
		NULL,NULL,NULL,
		// 90
		clif_parse_NpcClicked,
		NULL,NULL,NULL,
		clif_parse_GetCharNameRequest,
		NULL,
		clif_parse_Wis,
		NULL, NULL,
		clif_parse_GMmessage,
		NULL,
		clif_parse_ChangeDir,
		NULL,NULL,NULL,
		clif_parse_TakeItem,
		// a0
		NULL,NULL,
		clif_parse_DropItem,
		NULL,NULL,NULL,NULL,
		clif_parse_UseItem,
		NULL,
		clif_parse_EquipItem,
		NULL,
		clif_parse_UnequipItem,
		NULL,NULL,NULL,NULL,
		// b0
		NULL,NULL,
		clif_parse_Restart,
		NULL,NULL,NULL,NULL,NULL,
		clif_parse_NpcSelectMenu,
		clif_parse_NpcNextClicked,
		NULL,
		clif_parse_StatusUp,
		NULL,NULL,NULL,
		clif_parse_Emotion,

		// c0
		NULL,
		clif_parse_HowManyConnections,
		NULL,NULL,NULL,
		clif_parse_NpcBuySellSelected,
		NULL,NULL,
		clif_parse_NpcBuyListSend,
		clif_parse_NpcSellListSend,
		NULL,NULL,
		clif_parse_GMKick,
		NULL,NULL,NULL,
		// d0
		NULL,NULL,NULL,NULL,NULL,
		clif_parse_CreateChatRoom,
		NULL,NULL, NULL,
		clif_parse_ChatAddMember,
		NULL,NULL,NULL,NULL,
		clif_parse_ChatRoomStatusChange,
		NULL,
		// e0
		clif_parse_ChangeChatOwner,
		NULL,
		clif_parse_KickFromChat,
		clif_parse_ChatLeave,
		clif_parse_TradeRequest,
		NULL,
		clif_parse_TradeAck,
		NULL,
		clif_parse_TradeAddItem,
		NULL,NULL,
		clif_parse_TradeOk,
		NULL,
		clif_parse_TradeCansel,
		NULL,
		clif_parse_TradeCommit,
		// f0
		NULL,NULL,NULL,
		clif_parse_MoveToKafra,
		NULL,
		clif_parse_MoveFromKafra,
		NULL,
		clif_parse_CloseKafra,
		NULL,
		clif_parse_CreateParty,
		NULL,NULL,
		clif_parse_PartyInvite,
		NULL,NULL,
		clif_parse_ReplyPartyInvite,

		// 100
		clif_parse_LeaveParty,
		NULL,
		clif_parse_PartyChangeOption,
		clif_parse_RemovePartyMember,
		NULL,NULL,NULL,NULL, 
		clif_parse_PartyMessage,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		// 110
		NULL,NULL,
		clif_parse_SkillUp,
		clif_parse_UseSkillToId,
		NULL,NULL,
		clif_parse_UseSkillToPos,
		NULL,
		clif_parse_StopAttack,
		NULL,NULL,
		clif_parse_UseSkillMap,
		NULL,
		clif_parse_RequestMemo,
		NULL,NULL,
		// 120
		NULL,NULL,NULL,NULL,NULL,NULL,
		clif_parse_PutItemToCart,
		clif_parse_GetItemFromCart,
		clif_parse_MoveFromKafraToCart,
		clif_parse_MoveToKafraFromCart,
		clif_parse_RemoveOption,
		NULL,NULL,NULL,
		clif_parse_CloseVending,
		NULL,
		// 130
		clif_parse_VendingListReq,
		NULL,NULL,NULL,
		clif_parse_PurchaseReq,
		NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

		// 140
		clif_parse_MapMove,
		NULL,NULL,
		clif_parse_NpcAmountInput,
		NULL,NULL,
		clif_parse_NpcCloseClicked,
		NULL, NULL,NULL,NULL,NULL,NULL,
		clif_parse_GuildCheckMaster,
		NULL,
		clif_parse_GuildReqeustInfo,
		// 150
		NULL,
		clif_parse_GuildRequestEmblem,
		NULL,
		clif_parse_GuildChangeEmblem,
		NULL,
		clif_parse_GuildChangeMemberPosition,
		NULL,NULL, NULL,
		clif_parse_GuildLeave,
		NULL,
		clif_parse_GuildExplusion,
		NULL,
		clif_parse_GuildBreak,
		NULL,NULL,
		// 160
		NULL,
		clif_parse_GuildChangePositionInfo,
		NULL,NULL,NULL,
		clif_parse_CreateGuild,
		NULL,NULL, 
		clif_parse_GuildInvite,
		NULL,NULL,
		clif_parse_GuildReplyInvite,
		NULL,NULL,
		clif_parse_GuildChangeNotice,
		NULL,
		// 170
		clif_parse_GuildRequestAlliance,
		NULL,
		clif_parse_GuildReplyAlliance,
		NULL,NULL,NULL,NULL,NULL, 
		clif_parse_ItemIdentify,
		NULL,
		clif_parse_UseCard,
		NULL,
		clif_parse_InsertCard,
		NULL,
		clif_parse_GuildMessage,
		NULL,

		// 180
		clif_parse_GuildOpposition,
		NULL,NULL,
		clif_parse_GuildDelAlliance,
		NULL,NULL,NULL,NULL, NULL,NULL,
		clif_parse_QuitGame,
		NULL,NULL,NULL,
		clif_parse_ProduceMix,
		NULL,
		// 190
		NULL,NULL,NULL,
		clif_parse_SolveCharName,
		NULL,NULL,NULL,
		clif_parse_ResetChar,
		NULL,NULL,NULL,NULL,
		clif_parse_LGMmessage,
		clif_parse_Hide,	// /hide
		NULL,
		clif_parse_CatchPet,
		// 1a0
		NULL,
		clif_parse_PetMenu,
		NULL,NULL,NULL,
		clif_parse_ChangePetName,
		NULL,
		clif_parse_SelectEgg,
		NULL,
		clif_parse_SendEmotion,
		NULL,
		NULL,NULL,NULL,
		clif_arrow_created,
		clif_parse_ChangeCart,
		// 1b0
		NULL,NULL,
		clif_parse_OpenVending,
		NULL,NULL,NULL,NULL,NULL, NULL,NULL,
		clif_parse_Shift,	// /remove
		clif_parse_Shift,	// /shift
		clif_parse_Recall,	// /recall
		clif_parse_Recall,	// /summon
		NULL,NULL,

		// 1c0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		// 1d0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		clif_parse_GMReqNoChatCount,
		// 1e0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		clif_parse_CreateParty2,
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		// 1f0
		NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,

#if 0
	case 0xcc:	clif_parse_GMkill
	case 0xce:	clif_parse_GMkillall
	case 0xcf:	clif_parse_Ignore
	case 0xd0:	clif_parse_IgnoreAll
	case 0xd3:	clif_parse_IgnoreList
//	case 0xf3:	clif_parse_MoveToKafra
//	case 0xf5:	clif_parse_MoveFromKafra
//	case 0xf7:	clif_parse_CloseKafra
	case 0xf9:	clif_parse_CreateParty
	case 0xfc:	clif_parse_InviteParty
	case 0xff:	clif_parse_InvitePartyAck
	case 0x108:	clif_parse_PartyMessage
	case 0x11b:	clif_parse_TeleportSelect
	case 0x11d:	clif_parse_MemoRequest
	case 0x126:	clif_parse_MoveToCart
	case 0x127:	clif_parse_MoveFromCart
	case 0x128:	clif_parse_MoveToCartFromKafra
	case 0x129:	clif_parse_MoveFromCartToKafra
	case 0x12a:	clif_parse_RemoveOption
#endif
	};

	// char鯖に繋がってない間は接続禁止
	if(!chrif_isconnect())
		session[fd]->eof = 1;

	sd = session[fd]->session_data;

	// 接続が切れてるので後始末
	if(session[fd]->eof){
		if(sd && sd->state.auth)
			clif_quitsave(fd,sd);
		close(fd);
		delete_session(fd);
		return 0;
	}

	if(RFIFOREST(fd)<2)
		return 0;
	cmd = RFIFOW(fd,0);
	
	// 管理用パケット処理
	if(cmd>=30000){
		switch(cmd){
		case 0x7530:	// Athena情報所得
			WFIFOW(fd,0)=0x7531;
			WFIFOB(fd,2)=ATHENA_MAJOR_VERSION;
			WFIFOB(fd,3)=ATHENA_MINOR_VERSION;
			WFIFOB(fd,4)=ATHENA_REVISION;
			WFIFOB(fd,5)=ATHENA_RELEASE_FLAG;
			WFIFOB(fd,6)=ATHENA_OFFICIAL_FLAG;
			WFIFOB(fd,7)=ATHENA_SERVER_MAP;
			WFIFOW(fd,8)=ATHENA_MOD_VERSION;
			WFIFOSET(fd,10);
			RFIFOSKIP(fd,2);
			break;
		case 0x7532:	// 接続の切断
			close(fd);
			session[fd]->eof=1;
			break;
		}
		return 0;
	}
	
	// ゲーム用以外パケットか、認証を終える前に0072以外が来たら、切断する
	if(cmd>=0x200 || packet_len_table[cmd]==0 ||
	   ((!sd || (sd && sd->state.auth==0)) && cmd!=0x72) ){
		close(fd);
		session[fd]->eof = 1;
		if(packet_len_table[cmd]==0) {
			printf("clif_parse : %d %d %x\n",fd,packet_len,cmd);
			printf("%x length 0 packet disconnect %d\n",cmd,fd);
		}
		return 0;
	}
	// パケット長を計算
	packet_len = packet_len_table[cmd];
	if(packet_len==-1){
		if(RFIFOREST(fd)<4)
			return 0;	// 可変長パケットで長さの所までデータが来てない
		packet_len = RFIFOW(fd,2);
		if(packet_len<4 || packet_len>32768){
			close(fd);
			session[fd]->eof =1;
			return 0;
		}
	}
	if(RFIFOREST(fd)<packet_len)
		return 0;	// まだ1パケット分データが揃ってない

	if(sd && sd->state.auth==1 &&
		sd->state.waitingdisconnect==1 ){// 切断待ちの場合パケットを処理しない
		
	}else if(clif_parse_func_table[cmd]){
		// パケット処理
		//printf("clif_parse : %d %d %x\n",fd,packet_len,cmd);
		clif_parse_func_table[cmd](fd,sd);
	} else {
		// 不明なパケット
		printf("clif_parse : %d %d %x\n",fd,packet_len,cmd);
#ifdef DUMP_UNKNOWN_PACKET
		{
			int i;
			printf("---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
			for(i=0;i<packet_len;i++){
				if((i&15)==0)
					printf("\n%04X ",i);
				printf("%02X ",RFIFOB(fd,i));
			}
			printf("\n");
		}
#endif
	}
	RFIFOSKIP(fd,packet_len);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_clif(void)
{
	int i;

	set_defaultparse(clif_parse);
	for(i=0;i<10;i++){
		if(make_listen_port(map_port))
			break;
		sleep(20);
	}
	if(i==10){
		printf("cant bind game port\n");
		exit(1);
	}
	add_timer_func_list(clif_waitclose,"clif_waitclose");

	return 0;
}

