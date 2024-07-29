#include "MQTT.h"

#include <ArduinoJson.h>

#include "Constants.h"
#include "GUI.h"

void Mqtt::connect()
{
    // Set last will and connect
    const bool connected = mqtt.connect(mqttConfig.id.c_str(), mqttConfig.user.c_str(), mqttConfig.password.c_str(),
        (mqttConfig.topic + "/lwt").c_str(), 2, true, DISCONNECTED);

    if (connected)
    {
        // Publish connected message
        publish((mqttConfig.topic + "/lwt").c_str(), CONNECTED, true);

        // Update status
        notify(FPSTR(CONNECTED));

        setupLoadControl();

        if (mqttConfig.hadiscovery)
        {
            // Battery related
            publishSensorDiscovery("Cell 1 Voltage", "c1v", "voltage", "V", "measurement", "{{value_json.c1.vo}}");
            publishSensorDiscovery(
                "Cell 1 Temperature", "c1t", "temperature", "°C", "measurement", "{{value_json.c1.te}}");
            publishSensorDiscovery("Cell 2 Voltage", "c2v", "voltage", "V", "measurement", "{{value_json.c2.vo}}");
            publishSensorDiscovery(
                "Cell 2 Temperature", "c2t", "temperature", "°C", "measurement", "{{value_json.c2.te}}");
            publishSensorDiscovery("Cell 3 Voltage", "c3v", "voltage", "V", "measurement", "{{value_json.c3.vo}}");
            publishSensorDiscovery(
                "Cell 3 Temperature", "c3t", "temperature", "°C", "measurement", "{{value_json.c3.te}}");
            publishSensorDiscovery("Cell 4 Voltage", "c4v", "voltage", "V", "measurement", "{{value_json.c4.vo}}");
            publishSensorDiscovery(
                "Cell 4 Temperature", "c4t", "temperature", "°C", "measurement", "{{value_json.c4.te}}");
            publishSensorDiscovery("Cell 5 Voltage", "c5v", "voltage", "V", "measurement", "{{value_json.c5.vo}}");
            publishSensorDiscovery(
                "Cell 5 Temperature", "c5t", "temperature", "°C", "measurement", "{{value_json.c5.te}}");
            publishSensorDiscovery("Cell 6 Voltage", "c6v", "voltage", "V", "measurement", "{{value_json.c6.vo}}");
            publishSensorDiscovery(
                "Cell 6 Temperature", "c6t", "temperature", "°C", "measurement", "{{value_json.c6.te}}");
            publishSensorDiscovery("Cell 7 Voltage", "c7v", "voltage", "V", "measurement", "{{value_json.c7.vo}}");
            publishSensorDiscovery(
                "Cell 7 Temperature", "c7t", "temperature", "°C", "measurement", "{{value_json.c7.te}}");
            publishSensorDiscovery("Cell 8 Voltage", "c8v", "voltage", "V", "measurement", "{{value_json.c8.vo}}");
            publishSensorDiscovery(
                "Cell 8 Temperature", "c8t", "temperature", "°C", "measurement", "{{value_json.c8.te}}");
            publishSensorDiscovery("Cell 9 Voltage", "c9v", "voltage", "V", "measurement", "{{value_json.c9.vo}}");
            publishSensorDiscovery(
                "Cell 9 Temperature", "c9t", "temperature", "°C", "measurement", "{{value_json.c9.te}}");
            publishSensorDiscovery("Cell 10 Voltage", "c10v", "voltage", "V", "measurement", "{{value_json.c10.vo}}");
            publishSensorDiscovery(
                "Cell 10 Temperature", "c10t", "temperature", "°C", "measurement", "{{value_json.c10.te}}");
            publishSensorDiscovery("Cell 11 Voltage", "c11v", "voltage", "V", "measurement", "{{value_json.c11.vo}}");
            publishSensorDiscovery(
                "Cell 11 Temperature", "c11t", "temperature", "°C", "measurement", "{{value_json.c11.te}}");
            publishSensorDiscovery("Cell 12 Voltage", "c12v", "voltage", "V", "measurement", "{{value_json.c12.vo}}");
            publishSensorDiscovery(
                "Cell 12 Temperature", "c12t", "temperature", "°C", "measurement", "{{value_json.c12.te}}");
            publishSensorDiscovery("Cell 13 Voltage", "c13v", "voltage", "V", "measurement", "{{value_json.c13.vo}}");
            publishSensorDiscovery(
                "Cell 13 Temperature", "c13t", "temperature", "°C", "measurement", "{{value_json.c13.te}}");
            publishSensorDiscovery("Cell 14 Voltage", "c14v", "voltage", "V", "measurement", "{{value_json.c14.vo}}");
            publishSensorDiscovery(
                "Cell 14 Temperature", "c14t", "temperature", "°C", "measurement", "{{value_json.c14.te}}");
            publishSensorDiscovery("Cell 15 Voltage", "c15v", "voltage", "V", "measurement", "{{value_json.c15.vo}}");
            publishSensorDiscovery(
                "Cell 15 Temperature", "c15t", "temperature", "°C", "measurement", "{{value_json.c15.te}}");
            publishSensorDiscovery("Cell 16 Voltage", "c16v", "voltage", "V", "measurement", "{{value_json.c16.vo}}");
            publishSensorDiscovery(
                "Cell 16 Temperature", "c16t", "temperature", "°C", "measurement", "{{value_json.c16.te}}");

            publishSensorDiscovery(
                "Ambient Temperature 1", "a1te", "temperature", "°C", "measurement", "{{value_json.a1te}}");
            publishSensorDiscovery(
                "Ambient Temperature 2", "a2te", "temperature", "°C", "measurement", "{{value_json.a2te}}");

            publishSensorDiscovery(
                "Heater 1 Temperature", "h1te", "temperature", "°C", "measurement", "{{value_json.h1te}}");
            publishSensorDiscovery(
                "Heater 2 Temperature", "h2te", "temperature", "°C", "measurement", "{{value_json.h2te}}");

            publishSensorDiscovery(
                "BMS Temperature", "bmste", "temperature", "°C", "measurement", "{{value_json.bmste}}");

            publishSensorDiscovery("Cycles", "cy", "none", "cycles", "measurement", "{{value_json.cy}}");

            publishSensorDiscovery("Current", "cu", "current", "A", "measurement", "{{value_json.cu}}");
            publishSensorDiscovery("Voltage", "vo", "voltage", "V", "measurement", "{{value_json.vo}}");

            publishSensorDiscovery("Remaining", "rem", "energy", "Wh", "measurement", "{{value_json.rem}}");
            publishSensorDiscovery("Total", "tot", "energy", "Wh", "measurement", "{{value_json.tot}}");

            publishSensorDiscovery(
                "Charge Voltage Limit", "chlimvo", "voltage", "V", "measurement", "{{value_json.chlim.vo}}");
            publishSensorDiscovery(
                "Charge Current Limit", "chlimcu", "current", "A", "measurement", "{{value_json.chlim.cu}}");

            publishSensorDiscovery(
                "Discharge Voltage Limit", "dchlimvo", "voltage", "V", "measurement", "{{value_json.dchlim.vo}}");
            publishSensorDiscovery(
                "Discharge Current Limit", "dchlimcu", "current", "A", "measurement", "{{value_json.dchlim.cu}}");

            // Telemetry
            publishSensorDiscovery("RSSI", "rssi", "signal_strength", "dBm", "measurement", "{{value_json.rssi}}");

            // Output
            // TODO command_topic
            publishSwitchDiscovery(
                "Out 1", "o1", "{{'true' if value_json.o.o1 else 'false'}}", "mdi:numeric-1-box-outline");
            publishSwitchDiscovery(
                "Out 2", "o2", "{{'true' if value_json.o.o2 else 'false'}}", "mdi:numeric-2-box-outline");
            publishSwitchDiscovery(
                "Out 3", "o3", "{{'true' if value_json.o.o3 else 'false'}}", "mdi:numeric-3-box-outline");
        }
    }
    else
    {
        // Update status
        notify(F("Could not connect"));
    }
}

void Mqtt::disconnect()
{
    mqtt.disconnect();
    notify(FPSTR(DISCONNECTED));
}

void Mqtt::loop()
{
    mqtt.loop();
    if (!mqtt.connected())
    {
        if (_value.startsWith("C"))
        {
            notify(FPSTR(DISCONNECTED));
        }
        connect();
    }
    else
    {
        if (_value.startsWith("D"))
        {
            notify(FPSTR(CONNECTED));
        }
    }
}

void Mqtt::updateRenogyStatus(const Renogy::Data& data)
{
    const uint32_t timeS = millis() / 1000;
    if (timeS - lastUpdate >= mqttConfig.interval)
    {
        lastUpdate = timeS;

        const String topic = mqttConfig.topic + "/state";
        publishLarge(topic.c_str(), GUI::status.c_str(), true);

        if (mqttConfig.split)
        {
            publish((mqttConfig.topic + "/cell/1/v").c_str(), String(data.cellVoltage[0]).c_str(), false);
            publish((mqttConfig.topic + "/cell/1/t").c_str(), String(data.cellTemperature[0]).c_str(), false);
            publish((mqttConfig.topic + "/cell/2/v").c_str(), String(data.cellVoltage[1]).c_str(), false);
            publish((mqttConfig.topic + "/cell/2/t").c_str(), String(data.cellTemperature[1]).c_str(), false);
            publish((mqttConfig.topic + "/cell/3/v").c_str(), String(data.cellVoltage[2]).c_str(), false);
            publish((mqttConfig.topic + "/cell/3/t").c_str(), String(data.cellTemperature[2]).c_str(), false);
            publish((mqttConfig.topic + "/cell/4/v").c_str(), String(data.cellVoltage[3]).c_str(), false);
            publish((mqttConfig.topic + "/cell/4/t").c_str(), String(data.cellTemperature[3]).c_str(), false);
            publish((mqttConfig.topic + "/cell/5/v").c_str(), String(data.cellVoltage[4]).c_str(), false);
            publish((mqttConfig.topic + "/cell/5/t").c_str(), String(data.cellTemperature[4]).c_str(), false);
            publish((mqttConfig.topic + "/cell/6/v").c_str(), String(data.cellVoltage[5]).c_str(), false);
            publish((mqttConfig.topic + "/cell/6/t").c_str(), String(data.cellTemperature[5]).c_str(), false);
            publish((mqttConfig.topic + "/cell/7/v").c_str(), String(data.cellVoltage[6]).c_str(), false);
            publish((mqttConfig.topic + "/cell/7/t").c_str(), String(data.cellTemperature[6]).c_str(), false);
            publish((mqttConfig.topic + "/cell/8/v").c_str(), String(data.cellVoltage[7]).c_str(), false);
            publish((mqttConfig.topic + "/cell/8/t").c_str(), String(data.cellTemperature[7]).c_str(), false);
            publish((mqttConfig.topic + "/cell/9/v").c_str(), String(data.cellVoltage[8]).c_str(), false);
            publish((mqttConfig.topic + "/cell/9/t").c_str(), String(data.cellTemperature[8]).c_str(), false);
            publish((mqttConfig.topic + "/cell/10/v").c_str(), String(data.cellVoltage[9]).c_str(), false);
            publish((mqttConfig.topic + "/cell/10/t").c_str(), String(data.cellTemperature[9]).c_str(), false);
            publish((mqttConfig.topic + "/cell/11/v").c_str(), String(data.cellVoltage[10]).c_str(), false);
            publish((mqttConfig.topic + "/cell/11/t").c_str(), String(data.cellTemperature[10]).c_str(), false);
            publish((mqttConfig.topic + "/cell/12/v").c_str(), String(data.cellVoltage[11]).c_str(), false);
            publish((mqttConfig.topic + "/cell/12/t").c_str(), String(data.cellTemperature[11]).c_str(), false);
            publish((mqttConfig.topic + "/cell/13/v").c_str(), String(data.cellVoltage[12]).c_str(), false);
            publish((mqttConfig.topic + "/cell/13/t").c_str(), String(data.cellTemperature[12]).c_str(), false);
            publish((mqttConfig.topic + "/cell/14/v").c_str(), String(data.cellVoltage[13]).c_str(), false);
            publish((mqttConfig.topic + "/cell/14/t").c_str(), String(data.cellTemperature[13]).c_str(), false);
            publish((mqttConfig.topic + "/cell/15/v").c_str(), String(data.cellVoltage[14]).c_str(), false);
            publish((mqttConfig.topic + "/cell/15/t").c_str(), String(data.cellTemperature[14]).c_str(), false);
            publish((mqttConfig.topic + "/cell/16/v").c_str(), String(data.cellVoltage[15]).c_str(), false);
            publish((mqttConfig.topic + "/cell/16/t").c_str(), String(data.cellTemperature[15]).c_str(), false);
        }
    }
}

void Mqtt::setupLoadControl()
{
    subscribe(mqttConfig.topic + "/ol");
    subscribe(mqttConfig.topic + "/o1");
    subscribe(mqttConfig.topic + "/o2");
    subscribe(mqttConfig.topic + "/o3");

    mqtt.setCallback([&](char* topic, uint8_t* data, unsigned int size) {
        data[size] = '\0';

        const bool enable = strstr((char*)(data), "true") != nullptr;
        if (strstr(topic, "ol"))
        {
            // Serial.println("Control load");
            outputs.enableLoad(enable);
        }
        else if (strstr(topic, "o1"))
        {
            // Serial.println("Control out1");
            outputs.enableOut1(enable);
        }
        else if (strstr(topic, "o2"))
        {
            // Serial.println("Control out2");
            outputs.enableOut2(enable);
        }
        else if (strstr(topic, "o3"))
        {
            // Serial.println("Control out3");
            outputs.enableOut3(enable);
        }
    });
}

void Mqtt::addDeviceInfo(JsonDocument& json, const String& deviceID)
{
    auto dev = json["dev"];
    dev["mf"] = "enwi";
    dev["mdl"] = String(MODEL) + " " + HARDWARE_VERSION;
    dev["name"] = deviceID;
    dev["sw"] = SOFTWARE_VERSION;
    dev["cu"] = String("http://") + WiFi.localIP().toString();

    dev["ids"].to<JsonArray>().add(deviceID);
}

void Mqtt::publishSensorDiscovery(const String& name, const String& id, const String& valueTemplate, const String& icon)
{
    JsonDocument autoConfig;

    const String deviceID = getDeviceID();
    addDeviceInfo(autoConfig, deviceID);

    autoConfig["name"] = deviceID + " " + name;
    autoConfig["uniq_id"] = deviceID + "_" + id;
    autoConfig["~"] = mqttConfig.topic;
    autoConfig["avty_t"] = "~/lwt";
    autoConfig["pl_avail"] = CONNECTED;
    autoConfig["pl_not_avail"] = DISCONNECTED;
    autoConfig["stat_t"] = "~/state";
    autoConfig["val_tpl"] = valueTemplate;
    if (!icon.isEmpty())
    {
        autoConfig["ic"] = icon;
    }

    const String topic = "homeassistant/sensor/" + deviceID + "/" + id + "/config";
    publishJSON(topic, autoConfig, true);
}

void Mqtt::publishSensorDiscovery(const String& name, const String& id, const String& deviceClass, const String& unit,
    const String& stateClass, const String& valueTemplate, const String& icon)
{
    JsonDocument autoConfig;

    const String deviceID = getDeviceID();
    addDeviceInfo(autoConfig, deviceID);

    autoConfig["dev_cla"] = deviceClass;
    autoConfig["unit_of_meas"] = unit;
    autoConfig["stat_cla"] = stateClass;

    autoConfig["name"] = deviceID + " " + name;
    autoConfig["uniq_id"] = deviceID + "_" + id;
    autoConfig["~"] = mqttConfig.topic;
    autoConfig["avty_t"] = "~/lwt";
    autoConfig["pl_avail"] = CONNECTED;
    autoConfig["pl_not_avail"] = DISCONNECTED;
    autoConfig["stat_t"] = "~/state";
    autoConfig["val_tpl"] = valueTemplate;
    if (!icon.isEmpty())
    {
        autoConfig["ic"] = icon;
    }

    const String topic = "homeassistant/sensor/" + deviceID + "/" + id + "/config";
    publishJSON(topic, autoConfig, true);
}

void Mqtt::publishSwitchDiscovery(const String& name, const String& id, const String& valueTemplate, const String& icon)
{
    JsonDocument autoConfig;

    const String deviceID = getDeviceID();
    addDeviceInfo(autoConfig, deviceID);

    autoConfig["name"] = deviceID + " " + name;
    autoConfig["uniq_id"] = deviceID + "_" + id;
    autoConfig["~"] = mqttConfig.topic;
    autoConfig["avty_t"] = "~/lwt";
    autoConfig["pl_avail"] = CONNECTED;
    autoConfig["pl_not_avail"] = DISCONNECTED;
    autoConfig["stat_t"] = "~/state";
    autoConfig["val_tpl"] = valueTemplate;

    autoConfig["cmd_t"] = "~/" + id;
    autoConfig["payload_off"] = "false";
    autoConfig["payload_on"] = "true";
    if (!icon.isEmpty())
    {
        autoConfig["ic"] = icon;
    }

    const String topic = "homeassistant/switch/" + deviceID + "/" + id + "/config";
    publishJSON(topic, autoConfig, true);
}

void Mqtt::subscribe(const String& topic)
{
    const bool subscribed = mqtt.subscribe(topic.c_str());
    RNG_DEBUGF("Subscribed %s %s\n", topic.c_str(), subscribed ? "successfully" : "unsuccessfully");
}

bool Mqtt::publishJSON(const String& topic, const JsonDocument& json, const bool retain)
{
    const size_t size = measureJson(json);
    if (mqtt.beginPublish(topic.c_str(), size, true))
    {
        serializeJson(json, mqtt);
        return mqtt.endPublish();
    }
    return false;
}

bool Mqtt::publish(const String& payload, bool retain)
{
    return mqtt.publish(
        mqttConfig.topic.c_str(), reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length(), retain);
}

bool Mqtt::publish(const char* payload, bool retain)
{
    return publish(mqttConfig.topic.c_str(), payload, retain);
}

bool Mqtt::publish(const char* topic, const char* payload, bool retain)
{
    return mqtt.publish(topic, reinterpret_cast<const uint8_t*>(payload), strlen(payload), retain);
}
