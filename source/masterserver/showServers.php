<?php
	define( 'INCLUSION_PERMITTED', true );
	require_once( 'config.php' );
	require_once( 'functions.php' );

	define( 'DB_LINK', db_connect() );

	// consider replacing this by a cron job
	cleanupServerList( MYSQL_DATABASE );

	$servers_in_db = mysql_db_query( MYSQL_DATABASE, 'SELECT * FROM glestserver ORDER BY lasttime DESC;' );
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
echo <<<END
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
	<head>
		<title></title>
		<style type="text/css">
			body {
			}

			table {
				width:      100%;
				border:     2px solid black;
			}

			th, td { 
				width:      300;
				border:     1px solid black;
				text-align: left;
			}
		</style>
	</head>
	<body>
		<table>
			<tr>
				<th>glestVersion</th>
				<th>platform</th>
				<th>binaryCompileDate</th>
				<th>serverTitle</th>
				<th>ip</th>
				<th>tech</th>
				<th>map</th>
				<th>tileset</th>
				<th>activeSlots</th>
				<th>networkSlots</th>
				<th>connectedClients</th>
				<th>externalServerPort</th>
			</tr>

END;

	foreach( $all_servers as $server )
	{
		//array_walk( $server, 'htmlspecialchars', 'ENT_QUOTES' );
		echo "\t\t\t" . '<tr>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['glestVersion'],        ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['platform'],            ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['binaryCompileDate'],   ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['serverTitle'],         ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['ip'],                  ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['tech'],                ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['map'],                 ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['tileset'],             ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['activeSlots'],         ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['networkSlots'],        ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t\t<td>" . htmlspecialchars( $server['connectedClients'],    ENT_QUOTES ) . '</td>' . PHP_EOL;
                echo "\t\t\t\t<td>" . htmlspecialchars( $server['externalServerPort'],  ENT_QUOTES ) . '</td>' . PHP_EOL;
		echo "\t\t\t" . '</tr>' . PHP_EOL;
	}
	unset( $all_servers );
	unset( $server );

echo <<<END
		</table>
	</body>
</html>
END;
?>
