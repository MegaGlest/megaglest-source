<?php
	define( 'INCLUSION_PERMITTED', true );
	require_once( 'config.php' );
	require_once( 'functions.php' );

	define( 'DB_LINK', db_connect() );

	// consider replacing this by a cron job
	cleanupServerList();

	$servers_in_db = mysql_query( 'SELECT * FROM glestserver ORDER BY status, connectedClients>0 DESC, (networkSlots - connectedClients) , ip DESC;' );
	$all_servers = array();
	while ( $server = mysql_fetch_array( $servers_in_db ) )
	{
		array_push( $all_servers, $server );
	}
	unset( $servers_in_db );
	unset( $server );

	db_disconnect( DB_LINK );

	// Representation starts here
	header( 'Content-Type: text/plain; charset=utf-8' );
	foreach( $all_servers as &$server )
	{
		$outString =
			"${server['glestVersion']}|${server['platform']}|${server['binaryCompileDate']}|${server['serverTitle']}|${server['ip']}|${server['tech']}|${server['map']}|${server['tileset']}|${server['activeSlots']}|${server['networkSlots']}|${server['connectedClients']}|${server['externalServerPort']}|";

		if ( $server['country'] !== '' ) 
		{
			$outString = $outString . "${server['country']}|";
		}
		else 
		{
			$outString = $outString . DEFAULT_COUNTRY . "|";
		}

		$calculatedStatus = $server['status'];
		if($calculatedStatus == 0)
		{
			$gameFull = ($server['networkSlots'] <= $server['connectedClients']);
			if($gameFull == true)
			{
				$outString = $outString . "1|";
			}
		}
		$outString = $outString . "$calculatedStatus|\n";
		
		echo ($outString);
	}
	unset( $all_servers );
	unset( $server );
?>

