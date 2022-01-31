#include "MQTT.h"

#include "Constants.h"
#include "GUI.h"

const char* Mqtt::statusFormat PROGMEM
    = R"({"device":"%s","b":{"charge":%hhu,"voltage":%.1f,"current":%.2f,"temperature":%hhu},"l":{"voltage":%.1f,"current":%.2f,"power":%hu},"p":{"voltage":%.1f,"current":%.2f,"power":%hu},"s":{"state":%hhu,"error":%u,"temperature":%hhu}})";
const char* Mqtt::lastWillFormat PROGMEM = R"({"device":"%s","connected":false})";
const char* Mqtt::connectionMsgFormat PROGMEM = R"({"device":"%s","connected":true})";

void Mqtt::connect()
{
    // Set last will and connect
    char will[44];
    snprintf_P(will, 44, lastWillFormat, deviceMAC);
    const bool connected = mqtt.connect(mqttConfig.id.c_str(), mqttConfig.user.c_str(), mqttConfig.password.c_str(),
        mqttConfig.topic.c_str(), 2, true, will);

    if (connected)
    {
        // Publish connected message
        char connectionMsq[43];
        snprintf_P(connectionMsq, 43, connectionMsgFormat, deviceMAC);
        publish(connectionMsq, 2, true);

        // Update status
        _status = CONNECTED;
        updateListener();
    }
    else
    {
        // Update status
        _status = F("Could not connect");
        updateListener();
    }
}

void Mqtt::disconnect()
{
    mqtt.disconnect();
    _status = DISCONNECTED;
    updateListener();
}

void Mqtt::loop()
{
    if (!mqtt.connected())
    {
        if (_status.startsWith("C"))
        {
            _status = DISCONNECTED;
            updateListener();
        }
        connect();
    }
    else
    {
        if (_status.startsWith("D"))
        {
            _status = CONNECTED;
            updateListener();
        }
    }
    mqtt.loop();
}

void Mqtt::updateRenogyStatus(const Renogy::Data& data)
{

    // Max size under assumptions Voltage < 1000, Current < 1000: 256 + null terminator + tolerance
    //{"device":"0123456789AB","b":{"charge":254,"voltage":999.9,"current":999.99,"temperature":254},"l":{"voltage":999.9,"current":999.99,"power":65534},"p":{"voltage":999.9,"current":999.99,"power":65534},"s":{"state":254,"error":4294967295,"temperature":254}}
    char jsonBuf[280];
    snprintf_P(jsonBuf, 280, statusFormat, deviceMAC, data.batteryCharge, data.batteryVoltage, data.batteryCurrent,
        data.batteryTemperature, data.loadVoltage, data.loadCurrent, data.loadPower, data.panelVoltage,
        data.panelCurrent, data.panelPower, data.chargingState, data.errorState, data.controllerTemperature);
    publish(jsonBuf);
}

void Mqtt::publish(const String& payload, uint8_t qos, bool retain)
{
    // At most once (0)
    // At least once (1)
    // Exactly once (2)
    if (mqtt.connected())
    {
        mqtt.publish(
            mqttConfig.topic.c_str(), reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length(), retain);
    }
}

void Mqtt::publish(const char* payload, uint8_t qos, bool retain)
{
    publish(mqttConfig.topic.c_str(), payload, qos, retain);
}

void Mqtt::publish(const char* topic, const char* payload, uint8_t qos, bool retain)
{
    // At most once (0)
    // At least once (1)
    // Exactly once (2)
    if (mqtt.connected())
    {
        mqtt.publish(topic, reinterpret_cast<const uint8_t*>(payload), strlen(payload), retain);
    }
}
