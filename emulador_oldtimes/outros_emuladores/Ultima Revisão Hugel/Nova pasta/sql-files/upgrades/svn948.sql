ALTER TABLE `login` MODIFY `connect_until` int(11) unsigned NOT NULL default '0';

ALTER TABLE `guild` MODIFY `emblem_data` blob;

ALTER TABLE `friends` ADD INDEX ( `char_id` );
ALTER TABLE `memo` ADD INDEX ( `char_id` );
ALTER TABLE `char` ADD INDEX ( `name` );
ALTER TABLE `char` ADD INDEX ( `online` );