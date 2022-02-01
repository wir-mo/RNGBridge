#include "GUI.h"

#include <Updater.h>

#include "Constants.h"
#include "MQTT.h"
#include "PVOutput.h"

void GUI::updateRenogyStatus(const Renogy::Data& data)
{
    _status["b"]["ch"] = data.batteryCharge;
    _status["b"]["vo"] = data.batteryVoltage;
    _status["b"]["cu"] = data.batteryCurrent;
    _status["b"]["te"] = data.batteryTemperature;

    _status["l"]["en"] = data.loadEnabled;
    _status["l"]["vo"] = data.loadVoltage;
    _status["l"]["cu"] = data.loadCurrent;

    _status["p"]["vo"] = data.panelVoltage;
    _status["p"]["cu"] = data.panelCurrent;

    _status["s"]["st"] = data.chargingState;
    _status["s"]["er"] = data.errorState;
    _status["s"]["te"] = data.controllerTemperature;
}

void GUI::updateMQTTStatus(const String& status)
{
    _status["mqttstat"] = status;
}

void GUI::updatePVOutputStatus(const String& status)
{
    _status.garbageCollect();
    _status["pvostat"] = status;
}

void GUI::updateUptime(const uint32_t uptime)
{
    DEBUG(F("[System] Uptime: "));
    DEBUGLN(uptime);
    _status["up"] = uptime;
}

void GUI::updateHeap(const uint32_t heap)
{
    _status["he"] = heap;
}

void GUI::update()
{
    String serialized = {};
    serialized.reserve(measureJson(_status));
    serializeJson(_status, serialized);
    _networking.sendTestMessage(serialized);
}
