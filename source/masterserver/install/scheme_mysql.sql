--
-- Table structure for table `glestmaps`
--

DROP TABLE IF EXISTS `glestmaps`;
CREATE TABLE `glestmaps` (
  `updatetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `glestversion` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `mapname` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `playercount` int(11) NOT NULL,
  `crc` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `description` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  `url` varchar(1024) COLLATE utf8_unicode_ci NOT NULL,
  `imageUrl` varchar(1024) COLLATE utf8_unicode_ci DEFAULT NULL,
  `disabled` int(11) NOT NULL DEFAULT '0',
  `crcnew` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`mapname`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Table structure for table `glestscenarios`
--

DROP TABLE IF EXISTS `glestscenarios`;
CREATE TABLE `glestscenarios` (
  `updatetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `glestversion` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `scenarioname` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `crc` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `description` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  `url` varchar(1024) COLLATE utf8_unicode_ci NOT NULL,
  `imageUrl` varchar(1024) COLLATE utf8_unicode_ci DEFAULT NULL,
  `disabled` int(11) NOT NULL DEFAULT '0',
  `crcnew` varchar(100) COLLATE utf8_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`scenarioname`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Table structure for table `glestserver`
--

DROP TABLE IF EXISTS `glestserver`;
CREATE TABLE `glestserver` (
  `lasttime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `glestVersion` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `platform` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `binaryCompileDate` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `serverTitle` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `ip` varchar(15) COLLATE utf8_unicode_ci NOT NULL,
  `tech` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `map` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `tileset` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `activeSlots` int(11) NOT NULL,
  `networkSlots` int(11) NOT NULL,
  `connectedClients` int(11) NOT NULL,
  `externalServerPort` int(11) NOT NULL,
  `country` varchar(2) COLLATE utf8_unicode_ci NOT NULL,
  `status` int(11) NOT NULL DEFAULT '0',
  KEY `lasttime` (`lasttime`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Table structure for table `glesttechs`
--

DROP TABLE IF EXISTS `glesttechs`;
CREATE TABLE `glesttechs` (
  `updatetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `glestversion` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `techname` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `factioncount` int(11) NOT NULL,
  `crc` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `description` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  `url` varchar(1024) COLLATE utf8_unicode_ci NOT NULL,
  `imageUrl` varchar(1024) COLLATE utf8_unicode_ci DEFAULT NULL,
  `disabled` int(11) NOT NULL DEFAULT '0',
  `crcnew` varchar(100) COLLATE utf8_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`techname`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Table structure for table `glesttilesets`
--

DROP TABLE IF EXISTS `glesttilesets`;
CREATE TABLE `glesttilesets` (
  `updatetime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `glestversion` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `tilesetname` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `crc` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `description` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  `url` varchar(1024) COLLATE utf8_unicode_ci NOT NULL,
  `imageUrl` varchar(1024) COLLATE utf8_unicode_ci DEFAULT NULL,
  `disabled` int(11) NOT NULL DEFAULT '0',
  `crcnew` varchar(100) COLLATE utf8_unicode_ci DEFAULT NULL,
  PRIMARY KEY (`tilesetname`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Table structure for table `recent_servers`
--

DROP TABLE IF EXISTS `recent_servers`;
CREATE TABLE `recent_servers` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `server` varchar(25) COLLATE utf8_unicode_ci NOT NULL,
  `name` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `players` varchar(10) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=550 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
