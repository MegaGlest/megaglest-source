<?php
//	Copyright (C) 2012 Mark Vejvoda, Titus Tscharntke and Tom Reynolds
//	The Megaglest Team, under GNU GPL v3.0
// ==============================================================

	define( 'INCLUSION_PERMITTED', true );

	require_once( 'config.php' );
	require_once( 'functions.php' );

	// Representation starts here
	header( 'Content-Type: text/html; charset=utf-8' );

//	echo '		<h1>Game Stats</h1>' . PHP_EOL;
	echo '		<table>' . PHP_EOL;
	echo '			<tr>' . PHP_EOL;
	echo '				<th title="gameDuration">Game Duration</th>' . PHP_EOL;
	echo '				<th title="maxConcurrentUnitCount">Maximum Concurrent Units</th>' . PHP_EOL;
	echo '				<th title="totalEndGameConcurrentUnitCount">Total Units at End</th>' . PHP_EOL;
	echo '				<th title="isHeadlessServer">Headless Server</th>' . PHP_EOL;
	echo '			</tr>' . PHP_EOL;

	// get stats for a specific game
        $gameUUID = "";
	if ( isset( $_GET['gameUUID'] ) )
        {
                $gameUUID = (string) clean_str( $_GET['gameUUID'] );

                //printf( "Game UUID = %s\n", htmlspecialchars( $gameUUID,        ENT_QUOTES ), PHP_EOL );

                define( 'DB_LINK', db_connect() );        
	        // consider replacing this by a cron job
	        cleanupServerList();

                $whereClause = 'gameUUID=\'' . mysql_real_escape_string( $gameUUID ) . '\'';

	        $stats_in_db = mysql_query( 'SELECT * FROM glestgamestats WHERE ' . $whereClause . ';');
	        $all_stats = array();
	        while ( $stats = mysql_fetch_array( $stats_in_db ) )
	        {
		        array_push( $all_stats, $stats );
	        }
	        unset( $stats_in_db );
	        unset( $stats );

	        $player_stats_in_db = mysql_query( 'SELECT * FROM glestgameplayerstats WHERE ' . $whereClause . ' ORDER BY factionIndex;');
	        $all_player_stats = array();
	        while ( $player_stats = mysql_fetch_array( $player_stats_in_db ) )
	        {
		        array_push( $all_player_stats, $player_stats );
	        }
	        unset( $player_stats_in_db );
	        unset( $player_stats );

	        db_disconnect( DB_LINK );
	        unset( $linkid );

	        foreach( $all_stats as $stats )
	        {
		        echo "\t\t\t" . '<tr>' . PHP_EOL;

		        // Game Stats
                        $gameDuration = $stats['framesToCalculatePlaytime'];
                        $gameDuration = getTimeString($gameDuration);

                        printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $gameDuration,        ENT_QUOTES ), PHP_EOL );

                        printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $stats['maxConcurrentUnitCount'],        ENT_QUOTES ), PHP_EOL );
                        printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $stats['totalEndGameConcurrentUnitCount'],        ENT_QUOTES ), PHP_EOL );
                        printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $stats['isHeadlessServer'],        ENT_QUOTES ), PHP_EOL );

		        echo "\t\t\t" . '</tr>' . PHP_EOL;
                        echo '		</table>' . PHP_EOL;

                        // Player stats for Game

                        echo '		<table>' . PHP_EOL;
	                echo '			<tr>' . PHP_EOL;
	                echo '				<th title="factionIndex">Player #</th>' . PHP_EOL;
	                echo '				<th title="controlType">Player Type</th>' . PHP_EOL;
                        echo '				<th title="playerName">Player Name</th>' . PHP_EOL;
                        echo '				<th title="playerPlatform">Platform</th>' . PHP_EOL;
	                echo '				<th title="resourceMultiplier">Resource Multiplier</th>' . PHP_EOL;
	                echo '				<th title="factionTypeName">Faction Type</th>' . PHP_EOL;
                        echo '				<th title="teamIndex">Team</th>' . PHP_EOL;
                        echo '				<th title="wonGame">Winner</th>' . PHP_EOL;
                        echo '				<th title="killCount">Kills</th>' . PHP_EOL;
                        echo '				<th title="enemyKillCount">Enemy Kills</th>' . PHP_EOL;
                        echo '				<th title="deathCount">Deaths</th>' . PHP_EOL;
                        echo '				<th title="unitsProducedCount">Units Produced</th>' . PHP_EOL;
                        echo '				<th title="resourceHarvestedCount">Resources Harvested</th>' . PHP_EOL;
                        echo '				<th title="playerScore">Score</th>' . PHP_EOL;
                        echo '				<th title="quitBeforeGameEnd">Quit Before Game Ended</th>' . PHP_EOL;
                        echo '				<th title="quitTime">Quit Time</th>' . PHP_EOL;
	                echo '			</tr>' . PHP_EOL;

                        $best_score = 0;
                        $best_score_enemyKillCount = 0;
                        $best_score_unitsProducedCount = 0;
                        $best_score_resourceHarvestedCount = 0;
	                foreach( $all_player_stats as $player_stats )
	                {
                                if($best_score_enemyKillCount < $player_stats['enemyKillCount'])
                                {
                                        $best_score_enemyKillCount = $player_stats['enemyKillCount'];
                                }
                                if($best_score_unitsProducedCount < $player_stats['unitsProducedCount'])
                                {
                                        $best_score_unitsProducedCount = $player_stats['unitsProducedCount'];
                                }
                                if($best_score_resourceHarvestedCount < $player_stats['resourceHarvestedCount'])
                                {
                                        $best_score_resourceHarvestedCount = $player_stats['resourceHarvestedCount'];
                                }

                                $player_score = $player_stats['enemyKillCount'] * 100 + $player_stats['unitsProducedCount'] * 50 + $player_stats['resourceHarvestedCount'] / 10;

                                if($best_score < $player_score)
                                {
                                        $best_score = $player_score;
                                }
                        }

	                foreach( $all_player_stats as $player_stats )
	                {
		                echo "\t\t\t" . '<tr>' . PHP_EOL;

                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player_stats['factionIndex']+1,        ENT_QUOTES ), PHP_EOL );

                                $controlType = $player_stats['controlType'];
			        switch ( $controlType ) 
			        {
				        case 0:
					        $controlTypeTitle = "Closed";
					        break;
				        case 1:
					        $controlTypeTitle = "CPU Easy";
					        break;
				        case 2:
					        $controlTypeTitle = "CPU";
					        break;
				        case 3:
					        $controlTypeTitle = "CPU Ultra";
					        break;
				        case 4:
					        $controlTypeTitle = "CPU Mega";
					        break;
				        case 5:
					        $controlTypeTitle = "Network Player";
					        break;
				        case 6:
					        $controlTypeTitle = "Network Unassigned";
					        break;
				        case 7:
					        $controlTypeTitle = "Human Host";
					        break;
				        case 8:
					        $controlTypeTitle = "Network CPU Easy";
					        break;
				        case 9:
					        $controlTypeTitle = "Network CPU";
					        break;
				        case 10:
					        $controlTypeTitle = "Network CPU Ultra";
					        break;
				        case 11:
					        $controlTypeTitle = "Network CPU Mega";
					        break;
				        default:
					        $controlTypeTitle = 'unknown';
			        }

                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $controlTypeTitle,                   ENT_QUOTES ), PHP_EOL );
                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player_stats['playerName'],         ENT_QUOTES ),     PHP_EOL );
                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player_stats['platform'],           ENT_QUOTES ),     PHP_EOL );
                                printf( "\t\t\t\t<td>%s</td>%s", number_format(htmlspecialchars( $player_stats['resourceMultiplier'],  ENT_QUOTES ),2), PHP_EOL );
                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player_stats['factionTypeName'],    ENT_QUOTES ),     PHP_EOL );
                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player_stats['teamIndex']+1,        ENT_QUOTES ),     PHP_EOL );

                                $wonGame_class = "player_loser";
                                if($player_stats['wonGame'])
                                {
                                        $wonGame_class = "player_winner";
                                }

                                printf( "\t\t\t\t<td class='%s'>%s</td>%s", $wonGame_class, htmlspecialchars( ($player_stats['wonGame'] ? "yes" : "no"),        ENT_QUOTES ), PHP_EOL );
                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player_stats['killCount'],        ENT_QUOTES ), PHP_EOL );

                                $player_score_class = "player_losing_score";
                                if($best_score_enemyKillCount == $player_stats['enemyKillCount'])
                                {
                                        $player_score_class = "player_high_score";
                                }

                                printf( "\t\t\t\t<td class='%s'>%s</td>%s", $player_score_class, htmlspecialchars( $player_stats['enemyKillCount'],        ENT_QUOTES ), PHP_EOL );
                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $player_stats['deathCount'],        ENT_QUOTES ), PHP_EOL );

                                $player_score_class = "player_losing_score";
                                if($best_score_unitsProducedCount == $player_stats['unitsProducedCount'])
                                {
                                        $player_score_class = "player_high_score";
                                }

                                printf( "\t\t\t\t<td class='%s'>%s</td>%s", $player_score_class, htmlspecialchars( $player_stats['unitsProducedCount'],        ENT_QUOTES ), PHP_EOL );

                                $player_score_class = "player_losing_score";
                                if($best_score_resourceHarvestedCount == $player_stats['resourceHarvestedCount'])
                                {
                                        $player_score_class = "player_high_score";
                                }

                                printf( "\t\t\t\t<td class='%s'>%s</td>%s", $player_score_class, htmlspecialchars( $player_stats['resourceHarvestedCount'],        ENT_QUOTES ), PHP_EOL );

                                $player_score = $player_stats['enemyKillCount'] * 100 + $player_stats['unitsProducedCount'] * 50 + $player_stats['resourceHarvestedCount'] / 10;
                                $player_score_class = "player_losing_score";
                                if($player_score == $best_score)
                                {
                                        $player_score_class = "player_high_score";
                                }

                                printf( "\t\t\t\t<td class='%s'>%s</td>%s", $player_score_class, number_format(htmlspecialchars( $player_score,        ENT_QUOTES ),0), PHP_EOL );

                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( ($player_stats['quitBeforeGameEnd'] ? "yes" : "no"),        ENT_QUOTES ), PHP_EOL );

                                $quitTime = $player_stats['quitTime'];
                                $quitTime = getTimeString($quitTime);

                                printf( "\t\t\t\t<td>%s</td>%s", htmlspecialchars( $quitTime,        ENT_QUOTES ), PHP_EOL );

		                echo "\t\t\t" . '</tr>' . PHP_EOL;
	                }

                	unset( $all_player_stats );
                	unset( $player_stats );
	        }

        	unset( $all_stats );
        	unset( $stats );
        }

	echo '		</table>' . PHP_EOL;

	//echo '		<p>' . PHP_EOL;
	//echo '			<br />' . PHP_EOL;
	//echo '		</p>' . PHP_EOL;
?>
