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
   	$maps_in_db = mysql_db_query( MYSQL_DATABASE, 'SELECT * FROM glestmaps WHERE disabled=0 ORDER BY mapname;' );

	$all_maps = array();
	while ( $map = mysql_fetch_array( $maps_in_db ) )
	{
		array_push( $all_maps, $map );
	}
	unset( $maps_in_db );
	unset( $map );

	db_disconnect( DB_LINK );

	// Representation starts here
	header( 'Content-Type: text/plain; charset=utf-8' );
	foreach( $all_maps as &$map )
	{
                $itemVersion = 'v' . "${map['glestversion']}";
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
			                "${map['mapname']}|${map['playercount']}|${map['crc']}|${map['description']}|${map['url']}|${map['imageUrl']}|";
                        }
                        else {
		                $outString =
			                "${map['mapname']}|${map['playercount']}|${map['crcnew']}|${map['description']}|${map['url']}|${map['imageUrl']}|";
                        }
		        $outString = $outString . "\n";
		
		        echo ($outString);
                }
	}
	unset( $all_maps );
	unset( $map );
?>

