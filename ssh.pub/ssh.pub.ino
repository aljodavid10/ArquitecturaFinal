// inclucion de librerias necesarias
#include "AsyncTaskLib.h"
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <EEPROM.h>
#include "DHTStable.h"

DHTStable DHT;

#define DEBUG(a) Serial.print(millis()); Serial.print(": "); Serial.println(a); // definicion del serial debug

// definicion de umbrales de sensores
byte umb_temp_alta = 29;
byte umb_temp_baja = 26;
byte umb_temp_alarma  = 24;
byte umb_luz_alta=  800;  
byte umb_luz_baja= 300;
byte umb_luz = 34;


//variables keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {14, 15, 16, 17}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {18, 19, 20, 21}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


//variables contraseña
char clave[4] = {'2', '2', '5', '5'}; // Aqui va la clave, Para cambiar el tamaño de la clave se debe modificar el array
int posicion = 0;  // necesaria para contar los dijitos de la clave
int cursor = 0;    // posicion inicial de la clave en el LCD
int intentos = 0;   //para contar las veces de clave incorrecta
void press_key(char key);
bool f_clave_correcta = false;

//declaraciones de auxiliares
bool f_mensaje_ini_bloq = true;
bool f_mensaje_ini_conf = true;
void mostrar_main_menu();
unsigned long tiempo = 0;
byte pos_memoria = 0;


// pines para LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Variables para el sensor de luz
#define pin_photo A0
int photo_value = 0;
void read_photoresistor(void);

// VAriables para el sensor de temperatura
#define pin_temp 7
#define beta 4090
#define resistance 10
float temp_value = 0.0;
float temperatura = DHT.getTemperature();
void read_temperature(void);
int max_temp = 50;
int min_temp = 0;

//variables para el control del buzzer analogico
#define pin_buzz 8
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
int duraciones[] = { 8, 8, 4, 4 };    // array con la duracion de cada nota
int melodia[] = { NOTE_B6, NOTE_C7, NOTE_CS7, NOTE_D7 };//// array con las notasa a reproducir

// Definiciones de las tareas asincronas
AsyncTask asyncTaskTemp(2000, true, read_temperature);
AsyncTask asyncTaskPhoto(100, true, read_photoresistor);
AsyncTask asyncTaskMainMenu(0, false, mostrar_main_menu);


// definicion de los pines para el LEDrgb
int ledRojo = 9;  // pin para el LED rojo * cambiar pines del led a pwm para rgb completo en analogo
int ledVerde = 13; // pin para el LED verde
int ledAzul = 10;  // pin para el LED azul


// enumerador de los estados finitos
enum estado
{
  estado_bloq,// sistema bloqueado
  estado_conf,// estado de configuracion
  estado_Run,//Donde sucede el monitoreo
  estado_alarm// estado de alarma
};

estado currentState; // Variable global para control de estados

void read_photoresistor()// Funcion encargada de leer y procesar la informacion del sensor de luz
{
  photo_value = analogRead(pin_photo);// se lee en el pin analogo de el sensor
}

void read_temperature()// Funcion encargada de leer y procesar la informacion del sensor de temperatura
{
  // se usa la sigiente formula para convertir la informacion dada por el sensor en grados centigrados
  /*long a = 1023 - analogRead(pin_temp);
  temperatura = beta / (log((1025.0 * 10 / a - 10) / 10) + beta / 298.0) - 273.0;
  */
  delay(1000);
  int chk = DHT.read11(pin_temp);
  switch (chk)
  {
    case DHTLIB_OK:  
      Serial.print("OK,\t"); 
      break;
    case DHTLIB_ERROR_CHECKSUM: 
      Serial.print("Checksum error,\t"); 
      break;
    case DHTLIB_ERROR_TIMEOUT: 
      Serial.print("Time out error,\t"); 
      break;
    default: 
      Serial.print("Unknown error,\t"); 
      break;
  }
}

void setup()
{
  
  Serial.begin(9600);
  Serial.println("inicio");// se inicia el serial para debug

  // inicializacion del lcd
  lcd.begin(16, 2);
  lcd.clear();

  //se inicializan los pines del led  y buzzer como salidas
  //leds
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAzul, OUTPUT);
  //buzzer
  pinMode(pin_buzz, OUTPUT);

  currentState =  estado::estado_bloq; // se inicializa la maquina en estado A
  //funcion_bloq();

  // inicio de las tareas asincronas
  asyncTaskTemp.Start();
  asyncTaskPhoto.Start();

  asyncTaskMainMenu.Start();
}

void loop()
{
  actualizar_estado(); // actualiza la maquina de estados segun la entrada de temperatura
  // se actualizan las tareas asincronas
  asyncTaskTemp.Update();
  asyncTaskPhoto.Update();
}


void actualizar_estado() // se hace la seleccion de funciones a realizar segun el estado actual
{
  switch (currentState)
  {
    case estado::estado_bloq: funcion_bloq();   break;
    case estado::estado_conf: funcion_conf();   break;
    case estado::estado_Run: funcion_run();   break;
    case estado::estado_alarm: funcionAlarma(); break;
    default: break;
  }
}
void funcion_run() {
  read_temperature();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("temp: ");
  lcd.setCursor(0,1);
  lcd.print(DHT.getTemperature());
  delay(2000);
  if(DHT.getTemperature()>umb_temp_alarma){
    currentState = estado::estado_alarm;
    actualizar_estado();
  }else if(temperatura<umb_temp_baja){
    digitalWrite(ledAzul, HIGH);
    delay(1000);
    digitalWrite(ledAzul, LOW);
  } else{
    digitalWrite(ledVerde, HIGH);
    delay(1000);
    digitalWrite(ledVerde, LOW);
  }
  delay(3000);
  currentState = estado::estado_Run;
  asyncTaskMainMenu.Start();
}
void funcion_bloq() {
  if (f_mensaje_ini_bloq) {
    mensaje_inicial_bloq();
    f_mensaje_ini_bloq = false;
  }
  char key = keypad.getKey();
  if (key) {
    press_key(key);
  }
  if (f_clave_correcta) {
    currentState = estado::estado_conf;
    f_mensaje_ini_bloq = true;
    f_clave_correcta = false;
    lcd.clear();
    lcd.setCursor(0, 0);
  }
}
void funcion_conf() {
  
  char key = keypad.getKey();
  if (key) {
    funciones_main_menu(key);
  }
  if (f_mensaje_ini_bloq) {
    mostrar_main_menu();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ingrese opcion");
    f_mensaje_ini_bloq = false;
  }
}

void mostrar_main_menu() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1.Umb Tmp Alto");
  lcd.setCursor(0, 1);
  lcd.print("2.Umb Tmp Bajo");
  lcd.setCursor(0, 1);
  tiempo = millis();
  while (millis() < tiempo + 1000UL) {
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("2.Umb Tmp Bajo");
  lcd.setCursor(0, 1);
  lcd.print("3.Umb luz Alto");
  lcd.setCursor(0, 1);
  tiempo = millis();
  while (millis() < tiempo + 1000UL) {
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("3. Umb luz Alto");
  lcd.setCursor(0, 1);
  lcd.print("4. Umb luz Bajo");
  lcd.setCursor(0, 1);
  tiempo = millis();
  while (millis() < tiempo + 1000UL) {
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("4. Umb luz Bajo");
  lcd.setCursor(0, 1);
  lcd.print("5. Salir");
  lcd.setCursor(0, 1);
  tiempo = millis();
  while (millis() < tiempo + 1000UL) {
  }
  
}

void funciones_main_menu(char key) {  
  
  switch (key)
  {
    case '1': {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("umbral temp alta");
        lcd.setCursor(0, 1);
        lcd.print(umb_temp_alta);
        delay(1600);
        lcd.clear();
        lcd.setCursor(0,0);
      };   break;
    case '2': {
      lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("umbral temp baja");
        lcd.setCursor(0, 1);
        lcd.print(umb_temp_baja);
        delay(1600);
        lcd.clear();
      };   break;
    case '3': {
       lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("umbral luz alta");
        lcd.setCursor(0, 1);
        lcd.print(umb_luz_alta);
        delay(1600);
        lcd.clear();
      };   break;
    case '4': {
         lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("umbral luz baja");
        lcd.setCursor(0, 1);
        lcd.print(umb_luz_baja);
        delay(1600);
        lcd.clear();
      };   break;
    case '5': {
        lcd.clear();
        asyncTaskMainMenu.Update();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("SALIR");
        delay(1600);
      };   break;
    default: break;
  }
  currentState = estado::estado_Run;
  actualizar_estado();
}
void sonar_buzz() { // funcion encargada de hacer sonar el buzzer
  for (int i = 0; i < 3; i++) {      // bucle repite 4 veces, 1 por cada nota
    int duracion = 1000 / duraciones[i];    // duracion de la nota en milisegundos
    tone(pin_buzz, melodia[i], duracion);  // ejecuta el tono con la duracion
    int pausa = duracion * 1.30;      // calcula pausa
    delay(pausa);         // demora con valor de pausa
    noTone(pin_buzz);        // detiene reproduccion de tono
  }
}
void mensaje_inicial_bloq() {
  // mensaje inicial
  lcd.setCursor(0, 0);
  lcd.print(" SISTEMA DE");
  lcd.setCursor(0, 1);
  lcd.print("  SEGURIDAD !!");
  delay(400);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ingrese la clave");
  lcd.setCursor(0, 1);
}

void clave_correcta() {
  lcd.clear();
  lcd.setCursor(0, 0);     // situamos el cursor el la pos 0 de la linea 0.
  lcd.print("Clave correcta!! ");
  // escribimos en LCD
  digitalWrite(ledVerde, HIGH); // encendemos el verde
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);// situamos el cursor el la pos 0 de la linea 0.
  lcd.print("bienvenido!");
  delay(1000);
  digitalWrite(ledVerde, LOW); 
  f_clave_correcta = true;
  posicion = 0;
  intentos = 0;
  cursor = 0;
  lcd.clear();
  //lcd.setCursor(0, 0);
  //delay(1000);
}

void clave_incorrecta() {
  lcd.clear();
  cursor = 0;
  posicion = 0;
  intentos++;
  lcd.setCursor(0, 0);     // situamos el cursor el la pos 0 de la linea 0.
  lcd.print("Clave erronea!");         // escribimos en LCD
  digitalWrite(ledRojo, LOW);
  digitalWrite(ledAzul, HIGH);
  digitalWrite(ledVerde, LOW);
  delay(1000);
  digitalWrite(ledRojo, LOW);
  digitalWrite(ledAzul, LOW);
  digitalWrite(ledVerde, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ingrese la clave");
  lcd.setCursor(0, 1);
}

void sistema_bloqueado() {
  intentos = 0;
  lcd.clear();
  lcd.setCursor(0, 0);     // situamos el cursor el la pos 0 de la linea 0.
  lcd.print("Clave bloqueada");         // escribimos en LCD
  lcd.setCursor(0, 1);
  lcd.print("Espere...");
  //led rojo
  digitalWrite(ledRojo, HIGH);
  digitalWrite(ledAzul, LOW);
  digitalWrite(ledVerde, LOW);
  delay(2000);
  digitalWrite(ledRojo, LOW);
  lcd.clear();
  sonar_buzz();
  lcd.setCursor(0, 0);
  lcd.print("Ingrese la clave");
  lcd.setCursor(0, 1);
}

void press_key(char key) {

  lcd.print("*");
  cursor++;
  if (key == clave[posicion] && cursor <= 4) {
    posicion ++; // aumentamos posicion si es correcto el digito
    if (posicion == 4) { // comprobamos que se han introducido los 4 correctamente
      clave_correcta();
    }
  } else {
    posicion = 0;
  }
  if (cursor >= 4) {
    clave_incorrecta();
    if (intentos == 3 ) {
      sistema_bloqueado();
    }
  }

}

void funcionAlarma(){
  digitalWrite(ledRojo, HIGH);
  sonar_buzz();
  digitalWrite(ledRojo, LOW);
  delay(1000);
  currentState = estado::estado_Run;
  actualizar_estado();
}
//FIN DEL PROGRAMA
