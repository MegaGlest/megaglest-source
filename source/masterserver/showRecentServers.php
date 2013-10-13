<?php
        define( 'INCLUSION_PERMITTED', true );
        require_once( 'config.php' );
        require_once( 'functions.php' );

        $link = db_connect();
        mysql_select_db(MYSQL_DATABASE);
        $recents_query = mysql_query('SELECT name, players FROM recent_servers' );
        echo "<table>";
        while($recent_server = mysql_fetch_assoc($recents_query)) {
                echo "<tr><td>{$recent_server['name']}</td><td>{$recent_server['players']}</td></tr>";
        }
        echo "</table>";

        db_disconnect($link);
?>
