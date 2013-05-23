<?
include "readfile";
echo '<center>';
$corec = str_replace('	ShowMessage(""CL_XXBL"          ("CL_BT_YELLOW"        (c)2005 eAthena Development Team presents        "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // yellow writing (33)
',
'	ShowMessage(""CL_XXBL"          ("CL_BOLD"      _________                  v%2d.%02d.%02d               "CL_XXBL")"CL_CLL""CL_NORMAL"\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"      \\_   ___ \\_______  ____   ____  __ __  ______      "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"      /    \\  \\/\\_  __ \\/  _ \\ /    \\|  |  \\/  ___/      "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"      \\     \\____|  | \\(  <_> )   |  \\  |  /\\___ \\       "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"       \\______  /|__|   \\____/|___|  /____//____  >      "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"              \\/                   \\/           \\/       "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"                     Usando Fusion Advanced maps         "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BT_YELLOW"        Baseado em eAthena (c) 2005 Projeto Cronus       "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // yellow writing (33)
',$corec);

$corec = str_replace('	ShowMessage(""CL_XXBL"          ("CL_BOLD"       ______  __    __                                  "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
','',$corec);

$corec = str_replace('	ShowMessage(""CL_XXBL"          ("CL_BOLD"      /\\\  _  \\\/\\\ \\\__/\\\ \\\                     v%2d.%02d.%02d   "CL_XXBL")"CL_CLL""CL_NORMAL"\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"    __\\\ \\\ \\\_\\\ \\\ \\\ ,_\\\ \\\ \\\___      __    ___      __      "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
','',$corec);

$corec = str_replace("/'__`\\\ \\\  __ \\\ \\\ \\\/\\\ \\\  _ `\\\  /'__`\\\/' _ `\\\  /'__`\\",'',$corec);

$corec = str_replace('	ShowMessage(""CL_XXBL"          ("CL_BOLD"  \    "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
','',$corec);

$corec = str_replace('	ShowMessage(""CL_XXBL"          ("CL_BOLD" /\\\  __/\\\ \\\ \\\/\\\ \\\ \\\ \\\_\\\ \\\ \\\ \\\ \\\/\\\  __//\\\ \\\/\\\ \\\/\\\ \\\_\\\.\\\_  "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
','',$corec);

$corec = str_replace('\\\ \\\____\\\\\\\ \\\_\\\ \\\_\\\ \\\__\\\\\\\ \\\_\\\ \\\_\\\ \\\____\\\ \\\_\\\ \\\_\\\ \\\__/.\\\_\\\\','',$corec);

$corec = str_replace('	ShowMessage(""CL_XXBL"          ("CL_BOLD"  "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
','',$corec);

$corec = str_replace('	ShowMessage(""CL_XXBL"          ("CL_BOLD"  \\\/____/ \\\/_/\\\/_/\\\/__/ \\\/_/\\\/_/\\\/____/\\\/_/\\\/_/\\\/__/\\\/_/ "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"   _   _   _   _   _   _   _     _   _   _   _   _   _   "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"  / \\\ / \\\ / \\\ / \\\ / \\\ / \\\ / \\\   / \\\ / \\\ / \\\ / \\\ / \\\ / \\\  "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD" ( e | n | g | l | i | s | h ) ( A | t | h | e | n | a ) "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"  \\\_/ \\\_/ \\\_/ \\\_/ \\\_/ \\\_/ \\\_/   \\\_/ \\\_/ \\\_/ \\\_/ \\\_/ \\\_/  "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // 1: bold char, 0: normal char
	ShowMessage(""CL_XXBL"          ("CL_BOLD"                                                         "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // yellow writing (33)
	ShowMessage(""CL_XXBL"          ("CL_BT_YELLOW"  Advanced Fusion Maps (c) 2003-2005 The Fusion Project  "CL_XXBL")"CL_CLL""CL_NORMAL"\n"); // yellow writing (33)
','',$corec);


$corec = str_replace('	ShowWarning ("You are running eAthena as the root superuser.\n");
	ShowWarning ("It is unnecessary and unsafe to run eAthena with root privileges.\n");
',
		     '	ShowWarning ("You are running Cronus as the root superuser.\n");
	ShowWarning ("It is unnecessary and unsafe to run Cronus with root privileges.\n");
',$corec);

$fp = fopen($corec2, 'w');
if (!fwrite($fp, $corec)) {
echo "<font color=\"red\">O arquivo core.c não foi convertido</font><br>";
}
echo "<font color=\"green\">O arquivo core.c foi convertido</font><br>";
fclose($fp);

echo '</center>';
?>