#include <DallasTemperature.h>
#include <OneWire.h>

// подключение нескольких датчиков на разніе пині ардуино.
/*
Стандартная библиотека DallasTemperature несколько модернизирована и позволяет подключить несколько датчиков к одному пину Ардуино
и обращаться к ним по уникальным адресам, что удобно, если надо провести датчики температуры  на большие расстояния от Ардуино к
местам измерения температур, так как количество проводов уменьшается до трёх или  даже двух с паразитным подключением, и толщина
проводов может передать сигнал напряжением в 5 вольт на большие расстояния.  Но что делать, если датчик испортился, например из-за
перегрева, при замене цифрового датчика на шине с другими датчиками нужно заново прописывать в программе новый уникальный номер
08датчика DS18b20  и заново прошивать Ардуино.
Одно из решений подключить цифровые датчики, как аналоговые, каждый на свой вывод и считывать температуру с привязкой к
выводу, а не к уникальному адресу или к номеру в последовательности определения на шине.
Если требуется подключить несколько цифровых датчиков DS18b20 на разные выводы по шине onewire воспользуётесь примером скетча:
два цифровых датчика DS18B20 на отдельные пины  для быстрой замены с разрешениями для быстроты срабатывания (менее 2 секунд) и проверкой на отсутствие
*/

// пин d3 датчик температуры 1
OneWire oW_technichka(3), 
        oW_HESupply(4),
        oW_HEReturn(5),
        oW_H20Supply(6),
        oW_H20Return(7),
        oW_procedurka(8),
        oW_cryo(9);

DallasTemperature technichka(&oW_technichka),
                  HESupply(&oW_HESupply),
                  HEReturn(&oW_HEReturn),
                  H20Supply(&oW_H20Supply),
                  H20Return(&oW_H20Return),
                  procedurka(&oW_procedurka),
                  cryo(&oW_cryo);

DeviceAddress technichkaAddress, HESupplyAddress, HEReturnAddress, H20SupplyAddress, H2OReturnAddress, procedurkaAddress, cryoAddress;
int technichka1;
int HESupply2;
int HEReturn3;
int H2OSupply4;
int H2OReturn5;
int procedurka6;
int cryo7;
//int raznost;
//float floatObrat;
//float floatpodach;

void setup()
{
Serial.begin(9600);
technichka.begin();
HESupply.begin();
HEReturn.begin();
H2OSupply.begin();
H20Return.begin();
procedurka.begin();
cryo.begin();

technichka1.setResolution(11);
HESupply2.setResolution(11);
HEReturn3.setResolution(11);
H2OSupply4.setResolution(11);
H2OReturn5.setResolution(11);
procedurka6.setResolution(11);
cryo7.setResolution(11);

// при максимальной точности время срабатывания 2 секунды если не указана точность
// 12  точность 0.06
// 11 точность 0.12
// 10 точность 0.25
// 9 точность 0.5
// 6 точность 0.5
}

void loop(void)
{
Serial.println("Datchiki temperaturi:");
Obratka.requestTemperatures();
delay(40);
podacha.requestTemperatures();
delay(40);
Serial.print("Podacha:");
Serial.println(podacha.getTempCByIndex(0));
Serial.print("Obratka:");
Serial.println(Obratka.getTempCByIndex(0));
int Obrat = Obratka.getTempCByIndex(0);
int podach = podacha.getTempCByIndex(0);
int raznost = podach - Obrat;
Serial.print("RaZnost:");
Serial.println(raznost);
delay(100);
oW_podacha.reset_search();
oW_Obratka.reset_search();
if (  !podacha.getAddress(podachaAddress, 0) ) { Serial.print("datchika Podachi net");}
if (  !Obratka.getAddress(ObratkaAddress, 0) ) { Serial.print("datchika Obratki net");}
}
//Также часто требуется проверка на наличие  датчика в случае плохого контакта.
//Для ускорения работы цифровых датчиков следует установить разрешение  (точность ) снятия измерений, если не указывать разрешение ,
//по умолчанию оно будет максимальным, а значит процесс замера температуры займёт 2 секунды.

/*#include <DallasTemperature.h>
#include <OneWire.h>

// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress firstThermometer, secondThermometer, thirdThermometer, fourthThermometer;

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Assign address manually. The addresses below will beed to be changed
  // to valid device addresses on your bus. Device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
  //outsideThermometer   = { 0x28, 0x3F, 0x1C, 0x31, 0x2, 0x0, 0x0, 0x2 };

  // Search for devices on the bus and assign based on an index. Ideally,
  // you would do this to initially discover addresses on the bus and then 
  // use those addresses and manually assign them (see above) once you know 
  // the devices on your bus (and assuming they don't change).
  // 
  // method 1: by index
  if (!sensors.getAddress(firstThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  if (!sensors.getAddress(secondThermometer, 1)) Serial.println("Unable to find address for Device 1");
  if (!sensors.getAddress(thirdThermometer, 2)) Serial.println("Unable to find address for Device 2"); 
  if (!sensors.getAddress(thirdThermometer, 3)) Serial.println("Unable to find address for Device 3"); 

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices, 
  // or you have already retrieved all of them. It might be a good idea to 
  // check the CRC to make sure you didn't get garbage. The order is 
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to insideThermometer
  //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");
  // assigns the seconds address found to outsideThermometer
  //if (!oneWire.search(outsideThermometer)) Serial.println("Unable to find address for outsideThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(firstThermometer);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(secondThermometer);
  Serial.println();

  Serial.print("Device 2 Address: ");
  printAddress(thirdThermometer);
  Serial.println();

  Serial.print("Device 3 Address: ");
  printAddress(fourthThermometer);
  Serial.println();

  // set the resolution to 9 bit per device
  sensors.setResolution(firstThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(secondThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(thirdThermometer, TEMPERATURE_PRECISION);
  sensors.setResolution(fourthThermometer, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(firstThermometer), DEC); 
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(secondThermometer), DEC); 
  Serial.println();

  Serial.print("Device 2 Resolution: ");
  Serial.print(sensors.getResolution(thirdThermometer), DEC); 
  Serial.println();

  Serial.print("Device 3 Resolution: ");
  Serial.print(sensors.getResolution(fourthThermometer), DEC); 
  Serial.println();
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();    
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}

/*
 * Main function, calls the temperatures in a loop.

void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");

  // print the device information
  printData(firstThermometer);
  printData(secondThermometer);
  printData(thirdThermometer);
  printData(fourthThermometer);
} */

