#pragma once

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include "Constants.h"

struct NetworkConfig
{
    bool clientEnabled;
    String clientSsid;
    String clientPassword;
    bool dhcpEnabled;
    IPAddress clientMask;
    IPAddress clientGateway;
    IPAddress clientDns;
    IPAddress clientIp;
    bool apEnabled;
    String apSsid;
    String apPassword;

    /// @brief Verify that the object can be parsed
    /// @returns true if fromJson can be executed
    bool verify(const JsonObjectConst& object) const;
    void fromJson(const JsonObjectConst& object);
    void toJson(JsonObject& object) const;

    /// @brief Update all fields in object, if possible
    /// @returns true when any value was changed
    bool tryUpdate(const JsonObjectConst& object);

    ///@brief Set the contained data to the default
    void setDefaultConfig();
};

struct MqttConfig
{
    uint16_t port;
    uint8_t interval; /// Interval in seconds at which mqtt messages are published
    bool enabled;
    bool hadiscovery; /// Should one or more discovery messages be sent to haDiscoveryTopic
    bool split; /// Should data be split into separate topics
    String server;
    String id;
    String user;
    String password;
    String topic;
    String haDiscoveryTopic; /// Topic of homeassistant for discovery

    /// @brief Verify that the object can be parsed
    /// @returns true if fromJson can be executed
    bool verify(const JsonObjectConst& object) const;
    void fromJson(const JsonObjectConst& object);
    void toJson(JsonObject& object) const;

    /// @brief Update all fields in object, if possible
    /// @returns true when any value was changed
    bool tryUpdate(const JsonObjectConst& object);

    ///@brief Set the contained data to the default
    void setDefaultConfig();
};

struct PVOutputConfig
{
    bool enabled;
    uint32_t systemId;
    String apiKey;
    int8_t timeOffset;

    /// @brief Verify that the object can be parsed
    /// @returns true if fromJson can be executed
    bool verify(const JsonObjectConst& object) const;
    void fromJson(const JsonObjectConst& object);
    void toJson(JsonObject& object) const;

    /// @brief Update all fields in object, if possible
    /// @returns true when any value was changed
    bool tryUpdate(const JsonObjectConst& object);

    ///@brief Set the contained data to the default
    void setDefaultConfig();
};

enum class InputType
{
    disabled,
    bsoc,
    bvoltage,
    pvoltage,
    pcurrent,

};

static const String InputTypeToString(const InputType type)
{
    switch (type)
    {
    case InputType::bsoc:
        return "bsoc";
    case InputType::bvoltage:
        return "bvoltage";
    case InputType::pvoltage:
        return "pvoltage";
    case InputType::pcurrent:
        return "pcurrent";
    case InputType::disabled:
    default:
        return "disabled";
    }
}

static InputType StringToInputType(const String& str)
{
    if (str.equals("bsoc"))
    {
        return InputType::bsoc;
    }
    if (str.equals("bvoltage"))
    {
        return InputType::bvoltage;
    }
    if (str.equals("pvoltage"))
    {
        return InputType::pvoltage;
    }
    if (str.equals("pcurrent"))
    {
        return InputType::pcurrent;
    }
    return InputType::disabled;
}

struct OutputConfig
{
    InputType inputType;
    bool inverted;
    float min;
    float max;
    bool lastState = false;

    /// @brief Verify that the object can be parsed
    /// @returns true if fromJson can be executed
    bool verify(const JsonObjectConst& object) const;
    void fromJson(const JsonObjectConst& object);
    template <typename T>
    void toJson(T&& object) const
    {
        object["inputType"] = InputTypeToString(inputType);
        object["inverted"] = inverted;
        object["min"] = min;
        object["max"] = max;
    }

    /// @brief Update all fields in object, if possible
    /// @returns true when any value was changed
    bool tryUpdate(const JsonObjectConst& object);

    ///@brief Set the contained data to the default
    void setDefaultConfig();
};

struct DeviceConfig
{
    uint8_t address; /// Address of the modbus client
    String name;
    OutputConfig load;
    OutputConfig out1;
    OutputConfig out2;
    OutputConfig out3;

    /// @brief Verify that the object can be parsed
    /// @returns true if fromJson can be executed
    bool verify(const JsonObjectConst& object) const;
    void fromJson(const JsonObjectConst& object);
    void toJson(JsonObject& object) const;

    /// @brief Update all fields in object, if possible
    /// @returns true when any value was changed
    bool tryUpdate(const JsonObjectConst& object);

    ///@brief Set the contained data to the default
    void setDefaultConfig();
};

class Config
{
public:
    void initConfig();
    NetworkConfig& getNetworkConfig();
    MqttConfig& getMqttConfig();
    PVOutputConfig& getPvoutputConfig();
    DeviceConfig& getDeviceConfig();
    void setDefaultConfig();
    void saveConfig();
    void createJson(JsonDocument& output);

private:
    void readConfig();

private:
    NetworkConfig networkConfig;
    MqttConfig mqttConfig;
    PVOutputConfig pvoutputConfig;
    DeviceConfig deviceConfig;
}; // namespace Networking
