#ifndef RTC_H
#define RTC_H

#include <RtcDS1302.h>

// Объявление объектов для работы с DS1302
extern ThreeWire myWire;
extern RtcDS1302<ThreeWire> Rtc;

// Объявление функции для инициализации RTC
void initializeRtc();

#endif // RTC_H