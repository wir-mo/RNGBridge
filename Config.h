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

struct DeviceConfig
{
    String name;

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
