#include "GUI.h"

#include "Constants.h"

String GUI::status = "";

void GUI::updateMQTTStatus(const String& status)
{
    json["mqttsta"] = status;
}

void GUI::updatePVOutputStatus(const String& status)
{
    json["pvosta"] = status;
}

void GUI::updateOutputStatus(const OutputStatus& status)
{
    auto output = json["o"];
    output["o1"] = status.out1;
    output["o2"] = status.out2;
    output["o3"] = status.out3;
}

void GUI::updateOtaStatus(const String& status)
{
    json["otasta"] = status;
}

void GUI::updateUptime(const uint32_t uptime)
{
    json["up"] = uptime;
}

void GUI::updateHeap(const uint32_t heap)
{
    json["he"] = heap;
}

void GUI::update()
{
    json["rssi"] = RNGBridge::rssi;
    status.clear();
    serializeJson(json, status);
}
