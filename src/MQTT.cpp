#include "MQTT.h"

#include <ArduinoJson.h>

#include "Constants.h"
#include "GUI.h"

constexpr HaDevice device = HaDevice(nullptr, "wirmo", MODEL, SOFTWARE_VERSION);

void Mqtt::connect()
{
    // Set last will and connect
    const bool connected = mqtt.connect(mqttConfig.id.c_str(), mqttConfig.user.c_str(), mqttConfig.password.c_str(),
        (mqttConfig.topic + "/lwt").c_str(), 2, true, DISCONNECTED);

    HaDevice bla = device;
    bla.setUniqueId("");

    if (!connected)
    {
        // Update status
        notify(F("Could not connect"));
        return;
    }

    // Publish connected message
    publish((mqttConfig.topic + "/lwt").c_str(), CONNECTED, true);

    // Update status
    notify(FPSTR(CONNECTED));

    setupLoadControl();

    if (mqttConfig.hadiscovery)
    { }
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
        if (!_value.startsWith("D"))
        {
            notify(FPSTR(DISCONNECTED));
        }
        connect();
    }
    else
    {
        if (!_value.startsWith("C"))
        {
            notify(FPSTR(CONNECTED));
        }
    }
}

void Mqtt::publishData()
{
    const uint32_t timeS = millis() / 1000;
    if (timeS - lastUpdate >= mqttConfig.interval)
    {
        lastUpdate = timeS;

        const String topic = mqttConfig.topic + "/state";
        publishLarge(topic.c_str(), GUI::status.c_str(), true);
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
