// Подключаем все необходимые для работы библиотеки
#include <LiquidCrystal.h>  // билиотека LCD
#include <EEPROM.h> // библиотека EEPROM
#include <avr/pgmspace.h> // библиотека для работы с PROGMEM
#include <MsTimer2.h> //  библиотека MsTimer2.h

LiquidCrystal lcd(7, 6, 5, 4, 3, 2); // отводим цифровые ШИМ пины 7-2 под подключение LCD (RS, E, DB4, DB5, DB6, DB7)

// данные, используемые процедурой timerInterrupt, усредняющей входной аналоговый сигнал (в вольтах)
// задаем параметры делителя напряжения, подключенного ко входу A0, и время периода измерения/частоту опроса этого входа
#define MEASURE_PERIOD 100 // время периода измерения
#define R1  15.  // сопротивление резистора R1
#define R2  3.71 // сопротивление резистора R2

// объявляем переменные, присваиваем им пины
// объявляем переменные из цикла определения новых катушек
int buttonState = 0; // переменная для хранения состояния кнопки
bool button_state = false; // задаем состояние кнопки по-умолчанию "ложь"
int analogInput = 0;
int V1_8 = 0;
int V1 = 0;

// переменные, используемые для "сглаживания входного" напряжения
int value1; // переменная для усредненного значения в коде
int timeCount;  // счетчик времени
long  sumU1; // переменные для суммирования кодов АЦП
long  averageU1; // сумма кодов АЦП (среднее значение * 500)
boolean flagReady;  // признак готовности данных измерения

int button = 8; // отводим цифровой ШИМ пин 8 под подключение кнопки
int tuner = 9; // отводим пин 9 для подачи управляющего сигнала TUNE B (12В)
int buzzer = 10; // отводим пин 10 под + пьезоэлемента
int current = 11; // отводим пин 11 под подачу управляющего сигнала CURR (12В)

// объявляем переменные для управления "задержками"
long previousMillis = 0;        // храним время последнего переключения цикла
long interval = 3000;           // интервал между включение/выключением цикла
bool onTuneB = false;
unsigned long onTuneBtime;

// вычисление среднего арифметического для выборки из 5 значений
int inputRead()
{
  int average = 0; // переменная для суммирования данных

  for (int i = 0; i < 5; i++) // 5 итераций цикла
  {
    value1 = analogRead(A0); // считываем аналоговый сигнал со входа А0
    average = average + value1; // увеличиваем переменную для суммирования на входное значение
    delay(55); // ожидание 55мс перед каждым чтением
  }
  value1 = average / 5; // усреднить значения
  return (value1); // возвращаем среднее арифметическое типа int
}

int to_8()
{
  V1 = map(value1, 0, 1023, 0, 255); // ЦАП 10 бит => 8 бит
}

// создаем два собственных символа - букв Г и Ф для последующего вывода на дисплей
byte symbol_g [8] = /* создаем свой символ и присваиваем ему имя "symbol_g" */
{
  0b11111,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b00000,
};

byte symbol_f [11] =
{
  0b01110,
  0b10101,
  0b10101,
  0b10101,
  0b01110,
  0b00100,
  0b00100,
  0b00000,
};

//#define PIN_BUTTON 8 // определяем цифровой ШИМ пин 8 под подключение кнопки

// запись массива строк из массивов символов

/* const char FlexM[] PROGMEM = "Flex M is OK!";
  const char FlexL[] PROGMEM = "Flex L is OK!";
  const char FlexS[] PROGMEM = "Flex S is OK!";
  const char Body[] PROGMEM = "Body is OK!";
  const char Spine5[] PROGMEM = "Spine 5ch is OK!";
  const char Spine15[] PROGMEM = "Spine 15ch is OK!";
  const char Head6[] PROGMEM = "Head 6ch is OK!";
  const char Head1[] PROGMEM = "Head 1ch is OK!";
  const char Knee1[] PROGMEM = "Knee 1ch is OK!";
  const char Knee4[] PROGMEM = "Knee 4ch is OK!";

  const char* const coils_OK[] PROGMEM = {FlexM, FlexL, FlexS, Body, Spine5, Spine15, Head6, Head1, Knee1, Knee4}; */

const char* const coils_OK[] PROGMEM =
{
  "id 1 is OK - ", // 0
  "Head 1/Spine 5/FlexM", // 1
  "Flex M is OK!",    // 2
  "Flex L is OK!",    // 3
  "Flex S is OK!",    // 4
  "Body is OK!",      // 5
  "Spine 5ch is OK!", // 6
  "Spine 15ch is OK!",// 7
  "Head 6ch is OK!",  // 8
  "Head 1ch is OK!",  // 9
  "Knee 1ch is OK!",  // 10
  "Knee 4ch is OK!"   // 11
};

char buffer[30];

// обработка прерывания 1 мс
void  timerInterupt() // функция от Василия
{

  timeCount++;  // +1 счетчик выборок усреднения
  sumU1 += analogRead(A0); // суммирование кодов АЦП

  // проверка числа выборок усреднения
  if ( timeCount >= MEASURE_PERIOD )
  {
    timeCount = 0;
    averageU1 = sumU1; // перегрузка среднего значения
    //value1 = sumU1/100;
    sumU1 = 0;
    flagReady = true; // признак результат измерений готов
  }
}

void relay() // задержка на 3с
{
  // здесь будет код, который будет работать постоянно
  // и который не должен останавливаться на время между переключениями
  //unsigned long currentMillis = millis();

  //проверяем не прошел ли нужный интервал, если НЕ прошел то
  long previousMillis = millis();
  while (true) { // while
    do {
      digitalWrite(tuner, HIGH);
    }
    while (millis() - previousMillis < interval);
    break;
  }
  previousMillis = millis();
}

void tuneB() // TUNE B, во время чего V1 = V2
{
  digitalWrite(tuner, HIGH);
}

void detuneB()
{
  digitalWrite(tuner, LOW);
}

void id1_to_lcd()
{       
        lcd.setCursor(0, 0); 
        lcd.print("id1:10 -"); // вывод метки 10 бит
        lcd.setCursor(9, 0); // установка курсора в 11 знак 1 строки
        lcd.print(value1); // вывод id1 в 10 бит

        lcd.setCursor(12, 0); // установка курсора в 15 знак 1 строки
        lcd.print(";8 - "); // вывод метки 8 бит
        
        lcd.setCursor(17, 0); // установка курсора в 18 знак 1 строки
        lcd.print(V1); // вывод id1 в 8 бит

        lcd.setCursor(0, 1); // установка курсора в 1 знак 1 строки
}

void id2_to_lcd()
{
          lcd.setCursor(0, 2);
          lcd.print("id2:10 -"); // вывод метки 10 бит
          lcd.setCursor(9, 2);
          lcd.print(value1); // выводим id2 в 10 бит

          lcd.setCursor(12, 2); // установка курсора в 15 знак 1 строки
          lcd.print(";8 - "); // вывод метки 8 бит
          
          lcd.setCursor(17, 2);
          lcd.print(V1); // выводим id2 в 8 бит
          
          lcd.setCursor(0, 3);
}

void error1()
{       
        //lcd.clear();
        id1_to_lcd();
        lcd.setCursor(0, 1);
        lcd.print("Unknown id1!");
}

void error2()
{       
        id2_to_lcd();
        lcd.setCursor(0, 3);
        lcd.print("Unknown id2!");
}

// цикл настройки повторится единожды (после нажатия кнопки RESET (на плате)):
void setup()
{

  // инициализация серийного порта на обмен данными со скоростью 9600 б/с (на хуя?):
  Serial.begin(9600);

  MsTimer2::set(1, timerInterupt); // прерывания по таймеру, период 1 мс
  MsTimer2::start();              // разрешение прерывания

  // устанавливаем размер (количество столбцов и строк) экрана
  lcd.begin(20, 4); // 20 знаков в строке, 4 строки
  lcd.clear(); // очистка дисплея

  lcd.createChar(1, symbol_g); // создаем символ Г
  lcd.setCursor(4, 1); // установка курсора на 4 знак, 2 строку (нумерация как в массиве - с 0)
  lcd.print("PYCTOMO");

  lcd.setCursor(11, 1);
  lcd.print("\1");

  lcd.setCursor(12, 1);
  lcd.print("PA");

  lcd.createChar(2, symbol_f); //  создаем символ Ф
  lcd.setCursor(14, 1);
  lcd.print("\2");

  lcd.setCursor(3, 4); // установка курсора на 4 знак, 2 строку (нумерация как в массиве - с 0)
  lcd.print("rustomograf.ru"); // вывод ссылки на сайт
  delay(2000); // задержка 1с
  lcd.clear(); // очистка дисплея

  pinMode(button, INPUT_PULLUP); // режим пина 8 кнопки "подтянутый вход"
  pinMode(tuner, OUTPUT); // пин 9 выход
  pinMode(buzzer, OUTPUT); // пин 10 выход
  pinMode(current, OUTPUT); // пин 11 выход

  // запись экстремумов V1 в EEPROM                         // запись экстремумов V2 в EEPROM
  //EEPROM[0] = 185; /* FLEX M & Spine 5ch LOW */             EEPROM[512] = 118; /* FLEX M LOW */     EEPROM[520] = 183; /* SPINE 5CH LOW */
  //EEPROM[1] = 187; /* FLEX M & Spine 5ch HIGH */            EEPROM[513] = 120; /* FLEX M HIGH */    EEPROM[521] = 185; /* SPINE 5CH HIGH */
  //EEPROM[2] = 141; /* V1 FLEX L LOW & V2 BODY LOW */         EEPROM[514] = 96; /* FLEX L LOW */
  //EEPROM[3] = 143; /* V1 FLEX L HIGH & V2 BODY HIGH */       EEPROM[515] = 98; /* FLEX L HIGH */
  //EEPROM[4] = 5; /* FLEX S LOW */                           EEPROM[516] = 5; /* FLEX S LOW */
  //EEPROM[5] = 6; /* FLEX S HIGH */                          EEPROM[517] = 6; /* FLEX S HIGH */
  //EEPROM[6] = 180; /* BODY 4ch LOW */                      EEPROM[518] = 141; /* BODY 4ch LOW */
  //EEPROM[7] = 182; /* BODY 4ch HIGH */                     EEPROM[519] = 143; /* BODY 4ch HIGH */
  //EEPROM[8] = 240; /* HEAD 1CH LOW */                        EEPROM[522] = 170; /* HEAD 1CH LOW */
  //EEPROM[9] = 243; /* HEAD 1CH HIGH */                       EEPROM[523] = 175; /* HEAD 1CH HIGH */
  //EEPROM[10] = 169; /* HEAD 6CH LOW */          EEPROM[524] = 75; /* HEAD 6CH LOW */  EEPROM[526] = 75; /* KNEE 1CH LOW */
  //EEPROM[11] = 171; /* HEAD 6CH HIGH */        EEPROM[525] = 77; /* HEAD 6CH HIGH */  EEPROM[527] = 77; /* KNEE 1CH HIGH */
  //EEPROM[12] = 94; /* KNEE 4CH LOW */                       EEPROM[528] = 150; /* KNEE 4CH LOW */
  //EEPROM[13] = 96; /* KNEE 4CH HIGH */                      EEPROM[529] = 152; /* KNEE 4CH HIGH */
  //EEPROM[14] = 150; /* NV 16CH LOW */                        EEPROM[530] = 62; /* NV 16CH LOW */
  //EEPROM[15] = 154; /* NV 16CH HIGH */                        EEPROM[531] = 64; /* NV 16CH HIGH */
  //EEPROM[16] = 175; /* KNEE 1CH LOW */ EEPROM[18] = 92; /* HEAD 8CH LOW */                     EEPROM[532] = 82; /* HEAD 8CH LOW */
  //EEPROM[17] = 177; /* KNEE 1CH HIGH */ EEPROM[19] = 94; /*HEAD 8CH HIGH*/                     EEPROM[533] = 84 /* HEAD 8CH HIGH */
  //EEPROM[20] = 177; /* NV 8CH LOW */                        EEPROM[534] = 113; /* NV 8CH LOW */
  //EEPROM[21] = 181; /* NV 8CH HIGH */                       EEPROM[535] = 125; /* NV 8CH HIGH */
  //EEPROM[22] = 71; /* FOOTANKLE 8CH LOW */                    EEPROM[536] = 138; /* FOOTANKLE 8CH LOW */
  //EEPROM[23] = 75; /* FOOTANKLE 8CH HIGH */                   EEPROM[537] = 142; /* FOOTANKLE 8CH HIGH */
}

// присвоение экстремумов V1                              // присвоение экстремумов V2
int FlexMLow1 = EEPROM[0];  int Spine5Low1 = EEPROM[0];   int FlexMLow2 = EEPROM[512];
int FlexMHigh1 = EEPROM[1]; int Spine5High1 = EEPROM[1];  int FlexMHigh2 = EEPROM[513];
int FlexLLow1 = EEPROM[2];  int Body4Low2 = EEPROM[518];  int FlexLLow2 = EEPROM[514];
int FlexLHigh1 = EEPROM[3]; int Body4High2 = EEPROM[519]; int FlexLHigh2 = EEPROM[515];
int FlexSLow1 = EEPROM[4];                                int FlexSLow2 = EEPROM[516];
int FlexSHigh1 = EEPROM[5];                               int FlexSHigh2 = EEPROM[517];
int Body4Low1 = EEPROM[6];                                int Spine5Low2 = EEPROM[520];
int Body4High1 = EEPROM[7];                               int Spine5High2 = EEPROM[521];
int Head1Low1 = EEPROM[8];                                int Head1Low2 = EEPROM[522];
int Head1High1 = EEPROM[9];                               int Head1High2 = EEPROM[523];
int Head6Low1 = EEPROM[10];  int Knee1Low1 =  EEPROM[16]; int Head6Low2 = EEPROM[524];          int Knee1Low2 = EEPROM[526];
int Head6High1 = EEPROM[11]; int Knee1High1 = EEPROM[17]; int Head6High2 = EEPROM[525];         int Knee1High2 = EEPROM[527];
int Knee4Low1 = EEPROM[12];                               int Knee4Low2 = EEPROM[528];
int Knee4High1 = EEPROM[13];                              int Knee4High2 = EEPROM[529];
int NV16Low1 = EEPROM[14];                                int NV16Low2 = EEPROM[530];
int NV16High1 = EEPROM[15];                               int NV16High2 = EEPROM[531];
int NeckLow1;                                             int NeckLow2;
int NeckHigh1;                                            int NeckHigh2; 
int Head8Low1 = EEPROM[18];                               int Head8Low2 = EEPROM[532];
int Head8High1 = EEPROM[19];                              int Head8High2 = EEPROM[533];
int NV8Low1 = EEPROM[20];                                 int NV8Low2 = EEPROM[534];
int NV8High1 = EEPROM[21];                                int NV8High2 = EEPROM[535];
int FA8Low1 = EEPROM[22];                                 int FA8Low2 = EEPROM[536];
int FA8High1 = EEPROM[23];                                int FA8High2 = EEPROM[537];

bool key;

bool Flexl1 = false;
bool Flexm1 = false;
bool Flexs1 = false;
bool Body4ch1 = false;
bool Head1ch1 = false;
bool Head6ch1 = false;
bool Knee4ch1 = false;
bool NV16ch1 = false;
bool Neck1 = false;
bool Spine5ch1 = false;
bool Knee1ch1 = false;
bool Head8ch1 = false;
bool NV8ch1 = false;
bool FA8ch1 = false;

void flexM1()
{                                                       
        if ((V1 >= FlexMLow1) && (V1 <= FlexMHigh1))    
      { 
        Flexm1 = true;
        Spine5ch1 = true;
        //Head1ch1 = true;                                                 
        key = true;                                      
        lcd.setCursor(0, 1);                             
        lcd.print("FlexM/Spine 5/Head 1");                 
      }                                                   
}

void flexM2()                                          
{                                                       
        if ((V1 >= FlexMLow2) && (V1 <= FlexMHigh2) && (Flexm1 == true))    
      {                                                 
        key = true;                                      
        lcd.setCursor(0, 3);                             
        lcd.print("Flex M ident. is OK");                 
      }                                                   
}                                                         

void nv8ch1()                                          
{                                                       
        if ((V1 >= NV8Low1) && (V1 <= NV8High1))    
      {  
        NV8ch1 = true;
        Body4ch1 = true;                                               
        key = true;                                      
        lcd.setCursor(0, 1);                             
        lcd.print("NV8/Body4 id1 is OK");                 
      }                                                   
}   

void nv8ch2()                                          
{                                                       
        if ((V1 >= NV8Low2) && (V1 <= NV8High2) && (NV8ch1 == true))    
      {                                                 
        key = true;                                      
        lcd.setCursor(0, 3);                             
        lcd.print("NV 8 ch ident is OK");                 
      }                                                   
}    

void flexL1()
{
  if ((V1 >= FlexLLow1) && (V1 <= FlexLHigh1))
      {
        Flexl1 = true;
        key = true;
        lcd.setCursor(0, 1);
        lcd.print("Flex L id1 is OK");
      }
}

void flexL2()
{
  if ((V1 >= FlexLLow2) && (V1 <= FlexLHigh2) && (Flexl1 == true))
      {
        key = true;
        lcd.setCursor(0, 3);
        lcd.print("Flex L ident. is OK");
      }
}

void flexS1()
{
  if ((V1 >= FlexSLow1) && (V1 <= FlexSHigh1))
      {
        Flexs1 = true;
        key = true;
        lcd.setCursor(0, 1);
        lcd.print("Flex S id1 is OK");
      }
}

void flexS2()
{
  if ((V1 >= FlexSLow2) && (V1 <= FlexSHigh2) && (Flexs1 == true))
      {
        key = true;
        lcd.setCursor(0, 3);
        lcd.print("Flex S ident. is OK");
      }
}

/* void body4ch1()
{
  if ((V1 >= Body4Low1) && (V1 <= Body4High1))
      {
        NV16ch1 = false;
        Body4ch1 = true;
        key = true;
        lcd.setCursor(0, 1);
        lcd.print("Body id1 is OK");
      }
} */

void body4ch2()
{
  if ((V1 >= Body4Low2) && (V1 <= Body4High2) && (Body4ch1 == true))
      {
        key = true;
        lcd.setCursor(0, 3);
        lcd.print("Body/Foot id. is OK");
      }
}

void head1ch1()
{
  if ((V1 >= Head1Low1) && (V1 <= Head1High1))
      {
        Head1ch1 = true;
        key = true;
        lcd.setCursor(0, 1);
        lcd.print("Head 1 ch id1 is OK");
      }
}

void head1ch2()
{
  if ((V1 >= Head1Low2) && (V1 <= Head1High2) && (Head1ch1 == true))
      {
        key = true;
        lcd.setCursor(0, 3);
        lcd.print("Head 1ch ident is OK");
      }
}

void head6ch1()
{
  if ((V1 >= Head6Low1) && (V1 <= Head6High1))
      {
        Head6ch1 = true;
        key = true;
        lcd.setCursor(0, 1);
        lcd.print("Head 6 ch id1 is OK");
      }
  /*else if ((V1 >= Head6Low2) && (V1 <= Head6High2))
      {
        key = true;
        tuneB();
        delay(1000);
        inputRead();
        detuneB();
        delay(1000);
        to_8();
        lcd.setCursor(0, 1);
        lcd.print("Head 6 ch id1 is OK");  
      } */
}

void head6ch2()
{
  if ((V1 >= Head6Low2) && (V1 <= Head6High2) && (Head6ch1 == true))
      {
        key = true;
        lcd.setCursor(0, 3);
        lcd.print("Head 6 ch ident. OK");
      }
  /* else if ((V1 >= Head6Low1) && (V1 <= Head6High1))
      {
        tuneB();
        delay(1000);
        inputRead();
        detuneB();
        to_8();
        lcd.setCursor(0, 1);
        lcd.print("Head 6 ch is OK");   
      } */    
}

void knee1ch1()
{
  if ((V1 >= Knee1Low1) && (V1 <= Knee1High1))
      {
        Knee1ch1 = true;
        key = true;
        lcd.setCursor(0, 1);
        lcd.print("Knee 1ch id1 is OK");
      }
}

void knee1ch2()
{
  if ((V1 >= Knee1Low2) && (V1 <= Knee1High2) && (Knee1ch1 == true))
      {
        key = true;
        lcd.setCursor(0, 3);
        lcd.print("Knee 1ch ident is OK");
      }
}

void knee4ch1()
{
  if ((V1 >= Knee4Low1) && (V1 <= Knee4High1))
      {
        Knee4ch1 = true;
        key = true;
        lcd.setCursor(0, 1);
        lcd.print("Knee4 id1 is OK");
      }
}

void knee4ch2()
{
  if ((V1 >= Knee4Low2) && (V1 <= Knee4High2) && (Knee4ch1 == true))
      {
        key = true;
        lcd.setCursor(0, 3);
        lcd.print("Knee 4 ch ident is OK");
      }
}

void spine5ch2()
{
 if ((V1 >= Spine5Low2) && (V1 <= Spine5High2) && (Spine5ch1 == true))
      {
        key = true;
        lcd.setCursor(0, 3);
        lcd.print("Spine 5 ch is OK");
      } 
}

void nv16ch1()                                          
{                                                       
        if ((V1 >= NV16Low1) && (V1 <= NV16High1))    
      {  
        NV16ch1 = true;                                              
        key = true;                                      
        lcd.setCursor(0, 1);                             
        lcd.print("NV 16ch id1 is OK");                 
      }                                                   
}   

void nv16ch2()                                          
{                                                       
        if ((V1 >= NV16Low2) && (V1 <= NV16High2) && (NV16ch1 == true))    
      {                                                 
        key = true;                                      
        lcd.setCursor(0, 3);                             
        lcd.print("NV 16 ch ident is OK");                 
      }                                                   
}  

void fa8ch1()                                          
{                                                       
        if ((V1 >= FA8Low1) && (V1 <= FA8High1))    
      {  
        FA8ch1 = true;                                              
        key = true;                                      
        lcd.setCursor(0, 1);                             
        lcd.print("FootAnkle8 id1 is OK");                 
      }                                                   
}   

void fa8ch2()                                          
{                                                       
        if ((V1 >= FA8Low2) && (V1 <= FA8High2) && (FA8ch1 == true))    
      {                                                 
        key = true;                                      
        lcd.setCursor(0, 3);                             
        lcd.print("FootAnkle8 id. is OK");                 
      }                                                   
}

void noCoil()
{
  if ((V1 >= 195) && (V1 <= 197))
      {
        key = false;
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("No connection!");
      }
  else if ((V1 >=0) && (V1 <=2))
      
      {
        key = false;
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("Power off/not ident.");
      }  
}

void scanCoils1() // цикл определения катушки по id1
{
  
  nv16ch1();
  flexM1();
  flexS1();
  flexL1();
  //body4ch1();
  head1ch1();
  head6ch1();
  knee1ch1();
  knee4ch1();
  nv8ch1();
  fa8ch1();
}

void scanCoils2() // цикл определения катушки по id2
{
  
  nv16ch2();
  flexM2();
  flexS2();
  flexL2();
  body4ch2();
  knee1ch2();
  head6ch2();
  knee4ch2();
  head1ch2();
  spine5ch2();
  nv8ch2();
  fa8ch1();
}

void coil_new() // процедура вывода параметров новой катушки
{
  if ( flagReady == true )
  {
    flagReady = false;

    inputRead();

    V1_8 = map(value1, 0, 1023, 0, 255); // перевод 10 бит в 8 бит

    /* Serial.print(F(input_V1_Value)); // вывод в СОМ 10 бит входа
      Serial.print(F("; "));
      Serial.print(F(V1_8)); // вывод в СОМ 8 бит входа
      Serial.print(F("; "));
      Serial.print(F(V1)); // вывод входа в В
      Serial.print(F(".\n"));*/

    lcd.setCursor(9, 0); // установка курсора в 9 ячейку 1 строки
    lcd.print("id1");

    lcd.setCursor(1, 1); // установка курсора во 2 ячейку 2 строки
    lcd.print("10 bit:");

    lcd.setCursor(9, 1);
    lcd.print(value1); // вывод на дисплей 10 бит входа

    lcd.setCursor(1, 2); // установка во 2 ячейку 3 строки
    lcd.print("8 bit:");

    lcd.setCursor(9, 2);
    lcd.print(V1_8); // вывод на дисплей 8 бит входа

    lcd.setCursor(1, 3);
    lcd.print("Volts:");

    lcd.setCursor(9, 3); // установка во 2 ячейку 3 строки
    //lcd.print(V1); // вывод на дисплей входа в В
    lcd.print( (float)averageU1 / 500. * 5. / 1024. / R2 * (R1 + R2), 2);

    tuneB();
    delay(3000);

    inputRead();

    // далее повтор субцикла для ID2 - V2

    V1_8 = map(value1, 0, 1023, 0, 255); // перевод 10 бит в 8 бит

    /* Serial.print(F(input_V1_Value));
      Serial.print(F("; "));
      Serial.print(F(V1_8));
      Serial.print(F("; "));
      Serial.print(F(V1));
      Serial.print(F(". \n")); */

    lcd.setCursor(16, 0); // установка курсора в 9 ячейку 1 строки
    lcd.print("id2");

    lcd.setCursor(16, 1);
    lcd.print(value1); // вывод на дисплей 10 бит входа

    lcd.setCursor(16, 2);
    lcd.print(V1_8); // вывод на дисплей 8 бит входа

    lcd.setCursor(16, 3); // установка во 2 ячейку 3 строки
    //lcd.print(V1); // вывод на дисплей входа в В
    lcd.print( (float)averageU1 / 500. * 5. / 1024. / R2 * (R1 + R2), 2);

    detuneB();
    delay(3000);
  }
}

void beep() // процедуры подачи одиночного сигнала пьезиком
{
  analogWrite(10, 50); // включаем пьезик
  delay(1000); // на 1с
  analogWrite(10, 0); // выключаем пьезик
}

void buzz() // другая процедура подачи одиночного сигнала пьезиком
{
  tone(buzzer, 4000, 1000); // включаем на 4кГц на 1с
  delay(1000); // на 1с
}

void clean()
{
  delay(3000); // задержка 3с
  lcd.clear(); // очистка дисплея
}

void loop()
{ // открывается бесконечный цикл - "петля"
  switch (digitalRead(button))
  {
      case (LOW):
      button_state = true; // состояние кнопки становится "правда" и
      
      inputRead();
      to_8();

      // вывод информации на дисплей

      scanCoils1();
      
      switch (key)
      {
        case true:
        id1_to_lcd();
        break;
        case false:
        if (((V1 >=194) && (V1<=197)) || ((V1 >= 0) && (V1 <= 2)))
        { lcd.clear();
          noCoil();}
        else error1();
        break;
      }

       tuneB();
       delay(1000);
       inputRead();
       detuneB();
       delay(1000);
       to_8();

       scanCoils2();

       switch (key)
       {
        case true:
        id2_to_lcd();
        break;
        case false:
        if (((V1 >=194) && (V1<=197)) || ((V1 >= 0) && (V1 <= 2)))
        { 
          lcd.clear();
          noCoil();}
        else error2();
        break;
       }
        break; // стоп
        
      case (HIGH): // в случае, если кнопка "отжата"

        lcd.clear(); 
         
        // ИНАЧЕ запускаем функцию определения id подключенной катушки
        {
          coil_new(); // вызов функции
        }

        lcd.clear();
        
      }
}
