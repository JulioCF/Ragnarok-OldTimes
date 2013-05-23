<?
include "readfile";
echo '<center>';

$battleh = str_replace('	unsigned short display_version;','	unsigned short display_version;
	unsigned short display_site;',$battleh);

$fp = fopen($battleh2, 'w');
if (!fwrite($fp, $battleh)) {
echo "<font color=\"red\">O arquivo battle.h não foi convertido</font>";
}
echo "<font color=\"green\">O arquivo battle.h foi convertido</font>";
fclose($fp);

echo '</center>';
?>