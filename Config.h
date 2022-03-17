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
    bool enabled;
    String server;
    unsigned int port;
    String id;
    String user;
    String password;
    String topic;

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
};

static const String InputTypeToString(const InputType type)
{
    switch (type)
    {
    case InputType::disabled:
        return "disabled";
    case InputType::bsoc:
        return "bsoc";
    case InputType::bvoltage:
        return "bvoltage";
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
    return InputType::disabled;
}

struct OutputControl
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
    String name;
    OutputControl load;
    OutputControl out1;
    OutputControl out2;
    OutputControl out3;

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
