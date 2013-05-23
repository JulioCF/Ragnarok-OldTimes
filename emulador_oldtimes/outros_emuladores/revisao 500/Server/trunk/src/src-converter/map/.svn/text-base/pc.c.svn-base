<?
include "readfile";
echo '<center>';

$pcc = str_replace(
	'	if (battle_config.display_version == 1){
		char buf[256];
		sprintf(buf, "eAthena SVN version: %s", get_svn_revision());
		clif_displaymessage(sd->fd, buf);
	}',
	'	if (battle_config.display_version == 1){
		char buf[256];
		sprintf(buf, "Cronus Versão SVN: %s", get_svn_revision());
		clif_displaymessage(sd->fd, buf);
}
	if (battle_config.display_site == 1){
		clif_displaymessage(sd->fd, "http://www.cronus-emulator.com");
}',$pcc);

$fp = fopen($pcc2, 'w');
if (!fwrite($fp, $pcc)) {
echo "<font color=\"red\">O arquivo pc.c não foi convertido</font>";
}
echo "<font color=\"green\">O arquivo pc.c foi convertido</font>";
fclose($fp);

echo '</center>';
?>