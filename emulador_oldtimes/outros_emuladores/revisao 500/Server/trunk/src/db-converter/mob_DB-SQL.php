<?
$theFile = file_get_contents('mob_file.txt');

$lines = array();
$lines = explode("\n", $theFile);
$lineCount = count($lines);

for ($i = 0; $i < $lineCount; $i++){
 $lines2 = explode(",", $lines[$i]);
 
 echo "REPLACE INTO `mob_db` VALUES (".$lines2[0].",'".$lines2[1]."','".$lines2[2]."',".$lines2[3].",".$lines2[4].",".$lines2[5].",".$lines2[6].",".$lines2[7].",".$lines2[8].",".$lines2[9].",".$lines2[10].",".$lines2[11].",".$lines2[12].",".$lines2[13].",".$lines2[14].",".$lines2[15].",".$lines2[16].",".$lines2[17].",".$lines2[18].",".$lines2[19].",".$lines2[20].",".$lines2[21].",".$lines2[22].",".$lines2[23].",".$lines2[24].",".$lines2[25].",".$lines2[26].",".$lines2[27].",".$lines2[28].",".$lines2[29].",".$lines2[30].",".$lines2[31].",".$lines2[32].",".$lines2[33].",".$lines2[34].",".$lines2[35].",".$lines2[36].",".$lines2[37].",".$lines2[38].",".$lines2[39].",".$lines2[40].",".$lines2[41].",".$lines2[42].",".$lines2[43].",".$lines2[44].",".$lines2[45].",".$lines2[46].",".$lines2[47].",".$lines2[48].",".$lines2[49].",".$lines2[50].",".$lines2[51].",".$lines2[52].",".$lines2[53].",".$lines2[54].",".$lines2[55].",".$lines2[56].");<BR>";
}
?>