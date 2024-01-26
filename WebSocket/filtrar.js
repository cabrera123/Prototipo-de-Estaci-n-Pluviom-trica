exports.filtro = function (cadena) {

  let trama = null;
  let bat = null;
  let plv1 = null;

  const inicioUNI = cadena.indexOf("#UNI#");  //devuelve el indice donde comienza #UNI#
  
  if (inicioUNI !== -1) {
    const subcadena = cadena.substring(inicioUNI); //Corta la cadena desde #UNI#
    const elementos = subcadena.split("#"); //Divide la subcadena en partes separas por '#'

    trama = elementos[2];    //Obtien el numero de la trama
    bat = elementos[3].slice(4);  //Obtiene el valor de la bateria
    plv1 = elementos[4].slice(5); //obtiene el valor del pluviometro
  }
  // Devolver un objeto con los valores de plv y bat
  if (trama!=null && plv1!=null && bat!=null){
    return {trama, plv1, bat};
  } else {
    return -1;
  }
}

const axios = require('axios');

exports.enviarDatosAThingSpeak = function (plv1Value, batValue) {
  const apiKey = 'TU7IWTGG4GI14PCZ';
  const url = `https://api.thingspeak.com/update?api_key=${apiKey}&field1=${plv1Value}&field2=${batValue}`;

  axios.get(url)
    .then(response => {
      console.log('Respuesta de ThingSpeak:', response.data);
    })
    .catch(error => {
      console.error('Error al enviar los datos a ThingSpeak:', error);
    });
}





