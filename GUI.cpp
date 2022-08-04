#include "GUI.h"

#include <Updater.h>

#include "Constants.h"
#include "MQTT.h"
#include "PVOutput.h"

void GUI::updateRenogyStatus(const Renogy::Data& data)
{
    auto battery = _status["b"];
    battery["ch"] = data.batteryCharge;
    battery["vo"] = data.batteryVoltage;
    battery["cu"] = data.batteryCurrent;
    battery["te"] = data.batteryTemperature;

    auto load = _status["l"];
    load["vo"] = data.loadVoltage;
    load["cu"] = data.loadCurrent;

    auto panel = _status["p"];
    panel["vo"] = data.panelVoltage;
    panel["cu"] = data.panelCurrent;

    auto controller = _status["c"];
    controller["st"] = data.chargingState;
    controller["er"] = data.errorState;
    controller["te"] = data.controllerTemperature;

    auto output = _status["o"];
    output["l"] = data.loadEnabled;
}

void GUI::updateMQTTStatus(const String& status)
{
    _status["mqttsta"] = status;
}

void GUI::updatePVOutputStatus(const String& status)
{
    _status.garbageCollect();
    _status["pvosta"] = status;
}

void GUI::updateOutputStatus(const OutputControl::Status& status)
{
    auto output = _status["o"];
    output["o1"] = status.out1;
    output["o2"] = status.out2;
    output["o3"] = status.out3;
}

void GUI::updateOtaStatus(const String& status)
{
    _status["otasta"] = status;
}

void GUI::updateUptime(const uint32_t uptime)
{
    _status["up"] = uptime;
}

void GUI::updateHeap(const uint32_t heap)
{
    _status["he"] = heap;
}

void GUI::update()
{
    String serialized = {};
    _status["rssi"] = RNGBridge::rssi;
    serialized.reserve(measureJson(_status));
    serializeJson(_status, serialized);
    _networking.sendTestMessage(serialized);
}
