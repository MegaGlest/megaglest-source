<?php
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke and Tom Reynolds
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

	define( 'INCLUSION_PERMITTED', true );
	require_once( 'config.php' );
	require_once( 'functions.php' );

	// Consider using HTTP POST instead of HTTP GET here, data should always be sent via POST for privacy and security reasons
	// Alternatively, do not retrieve (and transmit) this data at all via HTTP (other than the IP address the game servers advertises) but fetch it from the game server instead

	// general info:
	$glestVersion      = (string) clean_str( $_GET['glestVersion'] );
	$platform          = (string) clean_str( $_GET['platform'] );
	$binaryCompileDate = (string) clean_str( $_GET['binaryCompileDate'] );
	if ( isset( $_GET['privacyPlease'] ) ) {
		$privacyPlease = (int) $_GET['privacyPlease'];
	}
	else
	{
		$privacyPlease = 0;
	}

	// game info:
	$serverTitle       = (string) clean_str( $_GET['serverTitle'] );
	$remote_ip         = (string) clean_str( $_SERVER['REMOTE_ADDR'] );

	// If the clients' IP address belongs to a RFC1918 IP range...
	if ( strncmp( $remote_ip, get_localsubnet_ip_prefix(), strlen( get_localsubnet_ip_prefix() ) ) == 0 )
	{
		// ...then replace it by the master servers' public IP address.
		$remote_ip = get_external_ip();
	}

	$service_port      = (int)    clean_str( $_GET['externalconnectport'] );

	// If the game server port was not transmitted...
	if ( $service_port == '' || $service_port == 0 ) 
	{
		// ..then assume the default port
		$service_port = 61357;
		// ... alternatively, refuse such servers
		/*
		header( 'Content-Type: text/plain; charset=utf-8' );
		die( 'Invalid external connect port.');
		*/
	}	

	
	// game setup info:
	$tech              = (string) clean_str( $_GET['tech'] );
	$map               = (string) clean_str( $_GET['map'] );
	$tileset           = (string) clean_str( $_GET['tileset'] );
	$activeSlots       = (int)    clean_str( $_GET['activeSlots'] );
	$networkSlots      = (int)    clean_str( $_GET['networkSlots'] );
	$connectedClients  = (int)    clean_str( $_GET['connectedClients'] );
	
	$status = 0;
    	if(isset($_GET["gameStatus"])) {
        	$status  	   = (int)    clean_str( $_GET['gameStatus'] );
	}
    
	$gameCmd = "";
    	if(isset($_GET["gameCmd"])) {
        	$gameCmd = (string)    clean_str( $_GET['gameCmd'] );
	}
	
	define( 'DB_LINK', db_connect() );

	// consider replacing this by a cron job
	cleanupServerList();

	$server_in_db = @mysql_query( 'SELECT ip, externalServerPort FROM glestserver WHERE ip=\'' . mysql_real_escape_string( $remote_ip ) . '\' AND externalServerPort=\'' . mysql_real_escape_string( $service_port ) . '\';' );
       	$server       = @mysql_fetch_row( $server_in_db );


	// Representation starts here (but it should really be starting much later, there is way too much logic behind this point)
	header( 'Content-Type: text/plain; charset=utf-8' );

	if ( (version_compare($glestVersion,"v3.4.0-dev","<") && $connectedClients == $networkSlots)  || $gameCmd == "gameOver")   // game servers' slots are all full
	{ // delete server; no checks are performed
		mysql_query( 'DELETE FROM glestserver WHERE ip=\'' . mysql_real_escape_string( $remote_ip ) . '\' && externalServerPort=\'' . mysql_real_escape_string( $service_port ) . '\';' );
		echo 'OK' ;	
	}
	else if ( $remote_ip == $server[0] && $service_port == $server[1] )    // this server is contained in the database
	{ // update database info on this game server; no checks are performed
		mysql_query( 'UPDATE glestserver SET ' .
			'glestVersion=\''      . mysql_real_escape_string( $glestVersion )      . '\', ' .
			'platform=\''          . mysql_real_escape_string( $platform )          . '\', ' .
			'binaryCompileDate=\'' . mysql_real_escape_string( $binaryCompileDate ) . '\', ' .
			'serverTitle=\''       . mysql_real_escape_string( $serverTitle )       . '\', ' .
			'tech=\''              . mysql_real_escape_string( $tech )              . '\', ' .
			'map=\''               . mysql_real_escape_string( $map )               . '\', ' .
			'tileset=\''           . mysql_real_escape_string( $tileset )           . '\', ' .
			'activeSlots=\''       . mysql_real_escape_string( $activeSlots )       . '\', ' .
			'networkSlots=\''      . mysql_real_escape_string( $networkSlots )      . '\', ' .
			'connectedClients=\''  . mysql_real_escape_string( $connectedClients )  . '\', ' .
			'externalServerPort=\''. mysql_real_escape_string( $service_port )      . '\', ' .
			'status=\''            . mysql_real_escape_string( $status )            . '\', ' .
			'lasttime='            . 'now()'                                        .    ' ' .
			'where ip=\'' . mysql_real_escape_string( $remote_ip ) . '\' && externalServerPort=\'' . mysql_real_escape_string( $service_port ) . '\';' );
		//updateServer($remote_ip, $service_port, $serverTitle, $connectedClients, $networkSlots);
		echo 'OK';
	}
	else                                        // this game server is not listed in the database, yet
	{ // check whether this game server is available from the Internet; if it is, add it to the database
		sleep(8); // was 3
		$socket = socket_create( AF_INET, SOCK_STREAM, SOL_TCP );
		if ( $socket < 0 ) {
		    echo 'socket_create() failed.' . PHP_EOL . ' Reason: ' . socket_strerror( $socket ) . PHP_EOL;
		} 
		socket_set_nonblock( $socket )
	      		or die( 'Unable to set nonblock on socket.' );
	      
	        $timeout = 10;  //timeout in seconds
		echo 'Trying to connect to \'' . $remote_ip . '\' using port \'' . $service_port . '\'...' . PHP_EOL;
		
		$canconnect = true;
		$time = time();
		error_reporting( E_ERROR );
		
		for ( ; !@socket_connect( $socket, $remote_ip, $service_port ); )
	    	{
	      		$socket_last_error = socket_last_error( $socket );
	      		if ( $socket_last_error == 115 || $socket_last_error == 114)
	      		{
	        		if ( ( time() - $time ) >= $timeout )
	        		{
	          			$canconnect = false;
	          			echo 'socket_connect() failed.' . PHP_EOL . ' Reason: (' . $socket_last_error . ') ' . socket_strerror( $socket_last_error ) . PHP_EOL;
	          			break;
	        		}
	        		sleep( 1 );
	        		continue;
	      		}
	      		// for answers on this see: http://bobobobo.wordpress.com/2008/11/09/resolving-winsock-error-10035-wsaewouldblock/
	      		else if($socket_last_error == 10035 || $socket_last_error == 10037) {
	      			break;
	      		}
	      		
	      		$canconnect = false;
	        	echo 'socket_connect() failed.' . PHP_EOL . ' Reason: (' . $socket_last_error . ') ' . socket_strerror( $socket_last_error ) . PHP_EOL;
	          	break;
	    	}	
		
		socket_set_block( $socket )
      			or die( 'Unable to set block on socket.' );

		//echo "and now read ....";
		//$buf = socket_read($socket, 161);
		//echo $buf ."\n";

		// Make sure its a glest server connecting
		//
		// struct Data{
		//	int8 messageType;
		//	NetworkString<maxVersionStringSize> versionString;
		//	NetworkString<maxNameSize> name;
		//	int16 playerIndex;
		//	int8 gameState;
		// };
		
		
		if ( $canconnect == true ) {
			$data_from_server = socket_read( $socket, 1 );
		}
		

		socket_close( $socket );
		
		error_reporting( E_ALL );

		if ( $canconnect == false )
       		{
			echo 'wrong router setup';
		}
 		/*
		else if ( $data_from_server != 1 )   // insert serious verification here
		{
			echo "invalid handshake!";
		}
		*/
		else  // connection to game server succeeded, protocol verification succeeded
		{ // add this game server to the database
			if ( extension_loaded('geoip') ) {
					
				if ( $privacyPlease == 0 )
				{
					$country = geoip_country_code_by_name( $remote_ip );
				}
				else
				{
					$country = '';
				}
			}
			// cleanup old entrys with same remote port and ip
			// I hope this fixes those double entrys of servers
			mysql_query( 'DELETE FROM glestserver WHERE '.
			'externalServerPort=\''. mysql_real_escape_string( $service_port )      . '\', ' .
			' AND ' .
			'ip=\''                . mysql_real_escape_string( $remote_ip )         . '\', '
			);
			// insert new entry
			mysql_query( 'INSERT INTO glestserver SET ' .
				'glestVersion=\''      . mysql_real_escape_string( $glestVersion )      . '\', ' .
				'platform=\''          . mysql_real_escape_string( $platform )          . '\', ' .
				'binaryCompileDate=\'' . mysql_real_escape_string( $binaryCompileDate ) . '\', ' .
				'serverTitle=\''       . mysql_real_escape_string( $serverTitle )       . '\', ' .
				'ip=\''                . mysql_real_escape_string( $remote_ip )         . '\', ' .
				'tech=\''              . mysql_real_escape_string( $tech )              . '\', ' .
				'map=\''               . mysql_real_escape_string( $map )               . '\', ' .
				'tileset=\''           . mysql_real_escape_string( $tileset )           . '\', ' .
				'activeSlots=\''       . mysql_real_escape_string( $activeSlots )       . '\', ' .
				'networkSlots=\''      . mysql_real_escape_string( $networkSlots )      . '\', ' .
				'connectedClients=\''  . mysql_real_escape_string( $connectedClients )  . '\', ' .
				'externalServerPort=\''. mysql_real_escape_string( $service_port )      . '\', ' .
				'country=\''           . mysql_real_escape_string( $country )           . '\', ' .
				'status=\''            . mysql_real_escape_string( $status )            . '\';'  
			);
			echo 'OK';
			//addLatestServer($remote_ip, $service_port, $serverTitle, $connectedClients, $networkSlots);
		}
	}
	db_disconnect( DB_LINK );
?>
