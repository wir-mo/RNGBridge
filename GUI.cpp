#include "GUI.h"

#include <Updater.h>

#include "Constants.h"
#include "MQTT.h"
#include "PVOutput.h"

void GUI::updateRenogyStatus(const Renogy::Data& data)
{
    _status["b"]["charge"] = data.batteryCharge;
    _status["b"]["voltage"] = data.batteryVoltage;
    _status["b"]["current"] = data.batteryCurrent;
    _status["b"]["temperature"] = data.batteryTemperature;

    _status["l"]["enabled"] = data.loadEnabled;
    _status["l"]["voltage"] = data.loadVoltage;
    _status["l"]["current"] = data.loadCurrent;

    _status["p"]["voltage"] = data.panelVoltage;
    _status["p"]["current"] = data.panelCurrent;

    _status["s"]["state"] = data.chargingState;
    _status["s"]["error"] = data.errorState;
    _status["s"]["temperature"] = data.controllerTemperature;
}

void GUI::updateMQTTStatus(const String& status)
{
    _status["mqtt"]["status"] = status;
}

void GUI::updatePVOutputStatus(const String& status)
{
    _status["pvo"]["status"] = status;
}

void GUI::updateUptime(const uint32_t uptime)
{
    DEBUG(F("Uptime: "));
    DEBUGLN(uptime);
    _status["uptime"] = uptime;
}

void GUI::updateHeap(const uint32_t heap)
{
    _status["heap"] = heap;
}

void GUI::update()
{
    String serialized = {};
    serialized.reserve(measureJson(_status));
    serializeJson(_status, serialized);
    _networking.sendTestMessage(serialized);
}
