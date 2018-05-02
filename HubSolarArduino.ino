#include <SimpleDHT.h>
#include "Arduino.h"
#include <EDB.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <DS3231.h>
#include <Wire.h>

SoftwareSerial blueToothSerial(6,5);
DS3231  rtc;
bool Century=false;
bool h12;
bool PM;

const char db_name[] = "/db/logs.db";
File dbFile;

// Definicion de la tabla.
struct LogEvent {
    int id;
    int temperatura;
    int humedad;
    char fecha[11];
    char hora[9];
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
SimpleDHT11 dht11;
int id = 0;
int cronInsertarRegistro = 300;
void setup() {
  Wire.begin();
  Serial.begin(9600);
  blueToothSerial.begin(9600);
  
  // Comenzamos el lector sd
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  randomSeed(analogRead(0));

  if (!SD.begin(53)) {
    Serial.println("No SD-card.");
    return;
  }
  // SD.remove(db_name);
  // Check dir for db files
  if (!SD.exists("/db")) {
    Serial.println("Direccion inexistente, creando...");
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
        //Serial.print("Cantidad actual ");
        //Serial.println(id);
      } else {
        //Serial.println("ERROR No se a encontrado"+String(db_name)+". Creando...");
        db.create(0, 8192, (unsigned int)sizeof(logEvent));
        return;
      }
    } else {
      //Serial.println("No pudo abrirse el archivo " + String(db_name));
      return;
    }
  } else {
      //Serial.print("Creando la tabla... ");
      // create table at with starting address 0
      dbFile = SD.open(db_name, FILE_WRITE);
      db.create(0, 8192, (unsigned int)sizeof(logEvent));
  }
  Serial.println("DONE");
  dbFile.close();
  while (!Serial) ;
  String fechastr= String(rtc.getDate())+"."+String(rtc.getMonth(Century))+"."+String(rtc.getYear());
  String horastr= String(rtc.getHour(h12, PM))+":"+String(rtc.getMinute())+":"+String(rtc.getSecond());
  Serial.println(fechastr + " " +horastr);
  setTime(rtc.getHour(h12, PM),rtc.getMinute(),rtc.getSecond(),rtc.getMonth(Century),rtc.getDate(),rtc.getYear());
  //Alarm.timerRepeat(300, insertarRegistro);
  Alarm.timerOnce(cronInsertarRegistro, insertarRegistro);
  Alarm.timerOnce(10, insertarRegistroOnce);
}

boolean lecturaListener = false;
boolean insertListener = false;
boolean printUSB = false;
int dthIntento = 0;
void loop(){
  if(id>=8192){
    copiarLog();
    borrarTodo();
    imprimirDisponibleLN("trgRegistroLLeno");
  }
  if (blueToothSerial.available() and !Serial.available()){
    printUSB = false;
    int opt = blueToothSerial.readString().toInt();
    switch(opt){
      case 0:
        lecturaListener = true;
        break;
      case 1:
        seleccionarTodo();
        break;
      case 2:
        borrarTodo();
        lecturaListener = true;
        break;
      case 3:
        insertListener = true;
        break;
      case 4:
        blueToothSerial.println("COUNT "+String(id));
        break;
      case 5:
        salvarLog();
        break;
      default:
        if(opt>=60){
          cronInsertarRegistro = opt;
          blueToothSerial.println("CRON");
        }
        break;
    }
  } else if (Serial.available() and !blueToothSerial.available()){
    printUSB = true;
    int opt = Serial.readString().toInt();
    switch(opt){
      case 0:
        lecturaListener = true;
        break;
      case 1:
        seleccionarTodo();
        break;
      case 2:
        borrarTodo();
        lecturaListener = true;
        break;
      case 3:
        insertListener = true;
        break;
      case 4:
        Serial.println("COUNT "+String(id));
        break;
      case 5:
        salvarLog();
        break;
      default:
        if(opt>=60){
          cronInsertarRegistro = opt;
          Serial.println("CRON");
        }
        break;
    }
  }
  
  // Delay por segundo
  Alarm.delay(1000);
  if(lecturaListener or insertListener){
    delay(2000);
    byte temperature = 0;
    byte humidity = 0;
    int err = SimpleDHTErrSuccess;
    if ((err = dht11.read(2, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      if (dthIntento>5){
        lecturaListener = false;
        insertListener = false;
        dthIntento = 0;
        imprimirDisponibleLN("FIN");
      } else {
        dthIntento = dthIntento + 1;
        imprimirDisponibleLN("DTH ERROR "+String(SimpleDHTErrSuccess));
      }
      return;
    } else {
      dthIntento = 0;
    }
    char fecha[11];
    String fechastr= String(rtc.getDate())+"."+String(rtc.getMonth(Century))+"."+String(rtc.getYear());
    fechastr.toCharArray(fecha,11);
    char hora[9];
    String horastr= String(rtc.getHour(h12, PM))+":"+String(rtc.getMinute())+":"+String(rtc.getSecond());
    horastr.toCharArray(hora,9);
    
    if(insertListener){
      insertListener = false;
      insertarLog((int)temperature,(int)humidity,fecha,hora);
    }
    if(lecturaListener){
      lecturaListener = false;
      imprimirDisponibleLN(lecturaActual((int)temperature,(int)humidity,fecha,hora));
    }
  }
}

String lecturaActual(int t,int h,char fecha[],char hora[]){
  return String(fecha)+" "+String(hora)+" Humedad: "+String(h)+" %\tTemperatura: "+String(t)+" *C";
}

void printError(EDB_Status err)
{
  imprimirDisponibleLN("SD ERROR: "+String(err));
  switch (err)
  {
    case EDB_OUT_OF_RANGE:
      imprimirDisponibleLN("Registro fuera de rango \n");
      break;
    case EDB_TABLE_FULL:
      imprimirDisponibleLN("Tabla llena");
      break;
    case EDB_OK:
    default:
      imprimirDisponibleLN("OK");
      break;
  }
}

void copyIntArray(char *dst, char *src, int nb){
  while(nb--) *dst++ = *src++;
}

void insertarLog(int t,int h,char fecha[],char hora[]){
  imprimirDisponible(String(id));
  imprimirDisponible(";");
  imprimirDisponible(String(t));
  imprimirDisponible(";");
  imprimirDisponible(String(h));
  imprimirDisponible(";");
  imprimirDisponible(String(fecha));
  imprimirDisponible(";");
  imprimirDisponible(String(hora));
  imprimirDisponibleLN(";");
  dbFile = SD.open(db_name, FILE_WRITE);
  logEvent.id = id;
  logEvent.temperatura = t;
  logEvent.humedad = h;
  copyIntArray(logEvent.fecha,fecha,11);
  copyIntArray(logEvent.hora,hora,9);
  EDB_Status result = db.appendRec(EDB_REC logEvent);
  if (result != EDB_OK){
    printError(result);
    id = id - 1;
  } else {
    id = id+1;
  }
  dbFile.close();
  imprimirDisponibleLN("INSERT");
}

void seleccionarTodo(){
  dbFile = SD.open(db_name, FILE_WRITE);
  for (int recno = 1; recno <= db.count(); recno++){
    EDB_Status result = db.readRec(recno, EDB_REC logEvent);
    if (result == EDB_OK){
      imprimirDisponible(String(logEvent.id));
      imprimirDisponible(";");
      imprimirDisponible(String(logEvent.temperatura));
      imprimirDisponible(";");
      imprimirDisponible(String(logEvent.humedad));
      imprimirDisponible(";");
      imprimirDisponible(String(logEvent.fecha));
      imprimirDisponible(";");
      imprimirDisponible(String(logEvent.hora));
      imprimirDisponibleLN(";");
    }
    else printError(result);
  }
  dbFile.close();
  imprimirDisponibleLN("FIN");
}

void borrarTodo(){
  dbFile = SD.open(db_name, FILE_WRITE);
  db.clear();
  id = 0;
  dbFile.close();
  imprimirDisponibleLN("DELETE");
}

void imprimirDisponible(String msg){
  if (printUSB){
    Serial.print(msg);
  } else {
    blueToothSerial.print(msg);
  }
}
void imprimirDisponibleLN(String msg){
  if (printUSB){
    Serial.println(msg);
  } else {
    blueToothSerial.println(msg);
  }
}

void insertarRegistro(){
  insertListener = true;
  imprimirDisponibleLN("cronInsertarRegistro");
  Alarm.timerOnce(cronInsertarRegistro, insertarRegistro);
}

void insertarRegistroOnce(){
  insertListener = true;
  imprimirDisponibleLN("cronInsertarRegistro");
}

void copiarLog(){
  String nombre = "/db/log"+String(day())+String(month())+String(year())+"-"+String(hour())+":"+String(minute())+":"+String(second())+".txt";
  char* dir;
  nombre.toCharArray(dir,29);
  dbFile = SD.open(db_name, FILE_READ);
  if (SD.exists(dir)) {
    SD.remove(dir);
  }

  File myFileOut = SD.open(dir, FILE_WRITE);
  while (dbFile.available()) {
    myFileOut.write(dbFile.read());
  }
  dbFile.close();
  myFileOut.close();
  imprimirDisponibleLN("COPY");
}

void salvarLog(){
  seleccionarTodo();
  copiarLog();
  borrarTodo();
  imprimirDisponibleLN("BACKUP");
}

