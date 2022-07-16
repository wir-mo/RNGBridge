#pragma once

#include <ArduinoJson.h>

#include "Networking.h"
#include "OutputControl.h"
#include "Renogy.h"

class GUI
{
public:
    GUI(Networking& networking) : _networking(networking) { }

    GUI(GUI&&) = delete;

    void updateRenogyStatus(const Renogy::Data& data);

    void updateMQTTStatus(const String& status);

    void updatePVOutputStatus(const String& status);

    void updateOutputStatus(const OutputControl::Status& status);

    void updateOtaStatus(const String& status);

    void updateUptime(const uint32_t uptime);

    void updateHeap(const uint32_t heap);

    void update();

private:
    Networking& _networking;
    StaticJsonDocument<1024> _status = {};
};
