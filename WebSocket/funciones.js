

// La función init se ejecuta cuando termina de cargarse la página
function init() {
    // Conexión con el servidor de websocket
    wsConnect();
}

// Invoca esta función para conectar con el servidor de WebSocket
function wsConnect() {
   
    websocket = new WebSocket("wss://render-websocket-test.onrender.com?myCustomID=001");
    
    // Asignación de callbacks
   
    websocket.onclose = function (evt) {
        onClose(evt)
    };
    websocket.onmessage = function (evt) {
        onMessage(evt)
    };
    websocket.onerror = function (evt) {
        onError(evt)
    };
}



// Se ejecuta cuando la conexión con el servidor se cierra
function onClose(evt) {


    // Intenta reconectarse cada 2 segundos
    setTimeout(function () {
        wsConnect()
    }, 2000);
}

// Se invoca cuando se recibe un mensaje del servidor
function onMessage(evt) {
    // Agregamos al textarea el mensaje recibido
    var area = document.getElementById("mensajes")
    area.innerHTML += evt.data + "\n";
    area.scrollTop = area.scrollHeight;
}

// Se invoca cuando se presenta un error en el WebSocket
function onError(evt) {
    console.log("ERROR: " + evt.data);
}

// Se invoca la función init cuando la página termina de cargarse
window.addEventListener("load", init, false);
