<?
include "readfile";
echo '<center>';

$scriptc = str_replace('int buildin_menu(struct script_state *st);','int buildin_menu(struct script_state *st);
int buildin_menu2(struct script_state *st);',$scriptc);

$scriptc = str_replace('{buildin_menu,"menu","*"},','{buildin_menu,"menu","*"},
	{buildin_menu2,"menu2","*"}, //by Mehah',$scriptc);

$scriptc = str_replace('{buildin_mes,"mes","s"},','{buildin_mes,"show","s"},',$scriptc);


$scriptc = str_replace('{buildin_jobchange,"jobchange","i*"},','{buildin_jobchange,"setjob","i*"},',$scriptc);

$scriptc = str_replace('{buildin_changebase,"changebase","i"}','{buildin_changebase,"setbase","i"}',$scriptc);

$scriptc = str_replace('{buildin_changesex,"changesex",""}','{buildin_changesex,"setsex",""}',$scriptc);

$scriptc = str_replace('{buildin_input,"input","*"}','{buildin_input,"digit","*"}',$scriptc);

$scriptc = str_replace('{buildin_getitem,"getitem","ii**"}','{buildin_getitem,"additem","ii**"}',$scriptc);

$scriptc = str_replace('{buildin_getitem2,"getitem2","iiiiiiiii*"}','{buildin_getitem2,"additem2","iiiiiiiii*"}',$scriptc);

$scriptc = str_replace('/*==========================================
 *
 *------------------------------------------
 */
int buildin_rand(struct script_state *st)','/*==========================================
 *
 *------------------------------------------
 */
int buildin_menu2(struct script_state *st)
{
	char *buf;
	int len,i;
	struct map_session_data *sd;

	sd=script_rid2sd(st);

	if(sd->state.menu_or_input==0){
		st->state=RERUNLINE;
		sd->state.menu_or_input=1;
		for(i=st->start+2,len=16;i<st->end;i+=2){
			conv_str(st,& (st->stack->stack_data[i]));
			len+=(int)strlen(st->stack->stack_data[i].u.str)+1;
		}
		buf=(char *)aCallocA(len+1,sizeof(char));
		buf[0]=0;
		for(i=st->start+2,len=0;i<st->end;i+=2){
			strcat(buf,st->stack->stack_data[i].u.str);
			strcat(buf,":");
		}
		clif_scriptmenu(script_rid2sd(st),st->oid,buf);
		aFree(buf);
	} else {
		pc_setreg(sd,add_str((unsigned char *) "@menu"),sd->npc_menu);
		sd->state.menu_or_input=0;
		if(sd->npc_menu>0 && sd->npc_menu<(st->end-st->start)/2){
			int pos;
			if( st->stack->stack_data[st->start+sd->npc_menu*2+1].type!=C_POS ){
				ShowError("script: menu: not label !\n");
				st->state=END;
				return 0;
			}
			pos=conv_num(st,& (st->stack->stack_data[st->start+sd->npc_menu*2+1]));
			st->pos=pos;
			st->state=GOTO;
		}
	}
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_rand(struct script_state *st)',$scriptc);

$fp = fopen($scriptc2, 'w');
if (!fwrite($fp, $scriptc)) {
echo "<font color=\"red\">O arquivo script.c não foi convertido</font>";
}
echo "<font color=\"green\">O arquivo script.c foi convertido</font>";
fclose($fp);

echo '</center>';
?>