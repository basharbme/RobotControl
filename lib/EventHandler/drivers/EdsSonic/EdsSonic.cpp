#include "EdsSonic.h"

EdsSonic::EdsSonic(EventData *data, MyDebugger *dbg, byte devId) {
    _Device = (uint8_t) EventData::DEVICE::sonic;
    Config(data, dbg, devId);
}

EdsSonic::~EdsSonic() {
}

/*
void EdsSonic::Initialize(EventData *data) {
    _data = data;
}
*/

/*
*       Ping:
*          Send:    A8 8A 04 02 00 01 07 ED
*          Return:  A8 8A 05 02 00 01 00 08 ED
*/

void EdsSonic::Setup(SSBoard *ssb) {
     if (_dbg->require(110)) _dbg->log(110, 0, "EdsSonic::Setup(*ssb)");
    _ssb = ssb;
    // Check if device available

    byte cmd[] = {0xA8, 0x8A, 0x04, 0x02, 0x00, 0x01, 0x07, 0xED};
    _isAvailable = _ssb->SendCommand((byte *) cmd, true);
    if (_isAvailable) { 
        if (_dbg->require(10)) _dbg->log(10, 0, "Sonic Sensor detected");
    } else {
        if (_dbg->require(10)) _dbg->log(10, 0, "Sonic Sensor not available");
    }
}

/*
*       Get Data:
*          Send:    A8 8A 06 02 00 02 28 02 34 ED	
*          Return:  A8 8A 08 02 00 02 28 02 00 B4 EA ED （B4，180cm）
*/

bool EdsSonic::GetData() {
    _thisDataReady = false;
    if (!IsReady()) return false;

    bool _prevDataRady = _lastDataReady;
    _lastDataReady = false;

    
    byte cmd[] = { 0xA8, 0x8A, 0x06, 0x02, 0x00, 0x02, 0x28, 0x02, 0x34, 0xED };

    unsigned long startMs = millis();
    if (!_ssb->SendCommand((byte *) cmd, true)) return false;
    
    unsigned long diff = millis() - startMs;
    //_dbg->msg("It taks %d ms to read PSX", diff); 

    Buffer *result = _ssb->ReturnBuffer();
    // Data returned: A8 8A 08 02 00 02 28 02 00 {*} EA ED

    uint16 data = result->peek(8) << 8 | result->peek(9);
    _data->SetData(_Device, _DevId, 0, data);
    if ((_dbg->require(100))) _dbg->log(100,0,"Sonic: [%d,%d,%d] => %d", _Device, _DevId, 0, data);

    return true;
}

void EdsSonic::PostHandler(bool eventMatched, bool isRelated, bool pending) {
    if (!IsReady()) return;
    if (_thisDataReady && isRelated) {
        _nextReportMs = millis() + EDS_DELAY_CHECK_MS;
    } else if (pending) {
        _dbg->msg("edsBattery pending");
        _nextReportMs = millis() + EDS_PENDING_CHECK_MS;
    } else {
        _dbg->msg("edsBattery normal");
        _nextReportMs = millis() + EDS_CONTINUE_CHECK_MS;
    }
}