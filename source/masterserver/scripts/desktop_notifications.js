//      Copyright (C) 2013 Mike Hoffert ("Omega")
//      Licensed under GNU GPL v3.0
// ==============================================================

var DEFAULT_REFRESH_TIME = 20000;

// Get query arguments
var get_data = {};
var args = location.search.substr(1).split(/&/);
var version = '';

// Break up URL to find the get queries
for (var i = 0; i < args.length; i++)
{
	var tmp = args[i].split(/=/);
	if(tmp[0] !== "")
	{
		get_data[decodeURIComponent(tmp[0])] = decodeURIComponent(tmp.slice(1).join("").replace("+", " "));
	}
}

// Check if there's a version query
if(get_data['version'])
{
	version = get_data['version'];
}

// Will store the data about servers last time we checked, so we can compare if there
// are any changes
var serverList = {};
var firstLoop = true;


// Modify the document body
var domUl = document.getElementById("noJsUsage");
var domBody = document.getElementsByTagName("body");

var wrapperDiv = document.createElement("div");
wrapperDiv.innerHTML = "The parameters used by the masterserver API will display when you move your mouse pointer over any of the table headings.<br /><br />" +
	"<input id=\"enableNotifications\" type=\"checkbox\" /> <label for=\"enableNotifications\">Check here to enable auto-refreshing of the table and desktop notifications</label><br />" +
	"<label for=\"refreshTimeId\">Refresh time (in seconds):</label> <input id=\"refreshTimeId\" type=\"number\" width=\"3\" min=\"10\" max=\"999\" /> <label for=\"refreshTimeId\">(minimum 10)</label>";
wrapperDiv.style.paddingLeft = "30px";

domBody[0].insertBefore(wrapperDiv, domUl);
domUl.parentNode.removeChild(domUl);


// Modifying string object to support startsWith(String) function
// Created by CMS <http://stackoverflow.com/a/646643/1968462>
if (typeof String.prototype.startsWith != 'function') {
  // see below for better implementation!
  String.prototype.startsWith = function (str){
    return this.indexOf(str) == 0;
  };
}

// Request permission for issuing desktop notifications when the checkbox is ticked
var notifications = document.getElementById("enableNotifications");
notifications.onclick = function()
{
	if(notifications.checked)
	{
		Notification.requestPermission();
	}
};


// Helper function for escpaing special characters
function escapeHtml(text) {
  return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;").replace(/'/g, "&#039;");
}


// Check the JSON data for changes at intervals and update table
function timedRequest()
{
	// Break out if the checkbox isn't ticked
	if(!notifications.checked)
	{
		return;
	}

	// Get JSON of server list
	var request = new XMLHttpRequest();
	request.open('GET', 'showServersJson.php', true);
	request.send();

	// Function calls as soon as we receive the right data from the site
	request.onreadystatechange = function()
	{
		if (request.readyState === 4 && request.status === 200)
		{
			// Parse the JSON data for safety
			var jsonText = JSON.parse(request.responseText);
			var newServerList = {};

			games_with_stats = 100;
			// Repopulate table content
			var table = "<tr>\n" +
				"	<th title=\"glestVersion\">Version</th>\n" +
				"	<th title=\"status\">Status</th>\n" +
				"	<th title=\"gameDuration\">Game Duration</th>\n" +
				"	<th title=\"country\">Country</th>\n" +
				"	<th title=\"serverTitle\">Title</th>\n" +
				"	<th title=\"tech\">Techtree</th>\n" +
				"	<th title=\"connectedClients\">Network players</th>\n" +
				"	<th title=\"networkSlots\">Network slots</th>\n" +
				"	<th title=\"activeSlots\">Total slots</th>\n" +
				"	<th title=\"map\">Map</th>\n" +
				"	<th title=\"tileset\">Tileset</th>\n" +
				"	<th title=\"ip\">IPv4 address</th>\n" +
				"	<th title=\"externalServerPort\">Game protocol port</th>\n" +
				"	<th title=\"platform\">Platform</th>\n" +
				"	<th title=\"lasttime\">Play date</th>\n" +
				"</tr>\n";

			// Loop through all json objects
			for(var i = 0; i < jsonText.length; i++)
			{
				// Check if version filter is active
				if(version === '' || jsonText[i].glestVersion == version)
				{
					////// DYNAMIC TABLE SECTION

					table += "<tr>";

					/// Version
					table += "<td><a href=\"?version=" + escapeHtml(jsonText[i].glestVersion) + "\" rel=\"nofollow\">" + escapeHtml(jsonText[i].glestVersion) + "</a></td>";

					/// Status
					var statusCode = parseInt(jsonText[i].status);

					// Change text if the server is full
					if((statusCode === 0) && (jsonText[i].networkSlots <= jsonText[i].connectedClients))
					{
						statusCode = 1;
					}
					var statusTitle, statusClass;
					// Note that the json value is stored as a string, not a number
					switch(statusCode)
					{
						case "0":
							statusTitle = 'waiting for players';
							statusClass = 'waiting_for_players';
							break;
						case "1":
							statusTitle = 'game full, pending start';
							statusClass = 'game_full_pending_start';
							break;
						case "2":
							statusTitle = 'in progress';
							statusClass = 'in_progress';
							break;
						case "3":
							statusTitle = 'finished';
							statusClass = 'finished';
							break;
						default:
							statusTitle = 'unknown';
							statusClass = 'unknown';
					}

					//debugger;
					if ((statusCode == "2" || statusCode == "3") && jsonText[i].gameUUID !== "")
					{
						var specialColHTML = "<td title=\"" + jsonText[i].status + "\" class=\"" + statusClass + "\"><a id=\"gameStats_" + games_with_stats + "\" href=\"#\" gameuuid=\"" + jsonText[i].gameUUID + "\">" + escapeHtml(statusTitle) + "</a></td>";
						table += specialColHTML;
					}
					else
					{
						table += "<td title=\"" + jsonText[i].status + "\" class=\"" + statusClass + "\">" + escapeHtml(statusTitle) + "</td>";
					}

					/// Game Duration
					table += "<td>" + escapeHtml(jsonText[i].gameDuration) + "</td>";

					/// Country
					if(jsonText[i].country !== "")
					{
						var flagFile = "flags/" + jsonText[i].country.toLowerCase() + ".png";
						table += "<td><img src=\"" + flagFile + "\" title=\"" + jsonText[i].country + "\" alt=\"" + jsonText[i].country + "\" /></td>";
					}
					else
					{
						table += "<td>Unknown</td>";
					}

					/// Server title
					table += "<td>" + escapeHtml(jsonText[i].serverTitle) + "</td>";

					///  Tech
					table += "<td>" + escapeHtml(jsonText[i].tech) + "</td>";

					///  Connected clients
					table += "<td>" + escapeHtml(jsonText[i].connectedClients) + "</td>";

					///  Network slots
					table += "<td>" + escapeHtml(jsonText[i].networkSlots) + "</td>";

					///  Active slots
					table += "<td>" + escapeHtml(jsonText[i].activeSlots) + "</td>";

					///  Map
					table += "<td>" + escapeHtml(jsonText[i].map) + "</td>";

					///  Tileset
					table += "<td>" + escapeHtml(jsonText[i].tileset) + "</td>";

					///  IP
					table += "<td>" + escapeHtml(jsonText[i].ip) + "</td>";

					///  Port
					table += "<td>" + escapeHtml(jsonText[i].externalServerPort) + "</td>";

					///  Platform
					table += "<td>" + escapeHtml(jsonText[i].platform) + "</td>";

					///  Play date
					table += "<td>" + escapeHtml(jsonText[i].lasttime) + "</td>";

					table += "</tr>";

					if ((statusCode === "2" || statusCode === "3") && jsonText[i].gameUUID !== "")
					{
							table += "<tr width='100%%' class='fullyhide' id='content_row_" + jsonText[i].gameUUID + "'>";
							table += "<td width='100%%' colspan='100'></td>";
							table += "</tr>";

							games_with_stats++;
					}

					////// DESKTOP NOTIFICATIONS SECTION

					// Store data in an array keyed by the concatenation of the IP and port
					var identifier = jsonText[i].ip + jsonText[i].externalServerPort;
					newServerList[identifier] = { 'ip': jsonText[i].ip, 'port': jsonText[i].externalServerPort, 'title': jsonText[i].serverTitle, 'free': (jsonText[i].networkSlots - jsonText[i].connectedClients), 'version': jsonText[i].glestVersion };

					// Only check for changes if NOT the first time
					if(!firstLoop)
					{
						if((newServerList[identifier].free > 0 && !serverList[identifier] && statusCode === 0) || // doesn't exist in old list
							(newServerList[identifier].free > 0 && serverList[identifier].free == 0 && statusCode === 0 && serverList[identifier].serverTitle.startsWith("Headless"))) // Headless server that previously had zero players
						{
							// Create notification
							var notification = new Notification("Open server", {
								iconUrl: 'images/game_icon.png',
								body: 'Server "' + newServerList[identifier].title + '" has ' + newServerList[identifier].free + ' free slots available. Click to join now.',
							});

							notification.onclick = function() { window.location.assign('http://play.mg/?version=' + newServerList[identifier].version + '&mgg_host=' + newServerList[identifier].ip + '&mgg_port=' + newServerList[identifier].port); };
						}
					}
					else
					{
						firstLoop = false;
					}
				}
			}
			// Replace old list with new one
			serverList = newServerList;

			// Write to actual table when done only, otherwise the browser trips as it tries to fix the partial table formatting
			var tableDOM = document.getElementById("gamesTable");
			tableDOM.innerHTML = table;

			//debugger;
			for(var gameIndex = 100; gameIndex < 200; ++gameIndex) {
					setupGameStatsLink(gameIndex);
			}

			// Catch empty case
			if(jsonText.length === 0)
			{
				serverList = { };
			}
		}
		// Empty server list
		else if(request.readyState === 4 && request.status === 0)
		{
			serverList = { };
		}
	};
}


// Default time in miliseconds between updates
var refreshTime = DEFAULT_REFRESH_TIME;

// Check if there's an HTTP refresh query. If so, we need to overwrite it
if(get_data['refresh'])
{
	// Get the base URL without any GET parameters (because we have to remove the
	// old refresh variable)
	var redirectLocation = location.href.split("?")[0] + "?";

	// If a version variable was specified, add that back in
	if(get_data['version'])
	{
		redirectLocation += "version=" + get_data['version'] + "&";
	}

	// Finally the new refresh variable just for JS use
	redirectLocation += "jsrefresh=" + get_data['refresh'];
	
	window.location.replace(redirectLocation);
}

// Check if there's a js refresh query
if(get_data['jsrefresh'])
{
	// In seconds, so multiply by 1000 for miliseconds
	refreshTime = parseInt(get_data['jsrefresh']) * 1000;
}

// Initialize value in text field
var refreshTimeBox = document.getElementById("refreshTimeId");
refreshTimeBox.value = refreshTime / 1000;


// Initiate interval
timedRequest();
var interval = setInterval(timedRequest, refreshTime);


// Catch changes to the refresh time box
refreshTimeBox.onchange = function()
{
	// Validate if the input is a number
	if(!isNaN(parseFloat(refreshTimeBox.value)) && isFinite(refreshTimeBox.value))
	{
		if(refreshTimeBox.value < 10)
		{
			refreshTime = 10000;
			refreshTimeBox.value = 10;
		}
		else if(refreshTimeBox.value > 999)
		{
			refreshTime = 999000;
			refreshTimeBox.value = 999;
		}
		else
		{
			refreshTime = refreshTimeBox.value * 1000;
		}
	}
	else
	{
		refreshTime = DEFAULT_REFRESH_TIME;
		refreshTimeBox.value = 20;
	}

	// Reset the interval
	clearInterval(interval);
	interval = setInterval(timedRequest, refreshTime);
};