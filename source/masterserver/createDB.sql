-- phpMyAdmin SQL Dump
-- version 2.11.9.6
-- http://www.phpmyadmin.net
--
-- Host: mysql.goneo.de
-- Erstellungszeit: 12. Mai 2010 um 03:08
-- Server Version: 5.0.90
-- PHP-Version: 4.4.9

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

--
-- Datenbank: `17632m6025_1`
--

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `glestserver`
--

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
  KEY `lasttime` (`lasttime`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

--
-- Daten für Tabelle `glestserver`
--

INSERT INTO `glestserver` (`lasttime`, `glestVersion`, `platform`, `binaryCompileDate`, `serverTitle`, `ip`, `tech`, `map`, `tileset`, `activeSlots`, `networkSlots`, `connectedClients`,`externalServerPort`) VALUES
('2010-05-12 01:41:43', '', '', '', '', '', '', '', '', 0, 0, 0, 0);

