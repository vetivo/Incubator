/* -------------------- TODO --------------------

добавить уведомление о необходимости овоскопировании яйца на 7.5, 11, 18 дни
добавить экран с информацией:
  время до поворота,
  время до проветривания

-------------------- TODO -------------------- */

/* -------------------- Подключаем библиотеки -------------------- */
#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C_Menu.h>
LiquidCrystal_I2C_Menu lcd(0x27, 20, 4);
#include <PID_v1.h>
#include <AHTxx.h>
AHTxx aht(AHTXX_ADDRESS_X38, AHT2x_SENSOR); // sensor address, sensor type

class TRelay
{
  // Переменные-участники класса устанавливаются при запуске
  int ledPin;   // Номер контакта со светодиодом
  unsigned long OnTime;  // длительность ВКЛ в мс
  unsigned long OffTime; // длительность ВЫКЛ в мс

  // Контроль текущего состояния
  int ledState;                 // устанавливает текущее состояние светодиода
  unsigned long previousMillis; // время последнего обновления состояния светодиода

  // Конструктор - создает объект RelayTimer, инициализирует переменные-участники и состояние
public:
  TRelay(int pin)
  {
    ledPin = pin;
    pinMode(ledPin, OUTPUT);
  }

  void Setup(unsigned long on, unsigned long off)
  {
    OnTime = on;
    OffTime = off;

    ledState = LOW;
    previousMillis = 0;
  }

  void Update(unsigned long currentMillis)
  {
    if ((ledState == LOW) && (currentMillis - previousMillis >= OnTime))
    {
      ledState = HIGH;                // ВКЛ
      previousMillis = currentMillis; // Запомнить время
      // digitalWrite(ledPin, ledState); // Обновить состояние светодиода
    }
    else if ((ledState == HIGH) && (currentMillis - previousMillis >= OffTime))
    {
      ledState = LOW; // ВЫКЛ
      // digitalWrite(ledPin, ledState); // Обновить состояние светодиода
    }
    digitalWrite(ledPin, ledState); // Обновить состояние светодиода
  }
};

/* -------------------- Макросы чтения параметром из EEPROM -------------------- */
#define LOAD_START_STOP_INCUBATION EEPROM.read(0)                 // Чтение значения типа byte (0-255)
#define LOAD_OLD_DAY EEPROM.read(1)                               // Чтение значения типа byte (0-255)
#define LOAD_CURRENT_DAY EEPROM.read(2)                           // Чтение значения типа byte (0-255)
#define LOAD_CURRENT_HOUR EEPROM.read(3)                          // Чтение значения типа byte (0-255)
#define LOAD_CURRENT_MINUTE EEPROM.read(4)                        // Чтение значения типа byte (0-255)
#define LOAD_BIRD EEPROM.read(5)                                  // Чтение значения типа byte (0-255)
#define LOAD_NEED_TEMPERATURE(value) EEPROM.get(6, value)         // Чтение значения типа float
#define LOAD_CORRECTIONS_TEMPERATURE(value) EEPROM.get(10, value) // Чтение значения типа byte (0-255)
#define LOAD_NEED_HUMIDITY(value) EEPROM.get(14, value)           // Чтение значения типа float
#define LOAD_CORRECTIONS_HUMIDITY(value) EEPROM.get(18, value)    // Чтение значения типа byte (0-255)
#define LOAD_HUMIDITY_HYSTERESIS EEPROM.read(19)                  // Чтение значения типа byte (0-255)
#define LOAD_TURN_PERIOD EEPROM.read(20)                          // Чтение значения типа byte (0-255)
#define LOAD_TURN_TIME EEPROM.read(21)                            // Чтение значения типа byte (0-255)
#define LOAD_AIRING_PERIOD EEPROM.read(22)                        // Чтение значения типа byte (0-255)
#define LOAD_AIRING_TIME EEPROM.read(23)                          // Чтение значения типа byte (0-255)
/* -------------------- Макросы записи параметров в EEPROM -------------------- */
#define SAVE_START_STOP_INCUBATION(value) EEPROM.update(0, value) // Чтение значения типа byte (0-255)
#define SAVE_OLD_DAY(value) EEPROM.update(1, value)               // Чтение значения типа byte (0-255)
#define SAVE_CURRENT_DAY(value) EEPROM.update(2, value)           // Чтение значения типа byte (0-255)
#define SAVE_CURRENT_HOUR(value) EEPROM.update(3, value)          // Чтение значения типа byte (0-255)
#define SAVE_CURRENT_MINUTE(value) EEPROM.update(4, value)        // Чтение значения типа byte (0-255)
#define SAVE_BIRD(value) EEPROM.update(5, value)                  // Чтение значения типа byte (0-255)
#define SAVE_NEED_TEMPERATURE(value) EEPROM.put(6, value)         // Чтение значения типа float
#define SAVE_CORRECTIONS_TEMPERATURE(value) EEPROM.put(10, value) // Чтение значения типа byte (0-255)
#define SAVE_NEED_HUMIDITY(value) EEPROM.put(14, value)           // Чтение значения типа float
#define SAVE_CORRECTIONS_HUMIDITY(value) EEPROM.put(18, value)    // Чтение значения типа byte (0-255)
#define SAVE_HUMIDITY_HYSTERESIS(value) EEPROM.update(19, value)  // Чтение значения типа byte (0-255)
#define SAVE_TURN_PERIOD(value) EEPROM.update(20, value)          // Чтение значения типа byte (0-255)
#define SAVE_TURN_TIME(value) EEPROM.update(21, value)            // Чтение значения типа byte (0-255)
#define SAVE_AIRING_PERIOD(value) EEPROM.update(22, value)        // Чтение значения типа byte (0-255)
#define SAVE_AIRING_TIME(value) EEPROM.update(23, value)          // Чтение значения типа byte (0-255)

/* -------------------- Настройка входов/выходов -------------------- */
#define ENCODER_CLK_PIN A1 // энкодер CLK
#define ENCODER_DT_PIN A2  // энкодер DT
#define ENCODER_SW_PIN A3  // энкодер SW
#define HEATER_PIN 4       // нагреватель
#define HUMIDIFER_PIN 5    // увлажнитель
#define AIRING_PIN 6       // вентилятор проветривания
#define TURN_PIN 7         // поворот лотка
#define ALARM_PIN 8        // авария (пищалка и/или светодиод при аварии)

// TRelay humidifer(HUMIDIFER_PIN);
// TRelay airing(AIRING_PIN);
TRelay turn(TURN_PIN);

/* -------------------- Объявляем глобальные переменные -------------------- */
uint8_t glyphsT[8] = {B00100, B01010, B01010, B01110, B01110, B11111, B11111, B01110}; // символ температуры
uint8_t glyphsH[8] = {B00100, B00100, B01010, B01010, B10001, B10001, B10001, B01110}; // символ влажности

/* -------------------- Описание массива {птица, период инкубации, {день, температура, влажность, проветривание (минут 2 раза в сутки), поворот (да/нет)}} -------------------- */
struct bird
{
  const char *name;
  const uint8_t period;
  const int mode[4][5];
} const bird[6] =
    {
        {"CHICK", 21, {{2, 382, 600, 0, 1}, {12, 376, 600, 3, 1}, {15, 374, 480, 10, 1}, {18, 370, 855, 10, 0}}},
        {"QUAIL", 17, {{2, 378, 550, 0, 1}, {7, 378, 500, 3, 1}, {14, 378, 500, 10, 1}, {16, 375, 700, 0, 0}}},
        {"DUCK", 28, {{7, 382, 700, 0, 1}, {14, 378, 600, 0, 1}, {25, 378, 600, 10, 1}, {28, 375, 855, 0, 0}}},
        {"MUSKUS", 21, {{2, 380, 600, 0, 1}, {7, 378, 550, 0, 1}, {29, 375, 450, 10, 1}, {30, 370, 750, 0, 0}}},
        {"PEREPEL", 30, {{7, 378, 700, 0, 1}, {14, 378, 600, 0, 1}, {27, 378, 600, 15, 1}, {30, 375, 855, 0, 0}}},
        {"TURKEY", 28, {{8, 377, 650, 0, 1}, {14, 377, 500, 0, 1}, {25, 375, 650, 10, 1}, {28, 375, 655, 10, 0}}},
};

boolean incubation = 0;              // старт - стоп инкубации
uint8_t current_bird = 0;            // выбранная птица
uint8_t heater_HIGH_LOW = LOW;       // флаг работы нагреателя
float current_temperature = 0.0;     // текущая температура
float corrections_temperature = 0.0; // коррекция температуры
float need_temperature = 38.2;       // заданная температура для текущего периода
uint8_t humidifier_HIGH_LOW = LOW;   // флаг работы влажности
float current_humidity = 0.0;        // текущая влажность
float corrections_humidity = 0.0;    // коррекция влажности
float need_humidity = 60.0;          // заданная влажность для текущего периода
boolean need_turn = false;           // нужен ли поворот яиц?
uint8_t turn_period = 5;             // периодичночть поворотов в сутки (час)
uint8_t turn_time = 10;              // продолжительность поворота лотка (сек)
uint8_t airing_period = 2;           // периодичночть проветривания в сутки (час)
uint8_t airing_time = 20;            // длительность проветривания (мин)

/* -------------------- PID регулятор -------------------- */
#define WindowSize 500
double Input, Output, Setpoint = need_temperature;              // объявляем переменные для ПИД регулятора
PID heater_pid(&Input, &Output, &Setpoint, 50, .5, .1, DIRECT); // инициализируем ПИД-библиотеку и коэффициенты

/* -------------------- переменные таймера на прерываниии -------------------- */
volatile uint8_t current_day = 0;    // день (обновляется каждый день)
volatile uint8_t current_hour = 0;   // час (обновляется каждый час)
volatile uint8_t current_minute = 0; // минуты
volatile uint8_t current_second = 0; // секунды

/* -------------------- Таймер: день, часы, минуты, секунды -------------------- */
ISR(TIMER1_OVF_vect)
{
  if (incubation)										//Состояние работы инкубатора (ВКЛ/ВЫКЛ)
  {
    if (++current_second >= 60)			//Увеличеваем счетчик секунд пока не достигнет 60
    {
      current_second = 0;						//Обнуляем сукунды
      if (++current_minute >= 60)		//Увеличеваем счетчик минут пока не достигнет 60
      {
        current_minute = 0;					//Обнуляем минуты
        if (++current_hour >= 24)		//Увеличеваем часов секунд пока не достигнет 24
        {
          current_hour = 0;					//Обнуляем часы
          ++current_day;						//Увеличеваем счетчик дней
        }
      }
    }
    SAVE_CURRENT_MINUTE(current_minute); // Сохраняем его в EEPROM
    SAVE_CURRENT_HOUR(current_hour);     // Сохраняем его в EEPROM
    SAVE_CURRENT_DAY(current_day);       // Сохраняем его в EEPROM
  }
}

/* -------------------- Прерывание вызывается один раз в миллисекунду, ищет любые новые данные, и если нашло, сохраняет их -------------------- */
ISR(TIMER0_COMPA_vect)
{
  unsigned long currentMillis = millis();

  // humidifer.Update(currentMillis);
  // airing.Update(currentMillis);
  turn.Update(currentMillis);
}

/* -------------------- Установка настроек в зависимоти от птици и дня инкубации -------------------- */
void settings()
{
  incubation = LOAD_START_STOP_INCUBATION;
  current_bird = LOAD_BIRD;
  
  for (int d = 3; d >= 0; d--)
  {
    if (current_day <= bird[current_bird].mode[d][0])
    {
      need_temperature = float(bird[current_bird].mode[d][1]) / 10;
      need_humidity = float(bird[current_bird].mode[d][2]) / 10;
      airing_time = bird[current_bird].mode[d][3];
      need_turn = bird[current_bird].mode[d][4];
    }
  }

  turn_period = LOAD_TURN_PERIOD;     // периодичночть поворотов в сутки
  turn_time = LOAD_TURN_TIME;         // время поворота лотка
  airing_period = LOAD_AIRING_PERIOD; // периодичночть проветривания в сутки
  LOAD_CORRECTIONS_TEMPERATURE(corrections_temperature);
  LOAD_CORRECTIONS_HUMIDITY(corrections_humidity);

  SAVE_OLD_DAY(current_day);

  // humidifer.Setup(90000, 10000);
  // airing.Setup(airing_period, airing_time);
  turn.Setup(turn_period * 30000, turn_time * 1000);
}

/* -------------------- Отображение информации на LCD экране -------------------- */
void screen(uint8_t page)
{
  switch (page)
  {
  case 1: // температура с уставками, PID регулятор
  {
    // температура с уставками
    lcd.setCursor(0, 0);
    lcd.print(F("\x07 "));
    lcd.print(current_temperature, 1);
    lcd.print(F("\xDF"
                "C  ("));
    lcd.print(need_temperature, 1);
    lcd.print(')');
    lcd.setCursor(17, 0);
    if (heater_HIGH_LOW)
      lcd.print(F("ON "));
    else
      lcd.print(F("OFF")); // отображение вкл/выкл нагреватель
    // влажность с уставками
    lcd.setCursor(0, 1);
    lcd.print(F("\x08 "));
    lcd.print(current_humidity, 1);
    lcd.print(F("%RH ("));
    lcd.print(need_humidity, 1);
    lcd.print(')');
    lcd.setCursor(17, 1);
    if (humidifier_HIGH_LOW)
      lcd.print(F("ON "));
    else
      lcd.print(F("OFF")); // отображение вкл/выкл увлажнитель
    // PID регулятор
    lcd.setCursor(1, 2);
    lcd.printf(" %03d%%   ", map(Output, 0, WindowSize, 0, 100));
    lcd.setCursor(8, 2);
    lcd.print(Output, 1);
    lcd.print(F("mS  "));
    lcd.setCursor(17, 2);
    if (incubation)
      lcd.print(F("ON "));
    else
      lcd.print(F("OFF")); // отображение вкл/выкл инкубация
    // птица, период инкубации
    lcd.setCursor(0, 3);
    lcd.printf("%s     ", bird[current_bird].name);
    lcd.setCursor(8, 3);
    lcd.printf("%02d(%02d) %02d:%02d", current_day, bird[current_bird].period, current_hour, current_minute);
    break;
  }
  }
}

/* -------------------- Обработчик для пункта меню setDateTimeInk -------------------- */
void setDateTime()
{
  lcd.clear();
  lcd.printfAt(0, 0, "Day    (d): %d", current_day);                      // Приглашение для ввода (дня) инкубации
  lcd.printfAt(0, 1, "Hour   (h): %d", current_hour);                     // Приглашение для ввода (секунд)
  lcd.printfAt(0, 2, "Minute (m): %d", current_minute);                   // Приглашение для ввода (минут)
  current_day = lcd.inputValAt<uint8_t>(12, 0, 0, 21, current_day);       // день
  current_hour = lcd.inputValAt<uint8_t>(12, 1, 0, 23, current_hour);     // час
  current_minute = lcd.inputValAt<uint8_t>(12, 2, 0, 59, current_minute); // минута
  SAVE_CURRENT_DAY(current_day);                                          // Сохраняем его в EEPROM
  SAVE_CURRENT_HOUR(current_hour);                                        // Сохраняем его в EEPROM
  SAVE_CURRENT_MINUTE(current_minute);                                    // Сохраняем его в EEPROM
}

/* -------------------- Обработчик для пункта меню setBird -------------------- */
void setBird()
{
  lcd.clear();
  String list_bird[6];
  for (int i = 0; i < 6; i++)
  {
    list_bird[i] = bird[i].name;
  }
  current_bird = lcd.selectVal("", list_bird, 6, true, current_bird); // Запрашиваем новое значение
  SAVE_BIRD(current_bird);                                            // Сохраняем его в EEPROM
}

/* -------------------- Обработчик для пункта меню setTurn -------------------- */
void setTurn()
{
  lcd.clear();
  lcd.printfAt(0, 0, "Period (h): %d", turn_period);                // Приглашение для ввода (час)
  lcd.printfAt(0, 1, "Time   (s): %d", turn_time);                  // Приглашение для ввода (сек)
  turn_period = lcd.inputValAt<uint8_t>(12, 0, 0, 12, turn_period); // количество переворотов в сутки
  turn_time = lcd.inputValAt<uint8_t>(12, 1, 0, 20, turn_time);     // время поворота лотка
  SAVE_TURN_PERIOD(turn_period);                                    // Сохраняем его в EEPROM
  SAVE_TURN_TIME(turn_time);                                        // Сохраняем его в EEPROM
}

/* -------------------- Обработчик для пункта меню setAiring -------------------- */
void setAiring()
{
  lcd.clear();
  lcd.printfAt(0, 0, "Period (h): %d", airing_period);                 // Приглашение для ввода (час)
  lcd.printfAt(0, 1, "Time   (m): %d", airing_time);                   // Приглашение для ввода (мин)
  airing_period = lcd.inputValAt<uint8_t>(12, 0, 0, 4, airing_period); // количество проветриваний в сутки
  airing_time = lcd.inputValAt<uint8_t>(12, 1, 0, 10, airing_time);    // время проветривания
  SAVE_AIRING_PERIOD(airing_period);                                   // Сохраняем его в EEPROM
  SAVE_AIRING_TIME(airing_time);                                       // Сохраняем его в EEPROM
}

/* -------------------- Обработчик для пункта меню SetCorrections -------------------- */
void setCorrections()
{
  lcd.clear();
  lcd.printfAt(0, 0, "Temperature : %d", corrections_temperature);                        // Приглашение для ввода (час)
  lcd.printfAt(0, 1, "Humidity    : %d", corrections_humidity);                           // Приглашение для ввода (мин)
  corrections_temperature = lcd.inputValAt<int>(14, 0, -99, 99, corrections_temperature); // количество проветриваний в сутки
  corrections_humidity = lcd.inputValAt<int>(14, 1, -99, 99, corrections_humidity);       // время проветривания
  SAVE_CORRECTIONS_TEMPERATURE(corrections_temperature);                                  // Сохраняем его в EEPROM
  SAVE_CORRECTIONS_HUMIDITY(corrections_humidity);                                        // Сохраняем его в EEPROM
}

/* -------------------- Объявим перечисление, используемое в качестве ключа пунктов меню -------------------- */
enum
{
  mBack,
  mRoot,
  mOptions,
  mSetDateTime,
  mSetBird,
  mSetTurn,
  mSetAiring,
  mSetCorrections
};
/* -------------------- Описание структуры пункта меню: {ParentKey, Key, Caption, [Handler]} -------------------- */
sMenuItem menu[] = {
    {mBack, mRoot, "Menu", NULL},
    {mRoot, mOptions, "Options", NULL},
    {mOptions, mSetDateTime, "Date | Time", setDateTime},
    {mOptions, mSetBird, "Bird", setBird},
    {mOptions, mSetTurn, "Turn", setTurn},
    {mOptions, mSetAiring, "Airing", setAiring},
    {mOptions, mSetCorrections, "Corrections", setCorrections},
    {mOptions, mBack, "Back", NULL},
    {mRoot, mBack, "Exit", NULL}};
uint8_t menuLen = sizeof(menu) / sizeof(sMenuItem);

/* -------------------- Обработчик для пункта меню StartStopIncubation -------------------- */
void StartStop()
{
  if (incubation)
  {
    incubation = false;
    heater_HIGH_LOW = LOW; // устанавливаем признак включения/отключения нагревателя
    humidifier_HIGH_LOW = LOW;
  }
  else
  {
    incubation = true;
    heater_HIGH_LOW = HIGH; // устанавливаем признак включения/отключения нагревателя
    humidifier_HIGH_LOW = HIGH;
  }
  SAVE_START_STOP_INCUBATION(incubation); // Сохраняем его в EEPROM
}

/* -------------------- Чтение показаний с датчиков температуры и влажности -------------------- */
void sensors()
{
  static unsigned long sensors_millis;
  if (millis() - sensors_millis > 2000)
  {
    sensors_millis = millis();
    current_temperature = aht.readTemperature(); // Считываем показание температуры
    current_humidity = aht.readHumidity();       // Считываем показание влажности
  }
}

/* -------------------- Управление нагревателем -------------------- */
void heater()
{
  static unsigned long heater_millis;
  Input = current_temperature;
  Setpoint = need_temperature;
  if (incubation)
  {
    heater_pid.SetMode(AUTOMATIC);
    heater_HIGH_LOW = HIGH;
  }
  else
  {
    heater_pid.SetMode(MANUAL);
    heater_HIGH_LOW = LOW;
    Output = 0;
  }
  heater_pid.Compute();
  if (millis() - heater_millis > WindowSize)
    heater_millis += WindowSize; // таймер на миллис
  if (Output > (millis() - heater_millis) && incubation)
  {
    heater_HIGH_LOW = HIGH;
  }
  else
  {
    heater_HIGH_LOW = LOW;
  }
  digitalWrite(HEATER_PIN, heater_HIGH_LOW);
}

/* -------------------- Управление влажностью -------------------- */
void humidity()
{
  if (incubation)
  {
    static unsigned long humidity_millis;
    uint8_t humidity_hysteresis = 2;
    if (millis() >= humidity_millis + 2000)
    { // сравниваем измеренную влажность с заданной
      if (need_humidity > current_humidity)
        humidifier_HIGH_LOW = HIGH;
      if (need_humidity < current_humidity + humidity_hysteresis)
        humidifier_HIGH_LOW = LOW;
    }
  }
  digitalWrite(HUMIDIFER_PIN, humidifier_HIGH_LOW);
}

///* -------------------- Упраывление поворотом лотка -------------------- */
// void turn()
//{
//   if (incubation)
//   {
//     static unsigned long turn_millis;
//     if (turn_period == 0 || need_turn == 0)
//       return;                                                                             // если нулевой период поворота, то не поворачиваем яйца.
//     if ((digitalRead(TURN_PIN) == HIGH) && (millis() - turn_millis >= turn_time * 10000)) // продолжительность переворота лотка // 10*1000=10000ms // 10 секунд
//     {
//       digitalWrite(TURN_PIN, LOW); // Update the actual relay
//     }
//     else if ((digitalRead(TURN_PIN) == LOW) && (millis() - turn_millis >= turn_period * 3600000)) // переодичность переворота в часах 3600000
//     {
//       turn_millis = millis();       // Remember the time
//       digitalWrite(TURN_PIN, HIGH); // Update the actual relay
//     }
//   }
//   else
//     digitalWrite(TURN_PIN, LOW);
// }
//
///* -------------------- Управление проветриванием -------------------- */
// void airing()
//{
//   if (incubation) // если идет инкубация
//   {
//     // если время проветривания(airing_time) больше 0 то отключаем нагреватель и включаем проветривание на время проветривания(airing_time) минут 2 раза в сутки
//     static unsigned long airing_millis;
//     if (airing_period == 0 || airing_time == 0) // если нулевой период проветривания, то не проветриваем инкубатор
//       return;
//     if ((digitalRead(AIRING_PIN) == HIGH) && (millis() - airing_millis >= airing_time * 60000)) // продолжительность проветривания в минутах
//     {
//       // heater_HIGH_LOW = LOW;            // устанавливаем признак включения/отключения нагревателя
//       digitalWrite(AIRING_PIN, LOW); // отключаем вентилятор проветривания
//     }
//     else if ((digitalRead(AIRING_PIN) == LOW) && (millis() - airing_millis >= airing_period * 3600000)) // переодичность проветривания в часах
//     {
//       // heater_HIGH_LOW = HIGH;              // устанавливаем признак включения/отключения нагревателя
//       airing_millis = millis();       // Remember the time
//       digitalWrite(AIRING_PIN, HIGH); // включаем вентилятор проветривания
//     }
//   }
//   else
//   {
//     // heater_HIGH_LOW = HIGH;
//     digitalWrite(AIRING_PIN, LOW); // отключаем вентилятор проветривания
//   }
// }

/* -------------------- Управляем авариями -------------------- */
void alarm()
{
  static uint8_t alarm_temperature = 5;
  // если измеренная температура выше заданной на величину аварии
  if (current_temperature > (need_temperature + alarm_temperature) || current_temperature < (need_temperature - alarm_temperature))
  {
    digitalWrite(ALARM_PIN, LOW);
  }
  else
    digitalWrite(ALARM_PIN, HIGH); // то включаем аварийный сигнал.

  // if (current_temperature > (need_temperature + alarm_temperature) && fanState == 1) needFan = 1;
  // if (current_temperature < (need_temperature + alarm_temperature - 2)) needFan = 0;
}

/* -------------------- Первоначальная установка -------------------- */
void setup()
{
  lcd.begin();                          //  Инициируем работу с LCD дисплеем
  lcd.backlight();                      //  Включаем подсветку LCD дисплея
  lcd.print(F("=< INCUBATOR v1.3 >=")); // Приветственный экран
  lcd.setCursor(0, 1);
  lcd.print(F("     08.02.2024     ")); // Приветственный экран
  delay(500);

  /* -------------------- Инициализация датчиков темепературы и влажности -------------------- */
  aht.begin();

  lcd.setCursor(0, 3);
  lcd.print(F("Starting"));
  for (int i = 0; i < 12; ++i)
  {
    lcd.print('.');
    delay(300);
  }
  lcd.clear();                // очищаем экран
  lcd.createChar(7, glyphsT); // cоздаем пользовательске символы
  lcd.createChar(8, glyphsH); // cоздаем пользовательске символы

  /* -------------------- Устанавливаем выводы -------------------- */
  pinMode(HEATER_PIN, OUTPUT); // нагреватель
  digitalWrite(HEATER_PIN, heater_HIGH_LOW);
  pinMode(HUMIDIFER_PIN, OUTPUT); // влажность
  digitalWrite(HUMIDIFER_PIN, humidifier_HIGH_LOW);
  pinMode(AIRING_PIN, OUTPUT); // проветривание
  digitalWrite(AIRING_PIN, LOW);
  pinMode(TURN_PIN, OUTPUT); // переворот
  digitalWrite(TURN_PIN, LOW);
  pinMode(ALARM_PIN, OUTPUT); // авария (пищалка, светодиод при аварии)
  digitalWrite(ALARM_PIN, LOW);

  lcd.attachEncoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, ENCODER_SW_PIN);

  /* -------------------- Настройка таймера -------------------- */
  cli();                                              // отключить глобальные прерывания
  TCCR1A = 0;                                         // установить TCCR1A регистр в 0
  TCCR1B = 0;                                         // установить TCCR1B регистр в 0
  TCCR1A = (1 << WGM11);                              // режим 14 FAST PWM
  TCCR1B = (1 << CS12) | (1 << WGM13) | (1 << WGM12); // делить частоту CPU на 256
  ICR1 = 62499;                                       // (16000000MHz / div 256) - 1 = 1 раз в секунду
  TIMSK1 = (1 << TOIE1);                              // разрешить прерывание
  sei();                                              // включить глобальные прерывания

  // Timer0 уже используется millis() - прерываемся примерно посередине и вызываем ниже функцию "Compare A"
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);

  heater_pid.SetOutputLimits(0, WindowSize); // задаем лимиты ширины ПИД-импульса от 0 до 1 секунды.
  heater_pid.SetMode(AUTOMATIC);             // включаем ПИД-регулирование

  current_day = LOAD_CURRENT_DAY;
  current_hour = LOAD_CURRENT_HOUR;
  current_minute = LOAD_CURRENT_MINUTE;

  settings();
}

/* -------------------- Основной цикл программы -------------------- */
void loop()
{
  static uint8_t screen_page = 1; // счетчик текущего номера экрана 

  if (LOAD_OLD_DAY != current_day) { settings(); }  // раз в день читаем настройки заново

  sensors();  // опрос датчиков температуры и влажности
  heater();   // управление нагревателем
  humidity(); // управление влажностью
  // airing();   // управление проветриванием
  // turn();     // поворот лотка
  alarm(); // управляем авариями

  // Опрашиваем энкодер
  eEncoderState EncoderState = lcd.getEncoderState();
  switch (EncoderState)
  {
   case eLeft:
  //    screen_page++;
  //    if (screen_page > 3)
  //      screen_page = 1;
     break;
   case eRight:
  //    screen_page--;
  //    if (screen_page < 1)
  //      screen_page = 3;
     break;
  case eButton:
    // Формируем названия пунктов меню
    lcd.showMenu(menu, menuLen, 1); // Вызываем меню
    lcd.createChar(7, glyphsT);     // cоздаем пользовательске символы
    lcd.createChar(8, glyphsH);     // cоздаем пользовательске символы
    break;
  case eLongButton:
    StartStop();
    break;
  case eNone:
    screen(screen_page);
    return;
  }
}
