//atcommand.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "socket.h"
#include "timer.h"

#include "clif.h"
#include "chrif.h"
#include "intif.h"
#include "itemdb.h"
#include "map.h"
#include "pc.h"
#include "skill.h"
#include "mob.h"
#include "pet.h"
#include "battle.h"
#include "party.h"
#include "guild.h"
#include "atcommand.h"

struct Atcommand_Config atcommand_config;

int atcommand(int fd,struct map_session_data *sd,char *message)
{
	int gm_level = pc_isGM(sd);
	int flag;
	char s_flag=0;
	//＠コマンドGM専用化
//	if (battle_config.atc_gmonly && !gm_level ) return 0;

	message += strlen(sd->status.name);
	while(*message && (isspace(*message) || (s_flag==0 && *message==':'))) {
		if(*message==':') s_flag=1;
		message++;
	}

	if (message && message[0] == '@') {	//メッセージ中に @ が現れたら
		char command[100];		//@rura などのシステムコマンド文字列が入る
		char temp0[100];			//引数1個目
		char temp1[100];			//引数2個目
//      char temp2[100];         //引数3個目
		char moji[200];			//長文文字列
		int x = 0, y = 0, z = 0;	//座標とか
		int i1 = 0, i2 = 0, i3 = 0, i;
		struct map_session_data *pl_sd;

		sscanf(message, "%s",command);

//バシルーラ
//「@rura+ 名前 マップファイル名 ｘ座標 y座標」と入力
		if ((strcmpi(command, "@rura+") == 0 || strcmpi(command, "@send") == 0 || strcmpi(command, "@warp+") == 0) && gm_level >= atcommand_config.rurap) {
			sscanf(message, "%s%s%s%d%d", command, temp1, temp0, &x, &y);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				if(gm_level > pc_isGM(pl_sd)) {
					if (x >= 0 && x < 400 && y >= 0 && y < 400) {
						if(pc_setpos(pl_sd, temp0, x, y, 3)==0){
							clif_displaymessage(pl_sd->fd,"They've been warped.");
						}else{
							clif_displaymessage(fd,"Map not found.");
						}
					}else{
						clif_displaymessage(fd,"Coordinates out of range.");
					}
				}
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
			return 1;
		}
//ルーラ
//「@rura マップファイル名 ｘ座標 y座標」と入力
		if ((strcmpi(command, "@rura") == 0 || strcmpi(command, "@warp") ==0) && gm_level >= atcommand_config.rura) {
			sscanf(message, "%s%s%d%d", command, temp0, &x, &y);
			if (x >= 0 && x < 400 && y >= 0 && y < 400) {
				if(pc_setpos(sd, temp0, x, y, 3)==0){
					clif_displaymessage(fd,"Warped!");
				}else{
					clif_displaymessage(fd,"Map not found.");
				}
			}else{
				clif_displaymessage(fd,"Coordinates out of range.");
			}
			return 1;
		}
//任意のキャラの居場所を調べる
//「@where キャラ名」と入力
		if(strcmpi(command, "@where") == 0 && gm_level >= atcommand_config.where){
			sscanf(message, "%s%s", command, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				sprintf(moji, "%s %s %d %d", temp1, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
				clif_displaymessage(fd,moji);
			}else{
//				clif_displaymessage(fd,"Character not found.");
				sprintf(moji, "%s %d %d", sd->mapname, sd->bl.x, sd->bl.y);
				clif_displaymessage(fd,moji);
			}
			return 1;
		}
//任意のキャラの所へ移動する
//「@jumpto キャラ名」と入力
		if((strcmpi(command, "@jumpto") == 0 || strcmpi(command, "@warpto") == 0) && gm_level >= atcommand_config.jumpto){
			sscanf(message, "%s %[^\n]", command, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				pc_setpos(sd, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y, 3);
				sprintf(moji,"Jump to %s",temp1);
				clif_displaymessage(fd,moji);
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
			return 1;
		}
//同マップ内の任意座標に跳ぶ
//「@jump x座標 y座標」と入力
		if(strcmpi(command, "@jump") == 0 && gm_level >= atcommand_config.jump){
			sscanf(message, "%s%d%d", command, &x, &y);
			if (x >= 0 && x < 400 && y >= 0 && y < 400) {
				pc_setpos(sd, sd->mapname, x, y, 3);
				sprintf(moji, "Jumping to %d %d now.", x, y);
				clif_displaymessage(fd,moji);
			}else{
				clif_displaymessage(fd,"Jump failed.");
			}
			return 1;
		}
//現在ログインしている全てのキャラ名を列挙する
//「@who」と入力
		if(strcmpi(command, "@who") == 0 && gm_level >= atcommand_config.who){
			for(i=0;i<fd_max;i++){	//人数分ループ
				if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
					sprintf(moji, "Name: %s | Location: %s %d %d", pl_sd->status.name, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
					clif_displaymessage(fd, moji);
				}
			}
			return 1;
		}
//Who2 by Sara, prints job and levels
		if(strcmpi(command, "@who2") == 0 && gm_level >= atcommand_config.who){
			for(i=0;i<fd_max;i++){
				if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
					//Check Job, copy name to temp variable
					if (pl_sd->status.class == 0){sprintf(temp0, "Novice");}
					if (pl_sd->status.class == 1){sprintf(temp0, "Swordsman");}
					if (pl_sd->status.class == 2){sprintf(temp0, "Mage");}
					if (pl_sd->status.class == 3){sprintf(temp0, "Archer");}
					if (pl_sd->status.class == 4){sprintf(temp0, "Acolyte");}
					if (pl_sd->status.class == 5){sprintf(temp0, "Merchant");}
					if (pl_sd->status.class == 6){sprintf(temp0, "Thief");}
					if (pl_sd->status.class == 7){sprintf(temp0, "Knight");}
					if (pl_sd->status.class == 8){sprintf(temp0, "Priest");}
					if (pl_sd->status.class == 9){sprintf(temp0, "Wizard");}
					if (pl_sd->status.class == 10){sprintf(temp0, "Blacksmith");}
					if (pl_sd->status.class == 11){sprintf(temp0, "Hunter");}
					if (pl_sd->status.class == 12){sprintf(temp0, "Assassin");}
					if (pl_sd->status.class == 13){sprintf(temp0, "Knight 2");}
					if (pl_sd->status.class == 14){sprintf(temp0, "Crusader");}
					if (pl_sd->status.class == 15){sprintf(temp0, "Monk");}
					if (pl_sd->status.class == 16){sprintf(temp0, "Sage");}
					if (pl_sd->status.class == 17){sprintf(temp0, "Rogue");}
					if (pl_sd->status.class == 18){sprintf(temp0, "Alchemist");}
					if (pl_sd->status.class == 19){sprintf(temp0, "Bard");}
					if (pl_sd->status.class == 20){sprintf(temp0, "Dancer");}
					if (pl_sd->status.class == 21){sprintf(temp0, "Crusader 2");}
					if (pl_sd->status.class == 22){sprintf(temp0, "Wedding");}
					if (pl_sd->status.class == 23){sprintf(temp0, "Super Novice");}
					sprintf(moji, "Name: %s | Job: %s | BLvl: %d | JLvl: %d", pl_sd->status.name, temp0, pl_sd->status.base_level, pl_sd->status.job_level);
					clif_displaymessage(fd, moji);
				}
			}
			return 1;
		}

//Who3 by Sara, prints guild and party names
		if(strcmpi(command, "@who3") == 0 && gm_level >= atcommand_config.who){
			for(i=0;i<fd_max;i++){
				if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
					struct guild *g=guild_search(pl_sd->status.guild_id);
					if(g==NULL){
						sprintf(temp1,"None");
					}else{
						sprintf(temp1, "%s", g->name);
					}
					struct party *p=party_search(pl_sd->status.party_id);
					if(p==NULL){
						sprintf(temp0, "None");
					}else{
						sprintf(temp0, "%s", p->name);
					}
					sprintf(moji, "Name: %s | Party: %s | Guild: %s", pl_sd->status.name, temp0, temp1);
					clif_displaymessage(fd, moji);
				}
			}
			return 1;
		}


//現在位置を含め、冒険の記録をセーブする
//「@save」と入力
		if (strcmpi(command, "@save") == 0 && gm_level >= atcommand_config.save) {
			pc_setsavepoint(sd,sd->mapname,sd->bl.x,sd->bl.y);
			pc_makesavestatus(sd);
			chrif_save(sd);
			clif_displaymessage(fd,"Character data and respawn point saved.");
			return 1;
		}
//セーブ地点にワープする
//「@load」と入力
		if (strcmpi(command, "@load") == 0 && gm_level >= atcommand_config.load) {
			pc_setpos(sd,sd->status.save_point.map , sd->status.save_point.x , sd->status.save_point.y, 0);
			clif_displaymessage(fd,"Warping to respawn point...");
			return 1;
		}
//歩行スピード変更
//「@speed スピード値[1～1000]」と入力,小さいほど速い、めちゃ挙動不審ｗ
		if (strcmpi(command, "@speed") == 0 && gm_level >= atcommand_config.speed) {
			sscanf(message, "%s%d", command, &x);
			if (x > 0 && x < 1000) {
				sd->speed = x;
				//sd->walktimer = x;
				//この文を追加 by れあ
				clif_updatestatus(sd,SP_SPEED);
				clif_displaymessage(fd,"Speed changed!");
			}
			return 1;
		}
//倉庫利用
//「@storage」と入力
		if(strcmpi(command, "@storage") == 0 && gm_level >= atcommand_config.storage) {
			storage_storageopen(sd);
			return 1;
		}
//状態変更
//「@option 1 20 15」のように値を記述
/*
	R 0119 <ID>.l <param1>.w <param2>.w <param3>.w ?.B
	見た目変更
	param1=01 石化？(固まる)
	param1=02 フロストダイバで凍り漬け?
	param1=03 ぴよる
	param1=04 眠り
	param1=06 暗闇(歩ける)
	
	param2=01 毒
	param2=02 背後霊
	param2=04 沈黙状態
	param2=16 周囲を暗くする
	
	param3=01 サイトかルワッチ?
	param3=02 ハイディング状態?
	param3=04 クローキング状態?
	param3=08 カート付き
	param3=16 鷹付き
	param3=32 ペコペコ乗り
	param3=64 消える
*/
		if (strcmpi(command, "@option") == 0 && gm_level >= atcommand_config.option) {
			sscanf(message, "%s%d%d%d", command, &x, &y, &z);
			/*
			   p->option は次のようなbitにおいて成り立っている？
			   pram1        0000 0000 0000 0000
			   pram2        0000 0000 0000 0000
			   pram3        0000 0000 0000 0000 

			 */
			sd->opt1=x;
			sd->opt2=y;
			sd->status.option=z;
			clif_changeoption(&sd->bl);
			clif_displaymessage(fd,"Options changed.");
			return 1;
		}
//消える
//「@hide」と入力
		if (strcmpi(command, "@hide") == 0 && gm_level >= atcommand_config.hide) {
			if(sd->status.option&4){
				sd->status.option-=4;
				clif_displaymessage(fd,"Hide: Off.");
			}else{
				sd->status.option+=4;
				clif_displaymessage(fd,"Hide: On.");
			}
			clif_changeoption(&sd->bl);
			return 1;
		}
//その場で転職
//「@jobchange 職業ID[0～21]」のように職業ごとに値を記述
/*
0	Novice
1	Swordman
2	Mage
3	Archer
4	Acolyte
5	Merchant
6	Thief
7	Knight
8	Priest
9	Wizard
10	Blacksmith
11	Hunter
12	Assassin
13	Knight2
14	Crusader
15	Monk
16	Sage
17	Rogue
18	Alchem
19	Bard
20	Dancer
21	Crusader2
*/
		if (strcmpi(command, "@jobchange") == 0 && gm_level >= atcommand_config.jobchange) {
			sscanf(message, "%s%d", command, &x);
			if ((x >= 0 && x < MAX_PC_CLASS)) {
				pc_jobchange(sd,x);
				clif_displaymessage(fd,"Your job has been changed.");
			}
			return 1;
		}
//自殺
//「@die」と入力する。
		if (strcmpi(command, "@die") == 0 && gm_level >= atcommand_config.die) {
			pc_damage(NULL,sd,sd->status.hp+1);
			clif_displaymessage(fd,"A pity! You've died.");
			return 1;
		}
//他殺
//Modified by Syrus22 - September 17, 2003
//@kill <char name>
//Description: Kills target character. Including yourself.
		if (strcmpi(command, "@kill") == 0 && gm_level >= atcommand_config.kill) {
			sscanf(message, "%s %[^\n]", command, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				if(gm_level > pc_isGM(pl_sd)) {
					pc_damage(NULL,pl_sd,pl_sd->status.hp+1);
					clif_displaymessage(fd,"Character killed.");
				}
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
			return 1;
		}
//生き返り
//[@alive]と入力する
		if (strcmpi(command, "@alive") == 0 && gm_level >= atcommand_config.alive) {
			sd->status.hp=sd->status.max_hp;
			pc_setstand(sd);
			if(battle_config.ghost_time > 0)
				pc_setghosttimer(sd,battle_config.ghost_time);
			clif_updatestatus(sd,SP_HP);
			clif_resurrection(&sd->bl,1);
			clif_displaymessage(fd,"You've been revived! It's a miracle!");
			return 1;
		}
//天の声
//「@kami 台詞」と入力
		if ((strcmpi(command, "@kami") == 0 || strcmpi(command, "@kamib") == 0) && gm_level >= atcommand_config.kami) {
			sscanf(message, "%s %[^\n]", command, moji);
			intif_GMmessage(moji,strlen(moji)+1,(command[5]=='b')?0x10:0);
			return 1;
		}
//HP,SP 更新
//「@heal 数値(HP) 数値(SP)」と入力
//省略して「@heal」とだけ打てば、両方全回復
		if (strcmpi(command, "@heal") == 0 && gm_level >= atcommand_config.heal) {
			sscanf(message, "%s%d%d", command, &x, &y);
			if (x == 0 && y == 0) {	//省略記述で全回復
				x=sd->status.max_hp-sd->status.hp;
				y=sd->status.max_sp-sd->status.sp;
			} else {
				if (x > sd->status.hp) {
					x = 0;
				}
				if (y > sd->status.sp) {
					y = 0;
				}
			}
			clif_heal(fd,SP_HP,x);
			clif_heal(fd,SP_SP,y);
			pc_heal(sd,x,y);
			clif_displaymessage(fd,"HP and SP restored.");
			return 1;
		}
//アイテムゲット
//「@item アイテムの名前orID 個数」と入力
		if (strcmpi(command, "@item") == 0 && gm_level >= atcommand_config.item) {
			struct item item_tmp;
			struct item_data *item_data;
			sscanf(message, "%s%s%d", command, temp0, &y);
			if(y==0) y=1;	//個数指定が無かったら１個にするbyれあ
			
			if( (x=atoi(temp0))>0 )
				x=((item_data=itemdb_search(x))?x:0);
			else if( (item_data=itemdb_searchname(temp0))!=NULL )
				x=item_data->nameid;
			
			if( x>0 ) {
				int a1=1,a2=y,i;
				if(item_data->type==4 || item_data->type==5 || item_data->type==7){
					a1=y;
					a2=1;
				}
				for(i=0;i<a1;i++){
					memset(&item_tmp,0,sizeof(item_tmp));
					item_tmp.nameid=x;
					item_tmp.identify=1;
					if((flag = pc_additem(sd,&item_tmp,a2)))
						clif_additem(sd,0,0,flag);
				}
				clif_displaymessage(fd,"Item created.");
			} else {
				clif_displaymessage(fd,"Invalid Item ID.");
			}
			return 1;
		}
//所持アイテムリセット
//「@itemreset」と入力　パラメータ無し（無視)
		if (strcmpi(command, "@itemreset") == 0 && gm_level >= atcommand_config.itemreset) {
			for(i=0;i<MAX_INVENTORY;i++) {
				if (sd->status.inventory[i].amount && sd->status.inventory[i].equip ==0)
					pc_delitem(sd,i,sd->status.inventory[i].amount,0);
			}
			clif_displaymessage(fd,"All of your items have been reset..");
			return 1;
		}
//Lvアップコマンド
//「@Lvup 増加値」と入力
		if ((strcmpi(command, "@lvup") == 0 || strcmpi(command, "@lvlup") == 0 || strcmpi(command, "@baseup") == 0 || strcmpi(command, "@baselvlup") == 0) && gm_level >= atcommand_config.lvup) {
			sscanf(message, "%s%d", command, &x);
			if(x >= 1){
				for(i=sd->status.base_level+1;i<=sd->status.base_level+x;i++){
					if(i>=99)
						sd->status.status_point += 22;
					else
						sd->status.status_point += (i+14) / 5 ;
				}
				sd->status.base_level+=x;
				clif_updatestatus(sd,SP_BASELEVEL);
				clif_updatestatus(sd,SP_NEXTBASEEXP);
				clif_updatestatus(sd,SP_STATUSPOINT);
				pc_calcstatus(sd,0);
				pc_heal(sd,sd->status.max_hp,sd->status.max_sp);
				clif_misceffect(&sd->bl,0);
				clif_displaymessage(fd,"Base Level up.");
			}else if(x<0 && sd->status.base_level+x>0){
				sd->status.base_level+=x;
				clif_updatestatus(sd,SP_BASELEVEL);
				clif_updatestatus(sd,SP_NEXTBASEEXP);
				pc_calcstatus(sd,0);
				clif_displaymessage(fd,"Base Level down.");
			}
			return 1;
		}
//JobLvアップコマンド
//「@jobLvup 増加値」と入力
		if((strcmpi(command, "@joblvup") == 0 || strcmpi(command, "@jlvup") == 0 || strcmpi(command, "@joblvlup") == 0 || strcmpi(command, "@jlvlup") ==0) && gm_level >= atcommand_config.joblvup){
			y = 50;
			sscanf(message, "%s%d", command, &x);
			if(sd->status.class == 0)
				y -= 40;
			if(sd->status.job_level == y){
				clif_displaymessage(fd,"Job level can't go any higher.");
			}else if(x >= 1){
				if(sd->status.job_level + x > y)
					x = y - sd->status.job_level;
				sd->status.job_level+=x;
				clif_updatestatus(sd,SP_JOBLEVEL);
				clif_updatestatus(sd,SP_NEXTJOBEXP);
				sd->status.skill_point+=x;
				clif_updatestatus(sd,SP_SKILLPOINT);
				pc_calcstatus(sd,0);
				clif_misceffect(&sd->bl,1);
				clif_displaymessage(fd,"Job Level up.");
			}else if(x<0 && sd->status.job_level+x>0 ){
				sd->status.job_level+=x;
				clif_updatestatus(sd,SP_JOBLEVEL);
				clif_updatestatus(sd,SP_NEXTJOBEXP);
				pc_calcstatus(sd,0);
				clif_displaymessage(fd,"Job Level down.");
			}
			return 1;
		}
//ヘルプコマンド　＠コマンドの全容を説明してくれる。
//「@help」と入力
		if ((strcmpi(command, "@h") == 0 || strcmpi(command, "@help") == 0) && gm_level >= atcommand_config.help) {
			char moji[400];
			FILE *file;
			if(	(file = fopen("help.txt", "r"))!=NULL){
				clif_displaymessage(fd,"Command Help File");
				while (fgets(moji, 380, file) != NULL) {
					{
						int i;
						for (i = 0; moji[i] != '\0'; i++) {
							if (moji[i] == '\r' || moji[i] == '\n') {
								moji[i] = '\0';
							}
						}
					}
					clif_displaymessage(fd,moji);
				}
				fclose(file);
			}else
				clif_displaymessage(fd,"Help.txt not found.");
			return 1;
		}
//GMになる！ 同垢の全キャラはPTから抜け、倉庫は空にして下さい。
//「@GM」と入力
		if (strcmpi(command, "@GM") == 0 && gm_level >= atcommand_config.gm) {
			moji[0]=0;
			sscanf(message, "%s %[^\n]", command,moji);
			if(sd->status.party_id)
				clif_displaymessage(fd,"You must leave your party first.");
			else if(sd->status.guild_id)
				clif_displaymessage(fd,"You must leave your guild first.");
			else{
				pc_makesavestatus(sd);
				chrif_save(sd);
				clif_displaymessage(fd,"Command completed successfully.");
				chrif_changegm(sd->status.account_id,moji,strlen(moji)+1);
			}
			return 1;
		}

//PVP解除　　ただ、マップ移動しないと解除されません。
//「@pvpoff」と入力
		if (strcmpi(command, "@pvpoff") == 0 && gm_level >= atcommand_config.pvpoff) {
			if(map[sd->bl.m].flag.pvp) {
				map[sd->bl.m].flag.pvp = 0;
				clif_send0199(sd->bl.m,0);
				for(i=0;i<fd_max;i++){	//人数分ループ
					if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
						if(sd->bl.m == pl_sd->bl.m) {
							clif_pvpset(pl_sd,0,0,2);
							if(pl_sd->pvp_timer != -1) {
								delete_timer(pl_sd->pvp_timer,pc_calc_pvprank_timer);
								pl_sd->pvp_timer = -1;
							}
						}
					}
				}
				clif_displaymessage(fd,"PVP: Off.");
			}
			return 1;
		}

//PVP実装（仮）
//「@pvp」と入力
		if ((strcmpi(command, "@pvpon") == 0 || strcmpi(command, "@pvp") == 0) && gm_level >= atcommand_config.pvpon) {
			if(!map[sd->bl.m].flag.pvp) {
				map[sd->bl.m].flag.pvp = 1;
				clif_send0199(sd->bl.m,1);
				for(i=0;i<fd_max;i++){	//人数分ループ
					if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
						if(sd->bl.m == pl_sd->bl.m && pl_sd->pvp_timer == -1) {
							pl_sd->pvp_timer=add_timer(gettick()+200,pc_calc_pvprank_timer,pl_sd->bl.id,0);
							pl_sd->pvp_rank=0;
							pl_sd->pvp_lastusers=0;
							pl_sd->pvp_point=5;
						}
					}
				}
				clif_displaymessage(fd,"PVP: On.");
			}
			return 1;
		}

//@gvgoff
		if (strcmpi(command, "@gvgoff") == 0 && gm_level >= atcommand_config.gvgoff) {
			if(map[sd->bl.m].flag.gvg) {
				map[sd->bl.m].flag.gvg = 0;
				clif_send0199(sd->bl.m,0);
				clif_displaymessage(fd,"GVG: Off.");
			}
			return 1;
		}

//@gvgon
		if ((strcmpi(command, "@gvgon") == 0 || strcmpi(command, "@gvg") == 0) && gm_level >= atcommand_config.gvgon) {
			if(!map[sd->bl.m].flag.gvg) {
				map[sd->bl.m].flag.gvg = 1;
				clif_send0199(sd->bl.m,3);
				clif_displaymessage(fd,"GVG: On.");
			}
			return 1;
		}

//髪型、髪の色、服の色、変更
//「@model x y z」のように値を入力、ただしこれらを変更した状態で2HQを使うとエラーが・・・(´Д｀)　解明しだい直します。
/*	例：@model 15 4 0

x [0～19]髪型
y [0～8]髪の色
z [0～4]服の色

*/
		if (strcmpi(command, "@model") == 0 && gm_level >= atcommand_config.model) {
			sscanf(message, "%s%d%d%d", command, &x, &y, &z);
			if (x >= 0 && x < 20 && y >= 0 && y < 9 && z >= 0 && z <= 4) {
				//服の色変更
				if ((sd->status.class == 12 || sd->status.class == 14 || sd->status.class == 15 || sd->status.class == 16 || sd->status.class == 17 || sd->status.class == 21) && z != 0) {
					//服の色未実装職の判定
					clif_displaymessage(fd,"You can't use this command with this class.");
				} else {
					pc_changelook(sd,LOOK_HAIR,x);
					pc_changelook(sd,LOOK_HAIR_COLOR,y);
					pc_changelook(sd,LOOK_CLOTHES_COLOR,z);
					clif_displaymessage(fd,"Appearance changed.");
				}
			} else {
				clif_displaymessage(fd,"An invalid number was specified.");
			}
			return 1;
		}

// ADDED
// ルーラ簡易版
//「@go 数字」と入力
		if (strcmpi(command, "@go") == 0 && gm_level >= atcommand_config.go) {
			struct { char map[16]; int x,y; } data[] = {
				{	"prontera.gat",	156, 191	},	//	0=プロンテラ
				{	"morocc.gat",	156,  93	},	//	1=モロク
				{	"geffen.gat",	119,  59	},	//	2=ゲフェン
				{	"payon.gat",	 89, 122	},	//	3=フェイヨン
				{	"alberta.gat",	192, 147	},	//	4=アルベルタ
				{	"izlude.gat",	128, 114	},	//	5=イズルード
				{	"aldebaran.gat",140, 131	},	//	6=アルデバラン
				{	"xmas.gat",		147, 134	},	//	7=ルティエ
				{	"comodo.gat",	209, 143	},	//	8=コモド
				{	"yuno.gat",		157,  51	},	//	9=ジュノー
				{	"amatsu.gat",	198,  84	},	//	10=アマツ
				{	"gonryun.gat",	160, 120	},	//	11=ゴンリュン
				{	"umbala.gat",	145, 155	},	//	12=Umbala (done by Sara)
			};
			sscanf(message, "%s%d", command, &x);
			if (x >= 0 && x<sizeof(data)/sizeof(data[0])) {
				pc_setpos(sd, data[x].map, data[x].x, data[x].y, 3);
				clif_displaymessage(fd,"Warped.");
			} else {
				clif_displaymessage(fd,"Invalid location number.");
			}
			return 1;
		}
//モンスター沸き
//「@monster 表示名 モンスターIDor名前 モンスター数 x座標 y座標」と入力
// x座標 y座標 は省略可（ランダム）
		if ((strcmpi(command, "@monster") == 0 || strcmpi(command, "@spawn") == 0) && gm_level >= atcommand_config.monster) {
			i1 = i2 = x = y = 0;
			sscanf(message, "%s %s %s %d%d%d", command, temp1, temp0, &i2, &x, &y);
			if( (i1=atoi(temp0))==0 )
				i1=mobdb_searchname(temp0);
			printf("%s name=%s id=%d count=%d (%d,%d)\n", command, temp1, i1, i2, x, y);
		
			if(i1>=1000 && i1<2000 && mob_db[i1].max_hp>0){
				for(i=0;i<i2;i++) {
					int mx,my;
					if(x<=0) mx = sd->bl.x + (rand()%10 - 5);
					else mx = x;
					if(y<=0) my=sd->bl.y + (rand()%10 - 5);
					else my = y;
					mob_once_spawn(sd,"this",mx,my,temp1,i1,1,"");
				}
				clif_displaymessage(fd,"Monster summoned.");
			}else{
				clif_displaymessage(fd,"Invalid Monster ID.");
			}
			
			return 1;
		}
// 精錬 @refine 装備場所ID +数値
// 右手=2 左手=32 両手=34 頭=256/257/768/769 体=16 肩=4 足=64
		if(strcmpi(command,"@refine")==0 && gm_level >= atcommand_config.refine){
			i1=i2=0;
			if(sscanf(message,"%s%d%d",command,&i1,&i2) >= 3 && i1 > 0) {
				for(i=0;i< MAX_INVENTORY;i++){
					if( sd->status.inventory[i].nameid &&	// 該当個所の装備を精錬する
					    (sd->status.inventory[i].equip==i1) ){
						if(i2<=0 || i2>10 ) i2=1;
						if( sd->status.inventory[i].refine<10 ){
							sd->status.inventory[i].refine+=i2;
							if(sd->status.inventory[i].refine>10)
								sd->status.inventory[i].refine=10;
							pc_unequipitem(sd,i);	// 装備解除
							clif_refine(fd,sd,0,i,sd->status.inventory[i].refine);	// 精錬エフェクト とりあえず必ず成功
							clif_delitem(sd,i,1);	// 精錬前アイテム消失
							clif_additem(sd,i,1,0);	// 精錬後アイテム所得
							pc_equipitem(sd,i,i1);	// 再装備（含ステータス計算）
							clif_misceffect(&sd->bl,3); // 他人にも成功を通知
						}
					}
				}
			}
			return 1;
		}
// 武器製造 @produce アイテムIDor名前 属性 星のかけらの数
// 属性 0=無し 1=水 2=地 3=炎 4=風
// 別に属性＋星のかけら３個でもつかえます。
		if(strcmpi(command,"@produce")==0 && gm_level >= atcommand_config.produce){
			i1=i2=i3=0;
			if(sscanf(message,"%s%s%d%d",command,temp0,&i2,&i3) >= 2) {
				if( (i1=atoi(temp0))==0){
					struct item_data *item_data=itemdb_searchname(temp0);
					if(item_data)
						i1=item_data->nameid;
				}
				if( (i1<=500 || i1>1099) && (i1<4001 || i1>4148) &&
					(i1<7001 || i1>10019) && itemdb_isequip(i1)){
					struct item tmp_item;
					if(i2<0 || i2>4) i2=0;	// 属性範囲
					if(i3<0 || i3>3 ) i3=0;	// 星のかけら個数
					memset(&tmp_item,0,sizeof(tmp_item));
					tmp_item.nameid=i1;
					tmp_item.amount=1;
					tmp_item.identify=1;
					tmp_item.card[0]=0x00ff; // 製造武器フラグ
					tmp_item.card[1]=((i3*5)<<8)+i2;	// 属性とつよさ
					*((unsigned long *)(&tmp_item.card[2]))=sd->char_id;	// キャラID
					clif_produceeffect(sd,0,i1);// 製造エフェクトパケット
					clif_misceffect(&sd->bl,3); // 他人にも成功を通知（精錬成功エフェクトと同じでいいの？）
					if((flag = pc_additem(sd,&tmp_item,1)))
						clif_additem(sd,0,0,flag);
				}else
					printf("@produce NOT WEAPON [%d]\n",i1);
			}
			return 1;
		}
// 任意位置メモ @memo 番号 番号は0~2
		if(strcmpi(command,"@memo")==0 && gm_level >= atcommand_config.memo){
			i1=0;
			sscanf(message,"%s%d",command,&i1);
			if(i1<0 || i1>2) i1=0;
			pc_memo(sd,i1);
			return 1;
		}

// デバグ用（周辺gatを調べる）
		if(strcmpi(command,"@gat")==0 && gm_level >= atcommand_config.gat){
			for(y=2;y>=-2;y--){
				sprintf(moji,"%s (x= %d, y= %d) %02X %02X %02X %02X %02X",map[sd->bl.m].name,sd->bl.x-2,sd->bl.y+y,
					map_getcell(sd->bl.m,sd->bl.x-2,sd->bl.y+y),map_getcell(sd->bl.m,sd->bl.x-1,sd->bl.y+y),
					map_getcell(sd->bl.m,sd->bl.x,sd->bl.y+y),map_getcell(sd->bl.m,sd->bl.x+1,sd->bl.y+y),
					map_getcell(sd->bl.m,sd->bl.x+2,sd->bl.y+y));
				clif_displaymessage(fd,moji);
			}
			return 1;
		}

// デバグ用（パケット色々）
		if(strcmpi(command,"@packet")==0 && gm_level >= atcommand_config.packet){
			sscanf(message,"%s %d %d",command,&x,&y);
			clif_status_change(&sd->bl,x,y);
			return 1;
		}

// ステータスポイント調整
		if(strcmpi(command,"@stpoint")==0 && gm_level >= atcommand_config.stpoint) {
			sscanf(message,"%s%d",command,&i1);
			if(i1>0 || sd->status.status_point+i1>=0){
				sd->status.status_point += i1;
				clif_updatestatus(sd,SP_STATUSPOINT);
				clif_displaymessage(fd,"Status Point(s) given.");
			}else
				clif_displaymessage(fd,"Invalid number.");
			return 1;
		}

// スキルポイント調整
		if(strcmpi(command,"@skpoint")==0 && gm_level >= atcommand_config.skpoint) {
			sscanf(message,"%s%d",command,&i1);
			if(i1>0 || sd->status.skill_point+i1>=0){
				sd->status.skill_point += i1;
				clif_updatestatus(sd,SP_SKILLPOINT);
				clif_displaymessage(fd,"Skill Point(s) given.");
			}else
				clif_displaymessage(fd,"Invalid number.");
			return 1;
		}

// ゼニーアップ調整
		if(strcmpi(command,"@zeny")==0 && gm_level >= atcommand_config.zeny) {
			sscanf(message,"%s%d",command,&i1);
			if(i1>0 || sd->status.zeny+i1>=0){
				sd->status.zeny += i1;
				clif_updatestatus(sd,SP_ZENY);
				clif_displaymessage(fd,"Zeny given.");
			}else
				clif_displaymessage(fd,"Invalid number.");
			return 1;
		}

// 基本パラメータ調整
		if(	( (i2=0,strcmpi(command,"@str")==0) ||
			(i2=1,strcmpi(command,"@agi")==0) ||
			(i2=2,strcmpi(command,"@vit")==0) ||
			(i2=3,strcmpi(command,"@int")==0) ||
			(i2=4,strcmpi(command,"@dex")==0) ||
			(i2=5,strcmpi(command,"@luk")==0) ) && gm_level >= atcommand_config.param){
			unsigned char *p[]={
				&sd->status.str,&sd->status.agi,&sd->status.vit,
				&sd->status.int_,&sd->status.dex,&sd->status.luk
			};

			sscanf(message,"%s%d",command,&i1);
			
			i=(int)(*p[i2])+i1;
			if(i< 1)i1=1-*p[i2];
			if(i>99)i1=99-*p[i2];
			*p[i2]+=i1;
			clif_updatestatus(sd,SP_STR+i2);
			clif_updatestatus(sd,SP_USTR+i2);
			pc_calcstatus(sd,0);
			clif_displaymessage(fd,"Stat changed.");
			return 1;
		}

// ギルドLv調整
		if(strcmpi(command,"@guildlvup")==0 && gm_level >= atcommand_config.guildlvup) {
			struct guild *g;
			sscanf(message,"%s%d",command,&i1);
			if( sd->status.guild_id<=0 || (g=guild_search(sd->status.guild_id))==NULL){
				clif_displaymessage(fd,"You're not in a guild.");
				return 1;
			}
			if( strcmp(sd->status.name,g->master)!=0 ){
				clif_displaymessage(fd,"You're not the master of your guild.");
				return 1;
			}
			
			if( g->guild_lv+i1>=1 && g->guild_lv+i1<=50){
				intif_guild_change_basicinfo(g->guild_id,GBI_GUILDLV,&i1,2);
				clif_displaymessage(fd,"Guild Level changed.");
			}else
				clif_displaymessage(fd,"Guild Level change failed.");
			return 1;
		}

// ペット作成
		if(strcmpi(command,"@makepet")==0 && gm_level >= atcommand_config.makepet) {
			sscanf(message,"%s%d",command,&i1);
			x = search_petDB_index(i1,PET_CLASS);
			if(x < 0)
				x = search_petDB_index(i1,PET_EGG);
			if(x >= 0) {
				sd->catch_target_class = pet_db[x].class;
				intif_create_pet(sd->status.account_id,sd->status.char_id,pet_db[x].class,mob_db[pet_db[x].class].lv,
					pet_db[x].EggID,0,pet_db[x].intimate,100,0,1,pet_db[x].jname);
			}
			return 1;
		}

// ペット親密度変更
		if(strcmpi(command,"@petfriendly")==0 && gm_level >= atcommand_config.petfriendly) {
			sscanf(message,"%s%d",command,&i1);
			if(sd->status.pet_id && sd->pet_npcdata) {
				if(i1 >= 0 && i1 <= 1000) {
					sd->pet.intimate = i1;
					clif_send_petstatus(sd);
					clif_displaymessage(fd,"Pet friendship level changed.");
				}
			}
			return 1;
		}

// ペット満腹度変更
		if(strcmpi(command,"@pethungry")==0 && gm_level >= atcommand_config.pethungry) {
			sscanf(message,"%s%d",command,&i1);
			if(sd->status.pet_id && sd->pet_npcdata) {
				if(i1 >= 0 && i1 <= 100) {
					sd->pet.hungry = i1;
					clif_send_petstatus(sd);
					clif_displaymessage(fd,"Pet hunger level changed.");
				}
			}
			return 1;
		}

// ペット名前変更
		if(strcmpi(command,"@petrename")==0 && gm_level >= atcommand_config.petrename) {
			if(sd->status.pet_id && sd->pet_npcdata) {
				sd->pet.rename_flag = 0;
				intif_save_petdata(sd->status.account_id,&sd->pet);
				clif_send_petstatus(sd);
				clif_displaymessage(fd,"Pet renaming enabled.");
			}
			return 1;
		}

//-Begin- Commands Added by Syrus22
//September 17, 2003
//@recall <char name>
//Description: Warps target character to you.
		if(strcmpi(command, "@recall") == 0 && gm_level >= atcommand_config.recall){
			sscanf(message, "%s %[^\n]", command, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				if(gm_level > pc_isGM(pl_sd)) {
					pc_setpos(pl_sd, sd->mapname, sd->bl.x, sd->bl.y, 2);
					sprintf(moji,"%s was recalled.",temp1);
					clif_displaymessage(fd,moji);
				}
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
			return 1;
		}
//September 17, 2003
//@charjob <job ID> <char name>
//Description: Changes target characters job.
		if(strcmpi(command, "@charjob") == 0 && gm_level >= atcommand_config.charjob){
			sscanf(message, "%s%d %[^\n]", command, &x, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				if(gm_level > pc_isGM(pl_sd)) {
					if ((x >= 0 && x < MAX_PC_CLASS)) {
						pc_jobchange(pl_sd,x);
						clif_displaymessage(fd,"Character's Job changed.");
					}else{
						clif_displaymessage(fd,"Invalid Job ID.");
					}
				}
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
		return 1;
		}
//September 17, 2003
//@revive <char name>
//Description: Revives target character.
		if (strcmpi(command, "@revive") == 0 && gm_level >= atcommand_config.revive) {
			sscanf(message, "%s %[^\n]", command, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				pl_sd->status.hp=pl_sd->status.max_hp;
				pc_setstand(pl_sd);
				if(battle_config.ghost_time > 0)
					pc_setghosttimer(sd,battle_config.ghost_time);
				clif_updatestatus(pl_sd,SP_HP);
				clif_resurrection(&pl_sd->bl,1);
				clif_displaymessage(fd,"Character revived.");
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
			return 1;
		}
//September 18, 2003
//@charstats <charname>
//Description: Displays a characters stats.
		if (strcmpi(command, "@charstats") == 0 && gm_level >= atcommand_config.charstats) {
			sscanf(message, "%s %[^\n]", command, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				sprintf(moji,"%s Character Statistics", pl_sd->status.name);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Base Level: %d", pl_sd->status.base_level);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Job Level: %d", pl_sd->status.job_level);
				clif_displaymessage(fd,moji);
				sprintf(moji,"HP: %d", pl_sd->status.hp);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Max HP: %d", pl_sd->status.max_hp);
				clif_displaymessage(fd,moji);
				sprintf(moji,"SP: %d", pl_sd->status.sp);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Max SP: %d", pl_sd->status.max_sp);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Strength: %d", pl_sd->status.str);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Agility: %d", pl_sd->status.agi);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Vitality: %d", pl_sd->status.vit);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Intelligence: %d", pl_sd->status.int_);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Dexterity: %d", pl_sd->status.dex);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Luck: %d", pl_sd->status.luk);
				clif_displaymessage(fd,moji);
				sprintf(moji,"Zeny: %d", pl_sd->status.zeny);
				clif_displaymessage(fd,moji);
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
			return 1;
		}
//September 18, 2003
//@charoption <param1> <param2> <param3> <charname>
//Description: Does the same as the @option command only to target character.
		if (strcmpi(command, "@charoption") == 0 && gm_level >= atcommand_config.charoption) {
			sscanf(message, "%s%d%d%d %[^\n]", command, &x, &y, &z, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				if(gm_level > pc_isGM(pl_sd)) {
				/*
			   	p->option は次のようなbitにおいて成り立っている？
			   	pram1        0000 0000 0000 0000
			   	pram2        0000 0000 0000 0000
			   	pram3        0000 0000 0000 0000 

			 	*/
					pl_sd->opt1=x;
					pl_sd->opt2=y;
					pl_sd->status.option=z;
					clif_changeoption(&pl_sd->bl);
					clif_displaymessage(fd,"Character's options changed.");
				}
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
			return 1;
		}
//September 20, 2003
//@charsave <map> <x> <y> <charname>
//Description: Changes the target players respawn point.
		if (strcmpi(command, "@charsave") == 0 && gm_level >= atcommand_config.charsave) {
			sscanf(message, "%s%s%d%d %[^\n]", command, temp0, &x, &y, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				if(gm_level > pc_isGM(pl_sd)) {
					pc_setsavepoint(pl_sd,temp0,x,y);
					clif_displaymessage(fd,"Character's respawn point changed.");
				}
			}else{
				clif_displaymessage(fd,"Character not found.");
			}
			return 1;
		}
//September 20, 2003
//@night
//Description: Uses @option 00 16 00 on all characters.
		if(strcmpi(command, "@night") == 0 && gm_level >= atcommand_config.night){
			sscanf(message, "%s", command);
			for(i=0;i<fd_max;i++){
				if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
					pl_sd->opt2 |= 0x10;
					clif_changeoption(&pl_sd->bl);
					clif_displaymessage(pl_sd->fd,"Night has fallen.");
				}
			}
			return 1;
		}
//September 20, 2003
//@day
//Description: Uses @option 00 00 00 on all characters.
			if(strcmpi(command, "@day") == 0 && gm_level >= atcommand_config.day){
				sscanf(message, "%s", command);
			for(i=0;i<fd_max;i++){
				if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
					pl_sd->opt2 &= !0x10;
					clif_changeoption(&pl_sd->bl);
					clif_displaymessage(pl_sd->fd,"Day has arrived.");
				}
			}
			return 1;
		}
//September 20, 2003
//Edited September 24, 2003
//@doom
//Description: Kills all NON GM chars on the server.
	if(strcmpi(command, "@doom") == 0 && gm_level >= atcommand_config.doom){
		sscanf(message, "%s", command);
		for(i=0;i<fd_max;i++){
			if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
				if(gm_level > pc_isGM(pl_sd)) {
					pc_damage(NULL,pl_sd,pl_sd->status.hp+1);
					clif_displaymessage(pl_sd->fd,"The holy messenger has given judgement.");
				}
			}
		}
		clif_displaymessage(fd,"Judgement was made.");
		return 1;
	}
//October 6, 2003
//@doommap
//Description: Kills all non GM characters on the map.
	if(strcmpi(command, "@doommap") == 0 && gm_level >= atcommand_config.doommap){
		sscanf(message, "%s", command);				
		for(i=0;i<fd_max;i++){
			if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
				if(sd->bl.m == pl_sd->bl.m){
					if(gm_level > pc_isGM(pl_sd)) {
						pc_damage(NULL,pl_sd,pl_sd->status.hp+1);
						clif_displaymessage(pl_sd->fd,"The holy messenger has given judgement.");
					}
				}
			}
		}
		clif_displaymessage(fd,"Judgement was made on the map.");
		return 1;
	}
//September 21, 2003
//Edited September 24, 2003
//@raise
//Description: Resurrects all characters on the server.
	if(strcmpi(command, "@raise") == 0 && gm_level >= atcommand_config.raise){
		sscanf(message, "%s", command);
		for(i=0;i<fd_max;i++){
			if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth
				 && pc_isdead(pl_sd)){
				pl_sd->status.hp=pl_sd->status.max_hp;
				pc_setstand(pl_sd);
				clif_updatestatus(pl_sd,SP_HP);
				clif_resurrection(&pl_sd->bl,1);
				clif_displaymessage(pl_sd->fd,"You've been granted mercy.");
			}
		}
		clif_displaymessage(fd,"Mercy has been granted.");
		return 1;
	}
//October 6, 2003
//@raisemap
//Description: Resurrects all characters on the map.
	if(strcmpi(command, "@raisemap") == 0 && gm_level >= atcommand_config.raisemap){
		sscanf(message, "%s", command);
		for(i=0;i<fd_max;i++){
			if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
				if(sd->bl.m == pl_sd->bl.m && pc_isdead(pl_sd)){
					pl_sd->status.hp=pl_sd->status.max_hp;
					pc_setstand(pl_sd);
					clif_updatestatus(pl_sd,SP_HP);
					clif_resurrection(&pl_sd->bl,1);
					clif_displaymessage(pl_sd->fd,"You've been granted mercy.");
				}
			}
		}
		clif_displaymessage(fd,"Mercy was granted on the map.");
		return 1;
	}
//@charbaselvl <#> <nickname>
//Description: Change a characters base level.
	if ((strcmpi(command, "@charbaselvl") == 0 || strcmpi(command, "@charlvl") == 0) && gm_level >= atcommand_config.charbaselvl) {
		sscanf(message, "%s%d %[^\n]", command, &x, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				if(gm_level > pc_isGM(pl_sd)) {
					if(x >= 1){
						for(i=pl_sd->status.base_level+1;i<=pl_sd->status.base_level+x;i++){
							if(i>=99)
								pl_sd->status.status_point += 22;
							else
								pl_sd->status.status_point += (i+14) / 5 ;
						}
						pl_sd->status.base_level+=x;
						clif_updatestatus(pl_sd,SP_BASELEVEL);
						clif_updatestatus(pl_sd,SP_NEXTBASEEXP);
						clif_updatestatus(pl_sd,SP_STATUSPOINT);
						pc_calcstatus(pl_sd,0);
						pc_heal(pl_sd,pl_sd->status.max_hp,pl_sd->status.max_sp);
						clif_misceffect(&pl_sd->bl,0);
						clif_displaymessage(fd,"Character's Base Level up.");
					}else if(x<0 && pl_sd->status.base_level+x>0){
						pl_sd->status.base_level+=x;
						clif_updatestatus(pl_sd,SP_BASELEVEL);
						clif_updatestatus(pl_sd,SP_NEXTBASEEXP);
						pc_calcstatus(pl_sd,0);
						clif_displaymessage(fd,"Character's Base Level down.");
					}
				}
			}
			return 1;
		}
//@charjlvl
		y = 50;
	if(strcmpi(command, "@charjlvl") == 0 && gm_level >= atcommand_config.charjlvl){
		sscanf(message, "%s%d %[^\n]", command, &x, temp1);
			if ((pl_sd=map_nick2sd(temp1))!=NULL) {
				if(gm_level > pc_isGM(pl_sd)) {
					if(pl_sd->status.class == 0)
						y -= 40;
					if(pl_sd->status.job_level == y){
						clif_displaymessage(fd,"Character's Job Level can't go any higher.");
					}else if(x >= 1){
						if(pl_sd->status.job_level + x > y)
							x = y - pl_sd->status.job_level;
						pl_sd->status.job_level+=x;
						clif_updatestatus(pl_sd,SP_JOBLEVEL);
						clif_updatestatus(pl_sd,SP_NEXTJOBEXP);
						pl_sd->status.skill_point+=x;
						clif_updatestatus(pl_sd,SP_SKILLPOINT);
						pc_calcstatus(pl_sd,0);
						clif_misceffect(&pl_sd->bl,1);
						clif_displaymessage(fd,"Character's Job Level up.");
					}else if(x<0 && sd->status.job_level+x>0 ){
						pl_sd->status.job_level+=x;
						clif_updatestatus(pl_sd,SP_JOBLEVEL);
						clif_updatestatus(pl_sd,SP_NEXTJOBEXP);
						pc_calcstatus(pl_sd,0);
						clif_displaymessage(fd,"Character's Job Level down.");
					}
				}
			}
			return 1;
		}
//-End- Commands added by Syrus22

		if(strcmpi(command, "@kick") == 0 && gm_level >= atcommand_config.kick){
			sscanf(message, "%s %[^\n]", command, temp1);
			if((pl_sd=map_nick2sd(temp1))!=NULL && gm_level > pc_isGM(pl_sd))
				clif_GM_kick(sd,pl_sd,1);
			else
				clif_GM_kickack(sd,0);
			return 1;
		}

		if(strcmpi(command, "@kickall") == 0 && gm_level >= atcommand_config.kickall){
			for(i=0;i<fd_max;i++){
				if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
					if(sd->status.account_id != pl_sd->status.account_id)
						clif_GM_kick(sd,pl_sd,0);
				}
			}
			clif_GM_kick(sd,sd,0);
			return 1;
		}

		if(strcmpi(command, "@questskill") == 0 && gm_level >= atcommand_config.questskill){
			sscanf(message, "%s %d", command, &x);
			if(skill_get_inf2(x)&0x01){
				pc_skill(sd,x,1,0);
				clif_displaymessage(fd,"You have learned the skill.");
			}
			return 1;
		}

		if(strcmpi(command, "@lostskill") == 0 && gm_level >= atcommand_config.lostskill){
			sscanf(message, "%s %d", command, &x);
			if(x>0 && x<MAX_SKILL && pc_checkskill(sd,x)>0){
				sd->status.skill[x].lv=0;
				sd->status.skill[x].flag=0;
				clif_skillinfoblock(sd);
				clif_displaymessage(fd,"You have forgotten the skill.");
			}
			return 1;
		}
		if(strcmpi(command, "@spiritball") == 0 && gm_level >= atcommand_config.spiritball){
			sscanf(message, "%s %d", command, &x);
			if(x>=0 && x<=0x7FFF){
				if(sd->spiritball > 0)
					pc_delspiritball(sd,sd->spiritball,1);
				sd->spiritball = x;
				clif_spiritball(sd);
			}
			return 1;
		}

		if(strcmpi(command, "@party") == 0 && gm_level >= atcommand_config.party){
			if(sscanf(message, "%s %s", command, moji) >= 2) {
				if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 7){
					party_create(sd,moji);
				}
				else
					clif_skill_fail(sd,1,0,4);
			}
			return 1;
		}

	}

	return 0;
}

int atcommand_config_read(const char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;

	memset(&atcommand_config,0,sizeof(atcommand_config));
	atcommand_config.kickall = 99;

	if(battle_config.atc_gmonly > 0) {
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
				{ "broadcast",&atcommand_config.broadcast },
				{ "local_broadcast",&atcommand_config.local_broadcast },
				{ "mapmove",&atcommand_config.mapmove },
				{ "resetstate",&atcommand_config.resetstate },
				{ "rura+",&atcommand_config.rurap },
				{ "rura",&atcommand_config.rura },
				{ "where",&atcommand_config.where },
				{ "jumpto",&atcommand_config.jumpto },
				{ "jump",&atcommand_config.jump },
				{ "who",&atcommand_config.who },
				{ "save",&atcommand_config.save },
				{ "load",&atcommand_config.load },
				{ "speed",&atcommand_config.speed },
				{ "storage",&atcommand_config.storage },
				{ "option",&atcommand_config.option },
				{ "hide",&atcommand_config.hide },
				{ "jobchange",&atcommand_config.jobchange },
				{ "die",&atcommand_config.die },
				{ "kill",&atcommand_config.kill },
				{ "alive",&atcommand_config.alive },
				{ "kami",&atcommand_config.kami },
				{ "heal",&atcommand_config.heal },
				{ "item",&atcommand_config.item },
				{ "itemreset",&atcommand_config.itemreset },
				{ "lvup",&atcommand_config.lvup },
				{ "joblvup",&atcommand_config.joblvup },
				{ "help",&atcommand_config.help },
				{ "GM",&atcommand_config.gm },
				{ "pvpoff",&atcommand_config.pvpoff },
				{ "pvpon",&atcommand_config.pvpon },
				{ "gvgoff",&atcommand_config.gvgoff },
				{ "gvgon",&atcommand_config.gvgon },
				{ "model",&atcommand_config.model },
				{ "go",&atcommand_config.go },
				{ "monster",&atcommand_config.monster },
				{ "refine",&atcommand_config.refine },
				{ "produce",&atcommand_config.produce },
				{ "memo",&atcommand_config.memo },
				{ "gat",&atcommand_config.gat },
				{ "packet",&atcommand_config.packet },
				{ "stpoint",&atcommand_config.stpoint },
				{ "skpoint",&atcommand_config.skpoint },
				{ "zeny",&atcommand_config.zeny },
				{ "param",&atcommand_config.param },
				{ "guildlvup",&atcommand_config.guildlvup },
				{ "makepet",&atcommand_config.makepet },
				{ "petfriendly",&atcommand_config.petfriendly },
				{ "pethungry",&atcommand_config.pethungry },
				{ "petrename",&atcommand_config.petrename },
				{ "recall",&atcommand_config.recall },
				{ "charjob",&atcommand_config.charjob },
				{ "revive",&atcommand_config.revive },
				{ "charstats",&atcommand_config.charstats },
				{ "charoption",&atcommand_config.charoption },
				{ "charsave",&atcommand_config.charsave },
				{ "charload",&atcommand_config.charload },
				{ "night",&atcommand_config.night },
				{ "day",&atcommand_config.day },
				{ "doom",&atcommand_config.doom },
				{ "doommap",&atcommand_config.doommap },
				{ "raise",&atcommand_config.raise },
				{ "raisemap",&atcommand_config.raisemap },
				{ "charbaselvl",&atcommand_config.charbaselvl },
				{ "charjlvl",&atcommand_config.charjlvl },
				{ "kick",&atcommand_config.kick },
				{ "questskill",&atcommand_config.questskill },
				{ "lostskill",&atcommand_config.lostskill },
				{ "spiritball",&atcommand_config.spiritball },
				{ "party",&atcommand_config.party },
			};
		
			if(line[0] == '/' && line[1] == '/')
				continue;
			i=sscanf(line,"%[^:]:%s",w1,w2);
			if(i!=2)
				continue;
			for(i=0;i<sizeof(data)/(sizeof(data[0]));i++)
				if(strcmpi(w1,data[i].str)==0)
					*data[i].val=atoi(w2);
		}
		fclose(fp);
	}

	return 0;
}
