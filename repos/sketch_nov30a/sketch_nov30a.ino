#include <DallasTemperature.h>
#include <OneWire.h>

#include <OneWire.h>
#include <DallasTemperature.h>
 
#define ONE_WIRE_BUS 2
 
OneWire oneWire(ONE_WIRE_BUS);
 
DallasTemperature sensors(&oneWire);
 
DeviceAddress Thermometer1 = { 
  0x28, 0x00, 0x54, 0xB6, 0x04, 0x00, 0x00, 0x92 };  // адрес датчика DS18B20 280054B604000092
DeviceAddress Thermometer2 = { 
  0x28, 0x9E, 0x95, 0xB5, 0x04, 0x00, 0x00, 0x57 }; 
 
void setup() {
 
  sensors.begin();
  sensors.setResolution(Thermometer1, 10);
  sensors.setResolution(Thermometer2, 10);
 
  Serial.begin(9600);
}
 
void printTemperature(DeviceAddress deviceAddress) {
  float tempC = sensors.getTempC(deviceAddress);
  Serial.println(tempC);
}
void loop() {
 
  sensors.requestTemperatures();
  Serial.print("Sensor1  ");
  printTemperature(Thermometer1);
 
  Serial.print("Sensor2  ");
  printTemperature(Thermometer2);
}
