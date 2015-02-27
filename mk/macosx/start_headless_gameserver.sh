#!/bin/sh
#
# Start a headless MegaGlest game server (and keep it running)
#
# Using this script, a headless game server is started, waiting for players to 
# connect (if your firewall configuration permits). Players connect and start a
# game. As soon as the game ends, the server will quit, and this script will 
# start up a new server immidiately. This is a stability measure to  rule out 
# the unlikely case where side effects (such as memory leaks or corruption) 
# could drain on system resources.
#
# For this to work, Internet originated traffic must be able to reach the 
# server on the following ports:
# TCP port 61357: game protocol port
# TCP port 61358: FTP control port
# TCP ports 61359 to 61366: FTP data ports
#
# Once publishing to the master server succeeded (this can be verified at
# <http://master.megaglest.org>) you may connect to your headless game server
# using a copy of MegaGlest you have installed on a Desktop computer. The first
# user connecting to a headless server controls it. If this user disconnects, 
# the next user who connects (or had already connected) takes control.
#
# Please read http://wiki.megaglest.org/Dedicated_Server for more information
#
# ----------------------------------------------------------------------------
# Written by Tom Reynolds <tomreyn[at]megaglest.org>
# Copyright (c) 2013 Tom Reynolds, The Megaglest Team, under GNU GPL v3.0+
# ----------------------------------------------------------------------------
LANG=C

# Install location
DIR_GAME="$(cd "$(dirname "$0")"; pwd)"

# Log file location (beware, this can grow large)
#LOG_SERVER=/dev/null
LOG_SERVER=~/.megaglest/server.log


# ---

cd $DIR_GAME
#ulimit -c unlimited

while true; do
  if [ -f "$LOG_SERVER" ]; then mv -f "$LOG_SERVER" "$LOG_SERVER.1"; fi
  if [ -f "core" ]; then mv -f "core" "core.1"; fi
  date > "$LOG_SERVER"
  echo 'Starting server...' | tee -a "$LOG_SERVER"
  ./megaglest --headless-server-mode=vps,exit >> "$LOG_SERVER" 2>&1
  if [ $? -ne 0 ];
  then
    echo 'ERROR: Server has quit unexpectedly.' >> "$LOG_SERVER"
    echo 'ERROR: Server has quit unexpectedly.' >&2
    echo '       Please inspect '"$LOG_SERVER"'.' >&2
    exit 1
  else
    echo 'Server has quit.' | tee -a "$LOG_SERVER"
  fi
done
