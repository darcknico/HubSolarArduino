
/******************************************************************
 *                           Librerias                            *
 ******************************************************************/ 

#include <DHT.h>
#include <DHT_U.h>
#include "Arduino.h"
#include <EDB.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>


/******************************************************************
 *                    Definicion de variables                     *
 ******************************************************************/
 
// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 2
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11

#define SD_PIN 10  // SD Card CS pin
#define TABLE_SIZE 8192

char* db_name = "/db/logs.db";
File dbFile;

// Definicion de la tabla.
struct LogEvent {
    int id;
    float temperatura;
    float humedad;
    float indiceCalor;
}
logEvent;

// The read and write handlers for using the SD Library
// Also blinks the led while writing/reading
void writer (unsigned long address, byte data) {
    digitalWrite(13, HIGH);
    dbFile.seek(address);
    dbFile.write(data);
    dbFile.flush();
    digitalWrite(13, LOW);
}

byte reader (unsigned long address) {
    digitalWrite(13, HIGH);
    dbFile.seek(address);
    byte b = dbFile.read();
    digitalWrite(13, LOW);
    return b;
}

// Creamos un objecto EDB con los manejadores write y reader
EDB db(&writer, &reader);
 
// Inicializamos el sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

int id = 0;

SoftwareSerial blueToothSerial(5,6);

void setup() {
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  Serial.begin(9600);
  blueToothSerial.begin(9600);
  blueToothSerial.write("AT+NAME=HubSolar");
  
  // Comenzamos el sensor DHT
  dht.begin();
  // Comenzamos el lector sd
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  randomSeed(analogRead(0));

  if (!SD.begin(SD_PIN)) {
    Serial.println("No SD-card.");
    return;
  }
  // Check dir for db files
  if (!SD.exists("/db")) {
    Serial.println("Dir for Db files does not exist, creating...");
    SD.mkdir("/db");
  }
  if (SD.exists(db_name)) {
    dbFile = SD.open(db_name, FILE_WRITE);
    // A veces no abrio al primer intento, especialmente antes del inicio en frio
    // Intentar una vez mas
    if (!dbFile) {
      dbFile = SD.open(db_name, FILE_WRITE);
    }
    if (dbFile) {
      Serial.print("Abriendo la tabla... ");
      EDB_Status result = db.open(0);
      if (result == EDB_OK) {
        id = db.count(); //ultima inserccion en la tabla
        Serial.print("Cantidad de registros actualmente ");
        Serial.println(id);
        Serial.println("DONE");
      } else {
        Serial.println("ERROR");
        Serial.println("No se a podido encontra una base de datos en el archivo " + String(db_name));
        Serial.print("Creando una nueva tabla ");
        db.create(0, TABLE_SIZE, (unsigned int)sizeof(logEvent));
        Serial.println("DONE");
        return;
      }
    } else {
      Serial.println("No pudo abrirse el archivo " + String(db_name));
      return;
    }
  } else {
      Serial.print("Creando la tabla... ");
      // create table at with starting address 0
      dbFile = SD.open(db_name, FILE_WRITE);
      db.create(0, TABLE_SIZE, (unsigned int)sizeof(logEvent));
      Serial.println("DONE");
  }
  dbFile.close();
}

boolean btListener = false;
boolean usbListener = false;

void loop(){
  if (blueToothSerial.available()){
    String request = blueToothSerial.readString();
    blueToothSerial.print(" BLUETHOOT:");
    blueToothSerial.print(request);
    String response ="";
    switch(request.toInt()){
      case 0:
        btListener = btListener?false:true;
        break;
      case 1:
        seleccionarTodoBT();
        break;
      case 2:
        response = borrarTodo();
        break;
      default:
        break;
    }
    blueToothSerial.print(response);
  }
  if (Serial.available()){
    String request = Serial.readString();
    Serial.print(" USB:");
    Serial.println(request);
    String response ="";
    switch(request.toInt()){
      case 0:
        usbListener = usbListener?false:true;
        break;
      case 1:
        seleccionarTodoUSB();
        break;
      case 2:
        response = borrarTodo();
        break;
      default:
        break;
    }
    Serial.println(response);
  }

  
  // Delay por segundo 
  delay(1000*5);
  // Leemos la humedad relativa
  float h = dht.readHumidity();
  // Leemos la temperatura en grados centÃ­grados (por defecto)
  float t = dht.readTemperature();
 
  // Comprobamos si ha habido algÃºn error en la lectura
  if (isnan(h) || isnan(t) ) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    return;
  }

  
  // Calcular el Ã­ndice de calor en grados centÃ­grados
  float hic = dht.computeHeatIndex(t, h, false);

  insertarLog(h,t,hic);

  if(btListener){
    blueToothSerial.println(lecturaActual(h,t,hic));
  }
  if(usbListener){
    Serial.println(lecturaActual(h,t,hic));
  }
}

String lecturaActual(float t,float h, float hic){
  String resultado = "";
  resultado += "Humedad: ";
  resultado += h;
  resultado += " %\t";
  resultado += "Temperatura: ";
  resultado += t;
  resultado += " *C ";
  resultado += "Ã�ndice de calor: ";
  resultado += hic;
  resultado += " *C ";
  return resultado;
}

void printError(EDB_Status err)
{
  Serial.print("ERROR: ");
  switch (err)
  {
    case EDB_OUT_OF_RANGE:
      Serial.println("Registro fuera de rango \n");
      break;
    case EDB_TABLE_FULL:
      Serial.println("Tabla llena");
      break;
    case EDB_OK:
    default:
      Serial.println("OK");
      break;
  }
}

void insertarLog(float t,float h, float hic){
  String resultado = "Ingresado";
  dbFile = SD.open(db_name, FILE_WRITE);
  logEvent.id = id;
  logEvent.temperatura = t;
  logEvent.humedad = h;
  logEvent.indiceCalor = hic;
  EDB_Status result = db.appendRec(EDB_REC logEvent);
  if (result != EDB_OK){
    printError(result);
    id = id - 1;
  } else {
    id = id+1;
  }
  dbFile.close();
  return resultado +" id: "+String(id);
}

void seleccionarTodoUSB(){
  dbFile = SD.open(db_name, FILE_WRITE);
  for (int recno = 1; recno <= db.count(); recno++){
    EDB_Status result = db.readRec(recno, EDB_REC logEvent);
    if (result == EDB_OK){
      Serial.print(logEvent.id);
      Serial.print(";");
      Serial.print(logEvent.temperatura);
      Serial.print(";");
      Serial.print(logEvent.humedad);
      Serial.print(";");
      Serial.print(logEvent.indiceCalor);
      Serial.println(";");
    }
    else printError(result);
  }
  dbFile.close();
}
void seleccionarTodoBT(){
  dbFile = SD.open(db_name, FILE_WRITE);
  for (int recno = 1; recno <= db.count(); recno++){
    EDB_Status result = db.readRec(recno, EDB_REC logEvent);
    if (result == EDB_OK){
      blueToothSerial.print(logEvent.id);
      blueToothSerial.print(";");
      blueToothSerial.print(logEvent.temperatura);
      blueToothSerial.print(";");
      blueToothSerial.print(logEvent.humedad);
      blueToothSerial.print(";");
      blueToothSerial.print(logEvent.indiceCalor);
      blueToothSerial.println(";");
    }
    else printError(result);
  }
  dbFile.close();
}

String borrarTodo(){
  String resultado = "eliminado";
  dbFile = SD.open(db_name, FILE_WRITE);
  db.clear();
  id = 0;
  dbFile.close();
  return resultado;
}


