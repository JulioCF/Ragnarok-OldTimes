#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "mmo.h"
/*#define MAX_STORAGE 100
struct storage {
	int account_id;
	short storage_status;
	short storage_amount;
	struct item storage[MAX_STORAGE];
};*/
int storage_storageopen(struct map_session_data *sd);
int storage_storageadd(struct map_session_data *sd,int index,int amount);
int storage_storageget(struct map_session_data *sd,int index,int amount);
int storage_storageaddfromcart(struct map_session_data *sd,int index,int amount);
int storage_storagegettocart(struct map_session_data *sd,int index,int amount);
int storage_storageclose(struct map_session_data *sd);
int do_init_storage(void);
void do_final_storage(void);
int account2storage(int account_id);
int storage_storage_quitsave(struct map_session_data *sd);

extern struct storage *storage;

#endif
