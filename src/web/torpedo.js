var button_connect = document.getElementById("connect");
var output = document.getElementById("output");
var socket;

var table = [];
var SIZE = 10;

function onopen() {
    output.innerHTML += "Status: Connected\n";
    init_game();
    button_connect.innerText = "Close";
};

function onclose() {
    output.innerHTML += "Status: Closed\n";
    button_connect.innerText = "Connect";
}

function onmessage(e) {
    output.innerHTML += "Server: " + e.data + "\n";
};

function init_game() {
    table = [
    1,0,1,0,1,0,1,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        1,1,0,0,1,0,1,1,0,0,
        0,0,0,0,0,0,0,0,0,0,
        1,1,1,0,1,1,1,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0
    ];
    /*
    table = [
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        1,0,1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
    ];
        */
}

function send_table() {
    if (socket && socket.readyState == WebSocket.OPEN) {
        socket.send(table.toString());
    }
}

function connect() {
    if (button_connect.innerText == "Connect") {
        socket = new WebSocket("ws://ongroa.com:8080/trpd");
        socket.addEventListener('open', onopen);
        socket.addEventListener('message', onmessage);
        socket.addEventListener('close', onclose);
    } else if (button_connect.innerText == "Close") {
        socket.send("close");
    }
}
