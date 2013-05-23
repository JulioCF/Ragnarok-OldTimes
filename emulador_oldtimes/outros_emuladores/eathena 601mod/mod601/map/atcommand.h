//atcommand.h
#ifndef _ATCOMMAND_H_
#define _ATCOMMAND_H_

int atcommand(int fd,struct map_session_data *sd,char *message);

extern struct Atcommand_Config {
	int broadcast;
	int local_broadcast;
	int mapmove;
	int resetstate;
	int rurap;
	int rura;
	int where;
	int jumpto;
	int jump;
	int who;
	int save;
	int load;
	int speed;
	int storage;
	int option;
	int hide;
	int jobchange;
	int die;
	int kill;
	int alive;
	int kami;
	int heal;
	int item;
	int itemreset;
	int lvup;
	int joblvup;
	int help;
	int gm;
	int pvpoff;
	int pvpon;
	int gvgoff;
	int gvgon;
	int model;
	int go;
	int monster;
	int refine;
	int produce;
	int memo;
	int gat;
	int packet;
	int stpoint;
	int skpoint;
	int zeny;
	int param;
	int guildlvup;
	int makepet;
	int petfriendly;
	int pethungry;
	int petrename;
	int recall;
	int charjob;
	int revive;
	int charstats;
	int charoption;
	int charsave;
	int charload;
	int night;
	int day;
	int doom;
	int doommap;
	int raise;
	int raisemap;
	int charbaselvl;
	int charjlvl;
	int kick;
	int kickall;
	int questskill;
	int lostskill;
	int spiritball;
	int party;
} atcommand_config;

#define ATCOMMAND_CONF_FILENAME	"conf/atcommand_athena.conf"
int atcommand_config_read(const char *cfgName);

#endif
