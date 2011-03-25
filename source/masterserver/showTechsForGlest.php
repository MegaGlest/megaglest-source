<?php

	define( 'INCLUSION_PERMITTED', true );
	require_once( 'config.php' );
	require_once( 'functions.php' );

	define( 'DB_LINK', db_connect() );

	$techs_in_db = mysql_db_query( MYSQL_DATABASE, 'SELECT * FROM glesttechs ORDER BY techname;' );
	$all_techs = array();
	while ( $tech = mysql_fetch_array( $techs_in_db ) )
	{
		array_push( $all_techs, $tech );
	}
	unset( $techs_in_db );
	unset( $tech );

	db_disconnect( DB_LINK );

	// Representation starts here
	header( 'Content-Type: text/plain; charset=utf-8' );
	foreach( $all_techs as &$tech )
	{
		$outString =
			"${tech['techname']}|${tech['factioncount']}|${tech['crc']}|${tech['description']}|${tech['url']}|";
		$outString = $outString . "\n";
		
		echo ($outString);
	}
	unset( $all_techs );
	unset( $tech );
?>

