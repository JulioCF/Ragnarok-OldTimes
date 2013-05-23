<?
include "readfile";
echo '<center>';

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter a player name (usage: @where <char name>).");',
			  'clif_displaymessage(fd, "Por favor, entre com o nome do jogador (usando: @where <nom do personagem>).");',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter a player name (usage: @jumpto/@warpto/@goto <char name>).");',
			  'clif_displaymessage(fd, "Por favor, entre com o nome do jogador (usando: @jumpto/@warpto/@goto <nome do personagem>).");',$atcommandc);

$atcommandc = str_replace('sprintf(atcmd_output, "Name: %s | Zeny: %d", pl_sd->status.name, pl_sd->status.zeny);',
			  'sprintf(atcmd_output, "Nome: %s | Zeny: %d", pl_sd->status.name, pl_sd->status.zeny);',$atcommandc);

$atcommandc = str_replace('sprintf(atcmd_output, "Please, enter a speed value (usage: @speed <%d-%d>).", MIN_WALK_SPEED, MAX_WALK_SPEED);',
			  'sprintf(atcmd_output, "Por favor, entre com um valor de velocidade válido (usando: @speed <%d-%d>).", MIN_WALK_SPEED, MAX_WALK_SPEED);',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter at least a option (usage: @option <param1:0+> <param2:0+> <param3:0+>).");',
			  'clif_displaymessage(fd, "Por favor, entre com pelo menos uma opção (usando: @option <param1:0+> <param2:0+> <param3:0+>).");',$atcommandc);

$atcommandc = str_replace(
'			{ "novice",		0 },
			{ "swordsman",	1 },
			{ "mage",		2 },
			{ "archer",		3 },
			{ "acolyte",	4 },
			{ "merchant",	5 },
			{ "thief",		6 },
			{ "knight",		7 },
			{ "priest",		8 },
			{ "priestess",	8 },
			{ "wizard",		9 },
			{ "blacksmith",	10 },
			{ "hunter",		11 },
			{ "assassin",	12 },
			{ "crusader",	14 },
			{ "monk",		15 },
			{ "sage",		16 },
			{ "rogue",		17 },
			{ "alchemist",	18 },
			{ "bard",		19 },
			{ "dancer",		20 },
			{ "super novice",	23 },
			{ "supernovice",	23 },
			{ "gunslinger",	24 },
			{ "gunner",	24 },
			{ "ninja",	25 },
			{ "high novice",	4001 },
			{ "swordsman high",	4002 },
			{ "mage high",		4003 },
			{ "archer high",	4004 },
			{ "acolyte high",	4005 },
			{ "merchant high",	4006 },
			{ "thief high",		4007 },
			{ "lord knight",	4008 },
			{ "high priest",	4009 },
			{ "high priestess",	4009 },
			{ "high wizard",	4010 },
			{ "whitesmith",		4011 },
			{ "sniper",		4012 },
			{ "assassin cross",	4013 },
			{ "paladin",	4015 },
			{ "champion",	4016 },
			{ "professor",	4017 },
			{ "stalker",	4018 },
			{ "creator",	4019 },
			{ "clown",		4020 },
			{ "gypsy",		4021 },
			{ "baby novice",	4023 },
			{ "baby swordsman",	4024 },
			{ "baby mage",		4025 },
			{ "baby archer",	4026 },
			{ "baby acolyte",	4027 },
			{ "baby merchant",	4028 },
			{ "baby thief",		4029 },
			{ "baby knight",	4030 },
			{ "baby priest",	4031 },
			{ "baby priestess",	4031 },
			{ "baby wizard",	4032 },
			{ "baby blacksmith",4033 },
			{ "baby hunter",	4034 },
			{ "baby assassin",	4035 },
			{ "baby crusader",	4037 },
			{ "baby monk",		4038 },
			{ "baby sage",		4039 },
			{ "baby rogue",		4040 },
			{ "baby alchemist",	4041 },
			{ "baby bard",		4042 },
			{ "baby dancer",	4043 },
			{ "super baby",		4045 },
			{ "taekwon",		4046 },
			{ "taekwon boy",	4046 },
			{ "taekwon girl",	4046 },
			{ "star gladiator",	4047 },
			{ "soul linker",	4049 },',
'			{ "aprendiz",		0 },
			{ "espadachim",		1 },
			{ "mago",		2 },
			{ "arqueiro",		3 },
			{ "noviço",		4 },
			{ "novico",		4 },
			{ "mercador",		5 },
			{ "gatuno",		6 },
			{ "cavaleiro",		7 },
			{ "sacerdote",		8 },
			{ "bruxo",		9 },
			{ "ferreiro",		10 },
			{ "caçador",		11 },
			{ "cacador",		11 },
			{ "mercenário",		12 },
			{ "mercenario",		12 },
			{ "templário",		14 },
			{ "templario",		14 },
			{ "monge",		15 },
			{ "sábio",		16 },
			{ "sabio",		16 },
			{ "arruaceiro",		17 },
			{ "alquimista",		18 },
			{ "bardo",		19 },
			{ "odalisca",		20 },
			{ "super aprendiz",	23 },
			{ "superaprendiz",	23 },
			//{ "high novice",	4001 },
			//{ "swordsman high",	4002 },
			//{ "mage high",		4003 },
			//{ "archer high",	4004 },
			//{ "acolyte high",	4005 },
			//{ "merchant high",	4006 },
			//{ "thief high",		4007 },
			//{ "lord knight",	4008 },
			//{ "high priest",	4009 },
			//{ "high priestess",	4009 },
			//{ "high wizard",	4010 },
			//{ "whitesmith",		4011 },
			//{ "sniper",		4012 },
			//{ "assassin cross",	4013 },
			//{ "paladin",	4015 },
			//{ "champion",	4016 },
			//{ "professor",	4017 },
			//{ "stalker",	4018 },
			//{ "creator",	4019 },
			//{ "clown",		4020 },
			//{ "gypsy",		4021 },
			//{ "baby novice",	4023 },
			//{ "baby swordsman",	4024 },
			//{ "baby mage",		4025 },
			//{ "baby archer",	4026 },
			//{ "baby acolyte",	4027 },
			//{ "baby merchant",	4028 },
			//{ "baby thief",		4029 },
			//{ "baby knight",	4030 },
			//{ "baby priest",	4031 },
			//{ "baby priestess",	4031 },
			//{ "baby wizard",	4032 },
			//{ "baby blacksmith",4033 },
			//{ "baby hunter",	4034 },
			//{ "baby assassin",	4035 },
			//{ "baby crusader",	4037 },
			//{ "baby monk",		4038 },
			//{ "baby sage",		4039 },
			//{ "baby rogue",		4040 },
			//{ "baby alchemist",	4041 },
			//{ "baby bard",		4042 },
			//{ "baby dancer",	4043 },
			//{ "super baby",		4045 },
			//{ "taekwon",        4046 },
			//{ "taekwon boy",	4046 },
			//{ "taekwon girl",	4046 },
			//{ "star gladiator",	4047 },
			//{ "soul linker",	4049 },',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter job ID (usage: @job/@jobchange <job ID>).");',
			  'clif_displaymessage(fd, "Por favor, entre com a ID da Classe (usando: @job/@jobchange <ID da Classe>).");',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter a player name (usage: @kill <char name>).");',
			  'clif_displaymessage(fd, "Por favor, entre com o nome do personagem (usando: @kill <nome do personagem>).");',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter a message (usage: @kami <message>).");',
			  'clif_displaymessage(fd, "Por favor, entre com uma mensagem (usando: @kami <mensagem>).");',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter an item name/id (usage: @item <item name or ID> [quantity]).");',
			  'clif_displaymessage(fd, "Por favor, entre com um nome de item ou id (usando: @item <nome do item ou ID> [quantidade]).");',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter all informations (usage: @item2 <item name or ID> <quantity>");',
			  'clif_displaymessage(fd, "Por favor, entre com todas as informações (usando: @item2 <nome do item ou ID> <quantidade>");',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter a level adjustement (usage: @lvup/@blevel/@baselvlup <number of levels>).");',
			  'clif_displaymessage(fd, "Por favor, entre com um nível (usando: @lvup/@blevel/@baselvlup <número de níveis>).");',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter a password (usage: @gm <password>).");',
			  'clif_displaymessage(fd, "Por favor, entre com uma senha (usando: @gm <senha>).");',$atcommandc);

$atcommandc = str_replace('sprintf(atcmd_output, "Please, enter at least a value (usage: @model <hair ID: %d-%d> <hair color: %d-%d> <clothes color: %d-%d>).",',
			  'sprintf(atcmd_output, "Por favor, entre com pelo menos um valor (usando: @model <ID do cabelo: %d-%d> <cor do cabelo: %d-%d> <cor da roupa: %d-%d>).",',$atcommandc);

$atcommandc = str_replace('sprintf(atcmd_output, "Please, enter a clothes color (usage: @dye/@ccolor <clothes color: %d-%d>).", MIN_CLOTH_COLOR, MAX_CLOTH_COLOR);',
			  'sprintf(atcmd_output, "Por favor, entre com um cor da roupa (usando: @dye/@ccolor <cor da roupa: %d-%d>).", MIN_CLOTH_COLOR, MAX_CLOTH_COLOR);',$atcommandc);

$atcommandc = str_replace('sprintf(atcmd_output, "Please, enter a hair style (usage: @hairstyle/@hstyle <hair ID: %d-%d>).", MIN_HAIR_STYLE, MAX_HAIR_STYLE);',
			  'sprintf(atcmd_output, "Por favor, entre com um estilo de cabelo (usando: @hairstyle/@hstyle <ID do cabelo: %d-%d>).", MIN_HAIR_STYLE, MAX_HAIR_STYLE);',$atcommandc);

$atcommandc = str_replace('sprintf(atcmd_output, "Please, enter a hair color (usage: @haircolor/@hcolor <hair color: %d-%d>).", MIN_HAIR_COLOR, MAX_HAIR_COLOR);',
			  'sprintf(atcmd_output, "Por favor, entre com uma cor de cabelo (usando: @haircolor/@hcolor <cor do cabelo: %d-%d>).", MIN_HAIR_COLOR, MAX_HAIR_COLOR);',$atcommandc);

$atcommandc = str_replace('		{ MAP_PRONTERA,	156, 191  },		//	 0=Prontera
		{ MAP_MORROC,		156, 93  },			//	 1=Morroc
		{ MAP_GEFFEN,		119, 59  },			//	 2=Geffen
		{ MAP_PAYON,		162, 233  },		//	 3=Payon
		{ MAP_ALBERTA,		192, 147  },	//	 4=Alberta
		{ MAP_IZLUDE,		128, 114  },		//	 5=Izlude
		{ MAP_ALDEBARAN,	140, 131  },		//	 6=Al de Baran
		{ MAP_LUTIE,		147, 134  },		//	 7=Lutie
		{ MAP_COMODO,		209, 143  },		//	 8=Comodo
		{ MAP_YUNO,		157,  51  },		//	 9=Yuno
		{ MAP_AMATSU,		198,  84  },		//	10=Amatsu
		{ MAP_GONRYUN,		160, 120  },	//	11=Gon Ryun
		{ MAP_UMBALA,		89,  157  },		//	12=Umbala
		{ MAP_NIFLHEIM,	21,  153  },		//	13=Niflheim
		{ MAP_LOUYANG,		217,  40  },	//	14=Lou Yang
		{ "new_1-1.gat",		53,  111  },	//	15=Training Grounds
		{ MAP_JAIL,		23,   61  },	//	16=Prison
		{ MAP_JAWAII,		249, 127  },		//  17=Jawaii
		{ MAP_AYOTHAYA,	151, 117  },		//  18=Ayothaya
		{ MAP_EINBROCH,	64,  200  },		//  19=Einbroch
		{ MAP_LIGHTHALZEN,	158,  92  },	//  20=Lighthalzen
		{ MAP_EINBECH,		70,   95  },	//  21=Einbech
		{ MAP_HUGEL,		96,  145  },		//  22=Hugel
',
			  '		{ MAP_PRONTERA,	156, 191  },		//	 0=Prontera
		{ MAP_MORROC,		156, 93  },			//	 1=Morroc
		{ MAP_GEFFEN,		119, 59  },			//	 2=Geffen
		{ MAP_PAYON,		162, 233  },		//	 3=Payon
		{ MAP_ALBERTA,		192, 147  },	//	 4=Alberta
		{ MAP_IZLUDE,		128, 114  },		//	 5=Izlude
		{ MAP_ALDEBARAN,	140, 131  },		//	 6=Al de Baran
		//{ MAP_LUTIE,		147, 134  },		//	 7=Lutie
		{ MAP_COMODO,		209, 143  },		//	 8=Comodo
		{ MAP_YUNO,		157,  51  },		//	 9=Juno
		{ MAP_AMATSU,		198,  84  },		//	10=Amatsu
		{ MAP_GONRYUN,		160, 120  },	//	11=Kunlun
		{ MAP_UMBALA,		89,  157  },		//	12=Umbala
		{ MAP_NIFLHEIM,	21,  153  },		//	13=Niflheim
		//{ MAP_LOUYANG,		217,  40  },	//	14=Lou Yang
		{ "new_1-1.gat",		53,  111  },	//	15=Campo de Treinamento
		{ MAP_JAIL,		23,   61  },	//	16=Prisão
		//{ MAP_JAWAII,		249, 127  },		//  17=Jawaii
		//{ MAP_AYOTHAYA,	151, 117  },		//  18=Ayothaya
		//{ MAP_EINBROCH,	64,  200  },		//  19=Einbroch
		//{ MAP_LIGHTHALZEN,	158,  92  },	//  20=Lighthalzen
		//{ MAP_EINBECH,		70,   95  },	//  21=Einbech
		//{ MAP_HUGEL,		96,  145  },		//  22=Hugel
',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(sd->fd,"You can not use @go on this map.");',
			  'clif_displaymessage(sd->fd,"Você não pode usar #go neste mapa.");',$atcommandc);

$atcommandc = str_replace('		clif_displaymessage(fd, " 0=Prontera         1=Morroc       2=Geffen");
		clif_displaymessage(fd, " 3=Payon            4=Alberta      5=Izlude");
		clif_displaymessage(fd, " 6=Al De Baran      7=Lutie        8=Comodo");
		clif_displaymessage(fd, " 9=Yuno             10=Amatsu      11=Gon Ryun");
		clif_displaymessage(fd, " 12=Umbala          13=Niflheim    14=Lou Yang");
		clif_displaymessage(fd, " 15=Novice Grounds  16=Prison      17=Jawaii");
		clif_displaymessage(fd, " 18=Ayothaya        19=Einbroch    20=Lighthalzen");
		clif_displaymessage(fd, " 21=Einbech         22=Hugel");
',
			  '		clif_displaymessage(fd, " 0=Prontera         1=Morroc       2=Geffen");
		clif_displaymessage(fd, " 3=Payon            4=Alberta      5=Izlude");
		clif_displaymessage(fd, " 6=Al De Baran      8=Comodo"); //7=Lutie
		clif_displaymessage(fd, " 9=Juno             10=Amatsu      11=Kunlun");
		clif_displaymessage(fd, " 12=Umbala          13=Niflheim");
		clif_displaymessage(fd, " 15=Campo de Treinamento  16=Prisão"); //14=Lou Yang
		//clif_displaymessage(fd, " 18=Ayothaya        19=Einbroch    20=Lighthalzen");
		//clif_displaymessage(fd, " 21=Einbech         22=Hugel");
',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Invalid monster ID"); // Invalid Monster ID.',
			  'clif_displaymessage(fd, "ID de monstro inválida"); // Invalid Monster ID.',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Give a monster name/id please.");',
			  'clif_displaymessage(fd, "Por favor, digite o nome/id do monstro.");',$atcommandc);

$atcommandc = str_replace('clif_displaymessage(fd, "Please, enter a position and a amount (usage: @refine <equip position> <+/- amount>).");',
			  'clif_displaymessage(fd, "Por favor, entre com a posição e uma quantidade (usando: @refine <posição do equip> <+/- quantidade>).");',$atcommandc);

$atcommandc = str_replace("Please, enter at least an item name/id (usage: @produce <equip name or equip ID> <element> <# of very's>",
			  "Por favor, entre com pelo menos um nome/id do item (usando: @produce <nome do equip ou ID do equip> <elemento> <# of very's>",$atcommandc);


$fp = fopen($atcommandc2, 'w');

if (!fwrite($fp, $atcommandc)) {
echo "<font color=\"red\">O arquivo atcommand.c não foi convertido</font>";
}
echo "<font color=\"green\">O arquivo atcommand.c foi convertido</font>";
fclose($fp);

echo '</center>';
?>