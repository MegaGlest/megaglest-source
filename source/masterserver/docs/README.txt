ABOUT

This is the MegaGlest master server. MegaGlest (http://megaglest.org) is a
libre software cross platform real-time strategy game. 

This master server does the following:
· publish game hosts (when a user decides to host)
· list hosted games (HTML, CSV, JSON output)
· list recently hosted gamed (HTML output) - this code is currently disabled
· list available game mods (CSV output)
· provide a version check for game installations

It uses a standard PHP/MySQL setup to achieve this. When instances of MegaGlest
engine based games publish their game information, they do so by pushing it to
this web server in regular intervals. Stale entries are removed when the next
client requests the list. When game instances retrieve the list of hosted games
or available game mods, they do so by pulling this information from the server.
Game and master server communicate using HTTP. The client sends requests by
HTTP GET passing along URL parameters while the server responds in a CSV format
or single field plain text. The version check is currently implemented as plain
text files (which use symbolic links for deduplication purposes) on the server.
This may be replaced by a single configurable PHP script in the future.

The MegaGlest Team hosts a live copy of this code at 
  http://master.megaglest.org
Please do not use this instance for your tests, but set up a copy of your own.



INSTALLATION

1. Setup a web server with PHP and a MySQL database server.
   Sucessfully tested configurations (on Debian GNU/Linux 6 and 7):
   · Apache 2.2 + mod_php 5.3.3
   · Nginx 1.2.1 + fastcgi + PHP-FPM 5.4.4
   · MySQL Community Server/Edition 5.1 (Oracle)
   · MySQL Server 5.5 (Percona)

2. Create a new MySQL database and a user who has has all standard permissions
   to work on this database after authentication.
   Example:
   CREATE DATABASE 'megaglest-master' ENGINE=InnoDB;
   CREATE USER 'megaglest-master'@'localhost' IDENTIFIED BY 'secret password';
   GRANT ALL ON 'megaglest-master.*' TO 'megaglest-master';

3. Copy all files (you can omit INSTALL and install/) to your webserver and
   edit config.php to reflect the MySQL connection parameters and game title;
   also replace the images in images/ by some which match your game title.

4. Connect the new user to the new database, then execute the SQL statments in
   install/scheme_mysql.sql.
   Example:
   mysql -u megaglest-master -p megaglest-master < install/scheme_mysql.sql

5. Set up the webserver to allow access to, and set up PHP to execute, the
   PHP files you placed on your webserver. Practically you may want to create
   a new "VirtualHost"/"Server" and make sure it points to where you placed
   the files and can run PHP.

To test and use this server with your MegaGlest engine based game, configure
the "Masterserver" property in glestuser.ini (if it's MegaGlest) or glest.ini
(if it's a different game).

To add mods to the game mod menu, edit the database contents using your 
favorite MySQL editor or develop a web based frontend to do so. In the latter
case, please let us know about it and try to use a compatible license.
