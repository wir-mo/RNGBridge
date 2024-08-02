#pragma once

#include <ArduinoJson.h>

#include "OutputControl.h"
#include "Renogy.h"

class GUI
{
public:
    GUI() { }

    GUI(GUI&&) = delete;

    void updateRenogyStatus(const Renogy::Data& data);

    void updateMQTTStatus(const String& status);

    void updatePVOutputStatus(const String& status);

    void updateOutputStatus(const OutputStatus& status);

    void updateOtaStatus(const String& status);

    void updateUptime(const uint32_t uptime);

    void updateHeap(const uint32_t heap);

    void update();

public:
    static String status;

private:
    JsonDocument _status;
};
