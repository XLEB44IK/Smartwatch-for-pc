#include "rtc.h"
#include <Arduino.h>

// Пины для DS1302
const int RST_PIN = 15;
const int DAT_PIN = 13;
const int CLK_PIN = 12;

// Объекты для работы с DS1302
ThreeWire myWire(DAT_PIN, CLK_PIN, RST_PIN);
RtcDS1302<ThreeWire> Rtc(myWire);

void initializeRtc() {
  Rtc.Begin();

  // Установка времени и даты на момент компиляции
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Rtc.SetDateTime(compiled);
}