CREATE TABLE IF NOT EXISTS `war_of_treasure` (
	`char_id` INT(11) PRIMARY KEY NOT NULL DEFAULT '0',
	`name` VARCHAR(24) NOT NULL DEFAULT '',
	`guild_id` MEDIUMINT(8) NOT NULL DEFAULT '0',
	`class` MEDIUMINT(8) NOT NULL DEFAULT '0',
	`points` SMALLINT(5) NOT NULL DEFAULT '0',

	KEY `name` (`name`),
	KEY `points` (`points`)
) ENGINE=MYISAM;