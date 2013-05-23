#ifndef _SCRIPT_H_
#define _SCRIPT_H_

struct script_data {
	int type;
	union {
		int num;
		char *str;
	} u;
};

struct script_stack {
	int sp,sp_max;
	struct script_data *stack_data;
};
struct script_state {
	struct script_stack *stack;
	int start,end;
	int pos,state;
	int rid,oid;
};

#define SCRIPT_CONF_NAME	"conf/script_athena.conf"

unsigned char * parse_script(unsigned char *,int);
int run_script(unsigned char *,int,int,int);

struct dbt* script_get_label_db();

int script_config_read(char *cfgName);
int do_init_script();

#endif

