#ifndef _ROBOTEVENTHANDLER_H_
#define _ROBOTEVENTHANDLER_H_

#include "EventData.h"
#include "EventHandler.h"

#define EVENT_IDEL_FILE "/alpha/event/idle.event"
#define EVENT_BUSY_FILE "/alpha/event/busy.event"

#define EVENT_HANDLER_VERSION  1

EventData eData;
EventHandler eIdle(&eData);
EventHandler eBusy(&eData);
EventHandler eTemp(&eData);

void InitEventHandler();
void RobotEventHandler();
void CheckPosition();
void CheckTouch();
void CheckVoltage() ;
byte GetPower(uint16_t v);

#endif