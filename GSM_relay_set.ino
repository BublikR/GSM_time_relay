#include <EEPROM.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeLib.h>

//формат указания текущего времени "ДД.ММ.ГГ чч:мм:сс"
//где ДД - день, ММ - месяц, ГГ - год, чч - часы, мм - минуты, сс - секунлы
//ГГ - от 00 до 99 для 2000-2099 годов

void setup()  {
  Serial.begin(9600);
  setSyncProvider(RTC.get);   // получаем время с RTC
  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC"); //синхронизация не удаласть
  else
    Serial.println("RTC has set the system time");
  EEPROM.write(0, 2); // Запись количества позиций

  // Первоначальные данные времени включения - отключения
  EEPROM.write(1, 17);
  EEPROM.write(2, 00);
  EEPROM.write(3, 01);
  EEPROM.write(4, 30);
  
  EEPROM.write(5, 04);
  EEPROM.write(6, 00);
  EEPROM.write(7, 05);
  EEPROM.write(8, 30);

  // Запись номеров телефона
  EEPROM.write(50, 2); // Количество номеров в памяти
  
  EEPROM.write(51, '3');
  EEPROM.write(52, '8');
  EEPROM.write(53, '0');
  EEPROM.write(54, '9');
  EEPROM.write(55, '0');
  EEPROM.write(56, '0');
  EEPROM.write(57, '0');
  EEPROM.write(58, '0');
  EEPROM.write(59, '0');
  EEPROM.write(60, '0');
  EEPROM.write(61, '0');
  EEPROM.write(62, '0');

  EEPROM.write(63, '3');
  EEPROM.write(64, '8');
  EEPROM.write(65, '0');
  EEPROM.write(66, '6');
  EEPROM.write(67, '6');
  EEPROM.write(68, '0');
  EEPROM.write(69, '0');
  EEPROM.write(70, '0');
  EEPROM.write(71, '0');
  EEPROM.write(72, '0');
  EEPROM.write(73, '0');
  EEPROM.write(74, '0');
  
  
}

void loop()
{
  if (Serial.available())  //поступила команда с временем
      setTimeFromFormatString(Serial.readStringUntil('\n'));
  
  digitalClockDisplay(); //вывод времени

  delay(1000);
}

void digitalClockDisplay() {
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits) {
  //выводим время через ":"
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void setTimeFromFormatString(String time1)
{
  //ДД.ММ.ГГ чч:мм:сс
  TimeElements te;
  te.Second = time1.substring(15, 17).toInt();
  te.Minute = time1.substring(12, 14).toInt();
  te.Hour = time1.substring(9, 11).toInt();
  te.Day = time1.substring(0, 2).toInt();
  te.Month = time1.substring(3, 5).toInt();
  te.Year = time1.substring(6, 8).toInt() + 30; //год в библиотеке отсчитывается с 1970. Мы хотим с 2000
  time_t timeVal = makeTime(te);
  RTC.set(timeVal);
  setTime(timeVal);
}
