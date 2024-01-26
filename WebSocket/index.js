// importamos las librerías requeridas
const path = require("path");
const express = require('express');
const cors = require('cors');
const app = express();
const server = require('http').Server(app);
const WebSocketServer = require('websocket').server;
const axios = require('axios');

const url = require('url');

let clientes=[];
let datos = [];

const miFuncion = require("./filtrar");

// Creamos el servidor de sockets y lo incorporamos al servidor de la aplicación
const wsServer = new WebSocketServer({
    httpServer: server,
    autoAcceptConnections: false,
});

//let clientPrincipal = null; // Variable para mantener la conexión del cliente principal

app.set("port", 3000);
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, "./public")));

wsServer.on("request", (request) =>{    
    const connection = request.accept(null, request.origin);

    // Obtener los parámetros de la URL de conexión
    const parameters = url.parse(request.resource, true).query;
    
    if (parameters.myCustomID === "001") {
        clientes.push(connection);
        //console.log(clientes.length + "clienteS +");
        //console.log(clientes.length + "clienteS +");
        enviarDatosHitoricos(connection);
    }

    connection.on("message", (message) => {
     
        const mensajeLimpio = message.utf8Data.replace(/\s*[\r\n]/gm, '');
        

        console.log("Rx: " + mensajeLimpio);
       
        const resultado = miFuncion.filtro(message.utf8Data);

        
        if (datos.length>29){
            datos.shift();
        }
       
        datos.push(mensajeLimpio);
        
        //sube a thinkspeak
        if (resultado != -1){

            const datosJSON = {
                trama: resultado.trama,
                lluvia: resultado.plv1,
                bateria: resultado.bat
            };
              
            const datosJSONString = JSON.stringify(datosJSON);
            console.log(datosJSONString)
            miFuncion.enviarDatosAThingSpeak(resultado.plv1,resultado.bat);
            
           
        }else {
            console.log('Dato invalido')
        }
        

        if (clientes.length>0){
            enviarClientesWeb(mensajeLimpio);
        }
    });
    
    connection.on("close", (reasonCode, description) => {
        
        clientes.splice(clientes.indexOf(connection), 1);
        
    });
});

// Iniciamos el servidor en el puerto establecido por la variable port (3000)
server.listen(app.get('port'), () =>{
    console.log('Servidor iniciado en el puerto: ' + app.get('port'));
})

function enviarClientesWeb (message) {
    for (var i=0; i<clientes.length; i++) {
        clientes[i].sendUTF(message);
    }
}

function enviarDatosHitoricos(cliente){
    for(var j=0; j<datos.length; j++){
            cliente.sendUTF(datos[j]);    
    }
}
