#include "GUI.h"

#include "Constants.h"

String GUI::status = "";

void GUI::updateRenogyStatus(const Renogy::Data& data)
{
    switch (data.cellCount)
    {
    case 16: {
        auto cell = _status["c16"];
        cell["te"] = data.cellTemperature[15];
        cell["vo"] = data.cellVoltage[15];
    }
    case 15: {
        auto cell = _status["c15"];
        cell["te"] = data.cellTemperature[14];
        cell["vo"] = data.cellVoltage[14];
    }
    case 14: {
        auto cell = _status["c14"];
        cell["te"] = data.cellTemperature[13];
        cell["vo"] = data.cellVoltage[13];
    }
    case 13: {
        auto cell = _status["c13"];
        cell["te"] = data.cellTemperature[12];
        cell["vo"] = data.cellVoltage[12];
    }
    case 12: {
        auto cell = _status["c12"];
        cell["te"] = data.cellTemperature[11];
        cell["vo"] = data.cellVoltage[11];
    }
    case 11: {
        auto cell = _status["c11"];
        cell["te"] = data.cellTemperature[10];
        cell["vo"] = data.cellVoltage[10];
    }
    case 10: {
        auto cell = _status["c10"];
        cell["te"] = data.cellTemperature[9];
        cell["vo"] = data.cellVoltage[9];
    }
    case 9: {
        auto cell = _status["c9"];
        cell["te"] = data.cellTemperature[8];
        cell["vo"] = data.cellVoltage[8];
    }
    case 8: {
        auto cell = _status["c8"];
        cell["te"] = data.cellTemperature[7];
        cell["vo"] = data.cellVoltage[7];
    }
    case 7: {
        auto cell = _status["c7"];
        cell["te"] = data.cellTemperature[6];
        cell["vo"] = data.cellVoltage[6];
    }
    case 6: {
        auto cell = _status["c6"];
        cell["te"] = data.cellTemperature[5];
        cell["vo"] = data.cellVoltage[5];
    }
    case 5: {
        auto cell = _status["c5"];
        cell["te"] = data.cellTemperature[4];
        cell["vo"] = data.cellVoltage[4];
    }
    case 4: {
        auto cell = _status["c4"];
        cell["te"] = data.cellTemperature[3];
        cell["vo"] = data.cellVoltage[3];
    }
    case 3: {
        auto cell = _status["c3"];
        cell["te"] = data.cellTemperature[2];
        cell["vo"] = data.cellVoltage[2];
    }
    case 2: {
        auto cell = _status["c2"];
        cell["te"] = data.cellTemperature[1];
        cell["vo"] = data.cellVoltage[1];
    }
    case 1: {
        auto cell = _status["c1"];
        cell["te"] = data.cellTemperature[0];
        cell["vo"] = data.cellVoltage[0];
    }
    }

    _status["a1te"] = data.ambientTemperature[0];
    _status["a2te"] = data.ambientTemperature[2];

    _status["h1te"] = data.heaterTemperature[0];
    _status["h2te"] = data.heaterTemperature[2];

    _status["bmste"] = data.bmsTemperature;

    _status["cy"] = data.cycles;

    _status["cu"] = data.current;
    _status["vo"] = data.voltage;

    _status["rem"] = data.remaining;
    _status["tot"] = data.total;

    auto chargeLimit = _status["chlim"];
    chargeLimit["vo"] = data.chargeVoltageLimit;
    chargeLimit["cu"] = data.chargeCurrentLimit;
    auto dischargeLimit = _status["dchlim"];
    dischargeLimit["vo"] = data.dischargeVoltageLimit;
    dischargeLimit["cu"] = data.dischargeCurrentLimit;
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
