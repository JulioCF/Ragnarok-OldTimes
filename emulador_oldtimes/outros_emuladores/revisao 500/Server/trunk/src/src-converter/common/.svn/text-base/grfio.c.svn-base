<?
include "readfile";
echo '<center>';

$grfioc = str_replace('strcmp(w1, "adata") == 0)	// Alpha version data file','strcmp(w1, "adata") == 0 ||	// Alpha version data file
					strcmp(w1, "db_info") == 0)	// DataBase Express',$grfioc);

$fp = fopen($grfioc2, 'w');
if (!fwrite($fp, $grfioc)) {
echo "<font color=\"red\">O arquivo grfio.c não foi convertido</font>";
}
echo "<font color=\"green\">O arquivo grfio.c foi convertido</font>";
fclose($fp);

echo '</center>';
?>