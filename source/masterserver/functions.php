<?php
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke and Tom Reynolds
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

	if ( !defined('INCLUSION_PERMITTED') || ( defined('INCLUSION_PERMITTED') && INCLUSION_PERMITTED !== true ) ) { die( 'This file must not be invoked directly.' ); }

	# This function cleans out special characters
	function clean_str( $text )
	{  // tomreyn says: I'm afraid this function is more likely to cause to trouble than to fix stuff (you have mysql escaping and html escaping elsewhere where it makes more sense, but strip off < and > here already, but then you don't filter non-visible bytes here)
		//$text=strtolower($text);
		//$code_entities_match   = array('!','@','#','$','%','^','&','*','(',')','_','+','{','}','|','"','<','>','?','[',']','\\',';',"'",',','/','*','+','~','`','=');
		//$code_entities_replace = array('','','','','','','','','','','','','','','','','','','','','');
		$code_entities_match   = array('$','%','^','&','_','+','{','}','|','"','<','>','?','[',']','\\',';',"'",'/','+','~','`','=');
		$code_entities_replace = array('','','','','','','','','','','','','');
        
		$text = str_replace( $code_entities_match, $code_entities_replace, $text );
		return $text;
	}

	function db_connect()
	{
		// If we may use persistent MYSQL database server links...
		if ( defined( 'MYSQL_LINK_PERSIST' ) && MYSQL_LINK_PERSIST === true ) 
		{
			// ...then reuse an existing link or create a new one, ...
			$linkid = mysql_pconnect( MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD );
		}
		else
		{
			// ...otherwise create a standard link
			$linkid = mysql_connect( MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD );
		}
		mysql_select_db( MYSQL_DATABASE );
		return $linkid;
	}

	function db_disconnect( $linkid )
	{
		// note that mysql_close() only closes non-persistent connections
		return mysql_close( $linkid );
	}

	function get_localsubnet_ip_prefix()
	{
		// If this is supposed to match any RFC1918 or even any unroutable IP address space then this is far from complete. 
		// Consider using something like the 'IANA Private list' provided at http://sites.google.com/site/blocklist/ instead. 
		// The data in this list is a subset of what IANA makes available at http://www.iana.org/assignments/ipv4-address-space/ipv4-address-space.xhtml
		// Note that you may want/need to add in the IPv6 equivalent, too: http://www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xhtml
		return "192.";
	}

	function get_external_ip()
	{
		return $_SERVER['SERVER_ADDR'];
		//return "209.52.70.192";
	}

	function cleanupServerList()
	{
		// on a busy server, this function should be invoked by cron in regular intervals instead (one SQL query less for the script)
		return mysql_query( 'DELETE FROM glestserver WHERE lasttime<DATE_add(NOW(), INTERVAL -1 minute);' );
		//return mysql_query( 'UPDATE glestserver SET status=\'???\' WHERE lasttime<DATE_add(NOW(), INTERVAL -2 minute);' );
	}

	function addLatestServer($remote_ip, $service_port, $serverTitle, $connectedClients, $networkSlots )
	{
		// insert the new server
		$server = "$remote_ip:$service_port";
		$players = "$connectedClients/$networkSlots";
		mysql_query( "INSERT INTO recent_servers (name, server, players) VALUES('$serverTitle', '$server', '$players')");

		// make sure there are not too much servers
		$count_query = mysql_fetch_assoc(mysql_query( "SELECT COUNT(*) as count FROM recent_servers"));
		$count = (int) $count_query['count'];
		$over = $count - MAX_RECENT_SERVERS;
		mysql_query( "DELETE FROM recent_servers ORDER BY id LIMIT $over");
	}

	function updateServer($remote_ip, $service_port, $serverTitle, $connectedClients, $networkSlots ) {
		// find the id of the server to update
		$server = "$remote_ip:$service_port";
		$players = "$connectedClients/$networkSlots";
		$find_query = mysql_fetch_assoc(mysql_query( "SELECT id FROM recent_servers WHERE server ='$server' ORDER BY id desc LIMIT 1"));
		$id = (int) $find_query['id'];

		// update it.
		mysql_query( "UPDATE recent_servers SET name='$serverTitle', players='$players' WHERE id=$id LIMIT 1");
	}

?>
