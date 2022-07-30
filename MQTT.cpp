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
        setupLoadControl();

        // if (mqttConfig.hadiscovery)
        {
            // update HA make it discover this device

            // Battery related
            publishSensorDiscovery("Battery SOC", "batsoc", "battery", "%", "measurement", "{{value_json.b.ch}}");
            publishSensorDiscovery(
                "Battery Voltage", "batvol", "voltage", "V", "measurement", "{{value_json.b.vo}}", "mdi:battery");
            publishSensorDiscovery(
                "Battery Current", "batcur", "current", "A", "measurement", "{{value_json.b.cu}}", "mdi:battery");
            publishSensorDiscovery("Battery Temperature", "battem", "temperature", "°C", "measurement",
                "{{value_json.b.te}}", "mdi:battery");

            // Load related
            publishSensorDiscovery("Load Voltage", "loavol", "voltage", "V", "measurement", "{{value_json.l.vo}}",
                "mdi:alpha-l-box-outline");
            publishSensorDiscovery("Load Current", "loacur", "current", "A", "measurement", "{{value_json.l.cu}}",
                "mdi:alpha-l-box-outline");
            publishSensorDiscovery("Load Power", "loapow", "power", "W", "measurement",
                "{{(value_json.l.vo*value_json.l.cu)|round(1)}}", "mdi:alpha-l-box-outline");

            // Panel related
            publishSensorDiscovery(
                "Panel Voltage", "panvol", "voltage", "V", "measurement", "{{value_json.p.vo}}", "mdi:solar-panel");
            publishSensorDiscovery(
                "Panel Current", "pancur", "current", "A", "measurement", "{{value_json.p.cu}}", "mdi:solar-panel");
            publishSensorDiscovery("Panel Power", "panpow", "power", "W", "measurement",
                "{{(value_json.p.vo*value_json.p.cu)|round(1)}}", "mdi:solar-panel");

            // Controller related
            publishSensorDiscovery("Controller State", "consta",
                "{{['Unknown',"
                "'Deactivated',"
                "'Activated',"
                "'MPPT',"
                "'Equalizing',"
                "'Boost',"
                "'Floating',"
                "'Overpower'][value_json.c.st|int(-1)+1]}}",
                "mdi:server");
            publishSensorDiscovery("Controller Error", "conerr", "{{value_json.c.er}}", "mdi:server");
            publishSensorDiscovery("Controller Temperature", "contem", "temperature", "°C", "measurement",
                "{{value_json.c.te}}", "mdi:server");

            // Output (incl. load)
            // TODO command_topic
            publishSwitchDiscovery(
                "Load", "ol", "{{'true' if value_json.o.l else 'false'}}", "mdi:alpha-l-box-outline");
            publishSwitchDiscovery(
                "Out 1", "o1", "{{'true' if value_json.o.o1 else 'false'}}", "mdi:numeric-1-box-outline");
            publishSwitchDiscovery(
                "Out 2", "o2", "{{'true' if value_json.o.o2 else 'false'}}", "mdi:numeric-2-box-outline");
            publishSwitchDiscovery(
                "Out 3", "o3", "{{'true' if value_json.o.o3 else 'false'}}", "mdi:numeric-3-box-outline");
        }

        // Publish connected message
        publish((mqttConfig.topic + "/lwt").c_str(), CONNECTED, true);

        // Update status
        updateStatus(FPSTR(CONNECTED));
    }
    else
    {
        // Update status
        updateStatus(F("Could not connect"));
    }
}

void Mqtt::disconnect()
{
    mqtt.disconnect();
    updateStatus(FPSTR(DISCONNECTED));
}

void Mqtt::loop()
{
    if (!mqtt.connected())
    {
        if (_status.startsWith("C"))
        {
            updateStatus(FPSTR(DISCONNECTED));
        }
        connect();
    }
    else
    {
        if (_status.startsWith("D"))
        {
            updateStatus(FPSTR(CONNECTED));
        }
    }
    mqtt.loop();
}

void Mqtt::updateRenogyStatus(const Renogy::Data& data)
{
    ++updateCount;
    if (updateCount >= mqttConfig.interval)
    {
        updateCount = 0;

        StaticJsonDocument<512> json;

        auto battery = json["b"];
        battery["ch"] = data.batteryCharge;
        battery["vo"] = data.batteryVoltage;
        battery["cu"] = data.batteryCurrent;
        battery["te"] = data.batteryTemperature;

        auto load = json["l"];
        load["vo"] = data.loadVoltage;
        load["cu"] = data.loadCurrent;
        load["po"] = data.loadPower;

        auto panel = json["p"];
        panel["vo"] = data.panelVoltage;
        panel["cu"] = data.panelCurrent;
        panel["po"] = data.panelPower;

        auto controller = json["c"];
        controller["st"] = data.chargingState;
        controller["er"] = data.errorState;
        controller["te"] = data.controllerTemperature;

        auto output = json["o"];
        output["l"] = data.loadEnabled;
        output["o1"] = outputStatus.out1;
        output["o2"] = outputStatus.out2;
        output["o3"] = outputStatus.out3;

        const String topic = mqttConfig.topic + "/state";
        publishJSON(topic, json, true);
    }
}

void Mqtt::setListener(Listener listener)
{
    _listener = listener;
    if (_listener)
    {
        _listener(_status);
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

        Serial.print("Received: ");
        Serial.println(topic);
        Serial.println((char*)(data));

        const bool enable = strstr((char*)(data), "true") != nullptr;
        if (strstr(topic, "ol"))
        {
            Serial.println("Control load");
            outputs.enableLoad(enable);
        }
        else if (strstr(topic, "o1"))
        {
            Serial.println("Control out1");
            outputs.enableOut1(enable);
        }
        else if (strstr(topic, "o2"))
        {
            Serial.println("Control out2");
            outputs.enableOut2(enable);
        }
        else if (strstr(topic, "o3"))
        {
            Serial.println("Control out3");
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

    dev.createNestedArray("ids").add(deviceID);
}

void Mqtt::publishSensorDiscovery(const String& name, const String& id, const String& valueTemplate, const String& icon)
{
    StaticJsonDocument<512> autoConfig;

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
    StaticJsonDocument<512> autoConfig;

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
    StaticJsonDocument<512> autoConfig;

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
    DEBUGF("Subscribed %s %s", topic.c_str(), subscribed ? "successfully" : "unsuccessfully");
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

void Mqtt::updateStatus(const String& status)
{
    _status = status;
    if (_listener)
    {
        _listener(_status);
    }
}
