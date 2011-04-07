<?php

	define( 'INCLUSION_PERMITTED', true );
	require_once( 'config.php' );
	require_once( 'functions.php' );

	define( 'DB_LINK', db_connect() );

	$tilesets_in_db = mysql_db_query( MYSQL_DATABASE, 'SELECT * FROM glesttilesets WHERE disabled=0 ORDER BY tilesetname;' );
	$all_tilesets = array();
	while ( $tileset = mysql_fetch_array( $tilesets_in_db ) )
	{
		array_push( $all_tilesets, $tileset );
	}
	unset( $tilesets_in_db );
	unset( $tileset );

	db_disconnect( DB_LINK );

	// Representation starts here
	header( 'Content-Type: text/plain; charset=utf-8' );
	foreach( $all_tilesets as &$tileset )
	{
		$outString =
			"${tileset['tilesetname']}|${tileset['crc']}|${tileset['description']}|${tileset['url']}|${map['imageUrl']}|";
		$outString = $outString . "\n";
		
		echo ($outString);
	}
	unset( $all_tilesets );
	unset( $tileset );
?>

