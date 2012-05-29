-- 
-- Table structure and sample data for the MegaGlest master server
-- 
-- SQL should be compatible to MySQL 5.0
-- 

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

CREATE TABLE IF NOT EXISTS `glestserver` (
  `lasttime` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `glestVersion` varchar(30) collate utf8_unicode_ci NOT NULL,
  `platform` varchar(30) collate utf8_unicode_ci NOT NULL,
  `binaryCompileDate` varchar(30) collate utf8_unicode_ci NOT NULL,
  `serverTitle` varchar(100) collate utf8_unicode_ci NOT NULL,
  `ip` varchar(15) collate utf8_unicode_ci NOT NULL,
  `tech` varchar(100) collate utf8_unicode_ci NOT NULL,
  `map` varchar(100) collate utf8_unicode_ci NOT NULL,
  `tileset` varchar(100) collate utf8_unicode_ci NOT NULL,
  `activeSlots` int(11) NOT NULL,
  `networkSlots` int(11) NOT NULL,
  `connectedClients` int(11) NOT NULL,
  `externalServerPort` int(11) NOT NULL,
  `country` varchar(2) collate utf8_unicode_ci NOT NULL,
  `status` int(11) NOT NULL default 0, -- valid statuses: 0 waiting for players, 1 = game full pending start, 2 game in progress, 3 game over
  KEY `lasttime` (`lasttime`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `glestserver` (`lasttime`, `glestVersion`, `platform`, `binaryCompileDate`, `serverTitle`, `ip`, `tech`, `map`, `tileset`, `activeSlots`, `networkSlots`, `connectedClients` ,`externalServerPort`, `country`) VALUES
('2010-05-12 01:41:43', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 'DE');


CREATE TABLE IF NOT EXISTS `glestmaps` (
  `updatetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `glestversion` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `mapname` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `playercount` int(11) NOT NULL,
  `crc` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `description` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  `url` varchar(1024) COLLATE utf8_unicode_ci NOT NULL,
  `imageUrl` varchar(1024) COLLATE utf8_unicode_ci DEFAULT NULL,
  `disabled` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`mapname`),
  KEY `mapname` (`mapname`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `glesttilesets` (
  `updatetime` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `glestversion` varchar(30) collate utf8_unicode_ci NOT NULL,
  `tilesetname` varchar(100) collate utf8_unicode_ci NOT NULL,
  `crc` varchar(100) collate utf8_unicode_ci NOT NULL,
  `description` varchar(255) collate utf8_unicode_ci,
  `url` varchar(1024) collate utf8_unicode_ci NOT NULL,
  `imageUrl` varchar(1024) COLLATE utf8_unicode_ci DEFAULT NULL,
  `disabled` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`tilesetname`),
  KEY `tilesetname` (`tilesetname`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

CREATE TABLE IF NOT EXISTS `glesttechs` (
  `updatetime` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `glestversion` varchar(30) collate utf8_unicode_ci NOT NULL,
  `techname` varchar(100) collate utf8_unicode_ci NOT NULL,
  `factioncount` int(11) NOT NULL,
  `crc` varchar(100) collate utf8_unicode_ci NOT NULL,
  `description` varchar(255) collate utf8_unicode_ci,
  `url` varchar(1024) collate utf8_unicode_ci NOT NULL,
  `imageUrl` varchar(1024) COLLATE utf8_unicode_ci DEFAULT NULL,
  `disabled` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`techname`),
  KEY `techname` (`techname`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;


CREATE TABLE IF NOT EXISTS `glestscenarios` (
  `updatetime` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `glestversion` varchar(30) collate utf8_unicode_ci NOT NULL,
  `scenarioname` varchar(100) collate utf8_unicode_ci NOT NULL,
  `crc` varchar(100) collate utf8_unicode_ci NOT NULL,
  `description` varchar(255) collate utf8_unicode_ci,
  `url` varchar(1024) collate utf8_unicode_ci NOT NULL,
  `imageUrl` varchar(1024) COLLATE utf8_unicode_ci DEFAULT NULL,
  `disabled` int(11) NOT NULL DEFAULT 0,
  PRIMARY KEY (`scenarioname`),
  KEY `scenarioname` (`scenarioname`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

