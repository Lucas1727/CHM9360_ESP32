var connected_flag = 0
var mqtt;
var reconnectTimeout = 2000;
var host;
var port;
var coll = document.getElementsByClassName("button collapsible");
var i;
var msgJSON = {
    command: "",
    text: ""
};

function StartUp() {
    DataStorage();
    MQTTconnect();
    SettingsCollapse();
    ESP32SetupMQTT();
}

function ESP32SetupMQTT() {
    xhr = new XMLHttpRequest();
    var url = "/";
    xhr.open("POST", url, true);
    xhr.setRequestHeader("Content-type", "application/json");
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            document.getElementById("status").innerHTML = this.responseText;
        }
    }
    xhr.send(host);
}

function SettingsCollapse() {
    for (i = 0; i < coll.length; i++) {
        coll[i].addEventListener("click", function () {
            this.classList.toggle("active");
            var content = this.nextElementSibling;
            if (content.style.display === "block") {
                content.style.display = "none";
            } else {
                content.style.display = "block";
            }
        });
    }
}

function DataStorage() {

    if (!localStorage.weather) {
        localStorage.weather = "Rio";
    }

    if (!localStorage.MQTThost) {
        localStorage.MQTThost = "192.168.1.00";
    }

    if (!localStorage.MQTTport) {
        localStorage.MQTTport = "8000";
    }

    if (!localStorage.OWMAPIKey) {
        localStorage.OWMAPIKey = "xxxx";
    }

    if (!localStorage.YTAPIKey) {
        localStorage.YTAPIKey = "xxxx";
    }

    host = localStorage.MQTThost;
    port = localStorage.MQTTport;

    document.getElementById("MQTTHostInput").value = host;
    document.getElementById("MQTTPortInput").value = port;

    document.getElementById("weatherLocation").innerHTML = localStorage.weather;
    document.getElementById("OpenWeatherMapAPIKeyInput").value = localStorage.OWMAPIKey;
    document.getElementById("YouTubeAPIKeyInput").value = localStorage.YTAPIKey;
}

function MQTTSendTransitionMode() {
    var msg = '{"command":"transMode","text":{' +
        '"Message":' + document.getElementById("transitionModeMessage").checked + ',' +
        '"Weather":' + document.getElementById("transitionModeWeather").checked + ',' +
        '"Clock":true,' +
        '"Sensor":' + document.getElementById("transitionModeSensor").checked + ',' +
        '"YouTube":' + document.getElementById("transitionModeYouTube").checked + '}}'

    msgJSON = JSON.parse(msg);

    MQTTSendJSONData();
}

function MQTTSendTransitionTime() {
    msgJSON = {
        command: "transTime",
        value: document.getElementById("textTransitionTimeInput").value
    };

    MQTTSendJSONData();
}

function MQTTSendJSONData() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }

    console.log(msgJSON);

    var topic = "SmartDisplay";
    message = new Paho.MQTT.Message(JSON.stringify(msgJSON));
    if (topic == "")
        message.destinationName = "test-topic"
    else
        message.destinationName = topic;
    mqtt.send(message);
}

function MQTTSendMessage() {
    msgJSON = {
        command: "message",
        text: document.getElementById("textInput").value
    };

    MQTTSendJSONData();
}

function MQTTSendClock() {
    msgJSON = {
        command: "clock",
        text: "text"
    };

    MQTTSendJSONData();
}

function MQTTSendWeather() {
    MQTTSendOpenWeatherMapAPIKey();

    msgJSON = {
        command: "weather",
        text: document.getElementById("textWeatherInput").value
    };

    localStorage.weather = document.getElementById("textWeatherInput").value;
    document.getElementById("weatherLocation").innerHTML = localStorage.weather;

    MQTTSendJSONData();
}

function MQTTSendOpenWeatherMapAPIKey() {
    msgJSON = {
        command: "OWMKey",
        text: document.getElementById("OpenWeatherMapAPIKeyInput").value
    };

    localStorage.OWMAPIKey = document.getElementById("OpenWeatherMapAPIKeyInput").value;

    MQTTSendJSONData();
}

function MQTTSendYouTubeAPIKey() {
    msgJSON = {
        command: "YTKey",
        text: document.getElementById("YouTubeAPIKeyInput").value
    };

    localStorage.YTAPIKey = document.getElementById("YouTubeAPIKeyInput").value;

    MQTTSendJSONData();
}

function MQTTSendSensorRequest() {
    msgJSON = {
        command: "sensor",
        text: "text"
    };

    MQTTSendJSONData();
}

function MQTTSendYoutube() {
    MQTTSendYouTubeAPIKey();

    msgJSON = {
        command: "youtube",
        text: document.getElementById("textYouTubeInput").value
    };

    MQTTSendJSONData();
}

function MQTTChangeTextColour() {
    var redSlider = document.getElementById("redSlider");
    var greenSlider = document.getElementById("greenSlider");
    var blueSlider = document.getElementById("blueSlider");

    msgJSON = {
        command: "textCol",
        colR: redSlider.value,
        colG: greenSlider.value,
        colB: blueSlider.value
    };

    MQTTSendJSONData();
}

function onConnectionLost() {
    console.log("connection lost");
    document.getElementById("status").innerHTML = "Connection Lost";
    document.getElementById("messages").innerHTML = "Connection Lost";
    connected_flag = 0;
}

function onFailure(message) {
    console.log("Failed");
    document.getElementById("messages").innerHTML = "Connection Failed- Retrying";
    setTimeout(MQTTconnect, reconnectTimeout);
}

function onMessageArrived(r_message) {
    out_msg = "Message received " + r_message.payloadString + "<br>";
    out_msg = out_msg + "Message received Topic " + r_message.destinationName;
    console.log(out_msg);
    document.getElementById("messages").innerHTML = out_msg;
}

function onConnected(recon, url) {
    console.log(" in onConnected " + reconn);
}

function onConnect() {
    document.getElementById("messages").innerHTML = "Connected to " + host + " on port " + port;
    connected_flag = 1
    document.getElementById("status").innerHTML = "Connected";
    console.log("on Connect " + connected_flag);
}

function MQTTconnect() {
    document.getElementById("messages").innerHTML = "";
    var s = document.forms["connform"]["server"].value;
    var p = document.forms["connform"]["port"].value;
    if (p != "") {
        console.log("ports");
        port = parseInt(p);
        localStorage.MQTTport = port;
        console.log("port" + port);
    } else {
        port = localStorage.MQTTport;
    }
    if (s != "") {
        host = s;
        localStorage.MQTThost = host;
        console.log("host");
    } else {
        host = localStorage.MQTThost;
    }

    console.log("connecting to " + host + " " + port);
    mqtt = new Paho.MQTT.Client(host, port, "clientjsaaa");

    var options = {
        timeout: 3,
        onSuccess: onConnect,
        onFailure: onFailure,

    };

    mqtt.onConnectionLost = onConnectionLost;
    mqtt.onMessageArrived = onMessageArrived;
    mqtt.onConnected = onConnected;

    mqtt.connect(options);
    return false;
}

function send_message() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }
    var msg = document.forms["smessage"]["message"].value;
    console.log(msg);

    var topic = document.forms["smessage"]["Ptopic"].value;
    message = new Paho.MQTT.Message(msg);
    if (topic == "")
        message.destinationName = "test-topic"
    else
        message.destinationName = topic;
    mqtt.send(message);
    return false;
}
