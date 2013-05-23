ALTER TABLE `party` ADD COLUMN `leader_char` int(11) NOT NULL DEFAULT '0' AFTER `leader_id`;
UPDATE `party`,`char` SET `leader_char` = `char`.char_id WHERE (`char`.party_id = `party`.party_id AND `party`.leader_id = `char`.account_id);

ALTER TABLE `mapreg` MODIFY `varname` varchar(32) NOT NULL;
ALTER TABLE `mapreg` MODIFY `value` varchar(255) NOT NULL;

ALTER TABLE `char` MODIFY `last_map` varchar(20) NOT NULL default 'prontera.gat';
ALTER TABLE `char` MODIFY `last_x` smallint(11) unsigned NOT NULL default '53';
ALTER TABLE `char` MODIFY `last_y` smallint(11) unsigned NOT NULL default '111';
ALTER TABLE `char` MODIFY `save_map` varchar(20) NOT NULL default 'prontera.gat';
ALTER TABLE `char` MODIFY `save_x` smallint(11) unsigned NOT NULL default '53';
ALTER TABLE `char` MODIFY `save_y` smallint(11) unsigned NOT NULL default '111';

ALTER TABLE `party` MODIFY `party_id` int(11) unsigned NOT NULL auto_increment;
ALTER TABLE `guild` MODIFY `guild_id` int(11) unsigned NOT NULL auto_increment;

ALTER TABLE `char` MODIFY `karma` tinyint(3) NOT NULL default '0';
ALTER TABLE `char` MODIFY `manner` tinyint(3) NOT NULL default '0';

update `char` set `class`='7' where `class`='13';
update `char` set `class`='14' where `class`='21';
update `char` set `class`='4008' where `class`='4014';
update `char` set `class`='4015' where `class`='4022';
update `char` set `class`='4030' where `class`='4036';
update `char` set `class`='4037' where `class`='4044';
update `char` set `class`='4047' where `class`='4048';
update `guild_member` set `class`='7' where `class`='13';
update `guild_member` set `class`='14' where `class`='21';
update `guild_member` set `class`='4008' where `class`='4014';
update `guild_member` set `class`='4015' where `class`='4022';
update `guild_member` set `class`='4030' where `class`='4036';
update `guild_member` set `class`='4037' where `class`='4044';
update `guild_member` set `class`='4047' where `class`='4048';

ALTER TABLE `mob_db2`
 CHANGE COLUMN `Name` `Sprite` TEXT NOT NULL DEFAULT '',
 CHANGE COLUMN `Name2` `kName` TEXT NOT NULL DEFAULT '',
 ADD COLUMN `iName` TEXT NOT NULL DEFAULT '' AFTER `kName`,
 CHANGE COLUMN `MEXP` `MEXP` MEDIUMINT(9) NOT NULL AFTER `dMotion`,
 CHANGE COLUMN `ExpPer` `ExpPer` SMALLINT(9) NOT NULL AFTER `MEXP`,
 CHANGE COLUMN `MVP1id` `MVP1id` SMALLINT(9) NOT NULL AFTER `ExpPer`,
 CHANGE COLUMN `MVP1per` `MVP1per` SMALLINT(9) NOT NULL AFTER `MVP1id`,
 CHANGE COLUMN `MVP2id` `MVP2id` SMALLINT(9) NOT NULL AFTER `MVP1per`,
 CHANGE COLUMN `MVP2per` `MVP2per` SMALLINT(9) NOT NULL AFTER `MVP2id`,
 CHANGE COLUMN `MVP3id` `MVP3id` SMALLINT(9) NOT NULL AFTER `MVP2per`,
 CHANGE COLUMN `MVP3per` `MVP3per` SMALLINT(9) NOT NULL AFTER `MVP3id`;

UPDATE `mob_db2` SET `iName` = `kName`;

ALTER TABLE `char` ADD INDEX ( `account_id` );

ALTER TABLE `loginlog` ADD INDEX ( `ip` );
ALTER TABLE `ipbanlist` ADD INDEX ( `list` );

UPDATE `loginlog` SET `ip` = inet_aton(`ip`);
ALTER TABLE `loginlog` CHANGE `ip` `ip` INT( 10 ) UNSIGNED ZEROFILL NOT NULL DEFAULT '0';

ALTER TABLE `char` MODIFY `party_id` int(11) unsigned NOT NULL default '0';
ALTER TABLE `char` MODIFY `guild_id` int(11) unsigned NOT NULL default '0';

update guild set name=REPLACE(name,'\\\'','\'');

#Delete 2^13 (Peco Knight)
update item_db set equip_jobs = equip_jobs&~0x2000 where equip_jobs&0x2000;
#Move 2^20 -> 2^19 (Dancer -> Bard)
update item_db set equip_jobs = (equip_jobs|0x80000)&~0x100000 where equip_jobs&0x100000;
#Remove 2^21 (Peco Crusader)
update item_db set equip_jobs = equip_jobs&~0x200000 where equip_jobs&0x200000;
#Remove 2^22 (Wedding)
update item_db set equip_jobs = equip_jobs&~0x400000 where equip_jobs&0x400000;
#Remove 2^23 (S. Novice)
update item_db set equip_jobs = equip_jobs&~0x800000 where equip_jobs&0x800000;
#Move 2^24 -> 2^21 (TK)
update item_db set equip_jobs = (equip_jobs|0x200000)&~0x1000000 where equip_jobs&0x1000000;
#Move 2^25 -> 2^22 (SG)
update item_db set equip_jobs = (equip_jobs|0x400000)&~0x2000000 where equip_jobs&0x2000000;
#Move 2^26 -> 2^23 (SL)
update item_db set equip_jobs = (equip_jobs|0x800000)&~0x8000000 where equip_jobs&0x8000000;
#Move 2^28 -> 2^24 (GS)
update item_db set equip_jobs = (equip_jobs|0x1000000)&~0x4000000 where equip_jobs&0x4000000;
#Move 2^27 -> 2^25 (NJ)
update item_db set equip_jobs = (equip_jobs|0x2000000)&~0x10000000 where equip_jobs&0x10000000;
#Make items usable by everyone into 0xFFFFFFFF
update item_db set equip_jobs = 0xFFFFFFFF where equip_jobs = 0x3EFDFFF;
#Make items usable by everyone but novice into 0xFFFFFFFE
update item_db set equip_jobs = 0xFFFFFFFE where equip_jobs = 0x0EFDFFE;
#Update items usable by everyone except acolyte/priest/monk/gunslinger
update item_db set equip_jobs = 0xFDFF7EEF where equip_jobs = 0x28F5EEF;