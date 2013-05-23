UPDATE `char` SET `last_map` = substring_index(`last_map`, '.', 1);
UPDATE `char` SET `save_map` = substring_index(`save_map`, '.', 1);
UPDATE `memo` SET `map` = substring_index(`map`, '.', 1);

ALTER TABLE `picklog` ADD INDEX (`type`);
ALTER TABLE `zenylog` ADD INDEX (`type`);
ALTER TABLE `branchlog` ADD INDEX (`account_id`), ADD INDEX (`char_id`);
ALTER TABLE `atcommandlog` ADD INDEX (`account_id`), ADD INDEX (`char_id`);
ALTER TABLE `npclog` ADD INDEX (`account_id`), ADD INDEX (`char_id`);
ALTER TABLE `chatlog` ADD INDEX (`src_accountid`), ADD INDEX (`src_charid`);

ALTER TABLE `item_db` modify `view` smallint(3) unsigned default null;
ALTER TABLE `item_db2` modify `view` smallint(3) unsigned default null;

ALTER TABLE `picklog` MODIFY `type` set('M','P','L','T','V','S','N','C','A','R','G') NOT NULL default 'P';

ALTER TABLE `login` CHANGE `email` `email` VARCHAR( 39 ) NOT NULL;
ALTER TABLE `login` CHANGE `sex` `sex` ENUM( 'M', 'F', 'S' ) NOT NULL DEFAULT 'M';

ALTER TABLE `char` CHANGE `last_map` `last_map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `char` CHANGE `save_map` `save_map` VARCHAR( 11 ) NOT NULL;

ALTER TABLE `memo` CHANGE `map` `map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `memo` CHANGE `x` `x` SMALLINT( 4 ) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `memo` CHANGE `y` `y` SMALLINT( 4 ) UNSIGNED NOT NULL DEFAULT '0';

ALTER TABLE `party` CHANGE `name` `name` VARCHAR( 24 ) NOT NULL;

ALTER TABLE `picklog` CHANGE `type` `type` ENUM( 'M', 'P', 'L', 'T', 'V', 'S', 'N', 'C', 'A', 'R', 'G' ) NOT NULL DEFAULT 'P';
ALTER TABLE `zenylog` CHANGE `type` `type` ENUM( 'M', 'T', 'V', 'S', 'N', 'A' ) NOT NULL DEFAULT 'S';
ALTER TABLE `chatlog` CHANGE `type` `type` ENUM( 'O', 'W', 'P', 'G', 'M' ) NOT NULL DEFAULT 'O';

ALTER TABLE `picklog` CHANGE `map` `map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `zenylog` CHANGE `map` `map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `branchlog` CHANGE `map` `map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `mvplog` CHANGE `map` `map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `atcommandlog` CHANGE `map` `map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `npclog` CHANGE `map` `map` VARCHAR( 11 ) NOT NULL;
ALTER TABLE `chatlog` CHANGE `src_map` `src_map` VARCHAR( 11 ) NOT NULL;

DROP TABLE IF EXISTS `hotkey`;
CREATE TABLE `hotkey` (
	`char_id` INT(11) NOT NULL,
	`hotkey` TINYINT(2) unsigned NOT NULL,
	`type` TINYINT(1) unsigned NOT NULL default '0',
	`itemskill_id` INT(11) unsigned NOT NULL default '0',
	`skill_lvl` TINYINT(4) unsigned NOT NULL default '0',
	PRIMARY KEY (`char_id`,`hotkey`),
	INDEX (`char_id`)
) TYPE=MYISAM;