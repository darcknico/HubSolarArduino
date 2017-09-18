#include <DHT.h>
#include <DHT_U.h>
#include "Arduino.h"
#include <EDB.h>
#include <SPI.h>
#include <SD.h>
 
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

void setup() {
  Serial.begin(9600);
  // Comenzamos el sensor DHT
  dht.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
    // Esperamos 5 segundos entre medidas
  delay(5000);
 
  // Leemos la humedad relativa
  float h = dht.readHumidity();
  // Leemos la temperatura en grados centígrados (por defecto)
  float t = dht.readTemperature();
 
  // Comprobamos si ha habido algún error en la lectura
  if (isnan(h) || isnan(t) ) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    return;
  }

  // Calcular el índice de calor en grados centígrados
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("Índice de calor: ");
  Serial.print(hic);
  Serial.print(" *C ");
 
}
