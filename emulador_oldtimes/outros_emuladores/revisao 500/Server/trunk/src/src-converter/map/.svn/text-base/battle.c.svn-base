<?
include "readfile";
echo '<center>';

$battlec = str_replace('	{ "display_version",	                  &battle_config.display_version}, // [Ancyker], for a feature by...?','	{ "display_version",	                  &battle_config.display_version}, // [Ancyker], for a feature by...?
	{ "display_site",	                  &battle_config.display_site}, // [Mehah]',$battlec);

$battlec = str_replace('	battle_config.display_version = 1;','	battle_config.display_version = 1;
	battle_config.display_site = 1;',$battlec);


$fp = fopen($battlec2, 'w');
if (!fwrite($fp, $battlec)) {
echo "<font color=\"red\">O arquivo battle.c não foi convertido</font>";
}
echo "<font color=\"green\">O arquivo battle.c foi convertido</font>";
fclose($fp);

echo '</center>';
?>