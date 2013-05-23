unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, ComCtrls, TabNotBk, StdCtrls, ArqCfg, Grids, ValEdit, ShellApi;

type
  TForm1 = class(TForm)
    TabbedNotebook1: TTabbedNotebook;
    GroupBox1: TGroupBox;
    Label1: TLabel;
    Edit1: TEdit;
    Label2: TLabel;
    Edit2: TEdit;
    Button3: TButton;
    Button4: TButton;
    CheckBox1: TCheckBox;
    CheckBox2: TCheckBox;
    CheckBox3: TCheckBox;
    Button5: TButton;
    GroupBox2: TGroupBox;
    Label3: TLabel;
    Edit3: TEdit;
    Label4: TLabel;
    Edit4: TEdit;
    Label5: TLabel;
    Edit5: TEdit;
    Label6: TLabel;
    Edit6: TEdit;
    Label7: TLabel;
    Edit7: TEdit;
    CheckBox4: TCheckBox;
    RadioButton1: TRadioButton;
    Label8: TLabel;
    RadioButton2: TRadioButton;
    RadioButton3: TRadioButton;
    RadioButton4: TRadioButton;
    Label9: TLabel;
    Edit8: TEdit;
    CheckBox5: TCheckBox;
    GroupBox3: TGroupBox;
    Label10: TLabel;
    Edit9: TEdit;
    Label11: TLabel;
    Edit10: TEdit;
    Label12: TLabel;
    Edit11: TEdit;
    Label13: TLabel;
    Edit12: TEdit;
    Label14: TLabel;
    Edit13: TEdit;
    GroupBox4: TGroupBox;
    GroupBox5: TGroupBox;
    Label15: TLabel;
    Edit14: TEdit;
    Label16: TLabel;
    Label17: TLabel;
    Edit15: TEdit;
    Edit16: TEdit;
    Label18: TLabel;
    Edit17: TEdit;
    Label19: TLabel;
    Edit18: TEdit;
    Label20: TLabel;
    Edit19: TEdit;
    Edit20: TEdit;
    Edit21: TEdit;
    Label21: TLabel;
    Edit22: TEdit;
    Edit23: TEdit;
    Label22: TLabel;
    Label23: TLabel;
    Label24: TLabel;
    Edit24: TEdit;
    Label25: TLabel;
    Edit25: TEdit;
    Label26: TLabel;
    Edit26: TEdit;
    CheckBox6: TCheckBox;
    GroupBox6: TGroupBox;
    Label27: TLabel;
    Edit27: TEdit;
    Label28: TLabel;
    Edit28: TEdit;
    Label29: TLabel;
    Edit29: TEdit;
    Label30: TLabel;
    Edit30: TEdit;
    Label31: TLabel;
    Edit31: TEdit;
    Label32: TLabel;
    Edit32: TEdit;
    GroupBox7: TGroupBox;
    Label33: TLabel;
    Label34: TLabel;
    Edit33: TEdit;
    Label35: TLabel;
    Label36: TLabel;
    Label37: TLabel;
    Edit34: TEdit;
    Label38: TLabel;
    Edit35: TEdit;
    Label39: TLabel;
    Edit36: TEdit;
    Label40: TLabel;
    Edit37: TEdit;
    CheckBox7: TCheckBox;
    Label41: TLabel;
    Edit38: TEdit;
    Label42: TLabel;
    Edit39: TEdit;
    Label43: TLabel;
    Edit40: TEdit;
    GroupBox8: TGroupBox;
    Label44: TLabel;
    Edit41: TEdit;
    Label45: TLabel;
    Edit42: TEdit;
    Label46: TLabel;
    Edit43: TEdit;
    Label47: TLabel;
    Edit44: TEdit;
    GroupBox9: TGroupBox;
    Label48: TLabel;
    Edit45: TEdit;
    Label49: TLabel;
    Edit46: TEdit;
    Label50: TLabel;
    Edit47: TEdit;
    Label51: TLabel;
    Edit48: TEdit;
    Label52: TLabel;
    Edit49: TEdit;
    Editor32: TEdit;
    Editor33: TEdit;
    Editor34: TEdit;
    GroupBox10: TGroupBox;
    Label53: TLabel;
    Edit50: TEdit;
    Edit51: TEdit;
    Label54: TLabel;
    Label55: TLabel;
    Label56: TLabel;
    Edit52: TEdit;
    Edit53: TEdit;
    RadioButton5: TRadioButton;
    RadioButton6: TRadioButton;
    RadioButton7: TRadioButton;
    Label57: TLabel;
    Label58: TLabel;
    Edit54: TEdit;
    Label59: TLabel;
    Edit55: TEdit;
    Label60: TLabel;
    Edit56: TEdit;
    Label61: TLabel;
    Edit57: TEdit;
    Label62: TLabel;
    Edit58: TEdit;
    Label63: TLabel;
    Label64: TLabel;
    Edit59: TEdit;
    Edit60: TEdit;
    GroupBox11: TGroupBox;
    Edit61: TEdit;
    Label65: TLabel;
    Label66: TLabel;
    Edit62: TEdit;
    Label67: TLabel;
    Edit63: TEdit;
    Label68: TLabel;
    Edit64: TEdit;
    Label69: TLabel;
    Edit65: TEdit;
    Label75: TLabel;
    GroupBox13: TGroupBox;
    Button1: TButton;
    Button2: TButton;
    Button6: TButton;
    Button7: TButton;
    Button8: TButton;
    Button9: TButton;
    Button10: TButton;
    Button11: TButton;
    Button12: TButton;
    Button13: TButton;
    Button14: TButton;
    Button15: TButton;
    Button16: TButton;
    procedure Button4Click(Sender: TObject);
    procedure Button5Click(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure Button3Click(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button6Click(Sender: TObject);
    procedure Button7Click(Sender: TObject);
    procedure Button8Click(Sender: TObject);
    procedure Button9Click(Sender: TObject);
    procedure Button10Click(Sender: TObject);
    procedure Button11Click(Sender: TObject);
    procedure Button12Click(Sender: TObject);
    procedure Button13Click(Sender: TObject);
    procedure Button14Click(Sender: TObject);
    procedure Button15Click(Sender: TObject);
    procedure Button16Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;
  a: ArquivoCfg;
  b: ArquivoCfg;
  c: ArquivoCfg;
  d: ArquivoCfg;
  e: ArquivoCfg;
  f: ArquivoCfg;
  GM_accountfile: String;

implementation

{$R *.dfm}

procedure TForm1.Button4Click(Sender: TObject);
begin
  Form1.Close;
end;

procedure TForm1.Button5Click(Sender: TObject);
begin
  if CheckBox1.Checked = True then
    if FileExists('logserv.bat') then
    WinExec('logserv.bat',1)
    else
    ShowMessage('"logserv.bat" não encontrado');

  if CheckBox2.Checked = True then
    if FileExists('charserv.bat') then
    WinExec('charserv.bat',1)
    else
    ShowMessage('"charserv.bat" não encontrado');

  if CheckBox3.Checked = True then
    if FileExists('mapserv.bat') then
    WinExec('mapserv.bat',1)
    else
    ShowMessage('"mapserv.bat" não encontrado');
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
// Leitura do Login
// login_athena.conf, incluido na pasta 'conf'
 a:=ArquivoCfg.Criar('conf\login_athena.conf','"conf\login_athena.conf" não encontrado');
 Edit3.Text:=a.Obter('login_port');
 Edit4.Text:=a.Obter('admin_pass');
 Edit5.Text:=a.Obter('gm_pass');
 Edit6.Text:=a.Obter('gm_account_filename');
 GM_accountfile:=a.Obter('gm_account_filename');
 Edit7.Text:=a.Obter('account_filename');
   if a.Obter('new_account') = '0' then
   CheckBox4.Checked:=False
   else if a.Obter('check_client_version') = 'yes' then
   CheckBox5.Checked:=True
   else if a.Obter('check_client_version') = 'no' then
   CheckBox5.Checked:=False
   else if a.Obter('date_format') = '0' then
   RadioButton1.Checked:=True
   else if a.Obter('date_format') = '1' then
   RadioButton2.Checked:=True
   else if a.Obter('date_format') = '2' then
   RadioButton3.Checked:=True
   else if a.Obter('date_format') = '3' then
   RadioButton4.Checked:=True;
 Edit8.Text:=a.Obter('client_version_to_connect');

// Leitura do Inter
// inter_athena.conf, incluido na pasta 'conf'
 b:=ArquivoCfg.Criar('conf\inter_athena.conf','"conf\inter_athena.conf" não encontrado');
   Edit9.Text:=b.Obter('login_server_ip');
   Edit10.Text:=b.Obter('login_server_port');
   Edit11.Text:=b.Obter('login_server_id');
   Edit12.Text:=b.Obter('login_server_pw');
   Edit13.Text:=b.Obter('login_server_db');
   Edit14.Text:=b.Obter('log_db_ip');
   Edit15.Text:=b.Obter('log_db_port');
   Edit16.Text:=b.Obter('log_db_id');
   Edit17.Text:=b.Obter('log_db_pw');
   Edit18.Text:=b.Obter('log_db');
   Edit19.Text:=b.Obter('login_db');
   Edit20.Text:=b.Obter('loginlog_db');
   Edit23.Text:=b.Obter('mail_server_ip');
   Edit22.Text:=b.Obter('mail_server_port');
   Edit24.Text:=b.Obter('mail_server_id');
   Edit25.Text:=b.Obter('mail_server_pw');
   Edit21.Text:=b.Obter('mail_server_db');
   Edit26.Text:=b.Obter('mail_db',);
   if b.Obter('mail_server_enable') = 'yes' then
     CheckBox6.Checked:=True
   else if b.Obter('mail_server_enable') = 'no' then
     CheckBox6.Checked:=False;
   Edit27.Text:=b.Obter('storage_txt');
   Edit28.Text:=b.Obter('party_txt');
   Edit29.Text:=b.Obter('guild_txt');
   Edit30.Text:=b.Obter('pet_txt');
   Edit31.Text:=b.Obter('castle_txt');
   Edit32.Text:=b.Obter('scdata_txt');
   Edit45.Text:=b.Obter('char_server_ip');
   Edit46.Text:=b.Obter('char_server_id');
   Edit47.Text:=b.Obter('char_server_port');
   Edit48.Text:=b.Obter('char_server_pw');
   Edit49.Text:=b.Obter('char_server_db');
   Edit61.Text:=b.Obter('map_server_ip');
   Edit62.Text:=b.Obter('map_server_id');
   Edit63.Text:=b.Obter('map_server_port');
   Edit64.Text:=b.Obter('map_server_pw');
   Edit65.Text:=b.Obter('map_server_db');

// Leitura do Char
// char_athena.conf, incluido na pasta 'conf'
 c:=ArquivoCfg.Criar('conf\char_athena.conf','"conf\char_athena.conf" não encontrado');
   Editor32.Text:=c.Obter('userid');
   Edit33.Text:=c.Obter('passwd');
   Editor34.Text:=c.Obter('server_name');
   Editor33.Text:=c.Obter('wisp_server_name');
   Edit34.Text:=c.Obter('char_ip');
   Edit35.Text:=c.Obter('char_port');
   Edit37.Text:=c.Obter('max_connect_user');
   Edit38.Text:=c.Obter('char_txt');
   Edit39.Text:=c.Obter('backup_txt');
   Edit40.Text:=c.Obter('friends_txt');
   if c.Obter('save_log') = 'yes' then
     CheckBox7.Checked:=True
   else if c.Obter('save_log') = 'no' then
     CheckBox7.Checked:=False;
   Edit36.Text:=c.Obter('login_ip');
   Edit41.Text:=c.Obter('start_point');
   Edit42.Text:=c.Obter('start_weapon');
   Edit43.Text:=c.Obter('start_armor');
   Edit44.Text:=c.Obter('start_zeny');

// Leitura do Map
// map_athena.conf, incluido na pasta 'conf'
 d:=ArquivoCfg.Criar('conf\map_athena.conf','"conf\map_athena.conf" não encontrado');
   Edit50.Text:=d.Obter('userid');
   Edit51.Text:=d.Obter('passwd');
   Edit52.Text:=d.Obter('map_ip');
   Edit53.Text:=d.Obter('map_port');
   Edit54.Text:=d.Obter('map_cache_file');
   Edit55.Text:=d.Obter('db_path');
   Edit56.Text:=d.Obter('autosave_time');
   Edit57.Text:=d.Obter('motd_txt');
   Edit58.Text:=d.Obter('mapreg_txt');
   Edit59.Text:=d.Obter('help_txt');
   Edit60.Text:=d.Obter('help2_txt');
   if d.Obter('read_map_from_cache') = '0' then
     RadioButton7.Checked:=True
   else if d.Obter('read_map_from_cache') = '1' then
     RadioButton6.Checked:=True
   else if d.Obter('read_map_from_cache') = '2' then
     RadioButton5.Checked:=True;

// Leitura do Grf-Files
// grf-files.txt, incluido na pasta 'conf'
 f:=ArquivoCfg.Criar('conf\grf-files.txt','"conf\grf-files.txt" não encontrado');
   Edit1.Text:=f.Obter('data');
   Edit2.Text:=f.Obter('adata');

end;

procedure TForm1.Button3Click(Sender: TObject);
begin
// Definição no Login
// login_athena.conf, incluido na pasta 'conf'
 a:=ArquivoCfg.Criar('conf\login_athena.conf','"conf\login_athena.conf" não encontrado');
 a.Definir('login_port',Edit3.Text);
 a.Definir('admin_pass',Edit4.Text);
 a.Definir('gm_pass',Edit5.Text);
 a.Definir('gm_account_filename',Edit6.Text);
 a.Definir('account_filename',Edit7.Text);
 GM_accountfile:=Edit6.Text;
   if CheckBox4.Checked = False then
   a.Definir('new_account','0')
   else if CheckBox5.Checked = True then
   a.Definir('check_client_version','yes')
   else if CheckBox5.Checked = False then
   a.Definir('check_client_version','no')
   else if CheckBox5.Checked = True then
   a.Definir('new_account','0')
   else if RadioButton1.Checked = True then
   a.Definir('date_format','0')
   else if RadioButton2.Checked = True then
   a.Definir('date_format','1')
   else if RadioButton3.Checked = True then
   a.Definir('date_format','2')
   else if RadioButton4.Checked = True then
   a.Definir('date_format','3');
 a.Definir('client_version_to_connect',Edit8.Text);
 a.Salvar();

// Definição do Inter
// inter_athena.conf, incluido na pasta 'conf'
 b:=ArquivoCfg.Criar('conf\inter_athena.conf','"conf\inter_athena.conf" não encontrado');
 b.Definir('login_server_ip',Edit9.Text);
 b.Definir('login_server_port',Edit10.Text);
 b.Definir('login_server_id',Edit11.Text);
 b.Definir('login_server_pw',Edit12.Text);
 b.Definir('login_server_db',Edit13.Text);
 b.Definir('log_db_ip',Edit14.Text);
 b.Definir('log_db_port',Edit15.Text);
 b.Definir('log_db_id',Edit16.Text);
 b.Definir('log_db_pw',Edit17.Text);
 b.Definir('log_db',Edit18.Text);
 b.Definir('login_db',Edit19.Text);
 b.Definir('loginlog_db',Edit20.Text);
 b.Definir('mail_server_ip',Edit23.Text);
 b.Definir('mail_server_port',Edit22.Text);
 b.Definir('mail_server_id',Edit24.Text);
 b.Definir('mail_server_pw',Edit25.Text);
 b.Definir('mail_server_db',Edit21.Text);
 b.Definir('mail_db',Edit26.Text);
 if CheckBox6.Checked = False then
  b.Definir('mail_server_enable','no')
 else if CheckBox6.Checked = True then
  b.Definir('mail_server_enable','yes');
 b.Definir('storage_txt',Edit27.Text);
 b.Definir('party_txt',Edit28.Text);
 b.Definir('guild_txt',Edit29.Text);
 b.Definir('pet_txt',Edit30.Text);
 b.Definir('castle_txt',Edit31.Text);
 b.Definir('scdata_txt',Edit32.Text);
 b.Definir('char_server_ip',Edit45.Text);
 b.Definir('char_server_port',Edit47.Text);
 b.Definir('char_server_id',Edit46.Text);
 b.Definir('char_server_pw',Edit48.Text);
 b.Definir('char_server_db',Edit49.Text);
 b.Definir('map_server_ip',Edit61.Text);
 b.Definir('map_server_id',Edit62.Text);
 b.Definir('map_server_port',Edit63.Text);
 b.Definir('map_server_pw',Edit64.Text);
 b.Definir('map_server_db',Edit65.Text);
 b.Salvar();

// Definição no Char
// char_athena.conf, incluido na pasta 'conf'
 c:=ArquivoCfg.Criar('conf\char_athena.conf','"conf\char_athena.conf" não encontrado');
   c.Definir('userid',Editor32.Text);
   c.Definir('passwd',Edit33.Text);
   c.Definir('server_name',Editor34.Text);
   c.Definir('wisp_server_name',Editor33.Text);
   c.Definir('char_ip',Edit34.Text);
   c.Definir('char_port',Edit35.Text);
   c.Definir('max_connect_user',Edit37.Text);
   c.Definir('char_txt',Edit38.Text);
   c.Definir('backup_txt',Edit39.Text);
   c.Definir('friends_txt',Edit40.Text);
   if CheckBox7.Checked = True then
     c.Definir('save_log','yes')
   else if CheckBox7.Checked = False then
     c.Definir('save_log','no');
   c.Definir('login_ip',Edit36.Text);
   c.Definir('start_point',Edit41.Text);
   c.Definir('start_weapon',Edit42.Text);
   c.Definir('start_armor',Edit43.Text);
   c.Definir('start_zeny',Edit44.Text);
   c.Definir('login_ip',Edit36.Text);
   c.Definir('login_port',Edit3.Text);
   c.Salvar();

// Definição do Map
// map_athena.conf, incluido na pasta 'conf'
 d:=ArquivoCfg.Criar('conf\map_athena.conf','"conf\map_athena.conf" não encontrado');
   d.Definir('userid',Edit50.Text);
   d.Definir('passwd',Edit51.Text);
   d.Definir('map_ip',Edit52.Text);
   d.Definir('map_port',Edit53.Text);
   d.Definir('map_cache_file',Edit54.Text);
   d.Definir('db_path',Edit55.Text);
   d.Definir('autosave_time',Edit56.Text);
   d.Definir('motd_txt',Edit57.Text);
   d.Definir('mapreg_txt',Edit58.Text);
   d.Definir('help_txt',Edit59.Text);
   d.Definir('help2_txt',Edit60.Text);
   if RadioButton7.Checked = True then
     d.Definir('read_map_from_cache','0')
   else if RadioButton6.Checked = True then
     d.Definir('read_map_from_cache','1')
   else if RadioButton5.Checked = True then
     d.Definir('read_map_from_cache','2');
   d.Definir('char_ip',Edit34.Text);
   d.Definir('char_port',Edit35.Text);
   d.Salvar();

// Definição do Grf-Files
// grf-files.txt, incluido na pasta 'conf'
 f:=ArquivoCfg.Criar('conf\grf-files.txt','"conf\grf-files.txt" não encontrado');
   f.Definir('data',Edit1.Text);
   f.Definir('adata',Edit2.Text);
   f.Salvar();

 ShowMessage('Salvo com sucesso!');
end;

procedure TForm1.Button1Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/battle.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button2Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/client.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button6Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/drops.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button7Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/exp.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button8Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/gm.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button9Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/guild.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button10Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/items.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button11Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/misc.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button12Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/monster.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button13Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/party.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button14Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/pet.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button15Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/player.conf', Nil, SW_ShowMaximized);
end;

procedure TForm1.Button16Click(Sender: TObject);
begin
ShellExecute (0, Nil, 'notepad.exe', 'conf/battle/skill.conf', Nil, SW_ShowMaximized);
end;

end.


