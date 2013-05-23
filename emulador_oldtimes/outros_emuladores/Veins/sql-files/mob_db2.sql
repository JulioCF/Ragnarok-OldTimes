#
# Table structure for table `mob_db2`
#

DROP TABLE IF EXISTS `mob_db2`;
CREATE TABLE `mob_db2` (
  `ID` mediumint(9) unsigned NOT NULL default '0',
  `Sprite` text NOT NULL,
  `kName` text NOT NULL,
  `iName` text NOT NULL,
  `LV` tinyint(6) unsigned NOT NULL default '0',
  `HP` int(9) unsigned NOT NULL default '0',
  `SP` mediumint(9) unsigned NOT NULL default '0',
  `EXP` mediumint(9) unsigned NOT NULL default '0',
  `JEXP` mediumint(9) unsigned NOT NULL default '0',
  `Range1` tinyint(4) unsigned NOT NULL default '0',
  `ATK1` smallint(6) unsigned NOT NULL default '0',
  `ATK2` smallint(6) unsigned NOT NULL default '0',
  `DEF` smallint(6) unsigned NOT NULL default '0',
  `MDEF` smallint(6) unsigned NOT NULL default '0',
  `STR` tinyint(4) unsigned NOT NULL default '0',
  `AGI` tinyint(4) unsigned NOT NULL default '0',
  `VIT` tinyint(4) unsigned NOT NULL default '0',
  `INT` tinyint(4) unsigned NOT NULL default '0',
  `DEX` tinyint(4) unsigned NOT NULL default '0',
  `LUK` tinyint(4) unsigned NOT NULL default '0',
  `Range2` tinyint(4) unsigned NOT NULL default '0',
  `Range3` tinyint(4) unsigned NOT NULL default '0',
  `Scale` tinyint(4) unsigned NOT NULL default '0',
  `Race` tinyint(4) unsigned NOT NULL default '0',
  `Element` tinyint(4) unsigned NOT NULL default '0',
  `Mode` smallint(6) unsigned NOT NULL default '0',
  `Speed` smallint(6) unsigned NOT NULL default '0',
  `aDelay` smallint(6) unsigned NOT NULL default '0',
  `aMotion` smallint(6) unsigned NOT NULL default '0',
  `dMotion` smallint(6) unsigned NOT NULL default '0',
  `MEXP` mediumint(9) unsigned NOT NULL default '0',
  `ExpPer` smallint(9) unsigned NOT NULL default '0',
  `MVP1id` smallint(9) unsigned NOT NULL default '0',
  `MVP1per` smallint(9) unsigned NOT NULL default '0',
  `MVP2id` smallint(9) unsigned NOT NULL default '0',
  `MVP2per` smallint(9) unsigned NOT NULL default '0',
  `MVP3id` smallint(9) unsigned NOT NULL default '0',
  `MVP3per` smallint(9) unsigned NOT NULL default '0',
  `Drop1id` smallint(9) unsigned NOT NULL default '0',
  `Drop1per` smallint(9) unsigned NOT NULL default '0',
  `Drop2id` smallint(9) unsigned NOT NULL default '0',
  `Drop2per` smallint(9) unsigned NOT NULL default '0',
  `Drop3id` smallint(9) unsigned NOT NULL default '0',
  `Drop3per` smallint(9) unsigned NOT NULL default '0',
  `Drop4id` smallint(9) unsigned NOT NULL default '0',
  `Drop4per` smallint(9) unsigned NOT NULL default '0',
  `Drop5id` smallint(9) unsigned NOT NULL default '0',
  `Drop5per` smallint(9) unsigned NOT NULL default '0',
  `Drop6id` smallint(9) unsigned NOT NULL default '0',
  `Drop6per` smallint(9) unsigned NOT NULL default '0',
  `Drop7id` smallint(9) unsigned NOT NULL default '0',
  `Drop7per` smallint(9) unsigned NOT NULL default '0',
  `Drop8id` smallint(9) unsigned NOT NULL default '0',
  `Drop8per` smallint(9) unsigned NOT NULL default '0',
  `Drop9id` smallint(9) unsigned NOT NULL default '0',
  `Drop9per` smallint(9) unsigned NOT NULL default '0',
  `DropCardid` smallint(9) unsigned NOT NULL default '0',
  `DropCardper` smallint(9) unsigned NOT NULL default '0',
  PRIMARY KEY  (`ID`)
) TYPE=MyISAM;

# // Monsters Additional Database
# //
# // Structure of Database :
# // ID,Sprite_Name,kROName,iROName,LV,HP,SP,EXP,JEXP,Range1,ATK1,ATK2,DEF,MDEF,STR,AGI,VIT,INT,DEX,LUK,Range2,Range3,Scale,Race,Element,Mode,Speed,aDelay,aMotion,dMotion,MEXP,ExpPer,MVP1id,MVP1per,MVP2id,MVP2per,MVP3id,MVP3per,Drop1id,Drop1per,Drop2id,Drop2per,Drop3id,Drop3per,Drop4id,Drop4per,Drop5id,Drop5per,Drop6id,Drop6per,Drop7id,Drop7per,Drop8id,Drop8per,Drop9id,Drop9per,DropCardid,DropCardper
# // Crusader quest monsters with poring stats (No drops)