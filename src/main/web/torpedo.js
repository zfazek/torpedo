var button_connect = document.getElementById("connect");
var button_random = document.getElementById("random");
var button_send = document.getElementById("send");
var button_clear = document.getElementById("clear");
var output = document.getElementById("output");
var socket;
var canvas = document.getElementById("canvas");
var ctx = canvas.getContext("2d");

var canvasLeft = canvas.offsetLeft + canvas.clientLeft;
var canvasTop = canvas.offsetTop + canvas.clientTop;

var own_table = [];
var other_table = [];
var can_click = false;
var shoot = false;
var end = false;
var random = false;
var SIZE = 10;

set_buttons_disabled(true);
init_table();

canvas.addEventListener('click', function(event) {
    if (socket && socket.readyState == WebSocket.OPEN && can_click) {
        var rect = canvas.getBoundingClientRect();
        var scaleX = canvas.width / rect.width;
        var scaleY = canvas.height / rect.height;
        var mouseX = (event.clientX - rect.left) * scaleX;
        var mouseY = (event.clientY - rect.top) * scaleY;
        var x = Math.floor(mouseX / (canvas.width / SIZE));
        var y = Math.floor(mouseY / (canvas.height / SIZE));
        var idx = y * SIZE + x;
        if (shoot) {
            if (other_table[idx] == 0) {
                can_click = false;
                socket.send(idx);
                output.innerText = "";
            }
        } else {
            var elem = own_table[idx];
            own_table[idx] = 1 - elem;
            draw(own_table);
        }
    }
}, false);

function onopen() {
    button_connect.innerText = "Close";
    init_game();
};

function onclose() {
    button_connect.innerText = "Connect";
    can_click = false;
    shoot = false;
    set_buttons_disabled(true);
}

function onmessage(e) {
    if (e.data == "OK") {
        can_click = false;
        set_buttons_disabled(true);
        output.innerHTML = e.data;
        return;
    }
    if (e.data == "You missed!") {
        can_click = false;
        set_buttons_disabled(true);
        draw(own_table);
        output.innerHTML = e.data;
        return;
    }
    if (e.data == "You won!") {
        draw(other_table);
        can_click = false;
        end = true;
        output.innerHTML = e.data;
        return;
    }
    if (e.data == "You lost!") {
        draw(other_table);
        can_click = false;
        end = true;
        output.innerHTML = e.data;
        return;
    }
    if (e.data == "Shoot!") {
        draw(other_table);
        can_click = true;
        shoot = true;
        output.innerHTML = e.data;
        return;
    }
    if (e.data.length > 0 && e.data[0] >= 'A' && e.data[0] <= 'Z') {
        draw(own_table);
        can_click = true;
        output.innerHTML = e.data;
        return;
    }
    if (end && e.data.length == SIZE * SIZE) {
        console.log(other_table);
        console.log(e.data);
        for (var i = 0; i < e.data.length; i++) {
            if (other_table[i] == 0) {
                other_table[i] = parseInt(e.data[i]);
            }
        }
        draw(other_table);
        return;
    }
    if (random && e.data.length == SIZE * SIZE) {
        own_table = [];
        for (var i = 0; i < e.data.length; i++) {
            own_table.push(parseInt(e.data[i]));
        }
        random = false;
        draw(own_table);
        return;
    }
    if (!end && e.data.length == SIZE * SIZE) {
        other_table = [];
        for (var i = 0; i < e.data.length; i++) {
            other_table.push(parseInt(e.data[i]));
        }
        return;
    }
    if (e.data.length > 0 && e.data.length < 4) {
        if (e.data[0] == 'm') {
            can_click = false;
            var idx = parseInt(e.data.slice(1));
            output.innerHTML = "He/she missed";
            own_table[idx] = 2;
            draw(own_table);
            return;
        }
        output.innerText = "You got a hit";
        can_click = false;
        var idx = parseInt(e.data);
        own_table[idx] = 5;
        draw(own_table);
        return;
    }
}

function init_table() {
    own_table = [
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
    draw(own_table);
}

function isIdxInindices(indices, idx) {
    for (var i = 0; i < indices.length; i++) {
        if (indices[i] == idx) {
            return true;
        }
    }
    return false;
}

function random_own_table() {
    random = true;
    socket.send("random");
}

function set_buttons_disabled(flag) {
    button_random.disabled = flag;
    button_send.disabled = flag;
    button_clear.disabled = flag;
}

function init_game() {
    for (var i = 0; i < SIZE * SIZE; i++) {
        other_table[i] = 0;
        if (own_table[i] > 0) {
            own_table[i] = 1;
        } else {
            own_table[i] = 0;
        }
    }
    can_click = true;
    shoot = false;
    end = false;
    random = false;
    set_buttons_disabled(false);
    output.innerText = "";
    draw(own_table);
}

function send_table() {
    if (socket && socket.readyState == WebSocket.OPEN) {
        socket.send(own_table.toString());
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
        set_buttons_disabled(true);
        output.innerText = "";
    }
}

function draw(table) {
    var size = canvas.width / SIZE;
    for (var i = 0; i < SIZE; i++) {
        for (var j = 0; j < SIZE; j++) {
            ctx.beginPath();
            if (socket && socket.readyState == WebSocket.OPEN) {
                ctx.strokeStyle = "black";
            } else {
//                ctx.strokeStyle = "gray";
                ctx.strokeStyle = "black";
            }
            ctx.rect(j * size, i * size, size, size);
            ctx.strokeRect(j * size, i * size, size, size);
            if (table[i * SIZE + j] == 0) {
                ctx.fillStyle = "gray";
            } else if (table[i * SIZE + j] == 1) {
                ctx.fillStyle = "blue";
            } else if (table[i * SIZE + j] == 2) {
                ctx.fillStyle = "black";
            } else if (table[i * SIZE + j] == 3) {
                ctx.fillStyle = "green";
            } else if (table[i * SIZE + j] == 4) {
                ctx.fillStyle = "yellow";
            } else if (table[i * SIZE + j] >= 5) {
                ctx.fillStyle = "red";
            }
            ctx.fill();
        }
    }
}
