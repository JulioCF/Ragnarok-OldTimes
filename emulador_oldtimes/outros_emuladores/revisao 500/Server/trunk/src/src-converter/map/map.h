<?
include "readfile";
echo '<center>';

$maph = str_replace('#define MAX_PC_CLASS 4050',
		    '#define MAX_PC_CLASS 4001 //original code: all classes (4050) by Mehah',$maph);

$fp = fopen($maph2, 'w');
if (!fwrite($fp, $maph)) {
echo "<font color=\"red\">O arquivo map.h não foi convertido</font>";
}
echo "<font color=\"green\">O arquivo map.h foi convertido</font>";
fclose($fp);

echo '</center>';
?>