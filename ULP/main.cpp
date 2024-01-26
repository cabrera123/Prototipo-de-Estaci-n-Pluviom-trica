#include <Arduino.h>
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"
#include "ulp_main.h"
#include "ulptool.h"

#define RAINFACTOR        0.2           // Resolución del pluviometro
#define ULPSLEEP          5000          // Tiempo de suspension del coprocesador ULP (5ms)
#define TIMEFACTOR        1000000       // factor entre segundos y microsegundos
#define TIMESLEEP         120           // Tiempo de suspension del ESP32
#define PIN_RAIN          GPIO_NUM_13   // GPIO asociado al pluviometro
#define PIN_BATTERY       GPIO_NUM_32   // pin ADC para leer el Voltaje de la bateria
#define PIN_XBEE          GPIO_NUM_19   // GPIO para despertar el XBee
#define RXD2 16   //UART2 RX
#define TXD2 17   //UART2 RX

RTC_DATA_ATTR int bootCount = 0;        // Contador numero de trama transmitida

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

static void initULP();                // Función Inicio del ULP
static uint32_t getPulseCount(void);  // Funcion para obtener el numero de pulsos del ULP 
static void rainTest();               // Función principal para realizar los calculos y transmision del nivel de lluvia
void initSleep(void);                 // Funcion para iniciar el modo de suspensión DeepSleep


static void gpioReset();
static void gpioIsolate();
static void gpioHoldOn();
static void gpioHoldOff();



void setup() {
  Serial.begin(115200);                           // Configuración comunicación serial para control
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);  // Configuración comunicación serial con XBee
  setCpuFrequencyMhz(80);             // Disminuye velocidad de procesador a 80MHz
  pinMode(PIN_XBEE, OUTPUT);          // Configura pin XBEE como salida
  


  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();  // Motivo del despertar del ESP32
  if (cause != ESP_SLEEP_WAKEUP_TIMER) {              // Si el despertar no es devido al temporizador (120s) inicia ULP.
    Serial.printf("Not ULP TIMER wakeup\n");
    initULP();
  } else {                                            // Si el despertar es devido al temporizador (120s), entra a rainTest();
    digitalWrite(PIN_XBEE, HIGH);               // Envia la señal para despertar al XBee.
    printf("ULP wakeup, saving pulse count\n");
    rainTest();                                   
  }
  
  digitalWrite(PIN_XBEE, LOW);                  // Envia la señal para suspender al XBee.
  
  delay(50);
  //inicia DeepSleep
  initSleep();
}

void loop() {

}

static void initULP(void)
{

  esp_err_t err_load = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
  ESP_ERROR_CHECK(err_load);

  /* GPIO usado para contar los pulsos. */
  gpio_num_t gpio_num = PIN_RAIN;
  assert(rtc_gpio_desc[gpio_num].reg && "GPIO used for pulse counting must be an RTC IO");

  ulp_debounce_counter = 3;   // Valor contador antirrebote (ciclos del ulp)
  ulp_debounce_max_count = 3; // Valor máximo contador antirrebota 
  ulp_pulse_edge = 1;         // Conador de flancos
  ulp_next_edge = 1;          // Valor proximo flanco
  ulp_io_number = rtc_gpio_desc[gpio_num].rtc_num; 

  /* Inicializar GPIO seleccionado como RTC_GPIO */
  rtc_gpio_init(gpio_num);
  rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_en(gpio_num);
  rtc_gpio_pullup_dis(gpio_num);
  rtc_gpio_hold_en(gpio_num);

  
  /* Establece el periodo de despertar del ULP.
   * El ancho mínimo del pulso, debe ser: T * (ulp_debounce_counter + 1). */
  ulp_set_wakeup_period(0, ULPSLEEP);

  /* Inicia el programa ULP */
  esp_err_t err_run = ulp_run(&ulp_entry - RTC_SLOW_MEM);
  ESP_ERROR_CHECK(err_run);
}

//Calculo nivel de lluvia
void rainTest() {
  float rain = 0;
  //obtiene el numero de pulsos del sensor.
  int pulsesRaw = getPulseCount();
  
  if (pulsesRaw >= 0) {
    //espera 400ms que el xbee encienda
    delay(400);

    if(bootCount>=256){ // Si contador numero de trama es mayor igual a 256 vuelve a 0.
      bootCount=0;
    }
      

    // Convierte los numeros de pulsos a mm de lluvia.
    rain = (float)pulsesRaw * RAINFACTOR;
      
    //Lee el volor analogico de la bateria.
    uint16_t batteryValue = 0;
    for(uint8_t i =0; i<5;i++){
      batteryValue = batteryValue + analogRead(PIN_BATTERY);  
      delay(20);
    }
    
    batteryValue = batteryValue / 5;
    //convierte valor digital a voltaje.
    float batteryVoltage = ((2.14*3.3*batteryValue)/4095);
    
    double batteryPercentage;
    //Convierte el voltaje medido a porcentaje de carga.
    if (batteryVoltage >= 4.1) {
      batteryPercentage = 100;
    } else if (batteryVoltage >= 4.0) {
      batteryPercentage = 94;
    } else if (batteryVoltage >= 3.9) {
      batteryPercentage = 85;
    } else if (batteryVoltage >= 3.8) {
      batteryPercentage = 76;
    } else if (batteryVoltage >= 3.7) {
      batteryPercentage = 65;
    } else if (batteryVoltage >= 3.6) {
      batteryPercentage = 54;
    } else if (batteryVoltage >= 3.5) {
      batteryPercentage = 26;
    } else if (batteryVoltage >= 3.4) {
      batteryPercentage = 12;
    } else{
      batteryPercentage = 0;
    }
    // Envia la trama al XBee con la informacion del nivel de lluvia, nivel de carga de la bateria y el contador de la trama.
    Serial2.print(F("#13A200414F1534#UNI#"));
    Serial2.print(bootCount);
    Serial2.print(F("#BAT:"));
    Serial2.print(batteryPercentage);
    Serial2.print(F("#PLV1:"));
    Serial2.print(rain);
    Serial2.println(F("#"));

  
    delay(800);
    //aumenta el contador de la trama
    bootCount++;
  }
  
  
}

// Configura e incia el modo de suspension DeepSleep.
void initSleep(void) {
  Serial.println(F("Preparing deep sleep now"));
  esp_sleep_enable_timer_wakeup(TIMEFACTOR*TIMESLEEP);

  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

 esp_deep_sleep_disable_rom_logging();

  Serial.println(F("Going into deep sleep now"));
  Serial.println(F("-------------------------"));
  esp_deep_sleep_start();
}

// Obtiene el numero de pulsos del ULP.
static uint32_t getPulseCount(void) {
  /* Convierte el numero de flancos registrados por el ULP en pulsos */
  uint32_t pulse_count_from_ulp = (ulp_edge_count & UINT16_MAX) / 2;
  /* En caso de que el numero de flancos sea inpar, deja el valor de uno para la sigiente medida */
  ulp_edge_count = ulp_edge_count % 2;
  return pulse_count_from_ulp;
}

