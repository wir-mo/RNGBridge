#pragma once

#include <ArduinoJson.h>

#include "Constants.h"
#include "OutputControl.h"

class GUI
{
public:
    GUI() { }

    GUI(GUI&&) = delete;

    void updateMQTTStatus(const String& status) { json["mqttsta"] = status; }

    void updatePVOutputStatus(const String& status) { json["pvosta"] = status; }

    void updateOutputStatus(const OutputStatus& status)
    {
        auto output = json["o"];
        output["o1"] = status.out1;
        output["o2"] = status.out2;
        output["o3"] = status.out3;
    }

    void updateOtaStatus(const String& status) { json["otasta"] = status; }

    void updateUptime(const uint32_t uptime) { json["up"] = uptime; }

    void updateHeap(const uint32_t heap) { json["he"] = heap; }

    void update()
    {
        json["rssi"] = RSBridge::rssi;
        status.clear();
        serializeJson(json, status);
    }

public:
    static String status;
    JsonDocument json;
};
