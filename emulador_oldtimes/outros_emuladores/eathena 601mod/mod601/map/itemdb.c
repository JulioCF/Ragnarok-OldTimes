// $Id: itemdb.c,v 1.8 2003/06/29 05:56:44 lemit Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "grfio.h"
#include "map.h"
#include "itemdb.h"
#include "script.h"
#include "pc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define MAX_RANDITEM	2000

// ** ITEMDB_OVERRIDE_NAME **
//   定義するとアイテムの名前をdata.grf(idnum2displaynametable.txt)から読みます
//   オーバーライドされたら困る場合はコメント化してください
//   override item name from data.grf(idnum2displaynametable.txt) if defined.
//#define ITEMDB_OVERRIDE_NAME			1

// ** ITEMDB_OVERRIDE_NAME_VERBOSE **
//   定義すると、itemdb.txtとgrfで名前が異なる場合、表示します.
//#define ITEMDB_OVERRIDE_NAME_VERBOSE	1


static struct dbt* item_db;

static struct random_item_data blue_box[MAX_RANDITEM],violet_box[MAX_RANDITEM],card_album[MAX_RANDITEM],gift_box[MAX_RANDITEM],scroll[MAX_RANDITEM];
static int blue_box_count=0,violet_box_count=0,card_album_count=0,gift_box_count=0,scroll_count=0;
static int blue_box_default=0,violet_box_default=0,card_album_default=0,gift_box_default=0,scroll_default=0;

/*==========================================
 * 名前で検索用
 *------------------------------------------
 */
int itemdb_searchname_sub(void *key,void *data,va_list ap)
{
	struct item_data *item=(struct item_data *)data,**dst;
	char *str;
	str=va_arg(ap,char *);
	dst=va_arg(ap,struct item_data **);
	if(strcmpi(item->name,str)==0 || strcmp(item->jname,str)==0)
		*dst=item;
	return 0;
}
/*==========================================
 * 名前で検索
 *------------------------------------------
 */
struct item_data* itemdb_searchname(const char *str)
{
	struct item_data *item=NULL;
	numdb_foreach(item_db,itemdb_searchname_sub,str,&item);
	return item;
}

/*==========================================
 * 箱系アイテム検索
 *------------------------------------------
 */
int itemdb_searchrandomid(int flags)
{
	int nameid,i,index;

	if(flags == 1) {
		nameid = blue_box_default;
		if(blue_box_count > 0) {
			for(i=0;i<1000;i++) {
				index = rand()%blue_box_count;
				if(rand()%1000000 < blue_box[index].per) {
					nameid = blue_box[index].nameid;
					break;
				}
			}
		}
	}
	else if(flags == 2) {
		nameid = violet_box_default;
		if(violet_box_count > 0) {
			for(i=0;i<1000;i++) {
				index = rand()%violet_box_count;
				if(rand()%1000000 < violet_box[index].per) {
					nameid = violet_box[index].nameid;
					break;
				}
			}
		}
	}
	else if(flags == 3) {
		nameid = card_album_default;
		if(card_album_count > 0) {
			for(i=0;i<1000;i++) {
				index = rand()%card_album_count;
				if(rand()%1000000 < card_album[index].per) {
					nameid = card_album[index].nameid;
					break;
				}
			}
		}
	}
	else if(flags == 4) {
		nameid = gift_box_default;
		if(gift_box_count > 0) {
			for(i=0;i<1000;i++) {
				index = rand()%gift_box_count;
				if(rand()%1000000 < gift_box[index].per) {
					nameid = gift_box[index].nameid;
					break;
				}
			}
		}
	}
	else if(flags == 5) {
		nameid = scroll_default;
		if(scroll_count > 0) {
			for(i=0;i<1000;i++) {
				index = rand()%scroll_count;
				if(rand()%1000000 < scroll[index].per) {
					nameid = scroll[index].nameid;
					break;
				}
			}
		}
	}
	else
		nameid = 0;
//	printf("get %d\n",nameid);
	return nameid;
}

/*==========================================
 *
 *------------------------------------------
 */
struct item_data* itemdb_search(int nameid)
{
	struct item_data *id;

	id=numdb_search(item_db,nameid);
	if(id) return id;

	id=malloc(sizeof(struct item_data));
	if(id==NULL){
		printf("out of memory : itemdb_search\n");
		exit(1);
	}
	memset(id,0,sizeof(struct item_data));
	numdb_insert(item_db,nameid,id);

	id->nameid=nameid;
	id->value=10;
	id->weight=10;

	if(nameid>500 && nameid<600)
		id->type=0;   //heal item
	else if(nameid>600 && nameid<700)
		id->type=2;   //use item
	else if((nameid>700 && nameid<1100) ||
			(nameid>7000 && nameid<8000))
		id->type=3;   //correction
	else if(nameid>=1750 && nameid<1771)
		id->type=10;  //arrow
	else if(nameid>1100 && nameid<2000)
		id->type=4;   //weapon
	else if((nameid>2100 && nameid<3000) ||
			(nameid>5000 && nameid<6000))
		id->type=5;   //armor
	else if(nameid>4000 && nameid<5000)
		id->type=6;   //card
	else if(nameid>9000 && nameid<10000)
		id->type=7;   //egg
	else if(nameid>10000)
		id->type=8;   //petequip

	return id;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_sellvalue(int nameid)
{
	return itemdb_search(nameid)->value/2;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip(int nameid)
{
	int type=itemdb_type(nameid);
	if(type==0 || type==2 || type==3 || type==6 || type==10)
		return 0;
	return 1;
}

/*==========================================
 * 装備可能なすべての箇所の組み合わせを返す
 *------------------------------------------
 */
int itemdb_equippoint(struct map_session_data *sd,int nameid)
{
	struct item_data *id=itemdb_search(nameid);
	if(sd){
		if((id->equip==2)&&(sd->status.class == 12)&&(pc_checkskill(sd, 133)!=-1))
			return 34;
	}
	return id->equip;
}

//
// 初期化
//
/*==========================================
 *
 *------------------------------------------
 */
static int itemdb_read_itemslottable(void)
{
	char *buf,*p;
	int s;

	buf=grfio_read("data\\itemslottable.txt");
	if(buf==NULL)
		return -1;
	s=grfio_size("data\\itemslottable.txt");
	buf[s]=0;
	for(p=buf;p-buf<s;){
		int nameid,equip;
		sscanf(p,"%d#%d#",&nameid,&equip);
		itemdb_search(nameid)->equip=equip;
		p=strchr(p,10);
		if(!p) break;
		p++;
		p=strchr(p,10);
		if(!p) break;
		p++;
	}
	free(buf);

	return 0;
}

/*==========================================
 * アイテムデータベースの読み込み
 *------------------------------------------
 */
static int itemdb_readdb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p,*np;
	struct item_data *id;

	fp=fopen("db/item_db.txt","r");
	if(fp==NULL){
		printf("can't read db/item_db.txt\n");
		return 0;
	}
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,np=p=line;j<17 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p){ *p++=0; np=p; }
		}
		if(str[0]==NULL)
			continue;
		
		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000)
			continue;
		ln++;

		//ID,Name,Jname,Type,Price,Sell,Weight,ATK,DEF,Range,Slot,Job,Gender,Loc,wLV,eLV,View
		id=itemdb_search(nameid);
		memcpy(id->name,str[1],24);
		memcpy(id->jname,str[2],24);
		id->type=atoi(str[3]);
		//Sellの項目をお店での売値にした。byれあ
		if(atoi(str[5]))
			id->value=atoi(str[5])*2;
		else
			id->value=atoi(str[4]);
		id->weight=atoi(str[6]);
		id->atk=atoi(str[7]);
		id->def=atoi(str[8]);
		id->range=atoi(str[9]);
		id->slot=atoi(str[10]);
		id->class=atoi(str[11]);
		id->sex=atoi(str[12]);
		if(id->equip != atoi(str[13])){
			//printf("%d : equip point %d -> %d\n",nameid,id->equip,atoi(str[13]));
			id->equip=atoi(str[13]);
		}
		id->wlv=atoi(str[14]);
		id->elv=atoi(str[15]);
		id->look=atoi(str[16]);

		id->use_script=NULL;
		id->equip_script=NULL;

		if((p=strchr(np,'{'))==NULL)
			continue;
		id->use_script = parse_script(p,0);
		if((p=strchr(p+1,'{'))==NULL)
			continue;
		id->equip_script = parse_script(p,0);
	}
	fclose(fp);
	printf("read db/item_db.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * アイテム価格テーブルのオーバーライド
 *------------------------------------------
 */
static int itemdb_read_itemvaluedb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p;
	struct item_data *id;
	
	if( (fp=fopen("db/item_value_db.txt","r"))==NULL){
		printf("can't read db/item_value_db.txt\n");
		return -1;
	}
	
	while(fgets(line,1020,fp)){

		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<5 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000)
			continue;

		id=itemdb_search(nameid);
		ln++;
		if(str[3]!=NULL){
			if(str[4]!=NULL && *str[4] && atoi(str[4]))
				id->value=atoi(str[4])*2;
			else if( *str[3] )
				id->value=atoi(str[3]);
		}
	}
	fclose(fp);
	printf("read db/item_value_db.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * 装備可能職業テーブルのオーバーライド
 *------------------------------------------
 */
static int itemdb_read_classequipdb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,i,j;
	char *str[32],*p;
	struct item_data *id;
	
	if( (fp=fopen("db/class_equip_db.txt","r"))==NULL ){
		printf("can't read db/class_equip_db.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<MAX_PC_CLASS+2 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000)
			continue;

		if(!itemdb_isequip(nameid) || (nameid>1750 && nameid<1771))
			continue;
		ln++;

		//ID,Name,class1,class2,class3, ...... ,class22
		id=itemdb_search(nameid);
		id->class = 0;
		for(i=2;i<MAX_PC_CLASS+2;i++) {
			if(str[i]!=NULL && str[i][0]) {
				j = atoi(str[i]);
				if(j == 99) {
					for(j=0;j<MAX_PC_CLASS;j++)
						id->class |= 1<<j;
					break;
				}
				else if(j >= 0 && j < MAX_PC_CLASS) {
					id->class |= 1<<j;
					if(j == 7)
						id->class |= 1<<13;
					if(j == 14)
						id->class |= 1<<21;
					if(j == 22)
						id->class |= 1<<23;
				}
			}
		}
	}
	fclose(fp);
	printf("read db/class_equip_db.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * ランダムアイテム出現データの読み込み
 *------------------------------------------
 */
static int itemdb_read_randomitem()
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,i,j;
	char *str[10],*p;
	
	const struct {
		char filename[64];
		struct random_item_data *pdata;
		int *pcount,*pdefault;
	} data[] = {
		{"db/item_bluebox.txt",		blue_box,	&blue_box_count, &blue_box_default		},
		{"db/item_violetbox.txt",	violet_box,	&violet_box_count, &violet_box_default	},
		{"db/item_cardalbum.txt",	card_album,	&card_album_count, &card_album_default	},
		{"db/item_giftbox.txt",	gift_box,	&gift_box_count, &gift_box_default	},
		{"db/item_scroll.txt",	scroll,	&scroll_count, &scroll_default	},
	};
	
	
	for(i=0;i<sizeof(data)/sizeof(data[0]);i++){
		struct random_item_data *pd=data[i].pdata;
		int *pc=data[i].pcount;
		int *pdefault=data[i].pdefault;
		char *fn=data[i].filename;
	
		if( (fp=fopen(fn,"r"))==NULL ){
			printf("can't read %s\n",fn);
			continue;
		}

		while(fgets(line,1020,fp)){
			if(line[0]=='/' && line[1]=='/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,p=line;j<3 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}

			if(str[0]==NULL)
				continue;
	
			nameid=atoi(str[0]);
			if(nameid<0 || nameid>=20000)
				continue;
			if(nameid == 0) {
				if(str[2])
					*pdefault = atoi(str[2]);
				continue;
			}

			if(str[2]){
				pd[ *pc   ].nameid = nameid;
				pd[(*pc)++].per = atoi(str[2]);
			}

			if(ln >= MAX_RANDITEM)
				break;
			ln++;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",fn,*pc);
	}

	return 0;
}

/*==========================================
 * アイテムの名前テーブルを読み込む
 *------------------------------------------
 */
static int itemdb_read_itemnametable(void)
{
	char *buf,*p;
	int s;

	buf=grfio_reads("data\\idnum2itemdisplaynametable.txt",&s);

	if(buf==NULL)
		return -1;

	buf[s]=0;
	for(p=buf;p-buf<s;){
		int nameid;
		char buf2[64];

		if(	sscanf(p,"%d#%[^#]#",&nameid,buf2)==2 ){

			if( strncmp(itemdb_search(nameid)->jname,buf2,24)!=0 )
#ifdef ITEMDB_OVERRIDE_NAME_VERBOSE
				printf("[override] %d %s => %s\n",nameid
					,itemdb_search(nameid)->jname,buf2);
#endif
			memcpy(itemdb_search(nameid)->jname,buf2,24);
		}
		
		p=strchr(p,10);
		if(!p) break;
		p++;
	}
	free(buf);
	printf("read data\\idnum2itemdisplaynametable.txt done.\n");

	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */
static int itemdb_final(void *key,void *data,va_list ap)
{
	struct item_data *id;

	id=data;
	if(id->use_script)
		free(id->use_script);
	if(id->equip_script)
		free(id->equip_script);
	free(id);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void do_final_itemdb(void)
{
	if(item_db){
		numdb_final(item_db,itemdb_final);
		item_db=NULL;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_itemdb(void)
{
	item_db = numdb_init();

	itemdb_read_itemslottable();
	itemdb_readdb();
	itemdb_read_classequipdb();
	itemdb_read_itemvaluedb();
	itemdb_read_randomitem();
#ifdef ITEMDB_OVERRIDE_NAME
	itemdb_read_itemnametable();
#endif
	
	return 0;
}
