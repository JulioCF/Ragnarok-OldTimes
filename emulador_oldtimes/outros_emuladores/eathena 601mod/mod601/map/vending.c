#include <string.h>

#include "clif.h"
#include "itemdb.h"
#include "map.h"
#include "vending.h"
#include "pc.h"

/*==========================================
 * ˜I“X•Â½
 *------------------------------------------
*/
void vending_closevending(struct map_session_data *sd)
{
	sd->vender_id=0;
	clif_closevendingboard(&sd->bl,0);
}

/*==========================================
 * ˜I“XƒAƒCƒeƒ€ƒŠƒXƒg—v‹
 *------------------------------------------
 */
void vending_vendinglistreq(struct map_session_data *sd,int id)
{
	struct map_session_data *vsd=map_id2sd(id);

	if(vsd==NULL)
		return;
	if(vsd->vender_id==0)
		return;
	clif_vendinglist(sd,id,vsd->vending);
}

/*==========================================
 * ˜I“XƒAƒCƒeƒ€w“ü
 *------------------------------------------
 */
void vending_purchasereq(struct map_session_data *sd,int len,int id,unsigned char *p)
{
	int i,j,w,z,new=0,blank=pc_inventoryblank(sd),vend_list[12];
	short amount,index;
	struct map_session_data *vsd=map_id2sd(id);

	if(vsd==NULL)
		return;
	if(vsd->vender_id==0)
		return;
	for(i=0,w=z=0;8+4*i<len;i++){
		amount=*(short*)(p+4*i);
		index=*(short*)(p+2+4*i)-2;
		for(j=0;j<vsd->vend_num;j++)
			if(0<vsd->vending[j].amount && amount<=vsd->vending[j].amount && vsd->vending[j].index==index)
				break;
		if(j==vsd->vend_num)
			return;	// ”„‚èØ‚ê
		vend_list[i]=j;
		z+=vsd->vending[j].value*amount;
		if(z > sd->status.zeny){
			clif_buyvending(sd,index,amount,1);
			return;	// zeny•s‘«
		}
		w+=itemdb_weight(vsd->status.cart[index].nameid)*amount;
		if(w+sd->weight > sd->max_weight){
			clif_buyvending(sd,index,amount,2);
			return;	// d—Ê’´‰ß
		}
		switch(pc_checkadditem(sd,vsd->status.cart[index].nameid,amount)){
		case ADDITEM_EXIST:
			break;
		case ADDITEM_NEW:
			new++;
			if(new > blank)
				return;	// í—Ş”’´‰ß
			break;
		case ADDITEM_OVERAMOUNT:
			return;	// ƒAƒCƒeƒ€”’´‰ß
		}
	}
	pc_payzeny(sd,z);
	pc_getzeny(vsd,z);
	for(i=0;8+4*i<len;i++){
		amount=*(short*)(p+4*i);
		index=*(short*)(p+2+4*i)-2;
		pc_additem(sd,&vsd->status.cart[index],amount);
		vsd->vending[vend_list[i]].amount-=amount;
		pc_cart_delitem(vsd,index,amount);
		clif_vendingreport(vsd,index,amount);
	}
}

/*==========================================
 * ˜I“XŠJİ
 *------------------------------------------
 */
void vending_openvending(struct map_session_data *sd,int len,char *message,int flag,unsigned char *p)
{
	int i;
	if(flag){
		sd->vender_id=sd->bl.id;
		for(i=0;85+8*i<len;i++){
			sd->vending[i].index=*(short*)(p+8*i)-2;
			sd->vending[i].amount=*(short*)(p+2+8*i);
			sd->vending[i].value=*(int*)(p+4+8*i);
		}
		sd->vend_num=i;
		strcpy(sd->message,message);
		clif_openvending(sd,sd->vender_id,sd->vending);
		clif_showvendingboard(&sd->bl,message,0);
	}
}

