// Pebble.addEventListener('ready', function(e) {
//   Pebble.sendAppMessage({'APP_READY': true});
// });

Pebble.addEventListener('appmessage', function(e) {
    var hyperMode = e.payload["0"];
    var text = e.payload["1"];
    takeAction(hyperMode, text);
});

function takeAction(hyperMode, text){
  var req = new XMLHttpRequest();
  req.open("POST", 'ENDPOINT_URL');
  req.setRequestHeader("Content-Type", "application/json");
  req.onload = function(e) {
    if (req.readyState == 4) {
      // 200 - HTTP OK
      if(req.status == 200) {
        var response = JSON.parse(req.responseText);
        if(response['responseText']) {
          var responseText = response['responseText'];
          Pebble.sendAppMessage({'GAssistantResponse':responseText});
        } else {
          Pebble.sendAppMessage({'GAssistantResponse':'I heard your request, but nothing came back. For this type of request you should use HYPER mode.'});
        }
      }
    }
    
  }
  req.send(JSON.stringify({
    command:text,
    hyperMode:hyperMode,
    tokens: {
      "access_token": "ACCESS_TOKEN", 
      "scope": "https://www.googleapis.com/auth/assistant-sdk-prototype", 
      "token_type": "Bearer", 
      "expires_in": 3599, 
      "refresh_token": "REFRESH_TOKEN"
    }
  }));
}
    
