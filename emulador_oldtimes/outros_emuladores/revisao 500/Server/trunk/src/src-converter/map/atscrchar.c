<?
if(file_exists('src/map/atcommand.c'))
{$a = file_get_contents('src/map/atcommand.c');
$a2 = 'src/map/atcommand.c';}

if(file_exists('src/map/script.c'))
{$b = file_get_contents('src/map/script.c');
$b2 = 'src/map/script.c';}

if(file_exists('src/map/charcommand.c'))
{$c = file_get_contents('src/map/charcommand.c');
$c2 = 'src/map/charcommand.c';}

$a = str_replace('if ((job >= 0 && job < MAX_PC_CLASS))','if ((job >= 0 && job < 24))',$a);

$b = str_replace('if ((job >= 0 && job < MAX_PC_CLASS))','if ((job >= 0 && job < 24))',$b);

$c = str_replace('if ((job >= 0 && job < MAX_PC_CLASS))','if ((job >= 0 && job < 24))',$c);

$fp1 = fopen($a2, 'w');
if (!fwrite($fp1, $a)) {
echo "<font color=\"red\">O arquivo atcommand.c não foi configurado corretamente o jobchange</font><br>";
}
echo "<font color=\"green\">O arquivo atcommand.c foi configurado corretamente o jobchange</font><br>";
fclose($fp1);


$fp2 = fopen($b2, 'w');
if (!fwrite($fp2, $b)) {
echo "<font color=\"red\">O arquivo script.c não foi configurado corretamente o jobchange</font><br>";
}
echo "<font color=\"green\">O arquivo script.c foi configurado corretamente o jobchange</font><br>";
fclose($fp2);

$fp3 = fopen($c2, 'w');
if (!fwrite($fp3, $c)) {
echo "<font color=\"red\">O arquivo charcommand.c não foi configurado corretamente o jobchange</font><br>";
}
echo "<font color=\"green\">O arquivo charcommand.c foi configurado corretamente o jobchange</font><br>";
fclose($fp3);

echo '</center>';
?>



?>