
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
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <DS3231.h>


/******************************************************************
 *                    Definicion de variables                     *
 ******************************************************************/
 
// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 2
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11

#define SD_PIN 53  // SD Card CS pin 10 uno 53 mega
#define TABLE_SIZE 8192

char* db_name = "/db/logs.db";
File dbFile;

// Definicion de la tabla.
struct LogEvent {
    int id;
    float temperatura;
    float humedad;
    float indiceCalor;
    String fechora;
}
logEvent;

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

DS3231  rtc(SDA, SCL);

void setup() {
  Serial.begin(9600);
  blueToothSerial.begin(9600);
  blueToothSerial.write("AT+NAME=HubSolar");
  
  // Comenzamos el sensor DHT
  dht.begin();
  // Comenzamos el lector sd
  pinMode(52, OUTPUT);
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

  rtc.begin();
  setSyncProvider(rtc.getUnixTime(rtc.getTime()));
  Alarm.timerRepeat(1200, insertarRegistro);
}

boolean lecturaListener = false;
boolean insertListener = false;
String request = "";
void loop(){
  if(id>999){
    copiarLog();
    borrarTodo();
    imprimirDisponible("trgRegistroLLeno");
  }
  if (blueToothSerial.available() and !Serial.available()){
    request = blueToothSerial.readString();
    switch(request.toInt()){
      case 0:
        lecturaListener = true;
        break;
      case 1:
        seleccionarTodoBT();
        break;
      case 2:
        borrarTodo();
        break;
      case 3:
        insertListener = true;
        lecturaListener = true;
        break;
      case 4:
        blueToothSerial.println(id);
        break;
      case 5:
        copiarLog();
        borrarTodo();
        break;
      default:
        break;
    }
    blueToothSerial.println(request);
  }
  if (Serial.available() and !blueToothSerial.available()){
    request = Serial.readString();
    switch(request.toInt()){
      case 0:
        lecturaListener = true;
        break;
      case 1:
        seleccionarTodoUSB();
        break;
      case 2:
        borrarTodo();
        Serial.println(0);
        break;
      case 3:
        insertListener = true;
        lecturaListener = true;
        break;
      case 4:
        Serial.println(id);
        break;
      case 5:
        copiarLog();
        borrarTodo();
        break;
      default:
        break;
    }
    Serial.println(request);
  }
  
  // Delay por segundo
  Alarm.delay(1000);
  if(lecturaListener or insertListener){
    delay(2000);
    // Leemos la humedad relativa
    float h = dht.readHumidity();
    // Leemos la temperatura en grados centÃ­grados (por defecto)
    float t = dht.readTemperature();
   
    // Comprobamos si ha habido algÃºn error en la lectura
    if (isnan(h) || isnan(t) ) {
      lecturaListener = false;
      insertListener = false;
      Serial.println("Error obteniendo los datos del sensor DHT11");
      return;
    }

    String fechora = String(rtc.getDateStr())+" "+String(rtc.getTimeStr());
     
    // Calcular el Ã­ndice de calor en grados centÃ­grados
    float hic = dht.computeHeatIndex(t, h, false);
    if(insertListener){
      insertListener = false;
      insertarLog(h,t,hic,fechora);
    }
    
    if(lecturaListener){
      lecturaListener = false;
      imprimirDisponible(lecturaActual(h,t,hic,fechora));
      imprimirDisponible(request);
    }
    
  }
}

String lecturaActual(float t,float h, float hic,String fechora){
  return fechora+" Humedad: "+String(h)+" %\tTemperatura: "+String(t)+" *C Ã�ndice de calor: "+String(hic)+" *C ";
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

void insertarLog(float t,float h, float hic,String fechora){
  dbFile = SD.open(db_name, FILE_WRITE);
  logEvent.id = id;
  logEvent.temperatura = t;
  logEvent.humedad = h;
  logEvent.indiceCalor = hic;
  logEvent.fechora = fechora;
  EDB_Status result = db.appendRec(EDB_REC logEvent);
  if (result != EDB_OK){
    printError(result);
    id = id - 1;
  } else {
    id = id+1;
  }
  dbFile.close();
  return "Ingresado id: "+String(id);
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
      Serial.print(";");
      Serial.print(logEvent.fechora);
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
      blueToothSerial.print(";");
      blueToothSerial.print(logEvent.fechora);
      blueToothSerial.println(";");
    }
    else printError(result);
  }
  dbFile.close();
}

void borrarTodo(){
  dbFile = SD.open(db_name, FILE_WRITE);
  db.clear();
  id = 0;
  dbFile.close();
}

void imprimirDisponible(String msg){
  if (Serial){
    Serial.println(msg);
  }
  if (blueToothSerial) {
    blueToothSerial.println(msg);
  }
}

void insertarRegistro(){
  insertListener = true;
  imprimirDisponible("cronInsertarRegistro");
}

void copiarLog(){
  String nombre = "/db/log"+String(day())+String(month())+String(year())+"-"+String(hour())+":"+String(minute())+":"+String(second())+".txt";
  dbFile = SD.open(db_name, FILE_READ);
  if (SD.exists(nombre)) {
    SD.remove(nombre);
  }
  File myFileOut = SD.open(nombre, FILE_WRITE);
  while (dbFile.available()) {
    myFileOut.write(dbFile.read());
  }
  dbFile.close();
  myFileOut.close();
}


