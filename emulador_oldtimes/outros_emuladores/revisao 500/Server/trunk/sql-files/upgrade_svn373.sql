ALTER TABLE `guild` DROP COLUMN `castle_id`;

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

#ZenyLog types (M)onsters,(T)rade,(V)ending Sell/Buy,(S)hop Sell/Buy,(N)PC Change amount,(A)dministrators
#Database: log
#Table: zenylog
CREATE TABLE `zenylog` (
  `id` int(11) NOT NULL auto_increment,
  `time` datetime NOT NULL default '0000-00-00 00:00:00',
  `char_id` int(11) NOT NULL default '0',
  `src_id` int(11) NOT NULL default '0',
  `type` set('M','T','V','S','N','A') NOT NULL default 'S',
  `amount` int(11) NOT NULL default '0',
  `map` varchar(20) NOT NULL default 'prontera.gat',
  PRIMARY KEY  (`id`)
) TYPE=MyISAM AUTO_INCREMENT=1 ;

# Upgrade login and character server tables for eAthena 0.5.2 to 1.0 RC 2

ALTER TABLE `cart_inventory` ADD `broken` int(11) NOT NULL default '0';

ALTER TABLE `char` MODIFY `base_level` bigint(20) unsigned NOT NULL default '1';
ALTER TABLE `char` MODIFY `job_level` bigint(20) unsigned NOT NULL default '1';
ALTER TABLE `char` MODIFY `base_exp` bigint(20) NOT NULL default '0';
ALTER TABLE `char` MODIFY `job_exp` bigint(20) NOT NULL default '0';
ALTER TABLE `char` ADD `partner_id` int(11) NOT NULL default '0';

ALTER TABLE `charlog` MODIFY `str` int(11) unsigned NOT NULL default '0';
ALTER TABLE `charlog` MODIFY `agi` int(11) unsigned NOT NULL default '0';
ALTER TABLE `charlog` MODIFY `vit` int(11) unsigned NOT NULL default '0';
ALTER TABLE `charlog` MODIFY `int` int(11) unsigned NOT NULL default '0';
ALTER TABLE `charlog` MODIFY `dex` int(11) unsigned NOT NULL default '0';
ALTER TABLE `charlog` MODIFY `luk` int(11) unsigned NOT NULL default '0';

ALTER TABLE `global_reg_value` MODIFY `value` varchar(255) NOT NULL default '0';
ALTER TABLE `global_reg_value` ADD PRIMARY KEY (`char_id`, `str`, `account_id`);
ALTER TABLE `global_reg_value` DROP INDEX `account_id`;
ALTER TABLE `global_reg_value` ADD INDEX `account_id` (`account_id`);

ALTER TABLE `guild_castle` ADD `gHP0` int(11) NOT NULL default '0';
ALTER TABLE `guild_castle` ADD `gHP1` int(11) NOT NULL default '0';
ALTER TABLE `guild_castle` ADD `gHP2` int(11) NOT NULL default '0';
ALTER TABLE `guild_castle` ADD `gHP3` int(11) NOT NULL default '0';
ALTER TABLE `guild_castle` ADD `gHP4` int(11) NOT NULL default '0';
ALTER TABLE `guild_castle` ADD `gHP5` int(11) NOT NULL default '0';
ALTER TABLE `guild_castle` ADD `gHP6` int(11) NOT NULL default '0';
ALTER TABLE `guild_castle` ADD `gHP7` int(11) NOT NULL default '0';

ALTER TABLE `guild_member` MODIFY `exp` bigint(20) NOT NULL default '0';

ALTER TABLE `guild_storage` ADD `broken` int(11) NOT NULL default '0';

ALTER TABLE `inventory` ADD `broken` int(11) NOT NULL default '0';

ALTER TABLE `login` MODIFY `account_id` int(11) NOT NULL default '0' AUTO_INCREMENT, AUTO_INCREMENT=1000057;
ALTER TABLE `login` MODIFY `userid` varchar(255) NOT NULL default '';
ALTER TABLE `login` MODIFY `user_pass` varchar(32) NOT NULL default '';
ALTER TABLE `login` ADD `error_message` int(11) NOT NULL default '0';
ALTER TABLE `login` ADD `connect_until` int(11) NOT NULL default '0';
ALTER TABLE `login` ADD `last_ip` varchar(100) NOT NULL default '';
ALTER TABLE `login` ADD `memo` int(11) NOT NULL default '0';
ALTER TABLE `login` ADD `ban_until` int(11) NOT NULL default '0';
ALTER TABLE `login` ADD `state` int(11) NOT NULL default '0';
ALTER TABLE `login` DROP INDEX `account_id`;
ALTER TABLE `login` DROP INDEX `userid`;
ALTER TABLE `login` ADD PRIMARY KEY (`account_id`), ADD INDEX `name` (`userid`);

CREATE TABLE `login_error` (
  `err_id` int(11) NOT NULL default '0',
  `reason` varchar(100) NOT NULL default 'Unknown',
  PRIMARY KEY  (`err_id`)
) TYPE=MyISAM; 

ALTER TABLE `ragsrvinfo` DROP `type`, DROP `text1`, DROP `text2`, DROP `text3`, DROP `text4`;
ALTER TABLE `ragsrvinfo` ADD `motd` varchar(255) NOT NULL default '';

ALTER TABLE `skill` ADD PRIMARY KEY (`char_id`,`id`);

ALTER TABLE `storage` ADD `broken` int(11) NOT NULL default '0';

ALTER TABLE `friends` ADD `friend_id0` int(11) NOT NULL default '0', ADD `name0` varchar(255) NOT NULL default '';