// Версия программы GSM Relay v.1.2
// Реализована перезагрузка всей системы раз в сутки
// Реализована перезагрузка всей системы по команде через sms-сообщение
// Реализована установка времени перезагрузки через sms-сообщение

#define TEST 1

#include <TimeLib.h>                        // библиотека работы с временем
#include <DS1307RTC.h>                      // библиотека для модуля часов реального времени
#include <EEPROM.h>                         // библиотека работы с энергонезависимой памятью
#include <SoftwareSerial.h>                 // библиотека работы с виртуальным последовательным портом
#define QOP 4                               // максимальное колличество позиций таймера
#define QON (1+5)                           // максимальное количество хранимых телефонных номеров

SoftwareSerial mySerial(5, 4);              // RX, TX

int ch = 0;                                 // для считываемых символов из последовательного порта
String val = "";                            // для сформированной строки из последовательности ch
String subval = "";                         // для подстроки в составном сообщении
String phone = "";                          // для хранения номера телефона
bool light = false;                         // флаг управления освещением при звонке (0 - выкл., 1 - вкл)
byte QuantityOfPositions = EEPROM.read(0);  // количество позиций таймера
byte Call[QOP * 4];                         // массив значений времени включения и отключения реле времени
byte Reset[2] = {EEPROM.read(500), EEPROM.read(501)};    // часы и минуты ежедневной перезагрузки
byte QuantityOfNumbers = EEPROM.read(50);   // количество телефонных номеров в памяти
int addr = 51;                              // номер ячейки в памяти с которой начинается запись номеров телефонов
String PhoneNumbers[QON];                   // массив телефонных номеров
bool _GSM = true;                           // управление настройками GSM модуля при запуске
bool block = false;                         // блокировка срабатывания по времени
bool changeDel = true;                      // проверка записи номера в освободившееся место
short index = 0;                            // индекс символа '#' в сообщении
void(*resetFunc)(void) = 0;                 // объявляем функцию reset

void setup() {
#if TEST
  Serial.begin(9600);
#endif
  pinMode(3, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  delay(150);
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
  delay(400);
  digitalWrite(3, LOW);
  delay(5000);                              // время на инициализацию модуля
  pinMode(6, OUTPUT);                       // порт управления реле на "выход"
  digitalWrite(6, LOW);                     // при запуске реле выключено
  setSyncProvider(RTC.get);                 // получаем время с RTC
  mySerial.begin(9600);                     // запускаем виртуальный последовательный порт 9600 бод
  delay(8000);
  mySerial.println("AT+ENPWRSAVE=0");       // спящий режим (0 - выкл, 1 - вкл.)
  delay(200);
  mySerial.println("AT+CNMI=2,2");
  delay(200);
  mySerial.println("AT+CMGF=1");
  delay(100);

  // Запись данных времени включения и выключения реле из памяти
  for (byte i = 1; i < QuantityOfPositions * 4 + 1; i++)
  {
    Call[i - 1] = EEPROM.read(i);
  }
  
  PhoneNumbers[0] = "380950000000";
  PhoneNumbersSynch(); // синхронизация номеров телефона из EEPROM с массивом в оперативной памяти
}

void loop() {

  if (mySerial.available()) {                 //если GSM модуль что-то послал нам, то
    delay(10);
    while (mySerial.available()) {          //сохраняем входную строку в переменную val
      ch = mySerial.read();                 // читаем посимвольно
      val += char(ch);                      // добавляем в строку
      delay(1);
    }
    // ОБРАБОТКА ЗВОНКА
    if (val.indexOf("RING") > -1)
    {
      mySerial.println("ATH0");           // разрываем связь
      for (byte t = 0; t <= QuantityOfNumbers; t++)      // проверяем номер
      {
        if (val.indexOf(PhoneNumbers[t]) > -1)
        {
          if (light == false)                 // если реле выключено
          {
            digitalWrite(6, HIGH);           // включаем
            light = true;                     // меняем флаг на вкл.
            delay(50);
          }
          else if (light == true)             // если реле включено
          {
            digitalWrite(6, LOW);            // выключаем
            light = false;                    // меняем флаг на выкл.
            delay(50);
          }
          break;
        }
      }
    }

    // ОБРАБОТКА СООБЩЕНИЯ
    else if (val.indexOf("+CMT") > -1) { // если получили СМС
      for (byte t = 0; t <= QuantityOfNumbers; t++)      // проверяем номер
      {
        if (val.indexOf(PhoneNumbers[t]) > -1)
        {
          phone = "+" + PhoneNumbers[t];        // запоминаем с какого номера пришло сообщение
          val = val.substring(48);              // записываем информацию из СМС в val
          val.trim();
          val.toLowerCase();
#if TEST
          Serial.println(val);  //печатаем в монитор порта пришедшую строку
#endif

          // =========== Блок обработки текста сообщения =========== //
          while ((index = val.indexOf('#')) > -1)
          {
            subval = val.substring(0, index);
            subval.trim();
            val = val.substring(index + 1);
            val.trim();
#if TEST
            Serial.println(subval);  //печатаем в монитор порта комманду
#endif
            Commands(subval);
            subval = "";
          }

          subval = val;
          val = "";
          Commands(subval);
          // =========== Конец блока обработки текста сообщения =========== //

          delay(500);
          mySerial.println("AT+CMGD=1,4");      // удаляем из памяти все СМС
          phone = "";
          break;
        }
      }
    }
#if TEST
    else
      Serial.println(val);  //печатаем в монитор порта пришедшую строку
#endif

    val = ""; // очищаем переменную в которой хранилось пришедшее сообщение
    subval = ""; // очищаем переменную в которой хранилось пришедшее сообщение
  }
#if TEST
  if (Serial.available()) {  //если в мониторе порта ввели что-то
    while (Serial.available()) {  //сохраняем строку в переменную val
      ch = Serial.read();
      val += char(ch);
      delay(10);
    }
    mySerial.println(val);  //передача всех команд, набранных в мониторе порта в GSM модуль
    val = "";  //очищаем
  }
#endif

  // СРАБАТЫВАНИЕ РЕЛЕ
  for (byte g = 0; g < QuantityOfPositions * 4; g += 4) {
    if ((hour() == Call[g]) && (minute() == Call[g + 1]) && (light == false) && (!block))
    {
      digitalWrite(6, HIGH);           //включаем
      light = true;                     // меняем флаг на вкл.
    }
    else if ((hour() == Call[g + 2]) && (minute() == Call[g + 3]) && (light == true)) // если реле включено
    {
      digitalWrite(6, LOW);            //выключаем
      light = false;                    // меняем флаг на выкл.
    }
  }

  // Ежедневная перезагрузка всей системы
  if ((hour() == Reset[0]) && (minute() == Reset[1]) && (second() == 0))
    resetFunc();

  // Проверка потери сети
  if (!(minute() % 15) && (second() == 0)) {
    mySerial.println("AT+CREG?");
    delay(1000);
    val = "";
    while (mySerial.available()) {
      ch = mySerial.read();
      if (char(ch) == '\n') { // пропускаем эхо
        while (mySerial.available()) { // пишем ответ
          ch = mySerial.read();
          if (char(ch) == '\n') break;
          val += char(ch);
          delay(1);
        }
        break;
      }
    }
    val.trim();
    if (!(val.indexOf("+CREG: 0,1") > -1)) {
      digitalWrite(2, HIGH);
      delay(150);
      digitalWrite(2, LOW);
      digitalWrite(3, HIGH);
      delay(400);
      digitalWrite(3, LOW);
      delay(5000);
      mySerial.println("AT+ENPWRSAVE=0");
      delay(200);
      mySerial.println("AT+CNMI=2,2");
      delay(200);
      mySerial.println("AT+CMGF=1");
      delay(100);
      _GSM = true;
    }
  }

  // Прописываем настройки GSM модуля при запуске (те что не влезли в setup())
  if (_GSM == true) {
    mySerial.println("AT+CSCS=\"GSM\"");
    delay(100);
    mySerial.println("AT+CLIP=1"); // включаем автоматический определитель номера
    delay(100);
    mySerial.println("ATE1"); // режим эха (0 - выкл, 1 - вкл.)
    delay(100);
    _GSM = false;
  }
}

//----------------- Функции -----------------//

// Запись телефонных номеров из памяти
void PhoneNumbersSynch()
{
  String phoneNumber = "";
  byte tempQuantityOfNumbers = QuantityOfNumbers; // с учетом удаленных номеров
  int tempaddr = addr;
  bool flag = true;
  for (int i = 1; i <= tempQuantityOfNumbers; i++)
  {
    flag = true;
    for (byte j = 0; j < 12; j++)
    {
      if ((j == 0) && ((char)EEPROM.read(tempaddr) == '_')) //если номер удален перескакиваем на следующий
      {
        tempQuantityOfNumbers++;
        tempaddr += 12;
        flag = false;
        break;
      }
      phoneNumber += (char)EEPROM.read(tempaddr + j);
    }
    if (flag) {
      PhoneNumbers[i] = phoneNumber;
      phoneNumber = "";
      tempaddr += 12;
    }
    else
      i--;
  }
}

// обработка полученного сообщения
void Commands(String subval)
{
  // Если получили сообщение с командой "On" включаем реле
  if (!subval.compareTo("on")) {
    digitalWrite(6, HIGH);             //включаем
    light = true;
  }

  // Если получили сообщение с командой "Off" выключаем реле
  else if (!subval.compareTo("off")) {
    digitalWrite(6, LOW);              //выключаем
    light = false;
  }

  // Если получили сообщение с командой "Reset"
  else if (!(subval.substring(0, 5)).compareTo("reset"))
  {
    if (subval.length() == 5){
        delay(500);
        mySerial.println("AT+CMGD=1,4");      // удаляем из памяти все СМС
        delay(500);
        resetFunc();
    }
    else
    {
      subval = subval.substring(6);
      subval.trim();
      Reset[0] = subval.substring(0, 2).toInt();
      EEPROM.write(500, Reset[0]);
      Reset[1] = subval.substring(3, 5).toInt();
      EEPROM.write(501, Reset[1]);
    }
  }

  // Если получили сообщение с командой "Time", отправляем текущее время и таблицу времени срабатывания реле
  else if (!subval.compareTo("time")) {
    subval = "Time: " + (String)((hour() < 10) ? "0" : "") + (String)hour() + ":" + (String)((minute() < 10) ? "0" : "") + (String)minute() + "\n" + "Set:\n"; // текущее время
    for (byte i = 0; i < QuantityOfPositions * 4; i += 4) {
      // формируем сообщение с таблицей времени срабатывания
      subval += (String)((Call[i] < 10) ? "0" : "") + (String)Call[i] + ":" + (String)((Call[i + 1] < 10) ? "0" : "") + (String)Call[i + 1] + " - " + (String)((Call[i + 2] < 10) ? "0" : "") + (String)Call[i + 2] + ":" + (String)((Call[i + 3] < 10) ? "0" : "") + (String)Call[i + 3] + "\n";
    }
    subval += light ? "on" : "off";
    subval += "\n";
    subval += block ? "block" : "unblock";
    subval += "\nreset: ";
    subval += (String)((Reset[0] < 10) ? "0" : "") + (String)Reset[0] + ":" + (String)((Reset[1] < 10) ? "0" : "") + (String)Reset[1];
    Sms(subval, phone);
  }

  // Если получили сообщение с командой "Clock", устанавливаем часы по данным из СМС
  else if (!(subval.substring(0, 5)).compareTo("clock")) {
    subval = subval.substring(6);
    subval.trim();
    TimeElements te;
    te.Second = 0;
    te.Minute = subval.substring(3, 5).toInt();
    te.Hour = subval.substring(0, 2).toInt();
    te.Day = 1;
    te.Month = 1;
    te.Year = 48;
    time_t timeVal = makeTime(te);
    RTC.set(timeVal);
    setTime(timeVal);
  }

  // Если получили сообщение с командой "Setup" устанавливаем время включения и отключения
  else if (!(subval.substring(0, 5)).compareTo("setup")) {
    subval = subval.substring(6);
    QuantityOfPositions = (subval.length()) / 11; // считаем количество позиций таймера
    EEPROM.write(0, QuantityOfPositions); // записываем количество позиций таймера в память
    for (byte i = 0, j = 0; i < QuantityOfPositions; i++, j += 4,  subval.remove(0, 12)) { // Записываем время вкл. и выкл.
      Call[j] = subval.substring(0, 2).toInt();             // в переменные
      EEPROM.write(j + 1, Call[j]);                      // и в память
      Call[j + 1] = subval.substring(3, 5).toInt();
      EEPROM.write(j + 2, Call[j + 1]);
      Call[j + 2] = subval.substring(6, 8).toInt();
      EEPROM.write(j + 3, Call[j + 2]);
      Call[j + 3] = subval.substring(9, 11).toInt();
      EEPROM.write(j + 4, Call[j + 3]);
    }
  }

  // Если получили сообщение с командой "Block" блокируем включение по часам
  else if (!subval.compareTo("block")) {
    block = true;
  }

  // Если получили сообщение с командой "Unblock" отключаем блокировку включения по часам
  else if (!subval.compareTo("unblock")) {
    block = false;
  }


  // Если получили сообщение с командой "Number":
  else if (!(subval.substring(0, 6)).compareTo("number")) {
    subval = subval.substring(7);
    // добавление номера
    if (!(subval.substring(0, 3)).compareTo("add")) {
      subval = "38" + subval.substring(4);
      for (int i = 51; i < (51 + QON * 12); i += 12) {
        if ((char)EEPROM.read(i) == '_') {
          for (byte j = i, k = 0; j < (i + 12); j++, k++) {
            EEPROM.write(j, subval[k]);
          }
          changeDel = false;
          break;
        }
      }
      if (changeDel) {
        for (int g = 51 + QuantityOfNumbers * 12, h = 0; g <= (51 + QuantityOfNumbers * 12 + 12); g++, h++) {
          EEPROM.write(g, subval[h]);
        }
      }
      EEPROM.write(50, ++QuantityOfNumbers);
      changeDel = true;
      PhoneNumbersSynch(); // синхронизируем номера
    }
    // удаление номера
    else if (!(subval.substring(0, 3)).compareTo("del")) {
      subval = "38" + subval.substring(4);
      for (int i = 54; i < (51 + QON * 12); i += 12) {
        for (byte j = i, k = 3; j < (i + 9); j++, k++) {
          if ((char)EEPROM.read(j) != subval[k]) {
            changeDel = false;
            break;
          }
        }
        if (changeDel) {
          EEPROM.write(i - 3, '_');
          QuantityOfNumbers--;
          EEPROM.write(50, QuantityOfNumbers);
          break;
        }
        changeDel = true;
      }
      PhoneNumbersSynch(); // синхронизируем номера
    }
    // отправка списка номеров
    else if (!(subval.substring(0, 4)).compareTo("info")) {
      subval = "";
      for (byte w = 1; w <= QuantityOfNumbers; w++) {
        // формируем сообщение с перечнем записаных номеров
        subval += PhoneNumbers[w].substring(2) + "\n";
      }
      Sms(subval, phone);
    }
  }
  // Если получили сообщение с командой для GSM-модуля
  /*else if(!(subval.substring(0, 3)).compareTo("com")){
    subval = subval.substring(4);
    mySerial.println(subval);
    delay(500);
    subval = "";
    while (mySerial.available()) {
      ch = mySerial.read();
      if(char(ch) == '\n'){ // пропускаем эхо
        while(mySerial.available()){// пишем ответ
          ch = mySerial.read();
          if(char(ch) == '\n') break;
          subval += char(ch);
          delay(1);
        }
        break;
      }
    }
    subval.trim();
    Sms(subval, phone);
    }*/
  //=======================================================================================================//

}

// отправка sms
void Sms(String text, String phone)  //процедура отправки СМС
{
  mySerial.println("AT+CMGF=1");
  delay(100);
  mySerial.println("AT+CSCS=\"GSM\"");
  delay(100);
  mySerial.println("AT+CMGS=\"" + phone + "\"");
  delay(500);
  mySerial.print(text);
  delay(500);
  mySerial.print((char)26);
  delay(500);
}
