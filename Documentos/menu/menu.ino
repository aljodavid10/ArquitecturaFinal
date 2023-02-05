#include <LiquidCrystal.h>
#include <LiquidMenu.h>
#include <Keypad.h>
#include "DHT.h"
#include <stdio.h>
#include "AsyncTaskLib.h"
#include "DHTStable.h"

#include <EEPROM.h>

// Variables a modificar
byte umb_temp_alta = 29;
byte umb_temp_baja = 26;
byte umb_luz_alta = 500;
byte umb_luz_baja = 100;
//Definicion de keypad
const byte ROWS = 4;
const byte COLS = 4;
char key;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {3, 2, 1, 0};
byte colPins[COLS] = {7, 6, 5, 4};
char Password[5]={'2', '2', '5', '5'};//Contraseña con una posicion de mas para confirmar la contraseña
//Lcd
const int rs = 23, en = 33, d4 = 25, d5 = 27, d6 = 29, d7 = 31;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

Keypad teclado = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

const byte analogPin = A1;
unsigned short analogReading = 0;
unsigned short lastAnalogReading = 0;

unsigned int period_check = 1000;
unsigned long lastMs_check = 0;

unsigned int period_nextScreen = 1000;
unsigned long lastMs_nextScreen = 0;
unsigned long lastMs_previousScreen = 0;

int posicion = 0;

//Pantallas
LiquidLine pantalla_bienvenida1(0, 0, " --Bienvenido-- ");
LiquidLine pantalla_bienvenida2(0, 1, "Menu");
LiquidScreen pantalla(pantalla_bienvenida1, pantalla_bienvenida2);

LiquidLine menu1(0, 0, "1.Umb Tmp Alto");
LiquidLine menu2(0, 1, "2.Umb Tmp Bajo");
LiquidScreen pantalla2(menu1, menu2);

LiquidLine menu3(0, 0, "3.Umb ligth Alto");
LiquidLine menu4(0, 1, "4.Umb ligth Bajo");
LiquidScreen pantalla3(menu3, menu4);

LiquidLine menu5(0, 0, "4.Umb ligth Bajo");
LiquidLine menu6(0, 1, "5. reiniciar");
LiquidScreen pantalla4(menu5, menu6);

#define DHTPIN 10 
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);


#define buttonPin = 2; 
int buttonState = 0;  

//Led
#define rojo 21
#define verde 20
#define azul 19

//Pines de entrada buzzer 
#define BUZZER 35
#define pin_buzz 8

//Melodia buzzer
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
int duraciones[] = { 8, 8, 4, 4 };    // array con la duracion de cada nota
int melodia[] = { NOTE_B6, NOTE_C7, NOTE_CS7, NOTE_D7 };//// array con las notasa a reproducir

//Variables
char Input[5];  
int i=0;
int cont=0;
void inpass();
bool contrasenaCorrecta = false;
int temperatura = 0;
int humedad = 0;

LiquidLine submenu1(0, 0, "Umb Tmp Alto");
LiquidLine submenu2(0, 1, umb_temp_alta);
LiquidScreen subscreen(submenu1, submenu2);

LiquidLine submenu3(0, 0, "Umb Tmp Bajo");
LiquidLine submenu4(0, 1, umb_temp_baja);
LiquidScreen subscreen2(submenu3, submenu4);

LiquidLine submenu5(0, 0, "Umb ligth Alto");
LiquidLine submenu6(0, 1, umb_luz_alta);
LiquidScreen subscreen3(submenu5, submenu6);

LiquidLine submenu7(0, 0, "Umb ligth Bajo");
LiquidLine submenu8(0, 1, umb_luz_baja);
LiquidScreen subscreen4(submenu7, submenu8);
LiquidMenu menu(lcd);

//Tareas asincronicas
AsyncTask Clave(100,true,inpass);
AsyncTask asyncTask1(500);
AsyncTask asyncTask2(1000);


void setup() {
  lcd.begin(16, 2);
  Serial.begin(115200);

 asyncTask1.OnFinish = action1;
  asyncTask2.OnFinish = action2;
  asyncTask1.Start();
  asyncTask2.Start();
  Clave.Start();

  pinMode(rojo,OUTPUT);
  pinMode(verde,OUTPUT);
  pinMode(azul,OUTPUT);

  pinMode(analogPin, INPUT);

  // Agregar las pantallas de menu
  menu.add_screen(pantalla);
  menu.add_screen(pantalla2);
  menu.add_screen(pantalla3);
  menu.add_screen(pantalla4);
  menu.add_screen(subscreen);
  menu.add_screen(subscreen2);
  menu.add_screen(subscreen3);
  menu.add_screen(subscreen4);
}
//leer el valor del teclado 
int obtenerNuevoValorTecleado(){
  Serial.println("Funcion 1");
  int nuevoValor = 0;
  bool salir = false;
  do{
    char valorKeypad = teclado.getKey();
    if (valorKeypad >= '0' && valorKeypad <= '9')
    {
      nuevoValor *= 10;
      nuevoValor += valorKeypad - '0';
      Serial.println(nuevoValor);
    }
  }while(salir != true);
  Serial.println("Salir Funcion 1");
  return nuevoValor;
}

void color (unsigned char red, unsigned char green, unsigned char blue) // the color generating function
{
  analogWrite(rojo, red);
  analogWrite(azul, blue);
  analogWrite(verde, green);
}

void inpass(){
key = teclado.getKey(); 
lcd.setCursor(0,0);
lcd.println("Contrasena");
  if (key){     
    lcd.setCursor(i, 1);
    if(key!='*'&&i<4){
      lcd.println('*');
      Input[i]=key;
      i++;
    }
    else{   
      lcd.clear();
      lcd.setCursor(0, 1);  
      i=0;
      Serial.println(Input);
      Serial.println(Password);
      if(strncmp(Password,Input,5)==0){
        cont=5;        
      }else{
        lcd.println("Incorrecta");
        cont++;
        color(0,255,0);               
        delay(2000);
        lcd.clear();             
      }              
      lcd.setCursor(0, 0); 
    }
  }  
}
void mostrar_main_menu() {
  char valorKeypad = teclado.getKey();
  int numeroObtenido=0;

  // Verifica si alguna tecla fue presionada
  if (millis() - lastMs_check > period_check) {
    lastMs_check = millis();
    analogReading = analogRead(analogPin);

    if (analogReading != lastAnalogReading) {
      lastAnalogReading = analogReading;
      menu.update();
    }
  }

  // Menu principal
  if (valorKeypad == '#') {
    if (posicion < 3) {
      lastMs_previousScreen = millis();
      menu.next_screen();
      posicion++;
    }
  }
  // Menu secundario
  if (valorKeypad == '1') {
    lastMs_previousScreen = millis();
    menu.change_screen(&subscreen);
    umb_temp_alta = obtenerNuevoValorTecleado();
    posicion = 4;
  }

  if (valorKeypad == '2') {
    lastMs_previousScreen = millis();
    menu.change_screen(&subscreen2);
    umb_temp_baja = obtenerNuevoValorTecleado();
    posicion = 5;
  }

  if (valorKeypad == '3') {
    lastMs_previousScreen = millis();
    menu.change_screen(&subscreen3);
    umb_luz_alta = obtenerNuevoValorTecleado();
    posicion = 6;
  }

  if (valorKeypad == '4') {
    lastMs_previousScreen = millis();
    menu.change_screen(&subscreen4);
    umb_luz_baja = obtenerNuevoValorTecleado();
    posicion = 7;
  }

  if (valorKeypad == '5') {
    lastMs_previousScreen = millis();
    menu.change_screen(&pantalla);
    umb_temp_alta = 29;
    umb_temp_baja = 26;
    umb_luz_alta = 500;
    umb_luz_baja = 100;
    posicion = 0;
  }
}

void action1()
{
  DEBUG("Expired 2");
  asyncTask1.Start();
  temperatura = dht.readTemperature();
  lcd.setCursor(0, 0);
  lcd.print("Temperatura :" + temperatura);
  
}

void action2()
{
  DEBUG("Expired 1");
  asyncTask2.Start();
  humedad = dht.readHumidity();
  lcd.setCursor(0, 1);
  lcd.print("Humedad : " + humedad);
}

void loop(){
  if(cont<3){
    Clave.Update();
  }else if(cont == 3){
    lcd.println("Bloqueado");
    for (int i = 0; i < 25; i++) {      // bucle de 25 iteraciones
          int duracion = 2000 / duraciones[i];    // duracion de la nota ms
          tone(BUZZER, melodia[i], duracion); // ejecuta el tono con la duracion
          int pausa = duracion * 1.30;      // calcula pausa
          delay(pausa);         // demora con valor de pausa
          noTone(BUZZER);          
        }
        cont=4;
  }else if(cont == 5){
        lcd.println("Correcta");       
        color(255,0,0);  
        cont=4;  
        contrasenaCorrecta = true;
  }
  if(contrasenaCorrecta){
    mostrar_main_menu();
  }
}
