<?php
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke and Tom Reynolds
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

	define( 'INCLUSION_PERMITTED', true );
	require_once( 'config.php' );
	require_once( 'functions.php' );

	define( 'DB_LINK', db_connect() );

        if ( isset( $_GET['glestVersion'] ) ) {
            $glestVersion = (string) clean_str( $_GET['glestVersion'] );
        }
        else {
                $glestVersion = "";
        }

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
                $itemVersion = 'v' . "${tileset['glestversion']}";
                $addItem = false;

                if($glestVersion == '') {
                     if (version_compare("v3.6.0.3",$itemVersion,">=")) {
                        $addItem = true;
                     }
                }
                else if (version_compare($glestVersion,$itemVersion,">=")) {
                        $addItem = true;
                }

                if($addItem == true) {
	                $mgversion = $_GET["version"];
	                if($mgversion == '')
	                {
                    		$outString =
                    			"${tileset['tilesetname']}|${tileset['crc']}|${tileset['description']}|${tileset['url']}|${tileset['imageUrl']}|";
                        }
                        else {
                    		$outString =
                    			"${tileset['tilesetname']}|${tileset['crcnew']}|${tileset['description']}|${tileset['url']}|${tileset['imageUrl']}|";
                        }
            		$outString = $outString . "\n";
            		
            		echo ($outString);
                }
	}
	unset( $all_tilesets );
	unset( $tileset );
?>

