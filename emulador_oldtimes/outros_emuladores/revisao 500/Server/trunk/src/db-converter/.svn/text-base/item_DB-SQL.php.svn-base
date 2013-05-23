<?
$theFile = file_get_contents('item_file.txt');

$lines = array();
$lines = explode("\n", $theFile);
$lineCount = count($lines);

for ($i = 0; $i < $lineCount; $i++){
 $lines2 = explode(",", $lines[$i]);
 $script = explode("{", $lines[$i]);
 
 $exp0 = $lines2[0];
 if($lines2[1] == ""){$exp1 = "NULL";}else{$exp1 = "'".$lines2[1]."'";}
 if($lines2[2] == ""){$exp2 = "NULL";}else{$exp2 = "'".$lines2[2]."'";}
 if($lines2[3] == ""){$exp3 = "NULL";}else{$exp3 = "'".$lines2[3]."'";}
 if($lines2[4] == ""){$exp4 = "NULL";}else{$exp4 = "'".$lines2[4]."'";}
 if($lines2[5] == ""){$exp5 = "NULL";}else{$exp5 = "'".$lines2[5]."'";}
 if($lines2[6] == ""){$exp6 = "NULL";}else{$exp6 = "'".$lines2[6]."'";}
 if($lines2[7] == ""){$exp7 = "NULL";}else{$exp7 = "'".$lines2[7]."'";}
 if($lines2[8] == ""){$exp8 = "NULL";}else{$exp8 = "'".$lines2[8]."'";}
 if($lines2[9] == ""){$exp9 = "NULL";}else{$exp9 = "'".$lines2[9]."'";}
 if($lines2[10] == ""){$exp10 = "NULL";}else{$exp10 = "'".$lines2[10]."'";}
 if($lines2[11] == ""){$exp11 = "NULL";}else{$exp11 = "'".$lines2[11]."'";}
 if($lines2[12] == ""){$exp12 = "NULL";}else{$exp12 = $lines2[12];}
 if($lines2[13] == ""){$exp13 = "NULL";}else{$exp13 = "'".$lines2[13]."'";}
 if($lines2[14] == ""){$exp14 = "NULL";}else{$exp14 = "'".$lines2[14]."'";}
 if($lines2[15] == ""){$exp15 = "NULL";}else{$exp15 = "'".$lines2[15]."'";}
 if($lines2[16] == ""){$exp16 = "NULL";}else{$exp16 = "'".$lines2[16]."'";}
 if($lines2[17] == ""){$exp17 = "NULL";}else{$exp17 = "'".$lines2[17]."'";}
 if($lines2[18] == ""){$exp18 = "NULL";}else{$exp18 = "'".$lines2[18]."'";}

 $script1 = str_replace("}","",$script[1]);
 if($script[1] == "}"){$script1 = "NULL";}else{$script1 = "'".$script1."'";}

 echo "REPLACE INTO `item_db` VALUES (".$exp0.",".$exp1.",".$exp2.",".$exp3.",".$exp4.",".$exp5.",".$exp6.",".$exp7.",".$exp8.",".$exp9.",".$exp10.",".$exp11.",".$exp12.",".$exp13.",".$exp14.",".$exp15.",".$exp16.",".$exp17.",".$exp18.",".$script1.");<BR>";
}
?>