<?php
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke and Tom Reynolds
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

	define( 'INCLUSION_PERMITTED', true );

	require_once( 'config.php' );
	require_once( 'functions.php' );
	define( 'DB_LINK', db_connect() );

	// $period = $_GET["period"];
	if (isset($_GET["period"])) {
		$period = $_GET["period"];
	} else {
		$period = "";
	}	

	$timelimit=" ";
	if( $period == "day") {
		$timelimit = " and lasttime >= DATE_SUB(NOW(), INTERVAL 1 DAY)  ";
	}
	else if( $period == "week") {
		$timelimit = " and lasttime >= DATE_SUB(NOW(), INTERVAL 1 WEEK)  ";
	}
	else if( $period == "month") {
		$timelimit = " and lasttime >= DATE_SUB(NOW(), INTERVAL 1 MONTH)  ";
	}

	$players_in_db = mysql_query( 'select playername, count(*) as c from glestgameplayerstats s where controltype>4 '.$timelimit.' group by playername having c >1 order by c desc,playername  LIMIT 100' );
	$all_players = array();
	while ( $players = mysql_fetch_array( $players_in_db ) )
	{
		array_push( $all_players, $players );
	}
	unset( $players_in_db );
	unset( $players );

	db_disconnect( DB_LINK );
	unset( $linkid );

	// Representation starts here
	header( 'Content-Type: text/html; charset=utf-8' );
	
	echo '<!DOCTYPE HTML>' . PHP_EOL;
	echo '<html>' . PHP_EOL;
	echo '	<head>' . PHP_EOL;
	echo '		<meta charset="UTF-8" />' . PHP_EOL;
	echo '		<title>' . htmlspecialchars( PRODUCT_NAME ) . ' top 100 players</title>' . PHP_EOL;
	echo '		<link rel="stylesheet" type="text/css" href="style/screen.css" />' . PHP_EOL;
	echo '		<link rel="shortcut icon" type="image/x-icon" href="images/' . htmlspecialchars( strtolower( PRODUCT_NAME ) ) . '.ico" />' . PHP_EOL;
	echo '	</head>' . PHP_EOL;
	echo '	<body>' . PHP_EOL;
	echo '		<h1><a href="' . htmlspecialchars( PRODUCT_URL ) . '">' . htmlspecialchars( PRODUCT_NAME ) . '</a> Top 100 Players</h1>' . PHP_EOL;

	if( $period == "day") {
		echo '		<b>' . PHP_EOL;
	}
	echo '		<a href="' . $_SERVER['PHP_SELF'].'?period=day ">' . htmlspecialchars( 'day' ) . '</a> ' . PHP_EOL;
	if( $period == "day") {
		echo '</b>' . PHP_EOL;
	}

	if( $period == "week") {
		echo '		<b>' . PHP_EOL;
	}
	echo '		<a href="' . $_SERVER['PHP_SELF'].'?period=week ">' . htmlspecialchars( 'week' ) . '</a> ' . PHP_EOL;
	if( $period == "week") {
		echo '</b>' . PHP_EOL;
	}

	if( $period == "month") {
		echo '		<b>' . PHP_EOL;
	}
	echo '		<a href="' . $_SERVER['PHP_SELF'].'?period=month ">' . htmlspecialchars( 'month' ) . '</a> ' . PHP_EOL;
	if( $period == "month") {
		echo '</b>' . PHP_EOL;
	}

	if( $period == "") {
		echo '		<b>' . PHP_EOL;
	}
	echo '		<a href="' . $_SERVER['PHP_SELF'].'">' . htmlspecialchars( 'all time' ) . '</a> ' . PHP_EOL;
	if( $period == "") {
		echo '</b>' . PHP_EOL;
	}
	echo '		<table>' . PHP_EOL;
	echo '			<tr>' . PHP_EOL;
	echo '				<th title="position">'.htmlspecialchars('#').'</th>' . PHP_EOL;
	echo '				<th title="playerName">Player Name</th>' . PHP_EOL;
	echo '				<th title="gamesPlayed">Games Played</th>' . PHP_EOL;
	echo '			</tr>' . PHP_EOL;

        $position = 0;
	foreach( $all_players as $player )
	{
		$position +=1;
		echo "\t\t\t" . '<tr>' . PHP_EOL;
		// #
		printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $position,               ENT_QUOTES ), PHP_EOL );
		// playername
		printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player['playername'],               ENT_QUOTES ), PHP_EOL );
		// # of games games
		printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player['c'],   ENT_QUOTES ), PHP_EOL );
	        echo "\t\t\t" . '</tr>' . PHP_EOL;
	}
	echo '		</table>' . PHP_EOL;

        //echo '		<script language="javascript" type="text/javascript" src="scripts/utils.js"></script>' . PHP_EOL;
	echo '	</body>' . PHP_EOL;
	echo '</html>' . PHP_EOL;

	unset( $all_players );
	unset( $player );
?>
