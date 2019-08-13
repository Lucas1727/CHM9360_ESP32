var connected_flag = 0
var mqtt;
var reconnectTimeout = 2000;
var host;
var port;
var coll = document.getElementsByClassName("button collapsible");
var i;

function StartUp() {
    DataStorage();
    MQTTconnect();
    SettingsCollapse();
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
        localStorage.MQTThost = "192.168.1.49";
    }

    if (!localStorage.MQTTport) {
        localStorage.MQTTport = "9001";
    }

    host = localStorage.MQTThost;
    port = localStorage.MQTTport;

    document.getElementById("MQTTHostInput").value = host;
    document.getElementById("MQTTPortInput").value = port;

    document.getElementById("weatherLocation").innerHTML = localStorage.weather;

}

function MQTTSendMessage() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }
    var msg = {
        command: "message",
        text: document.getElementById("textInput").value
    };

    console.log(msg);

    var topic = "SmartDisplay";
    message = new Paho.MQTT.Message(JSON.stringify(msg));
    if (topic == "")
        message.destinationName = "test-topic"
    else
        message.destinationName = topic;
    mqtt.send(message);
}

function MQTTSendClock() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }
    var msg = {
        command: "clock",
        text: "text"
    };

    console.log(msg);

    var topic = "SmartDisplay";
    message = new Paho.MQTT.Message(JSON.stringify(msg));
    message.destinationName = topic;
    mqtt.send(message);
}

function MQTTSendWeather() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }
    var msg = {
        command: "weather",
        text: document.getElementById("textWeatherInput").value
    };

    console.log(msg);

    localStorage.weather = document.getElementById("textWeatherInput").value;
    document.getElementById("weatherLocation").innerHTML = localStorage.weather;

    var topic = "SmartDisplay";
    message = new Paho.MQTT.Message(JSON.stringify(msg));
    message.destinationName = topic;
    mqtt.send(message);
}

function MQTTSendOpenWeatherMapAPIKey() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }
    var msg = {
        command: "OWMKey",
        text: document.getElementById("OpenWeatherMapAPIKeyInput").value
    };

    console.log(msg);

    var topic = "SmartDisplay";
    message = new Paho.MQTT.Message(JSON.stringify(msg));
    message.destinationName = topic;
    mqtt.send(message);
}

function MQTTSendSensorRequest() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }
    var msg = {
        command: "sensor",
        text: "text"
    };

    console.log(msg);

    var topic = "SmartDisplay";
    message = new Paho.MQTT.Message(JSON.stringify(msg));
    message.destinationName = topic;
    mqtt.send(message);
}

function MQTTSendYoutube() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }
    var msg = {
        command: "weather",
        text: "Aberystwyth,GB"
    };

    console.log(msg);

    var topic = "SmartDisplay";
    message = new Paho.MQTT.Message(JSON.stringify(msg));
    message.destinationName = topic;
    mqtt.send(message);
}

function MQTTChangeTextColour() {

    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }

    var redSlider = document.getElementById("redSlider");
    var greenSlider = document.getElementById("greenSlider");
    var blueSlider = document.getElementById("blueSlider");

    var msg = {
        command: "textCol",
        colR: redSlider.value,
        colG: greenSlider.value,
        colB: blueSlider.value
    };

    console.log(msg);

    var topic = "SmartDisplay";
    message = new Paho.MQTT.Message(JSON.stringify(msg));
    message.destinationName = topic;
    mqtt.send(message);
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
    //console.log("Message received ",r_message.payloadString);
    console.log(out_msg);
    document.getElementById("messages").innerHTML = out_msg;
}

function onConnected(recon, url) {
    console.log(" in onConnected " + reconn);
}

function onConnect() {
    // Once a connection has been made, make a subscription and send a message.
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
        console.log("port" + port);
    } else {
        port = localStorage.MQTTport;
    }
    if (s != "") {
        host = s;
        console.log("host");
    } else {
        host = localStorage.MQTThost;
    }

    console.log("connecting to " + host + " " + port);
    mqtt = new Paho.MQTT.Client(host, port, "clientjsaaa");
    //document.write("connecting to "+ host);
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
