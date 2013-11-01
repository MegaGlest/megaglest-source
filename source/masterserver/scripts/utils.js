function asynchGet(url, target) {
  //debugger;
  //alert(target);
  //document.getElementById(target).style.display='visible';
  document.getElementById(target).innerHTML = ' Fetching data...';
  if (window.XMLHttpRequest) {
    req = new XMLHttpRequest();
  } else if (window.ActiveXObject) {
    req = new ActiveXObject("Microsoft.XMLHTTP");
  }
  if (req != undefined) {
    req.onreadystatechange = function() {asynchDone(url, target);};
    req.open("GET", url, true);
    req.send("");
  }
}  

function asynchDone(url, target) {
  //debugger;
  if (req.readyState == 4) { // only if req is "loaded"
    if (req.status == 200) { // only if "OK"
      document.getElementById(target).innerHTML = req.responseText;
    } else {
      document.getElementById(target).innerHTML=" MG Error:\n"+ req.status + "\n" +req.statusText;
    }
  }
}

function setupGameStatsLink(gameIndex) {
   if(document.getElementById('gameStats_' + gameIndex) ) {
       var link = document.getElementById('gameStats_' + gameIndex);
       link.onclick = function() {
          var row = document.getElementById('content_row_' + this.getAttribute('gameuuid'));
          //if(row && row.className == 'fullyhide') {
          if(row) {
              row.className = 'fullyshow';
              row.innerHTML = '<td width=\'100%\' colspan=\'100\'><a id=\'hide_stats_' + this.getAttribute('gameuuid') + '\' href=\'#\'>Hide Stats</a><div width=\'100%\' id=\'content_' + this.getAttribute('gameuuid') + '\'></div></td>';

              var link2 = document.getElementById('hide_stats_' + this.getAttribute('gameuuid'));
              link2.onclick = function() {
                  this.parentNode.parentNode.className = 'fullyhide';
              };
                
              var div = document.getElementById('content_' + this.getAttribute('gameuuid'));
              asynchGet('showGameStats.php?gameUUID=' + this.getAttribute('gameuuid'),div.id);
          }
          return false;
       };
   }
}
//debugger;

for(var gameIndex = 1; gameIndex < 200; ++gameIndex) {
        setupGameStatsLink(gameIndex);
}

