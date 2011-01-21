---
--- Table structure and sample data for the MegaGlest master server
---
--- SQL should be compatible to MySQL 5.0
---

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
  KEY `lasttime` (`lasttime`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

INSERT INTO `glestserver` (`lasttime`, `glestVersion`, `platform`, `binaryCompileDate`, `serverTitle`, `ip`, `tech`, `map`, `tileset`, `activeSlots`, `networkSlots`, `connectedClients` ,`externalServerPort`, `country`) VALUES
('2010-05-12 01:41:43', '', '', '', '', '', '', '', '', 0, 0, 0, 0, 'DE');
