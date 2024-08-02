#include "GUI.h"

#include "Constants.h"

String GUI::status = "";

void GUI::updateRenogyStatus(const Renogy::Data& data)
{
    auto battery = _status["b"];
    battery["ch"] = data.batteryCharge;
    battery["vo"] = data.batteryVoltage;
    battery["cu"] = data.batteryCurrent;
    battery["te"] = data.batteryTemperature;
    battery["ge"] = data.generation;
    battery["co"] = data.consumption;
    battery["to"] = data.total;

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
    _status["pvosta"] = status;
}

void GUI::updateOutputStatus(const OutputStatus& status)
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
    _status["rssi"] = RNGBridge::rssi;
    status.clear();
    serializeJson(_status, status);
}
