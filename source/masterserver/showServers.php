<?php
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke and Tom Reynolds
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

	define( 'INCLUSION_PERMITTED', true );

	require_once( 'config.php' );
	require_once( 'functions.php' );
	define( 'DB_LINK', db_connect() );

	// allow for automatic refreshing in web browser by appending '?refresh=VALUE', where VALUE is a numeric value in seconds.
	define( 'REFRESH_INTERVAL', (int) $_GET['refresh'] );

	// allow for filtering by gameserver version
	define( 'FILTER_VERSION', $_GET['version'] );

	define( 'MGG_HOST', $_GET['mgg_host'] );
	define( 'MGG_PORT', $_GET['mgg_port'] );

	if ( MGG_HOST != '' ) {
		$body = MGG_HOST . ':' . MGG_PORT;
		header( 'Content-Type: application/x-megaglest-gameserver; charset=utf-8' );
		header( 'Content-Disposition: attachment; filename="megaglest_gameserver.mgg' );
		header( 'Content-Length: ' . strlen( $body ));
		header( 'Accept-Ranges: bytes' );
		echo $body;
		exit;
	}

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
	unset( $linkid );

	// Representation starts here
	header( 'Content-Type: text/html; charset=utf-8' );
	if ( REFRESH_INTERVAL != 0 ) {
		if ( REFRESH_INTERVAL <= 10 ) {
			header( 'Refresh: 10' );
		} else {
			header( 'Refresh: ' . REFRESH_INTERVAL );
		}
	}
	echo '<!DOCTYPE HTML>' . PHP_EOL;
	echo '<html>' . PHP_EOL;
	echo '	<head>' . PHP_EOL;
	echo '		<meta charset="UTF-8" />' . PHP_EOL;
	echo '		<title>' . htmlspecialchars( PRODUCT_NAME ) . ' gameservers</title>' . PHP_EOL;
	echo '		<link rel="stylesheet" type="text/css" href="style/screen.css" />' . PHP_EOL;
	echo '	</head>' . PHP_EOL;
	echo '	<body>' . PHP_EOL;
	echo '		<h1>' . htmlspecialchars( PRODUCT_NAME ) . ' gameservers</h1>' . PHP_EOL;
	echo '		<table>' . PHP_EOL;
	echo '			<tr>' . PHP_EOL;
	echo '				<th title="glestVersion">Version</th>' . PHP_EOL;
	echo '				<th title="status">Status</th>' . PHP_EOL;
	echo '				<th title="country">Country</th>' . PHP_EOL;
	echo '				<th title="serverTitle">Title</th>' . PHP_EOL;
	echo '				<th title="tech">Techtree</th>' . PHP_EOL;
	echo '				<th title="connectedClients">Network players</th>' . PHP_EOL;
	echo '				<th title="networkSlots">Network slots</th>' . PHP_EOL;
	echo '				<th title="activeSlots">Total slots</th>' . PHP_EOL;
	echo '				<th title="map">Map</th>' . PHP_EOL;
	echo '				<th title="tileset">Tileset</th>' . PHP_EOL;
	echo '				<th title="ip">IPv4 address</th>' . PHP_EOL;
	echo '				<th title="externalServerPort">Game protocol port</th>' . PHP_EOL;
	echo '				<th title="platform">Platform</th>' . PHP_EOL;
	echo '				<th title="binaryCompileDate">Build date</th>' . PHP_EOL;
	echo '			</tr>' . PHP_EOL;

	foreach( $all_servers as $server )
	{
		# Filter by version if requested
		if ( FILTER_VERSION == $server['glestVersion'] OR FILTER_VERSION == '' )
		{
			echo "\t\t\t" . '<tr>' . PHP_EOL;

			// glestVersion
			printf( "\t\t\t\t<td><a href=\"?version=%s\">%s</a></td>%s", htmlspecialchars( $server['glestVersion'], ENT_QUOTES ), htmlspecialchars( $server['glestVersion'], ENT_QUOTES ), PHP_EOL );

			// status
			$status_code = $server['status'];
			if ( $status_code == 0)
			{
				$gameFull = ( $server['networkSlots'] <= $server['connectedClients'] );
				if ( $gameFull == true )
				{
					$status_code = 1;
				}
			}
			switch ( $status_code ) 
			{
				case 0:
					$status_title = 'waiting for players';
					$status_class = 'waiting_for_players';
					break;
				case 1:
					$status_title = 'game full, pending start';
					$status_class = 'game_full_pending_start';
					break;
				case 2:
					$status_title = 'in progress';
					$status_class = 'in_progress';
					break;
				case 3:
					$status_title = 'finished';
					$status_class = 'finished';
					break;
				default:
					$status_title = 'unknown';
					$status_class = 'unknown';
			}
			printf( "\t\t\t\t<td title=\"%s\" class=\"%s\">%s</td>%s", $server['status'], $status_class, htmlspecialchars( $status_title, ENT_QUOTES ), PHP_EOL );

			// country
			if ( $server['country'] !== '' ) {
				$flagfile = 'flags/' . strtolower( $server['country'] ).'.png';
				if ( file_exists( $flagfile ) ) {
					printf( "\t\t\t\t<td><img src=\"%s\" title=\"%s\" alt=\"%s country flag\" /></td>%s", $flagfile,  $server['country'], $server['country'], PHP_EOL );
				} else {
					printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['country'], ENT_QUOTES ), PHP_EOL );
				}
			}
			else {
				printf( "\t\t\t\t<td>unknown</td>%s", PHP_EOL );
			}

			// serverTitle
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['serverTitle'],        ENT_QUOTES ), PHP_EOL );

			// tech
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['tech'],               ENT_QUOTES ), PHP_EOL );

			// connectedClients
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['connectedClients'],   ENT_QUOTES ), PHP_EOL );

			// networkSlots
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['networkSlots'],       ENT_QUOTES ), PHP_EOL );

			// activeSlots
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['activeSlots'],        ENT_QUOTES ), PHP_EOL );

			// map
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['map'],                ENT_QUOTES ), PHP_EOL );

			// tileset
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['tileset'],            ENT_QUOTES ), PHP_EOL );

			// ip
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['ip'],                 ENT_QUOTES ), PHP_EOL );

			// externalServerPort
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['externalServerPort'], ENT_QUOTES ), PHP_EOL );

			// platform
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['platform'],           ENT_QUOTES ), PHP_EOL );

			// binaryCompileDate
			printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $server['binaryCompileDate'],  ENT_QUOTES ), PHP_EOL );

			echo "\t\t\t" . '</tr>' . PHP_EOL;
		}
	}

	echo '		</table>' . PHP_EOL;

	echo '		<p>' . PHP_EOL;
	echo '			<br />' . PHP_EOL;
	echo '		</p>' . PHP_EOL;

	if ( FILTER_VERSION != '' )
	{
		echo "\t\t<p>Filters active:</p>" . PHP_EOL;
		echo "\t\t<ul>" . PHP_EOL;
		printf( "\t\t\t<li>Version <a href=\"?\">%s</a></li>%s", htmlspecialchars( FILTER_VERSION, ENT_QUOTES ), PHP_EOL );
		echo "\t\t</ul>" . PHP_EOL;
	}

	echo '		<p>Usage:</p>' . PHP_EOL;
	echo '		<ul>' . PHP_EOL;
	echo '			<li>You can have this page auto <a href="?refresh=60">refresh every 60 seconds</a> by appending <code>?refresh=60</code> to the URL. Minimum refresh time is 10 seconds.</li>' . PHP_EOL;
	echo '			<li>The parameters used by the masterserver API will display when you move your mouse pointer over any of the table headings.</li>' . PHP_EOL;
	echo '		</ul>' . PHP_EOL;
	echo '	</body>' . PHP_EOL;
	echo '</html>' . PHP_EOL;

	unset( $all_servers );
	unset( $server );
?>
