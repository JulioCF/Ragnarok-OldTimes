#ifndef _NPC_H_
#define _NPC_H_

#define START_NPC_NUM 110000000

int npc_event_dequeue(struct map_session_data *sd);
int npc_event_timer(int tid,unsigned int tick,int id,int data);
int npc_event(struct map_session_data *sd,const char *npcname);
int npc_touch_areanpc(struct map_session_data *,int,int,int);
int npc_click(struct map_session_data *,int);
int npc_scriptcont(struct map_session_data *,int);
int npc_checknear(struct map_session_data *,int);
int npc_buysellsel(struct map_session_data *,int,int);
int npc_buylist(struct map_session_data *,int,unsigned short *);
int npc_selllist(struct map_session_data *,int,unsigned short *);
int npc_parse_mob(char *w1,char *w2,char *w3,char *w4);

int npc_enable(const char *name,int flag);

int npc_get_new_npc_id(void);

void npc_addsrcfile(char *);
int do_init_npc(void);
int npc_event_do_oninit(void);
int npc_do_ontimer(int,struct map_session_data *,int);


#endif

