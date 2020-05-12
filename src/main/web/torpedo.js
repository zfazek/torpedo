var button_connect = document.getElementById("connect");
var output = document.getElementById("output");
var socket;
var canvas = document.getElementById("canvas");
var ctx = canvas.getContext("2d");

var canvasLeft = canvas.offsetLeft + canvas.clientLeft;
var canvasTop = canvas.offsetTop + canvas.clientTop;

var table = [];
var SIZE = 10;

init_game();

canvas.addEventListener('click', function(event) {
    if (socket && socket.readyState == WebSocket.OPEN) {
        var rect = canvas.getBoundingClientRect();
        var scaleX = canvas.width / rect.width;
        var scaleY = canvas.height / rect.height;
        var mouseX = (event.clientX - rect.left) * scaleX;
        var mouseY = (event.clientY - rect.top) * scaleY;
        var x = Math.floor(mouseX / (canvas.width / SIZE));
        var y = Math.floor(mouseY / (canvas.height / SIZE));
        var idx = y * SIZE + x;
        var elem = table[idx];
        table[idx] = 1 - elem;
        draw();
    }
}, false);

function onopen() {
    button_connect.innerText = "Close";
    output.innerHTML = "";
    draw();
};

function onclose() {
    button_connect.innerText = "Connect";
    output.innerHTML = "";
    draw();
}

function onmessage(e) {
    if (e.data[0] == 'v') {
        if (e.data[1] == 'O' && e.data[2] == 'K') {
            draw();
            return;
        }
        random_table();
        return;
    }
    output.innerHTML = e.data + "\n";
    draw();
};

function init_table() {
    table = [
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,
    ];
    draw();
}

function isIdxInindices(indices, idx) {
    for (var i = 0; i < indices.length; i++) {
        if (indices[i] == idx) {
            return true;
        }
    }
    return false;
}

function random_table() {
    table = [0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0];
    /*
    var counter = 20;
    var indices = [];
    while (counter > 0) {
        var idx = Math.floor(Math.random() * SIZE * SIZE);
        if (!isIdxInindices(indices, idx)) {
            indices.push(idx);
            counter--;
        }
    }
    init_table();
    for (var i = 0; i < indices.length; i++) {
        table[indices[i]] = 1;
    }
    */
    draw();
    if (socket && socket.readyState == WebSocket.OPEN) {
        socket.send("v" + table.toString());
    }
}

function init_game() {
    init_table();
}

function send_table() {
    draw();
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

function draw() {
    var size = canvas.width / SIZE;
    for (var i = 0; i < SIZE; i++) {
        for (var j = 0; j < SIZE; j++) {
            ctx.beginPath();
            if (socket && socket.readyState == WebSocket.OPEN) {
                ctx.strokeStyle = "black";
            } else {
                ctx.strokeStyle = "gray";
            }
            ctx.rect(j * size, i * size, size, size);
            ctx.strokeRect(j * size, i * size, size, size);
            if (table[i * SIZE + j] == 0) {
                ctx.fillStyle = "gray";
            } else {
                ctx.fillStyle = "blue";
            }
            ctx.fill();
        }
    }
}
