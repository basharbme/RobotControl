#include "robot.h"

void InitEventHandler() {
	// Turn on 6050 even not set for autoStand, MPU can be used in other area
	//if (config.autoStand()) {
		MpuInit();
	//}
	eIdle.LoadData(EVENT_IDEL_FILE);
	eBusy.LoadData(EVENT_BUSY_FILE);

}


unsigned long nextMpuCheckMs = 0;
unsigned long nextTouchCheckMs = 0;
unsigned long nextBatteryMs = 0;

void RobotEventHandler() {

#ifdef NEW_EVENT_HANDLER
/*
*	New version will EventHandler object
*	- TODO: define generic data source class for better program flow
*           sensors should be inherit from this data source class
*           Generic method for data soruce:
*             - Initialization
*             - Data Colection
*             - Post Checking Control
*           with internal logic to handle frequency of checking
*
*	1) Data collection
*	2) Condition checking
*	3) Post checking control 
*	4) Action
*
*/
	bool showResult = false;

	// Data Collection
	eData.Clear();

	EventHandler *eActive;
	if (V2_ActionPlaying) {
		eActive = &eBusy;
	} else {
		eActive = &eIdle;
	}

	// Touch
	// To make touch works for counting number of touch, it must be executed all the time
	// Maybe the reporting frequency has to be handled in post chekcing control once touch event is handled
	if (config.enableTouch()) {
		uint8_t touchMotion = CheckTouchAction();
		if (eActive->IsRequired((uint8_t) EventData::DEVICE::touch)) {
			eData.SetData(EventData::DEVICE::touch, 0, 0, touchMotion);
		}
	}

	// MPU 6050
	if (millis() > nextMpuCheckMs) {
		if (mpuExists && eActive->IsRequired((uint8_t) EventData::DEVICE::mpu)) {
			if (MpuGetData()) {
				eData.SetData(EventData::DEVICE::mpu, 0, 0, ax);
				eData.SetData(EventData::DEVICE::mpu, 0, 1, ay);
				eData.SetData(EventData::DEVICE::mpu, 0, 2, az);
			}
		}
		nextMpuCheckMs = millis() + (1000 / config.positionCheckFreq());
	}

	// Battery
	if (millis() > nextBatteryMs) {
		if (eActive->IsRequired((uint8_t) EventData::DEVICE::battery_level) ||
			eActive->IsRequired((uint8_t) EventData::DEVICE::battery_reading)) {
			uint16_t v = analogRead(0);
			DEBUG.printf("Battery reading: %d\n", v);
			eData.SetData(EventData::DEVICE::battery_reading, 0, 0, v);
			if (eActive->IsRequired((uint8_t) EventData::DEVICE::battery_level)) {
				int iPower = GetPower(v);
				DEBUG.printf("Battery level: %d\n", iPower);
				eData.SetData(EventData::DEVICE::battery_level, 0, 0, iPower);
			}
		} else {
			DEBUG.printf("Battery is not required: %d\n", eActive->Count());
		}
		showResult = true;
		nextBatteryMs = millis() + 5000;
	}


	// Condition checking

	EventHandler::EVENT event = eActive->CheckEvents();
    EventHandler::ACTION action = event.data.action;

	if (showResult) {
		eData.DumpData(&DEBUG);
		eActive->DumpEvents(&DEBUG);
		DEBUG.printf("Event matched: %d\n", event.data.type);
	}

	// Post checking control 
	/*
	*	Need to think about how to prevent keep triggering the same event as condition may not changed
	*   May add time interval for eData once handled
	*/

	// Action	
	if (event.data.type) {
		switch (action.data.type) {

            case (uint8_t) EventHandler::ACTION_TYPE::na:
                break;
	
            case (uint8_t) EventHandler::ACTION_TYPE::playAction:
                if (debug) DEBUG.printf("Play action %d \n", action.data.parm_1);
                break;

            case (uint8_t) EventHandler::ACTION_TYPE::stopAction:
                if (debug) DEBUG.println("Stop action");
                break;

            case (uint8_t) EventHandler::ACTION_TYPE::mp3PlayMp3:
                if (debug) DEBUG.printf("Play mp3 %d\n", action.u16data.parm_u16);
                break;

            case (uint8_t) EventHandler::ACTION_TYPE::mp3PlayFile:
                if (debug) DEBUG.printf("Play mp3 at  %d : %d \n", action.data.parm_1, action.data.parm_2);
                break;

            case (uint8_t) EventHandler::ACTION_TYPE::gpio:
                if (debug) DEBUG.printf("Set gpio %d to %s\n", action.data.parm_1, 
                                                    (action.data.parm_2 ? "HIGH" : "LOW"));
                break;

            default:
                if (debug) DEBUG.printf("Unknown action %d \n", action.data.type);
                break;			
		}
	}


#else

	// Old version with direct setting

	#ifndef DISABLE_BATTERY_CHECK
   	CheckVoltage();
	#endif

	// Check auto-response action (should only play if not in action)
	if (V2_ActionPlaying) return;
	CheckPosition();
	CheckTouch();  // TODO: Too many messages!

#endif

}


unsigned nextPositionCheckMs = 0;

void CheckPosition() {
  if (!config.autoStand()) return;
  if (millis() > nextPositionCheckMs) {
    // DEBUG.println("No Client connected, or connection lost");
      // if (EN_MPU6050) MpuGetActionHandle();
    // if (debug) DEBUG.println("MpuGetActionHandle"); // too many message will cause debug port hang
    MpuGetActionHandle();

    // TODO, use config for frequency
    nextPositionCheckMs = millis() + (1000 / config.positionCheckFreq());
  }
}

void CheckTouch() {
	if (!config.enableTouch()) return;
	//touch handle
	
	// uint8_t touchMotion = DetectTouchMotion();
	uint8_t touchMotion = CheckTouchAction();
	//if(touchMotion == TOUCH_LONG) ReserveEyeBreath();
	//if(touchMotion == TOUCH_DOUBLE) ReserveEyeBlink();
	EyeLedHandle();
}

// ADC_MODE(ADC_VCC);  -- only use if checking input voltage ESP.getVcc() is required.
unsigned long nextVoltageMs = 0;
unsigned long nextVoltageAlarmMs = 0;

void CheckVoltage() {
	if (millis() > nextVoltageMs) {

		uint16_t v = analogRead(0);
		// if (debug) DEBUG.printf("analgeRead(0): %d\n", v);
		int iPower = GetPower(v);
		if (config.enableOLED()) {

			// Code form James
			myOLED.printNum(104,0,iPower, 10, 3, false);
			myOLED.print(122,0,"%");
			myOLED.show();

			// Code from L, TODO: use config to get reference voltage for conversion
			myOLED.print(0,5,"Volt: ");
			myOLED.printFloat((float)(analogRead(A0)/1024.0*11.0));
			// myOLED.printNum(0,10,analogRead(A0));
			myOLED.show();
		}

		if (v < config.alarmVoltage())	{
			if (millis() > nextVoltageAlarmMs) {
				nextVoltageAlarmMs += (config.voltageAlarmInterval() * 1000);
				if (debug) DEBUG.printf("Battery low: %d (%d) \n", v, config.alarmVoltage());
				if (config.voltageAlarmMp3()) {
					mp3.begin();
					mp3.stop();
					mp3.playMp3File(config.voltageAlarmMp3());
					mp3.end();
				}
			}
		} 

		nextVoltageMs = millis() + 1000;
		// DEBUG.printf("v: %d, min: %d, max: %d, alarm: %d, power: %d%%\n\n", v, config.minVoltage(), config.maxVoltage(), config.alarmVoltage(), iPower);
	}

}

// For consistence, build common function to convert A0 value to Power rate
byte GetPower(uint16_t v) {
	// Power in precentage instead of voltage
	float power = ((float) (v - config.minVoltage()) / (config.maxVoltage() - config.minVoltage()) * 100.0f);
	int iPower = (int) (power + 0.5); // round up
	if (iPower > 100) iPower = 100;
	if (iPower < 0) iPower = 0;
	return (byte) iPower;
}