<?
include "readfile";
echo '<center>';

$loginctxt = str_replace('for stat/lstat/fstat','para stat/lstat/fstat',$loginctxt);

$loginctxt = str_replace('Timer to check if GM_account file has been changed and reload GM account automaticaly (in seconds; default: 15)','Timer para checar se o arquivo GM_account foi modificado e reabrir o GM account automaticamente (em segundos; padrão: 15)',$loginctxt);

$loginctxt = str_replace('0: no, 1: yes','0: nao, 1: sim',$loginctxt);

$loginctxt = str_replace('(without packet 0x2714), 2: all packets','(sem o pacote 0x2714), 2: todos os pacotes',$loginctxt);

$loginctxt = str_replace('When set to 1, login server rejects incoming players that are already registered as online. [Skotlex]','quando 1, o login-server rejeita todos as tentativas de log dos players que estão registrados como online. [Skotlex]',$loginctxt);

$loginctxt = str_replace('Account flood protection [Kevin]','proteção de account flood [Kevin]',$loginctxt);

$loginctxt = str_replace('Init this to 10 seconds. [Skotlex]','Inicie essa variavel com 10. [Skotlex]',$loginctxt);

$loginctxt = str_replace('minimum level of player/GM (0: player, 1-99: gm) to connect on the server','minimo level do jogador/GM (0: jogador, 1-99: GM) para se conectar no server',$loginctxt);

$loginctxt = str_replace('Give possibility or not to adjust (ladmin command: timeadd) the time of an unlimited account.','dá ou naum a possibilidade  de ajustar (comando ladmin: timeadd) o tempo de contas ilimitadas.',$loginctxt);

$loginctxt = str_replace('Starting additional sec from now for the limited time at creation of accounts (-1: unlimited time, 0 or more: additional sec from now)','Tempo de inicio adicial (em segundos) de agora até o tempo limite de criação de contas (-1: ilimitado, 0 ou mais: tempo adicional em segundos de agora em diante)',$loginctxt);

$loginctxt = str_replace("It's to check IP of a player between login-server and char-server (part of anti-hacking system)","Checar o IP do jogador entre o login e o char-server(sistema anti-hack)",$loginctxt);

$loginctxt = str_replace('Client version check ON/OFF .. (sirius)','Checar versao do client (1-sim 2-nao) .. (sirius)',$loginctxt);

$loginctxt = str_replace('Client version needed to connect ..(sirius)','Versao do cliente ..(sirius)',$loginctxt);

$loginctxt = str_replace('33 for 32 + NULL terminated','33 para 32 + terminação nula (\0)',$loginctxt);

$loginctxt = str_replace('packet 0x006a value + 1 (0: compte OK)','o pacote 0x006a valor + 1 (0: compte OK)',$loginctxt);

$loginctxt = str_replace('e-mail (by default: a@a.com)','e-mail (padrão: a@a.com)',$loginctxt);

$loginctxt = str_replace('Message of error code #6 = Your are Prohibited to log in until %s (packet 0x006a)','mensagem de erro codigo #6 = Você está proibido de logar até %s (pacote 0x006a)',$loginctxt);

$loginctxt = str_replace('# of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)','# de segundos 1/1/1970 (timestamp): tempo limite de ban para as contas (0 =sem ban)',$loginctxt);

$loginctxt = str_replace('# of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)','# de segundos 1/1/1970 (timestamp): tempo limite de validade das contas(0 = ilimitado)',$loginctxt);

$loginctxt = str_replace('save of last IP of connection','salvear o ultimo IP conectado',$loginctxt);

$loginctxt = str_replace('a memo field','um campo de lembrete',$loginctxt);

$loginctxt = str_replace("// define the number of times that some players must authentify them before to save account file.
// it's just about normal authentification. If an account is created or modified, save is immediatly done.
// An authentification just change last connected IP and date. It already save in log file.
// set minimum auth change before save:","// defina o numero de vezes que os jogadores devem autenticarantes de salvar o arquivo de contas.
// apenas para autenticação normal. se uma conta for criada ou modificada, o arquivo é salvo imediatamente.
// Uma utenticação muda apenas o ultimo IP conectado e a data. Já salvo no arquivo de log.
// coloque a autenticação minima antes de salvar:",$loginctxt);

$loginctxt = str_replace('set divider of auth_num to found number of change before save','coloque o divisor do auth_num para um numero encontrado fora do tempo minimo de autenticação antes de salvar.',$loginctxt);

$loginctxt = str_replace('Counter. First save when 1st char-server do connection.','Contador. Primoro a savar quando o primeiro conectar no char-server.',$loginctxt);

$loginctxt = str_replace('Writing function of logs file','Escrevendo função para criar arquivos de log',$loginctxt);

$loginctxt = str_replace('jump a line if no message','pula se a linha estiver vazia',$loginctxt);

$loginctxt = str_replace('				// Platform/Compiler dependant clock() for time check is removed. [Lance]
				// clock() is originally used to track processing ticks on program execution.','				// Plataforma/compilador dependo do clock() para chegar se o tempo foi removido[Lance]
				// clock() é originalmente usado para processar as trilhas na execução do programa.(texto obscuro)',$loginctxt);

$loginctxt = str_replace('under cygwin or windows, if software is stopped, data are not written in the file -> fflush at every line','sobre cygwin ou windows, se o software estiver parado, datas não são escritas no arquivo -> fflush em cada linha',$loginctxt);

$loginctxt = str_replace('Online User Database [Wizputer]','Banco de DAdos de Usuarios online[Wizputer]',$loginctxt);

$loginctxt = str_replace('reset all to offline','reset todos para offline',$loginctxt);

$loginctxt = str_replace('purge db','remoção do bd',$loginctxt);

$loginctxt = str_replace("// Determine if an account (id) is a GM account
// and returns its level (or 0 if it isn't a GM account or if not found)","// Determina se uma conta é uma conta de GM
// e retprma p lvl do GM(ou 0 senão for GM ou não for encontrado)",$loginctxt);

$loginctxt = str_replace('Adds a new GM using acc id and level','Adiciona um novo GM usando o acc id e o nivel',$loginctxt);

$loginctxt = str_replace('addGM: GM account %d defined twice (same level: %d).\n','adicionando GM: a conta de GM %d foi definida duas vezes (mesmo level: %d).\n',$loginctxt);

$loginctxt = str_replace('addGM: GM account %d defined twice (levels: %d and %d).\n','adicionando GM: a conta de GM %d foi definida duas vezes(levels: %d e %d).\n',$loginctxt);

$loginctxt = str_replace('if new account','se for uma nova conta',$loginctxt);

$loginctxt = str_replace('4000 GM accounts found. Next GM accounts are not read.\n','4000 contas de GM. as proximas contas de GM não serão lidas.\n',$loginctxt);

$loginctxt = str_replace('***WARNING: 4000 GM accounts found. Next GM accounts are not read.','***AVISO: 4000 contas de GM. as proximas contas de GM não serão lidas.',$loginctxt);

$loginctxt = str_replace('Reading function of GM accounts file (and their level)','Lendo informações do arquivo de contas de GM(e seus respectivos levels)',$loginctxt);

$loginctxt = str_replace('read_gm_account: GM accounts file [%s] not found.','lendo_contas_GM: o arquivo de contas dos GM [%s] nao foi encontrado.',$loginctxt);

$loginctxt = str_replace('Actually, there is no GM accounts on the server.','Realmente, não há contas de GM no servidor.',$loginctxt);

$loginctxt = str_replace('limited to 4000, because we send information to char-servers (more than 4000 GM accounts???)','limitado a 4000, porque se mandarmos mais informação (mais de 4000 contad de GM???)',$loginctxt);

$loginctxt = str_replace('int (id) + int (level) = 8 bytes * 4000 = 32k (limit of packets in windows)','int (id) + int (level) = 8 bytes * 4000 = 32k (limite dos pacotes no windows)',$loginctxt);

$loginctxt = str_replace('ID Range [MC Cameri]','escala de ID [MC Cameri]',$loginctxt);

$loginctxt = str_replace("read_gm_account: file [%s], invalid 'acount_id|range level' format (line #%d).","lendo_contas_GM: arquivo [%s], invalido 'id da conta|escala de level' formato (linha #%d).",$loginctxt);

$loginctxt = str_replace('read_gm_account: file [%s] %dth account (line #%d) (invalid level [0 or negative]: %d).\n','read_gm_account: arquivo [%s] %d conta (linha #%d) (level invalido [0 ou negativo]: %d).\n',$loginctxt);

$loginctxt = str_replace('read_gm_account: file [%s] %dth account (invalid level, but corrected: %d->99).\n','lendo_contas_GM: arquivo [%s] %d conta (level invalido, mas corrigido: %d->99).\n',$loginctxt);

$loginctxt = str_replace('read_gm_account: file [%s] invalid range, beginning of range is equal to end of range (line #%d).\n','lendo_contas_GM: arquivo [%s] escala invalida, o comeco da escala e igual ao final da escala (linha #%d).\n',$loginctxt);

$loginctxt = str_replace('read_gm_account: file [%s] invalid range, beginning of range must be lower than end of range (line #%d).\n','lendo_contas_GM: arquivo [%s] escala invalida, o comeco da escala deve ser menor que o fim da escala (linha #%d).\n',$loginctxt);

$loginctxt = str_replace("read_gm_account: file '%s' read (%d GM accounts found).","lendo_contas_GM: arquivo '%s' lido (%d contas de GM encontrados).",$loginctxt);

$loginctxt = str_replace('// Test of the IP mask
// (ip: IP to be tested, str: mask x.x.x.x/# or x.x.x.x/y.y.y.y)','// teste de mascara de IP
// (ip: IP para ser testado, str: mask x.x.x.x/# ou x.x.x.x/y.y.y.y)',$loginctxt);

$loginctxt = str_replace('check_ipmask: invalid mask [%s].\n','checando: mascara invalida [%s].\n',$loginctxt);

$loginctxt = str_replace('//	printf("Tested IP: %08x, network: %08x, network mask: %08x\n",
//	       (unsigned int)ntohl(ip), (unsigned int)ntohl(ip2), (unsigned int)mask);','//	printf("IP testado: %08x, rede: %08x, mascara de rede: %08x\n",
//	       (unsigned int)ntohl(ip), (unsigned int)ntohl(ip2), (unsigned int)mask);',$loginctxt);

$loginctxt = str_replace('Access control by IP','Controle de Acesso por IP',$loginctxt);

$loginctxt = str_replace('When there is no restriction, all IP are authorised.','Quando não há restrições, todos os IP são permitidos',$loginctxt);

$loginctxt = str_replace("//	+   012.345.: front match form, or
//	    all: all IP are matched, or
//	    012.345.678.901/24: network form (mask with # of bits), or
//	    012.345.678.901/255.255.255.0: network form (mask with ip mask)
//	+   Note about the DNS resolution (like www.ne.jp, etc.):
//	    There is no guarantee to have an answer.
//	    If we have an answer, there is no guarantee to have a 100% correct value.
//	    And, the waiting time (to check) can be long (over 1 minute to a timeout). That can block the software.
//	    So, DNS notation isn't authorised for ip checking.","//	+   012.345.: formulario frontal encontrado or
//	    all: all IP encontrado, ou
//	    012.345.678.901/24: formulario de rede (mascara com # de bits), ou
//	    012.345.678.901/255.255.255.0: formulario de rede (mascara com mascara de IP)
//	+   Nota sobre a resolução de DNS (como www.ne.jp, etc.):
//	    Não há garantia que haverá uma resposta.
//	    Se houver uma resposta, não há nenhuma garatia que o valor esteja 100% correto.
//	    e, o tempo de espera (a checar) pode ser longo (acima de 1 minuto ou timedout). Que pode blockear o programa.
//	    Então, a notação de DNS não é autorizada para checagem de IP.",$loginctxt);

$loginctxt = str_replace("With 'allow, deny' (deny if not allow), allow has priority","com 'permissao, não-permissao' (não-permissao não é permitido), permissão tem prioridade",$loginctxt);

$fp = fopen($loginctxt2, 'w');
if (!fwrite($fp, $loginctxt)) {
echo "<font color=\"red\">O arquivo login.c não foi convertido</font>";
}
echo "<font color=\"green\">O arquivo login.c foi convertido</font>";
fclose($fp);

echo '</center>';
?>