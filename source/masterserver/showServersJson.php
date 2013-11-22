<?php
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke and Tom Reynolds
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

	define( 'INCLUSION_PERMITTED', true );
	require_once( 'config.php' );
	require_once( 'functions.php' );

	define( 'DB_LINK', db_connect() );

	// consider replacing this by a cron job
	cleanupServerList();

	$servers_in_db = mysql_query( 'SELECT a.*,b.framesToCalculatePlaytime FROM glestserver a LEFT JOIN glestgamestats b ON a.gameUUID = b.gameUUID WHERE status <> 3 OR (status = 3 AND a.lasttime > DATE_add(NOW(), INTERVAL - ' . MAX_HOURS_OLD_GAMES . ' hour)) ORDER BY status, a.lasttime DESC, connectedClients > 0 DESC, (networkSlots - connectedClients), ip DESC;' );
	$all_servers = array();
	while ( $server = mysql_fetch_array( $servers_in_db ) )
	{
	        // Game Stats
                $rowIndex = count($all_servers);
                $gameDuration = $server["framesToCalculatePlaytime"];
                $server["gameDuration"] = getTimeString($gameDuration);

		array_push( $all_servers, $server );

	}
	unset( $servers_in_db );
	unset( $server );

	db_disconnect( DB_LINK );
	unset( $linkid );

        header('Content-type: application/json');
	echo json_encode($all_servers);
	unset( $all_servers );
?>
