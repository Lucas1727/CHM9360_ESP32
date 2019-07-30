var webSocket = "ws://192.168.1.45:80";
var connected_flag = 0
var mqtt;
var reconnectTimeout = 2000;
var host = "192.168.1.49";
var port = 9001;

function JavaScriptTest() {
    alert("Command recieved successfully!");
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
    if (topic == "")
        message.destinationName = "test-topic"
    else
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
        text: "Aberystwyth,GB"
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

function MQTTSendTestMessage() {
    document.getElementById("messages").innerHTML = "";
    if (connected_flag == 0) {
        out_msg = "<b>Not Connected so can't send</b>"
        console.log(out_msg);
        document.getElementById("messages").innerHTML = out_msg;
        return false;
    }
    var msg = {
        command: "message",
        text: "Test"
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
    if (topic == "")
        message.destinationName = "test-topic"
    else
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
    if (topic == "")
        message.destinationName = "test-topic"
    else
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
    }
    if (s != "") {
        //host=s;
        console.log("host");
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
