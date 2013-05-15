CREATE TABLE IF NOT EXISTS `sala_cristais` (
	`name` VARCHAR(24) PRIMARY KEY NOT NULL DEFAULT '',
	`kills` SMALLINT(5) NOT NULL DEFAULT '0',
	`first` INT(11) NOT NULL DEFAULT '0',
	KEY `kills` (`kills`),
	KEY `first` (`first`)
) ENGINE=MyISAM;