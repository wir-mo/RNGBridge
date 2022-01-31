#include "Config.h"

constexpr int documentSizeConfig = 1024;

namespace
{
    /// @brief Helper to update a value from json and also check if it was changed
    /// @param object Json object, maybe null
    /// @param field Field name of the value
    /// @param val Reference to the variable that should be changed if possible
    /// @returns true if the value in object was valid and different from val
    template <typename T>
    bool updateField(const JsonObjectConst& object, const char* field, T& val)
    {
        T newValue = object[field] | val;
        if (newValue != val)
        {
            val = newValue;
            return true;
        }
        return false;
    }
    template <>
    bool updateField<String>(const JsonObjectConst& object, const char* field, String& val)
    {
        const char* newValue = object[field] | val.c_str();
        if (val != newValue)
        {
            val = newValue;
            return true;
        }
        return false;
    }
    template <>
    bool updateField<IPAddress>(const JsonObjectConst& object, const char* field, IPAddress& val)
    {
        const char* newValue = object[field] | "";
        IPAddress newIp;
        if (!newIp.fromString(newValue))
        {
            return false;
        }
        if (newIp != val)
        {
            val = newIp;
            return true;
        }
        return false;
    }
} // namespace

void Config::initConfig()
{
    if (SPIFFS.begin())
    {
        DEBUGLN(F("Mounted file system"));
        if (SPIFFS.exists("/config.json"))
        {
            readConfig();
        }
        else
        {
            DEBUGLN(F("Config file does not exist"));
            setDefaultConfig();
            saveConfig();
        }
    }
    else
    {
        DEBUGLN(F("Failed to mount FS"));
        Config::setDefaultConfig();
    }
}

NetworkConfig& Config::getNetworkConfig()
{
    return networkConfig;
}

MqttConfig& Config::getMqttConfig()
{
    return mqttConfig;
}

PVOutputConfig& Config::getPvoutputConfig()
{
    return pvoutputConfig;
}

void Config::setDefaultConfig()
{
    networkConfig.setDefaultConfig();
    mqttConfig.setDefaultConfig();
    pvoutputConfig.setDefaultConfig();
}

void Config::saveConfig()
{
    DEBUGLN(F("Writing config file"));
    File configFile = SPIFFS.open("/config.json", "w");

    StaticJsonDocument<documentSizeConfig> json;
    createJson(json);

    if (serializeJson(json, configFile) == 0)
    {
        DEBUGLN(F("Failed to write to file"));
    }
    else
    {
        DEBUGLN(F("Successfully updated config."));
    }
    configFile.close();
    if (configFile)
    {
        DEBUGLN(F("[Error] File was not closed."));
    }
}

void Config::createJson(JsonDocument& output)
{
    JsonObject wifi = output.createNestedObject("wifi");
    networkConfig.toJson(wifi);
    JsonObject mqtt = output.createNestedObject("mqtt");
    mqttConfig.toJson(mqtt);
    JsonObject pvo = output.createNestedObject("pvo");
    pvoutputConfig.toJson(pvo);
}

void Config::readConfig()
{
    DEBUGLN(F("Reading config file"));
    File configFile = SPIFFS.open("/config.json", "r");

    if (configFile)
    {
        DEBUGLN(F("Opened config file"));

        StaticJsonDocument<documentSizeConfig> json;

        DeserializationError error = deserializeJson(json, configFile);
        if (error)
        {
            DEBUGLN(F("Failed to read file, using default configuration"));
            setDefaultConfig();
            return;
        }
        else
        {
            DEBUGLN(F("Successfully loaded config file"));
        }
        configFile.close();
        const auto& jWifi = json["wifi"];
        const auto& jMqtt = json["mqtt"];
        const auto& jPvo = json["pvo"];
        if (networkConfig.verify(jWifi) && mqttConfig.verify(jMqtt) && pvoutputConfig.verify(jPvo))
        {
            networkConfig.fromJson(jWifi);
            mqttConfig.fromJson(jMqtt);
            pvoutputConfig.fromJson(jPvo);
        }
        else
        {
            DEBUGLN(F("Invalid file contents, removing config.json"));
            SPIFFS.remove("/config.json");
        }
    }
}

bool NetworkConfig::verify(const JsonObjectConst& object) const
{
    DEBUGLN(F("Verifying NetworkConfig"));
    if (!object["client_enabled"].is<bool>())
    {
        DEBUGLN(F("client_enabled not bool"));
        return false;
    }
    if (!object["client_dhcp_enabled"].is<bool>())
    {
        DEBUGLN(F("client_dhcp_enabled not bool"));
        return false;
    }
    if (!object["client_ssid"].is<const char*>())
    {
        DEBUGLN(F("client_ssid not const char*"));
        return false;
    }
    if (!object["client_password"].is<const char*>())
    {
        DEBUGLN(F("client_password not const char*"));
        return false;
    }
    if (!object["client_gateway"].is<const char*>())
    {
        DEBUGLN(F("client_gateway not const char*"));
        return false;
    }
    if (!object["client_dns"].is<const char*>())
    {
        DEBUGLN(F("client_dns not const char*"));
        return false;
    }
    if (!object["client_mask"].is<const char*>())
    {
        DEBUGLN(F("client_mask not const char*"));
        return false;
    }
    if (!object["ap_enabled"].is<bool>())
    {
        DEBUGLN(F("ap_enabled not bool"));
        return false;
    }
    if (!object["ap_ssid"].is<const char*>())
    {
        DEBUGLN(F("ap_ssid not const char*"));
        return false;
    }
    if (!object["ap_password"].is<const char*>())
    {
        DEBUGLN(F("ap_password not const char*"));
        return false;
    }
    return true;

    // return object["client_enabled"].is<bool>() && object["client_dhcp_enabled"].is<bool>()
    //     && object["client_ssid"].is<const char*>() && object["client_password"].is<const char*>()
    //     && object["client_gateway"].is<const char*>()
    //     && object["client_dns"].is<const char*>() && object["client_mask"].is<const char*>()
    //     && object["ap_enabled"].is<bool>() && object["ap_ssid"].is<const char*>()
    //     && object["ap_password"].is<const char*>();
}

void NetworkConfig::fromJson(const JsonObjectConst& object)
{
    constexpr const char* emptyString = "";
    clientEnabled = object["client_enabled"];
    dhcpEnabled = object["client_dhcp_enabled"];
    clientSsid = object["client_ssid"] | emptyString;
    clientPassword = object["client_password"] | emptyString;
    clientIp.fromString(object["client_ip"] | emptyString);
    clientGateway.fromString(object["client_gateway"] | emptyString);
    clientDns.fromString(object["client_dns"] | emptyString);
    clientMask.fromString(object["client_mask"] | emptyString);
    apEnabled = object["ap_enabled"];
    apSsid = object["ap_ssid"] | emptyString;
    apPassword = object["ap_password"] | emptyString;
}

void NetworkConfig::toJson(JsonObject& object) const
{
    object["client_enabled"].set<bool>(clientEnabled);
    object["client_dhcp_enabled"].set<bool>(dhcpEnabled);
    object["client_ssid"] = clientSsid;
    object["client_password"] = clientPassword;
    object["client_ip"] = clientIp.toString();
    object["client_gateway"] = clientGateway.toString();
    object["client_dns"] = clientDns.toString();
    object["client_mask"] = clientMask.toString();
    object["ap_enabled"].set<bool>(apEnabled);
    object["ap_ssid"] = apSsid;
    object["ap_password"] = apPassword;
}

bool NetworkConfig::tryUpdate(const JsonObjectConst& object)
{
    if (object.isNull())
    {
        return false;
    }
    bool changed = false;
    changed |= updateField(object, "client_enabled", clientEnabled);
    changed |= updateField(object, "client_dhcp_enabled", dhcpEnabled);
    changed |= updateField(object, "client_ssid", clientSsid);
    changed |= updateField(object, "client_password", clientPassword);
    changed |= updateField(object, "client_ip", clientIp);
    changed |= updateField(object, "client_gateway", clientGateway);
    changed |= updateField(object, "client_dns", clientDns);
    changed |= updateField(object, "client_mask", clientMask);
    changed |= updateField(object, "ap_enabled", apEnabled);
    changed |= updateField(object, "ap_ssid", apSsid);
    changed |= updateField(object, "ap_password", apPassword);
    return changed;
}

void NetworkConfig::setDefaultConfig()
{
    clientEnabled = false;
    clientSsid = "YourWifi";
    clientPassword = "YourPassword";

    char ssid[33] = {};
    sniprintf(ssid, sizeof(ssid), "%s %s", HOSTNAME, deviceMAC);
    apSsid = ssid;
    dhcpEnabled = true;
    apEnabled = true;
}

bool MqttConfig::verify(const JsonObjectConst& object) const
{
    DEBUGLN(F("Verifying MqttConfig"));
    if (!object["enabled"].is<bool>())
    {
        DEBUGLN(F("enabled not bool"));
        return false;
    }
    if (!object["server"].is<const char*>())
    {
        DEBUGLN(F("server not const char*"));
        return false;
    }
    if (!object["port"].is<unsigned int>())
    {
        DEBUGLN(F("port not unsigned int"));
        return false;
    }
    if (!object["id"].is<const char*>())
    {
        DEBUGLN(F("id not const char*"));
        return false;
    }
    if (!object["user"].is<const char*>())
    {
        DEBUGLN(F("user not const char*"));
        return false;
    }
    if (!object["password"].is<const char*>())
    {
        DEBUGLN(F("password not const char*"));
        return false;
    }
    if (!object["topic"].is<const char*>())
    {
        DEBUGLN(F("topic not const char*"));
        return false;
    }
    return true;

    // return object["enabled"].is<bool>() && object["server"].is<const char*>() && object["port"].is<unsigned int>()
    //     && object["id"].is<const char*>() && object["user"].is<const char*>() && object["password"].is<const char*>()
    //     && object["topic"].is<const char*>();
}

void MqttConfig::fromJson(const JsonObjectConst& object)
{
    constexpr const char* emptyString = "";
    enabled = object["enabled"];
    server = object["server"] | emptyString;
    port = object["port"];
    id = object["id"] | emptyString;
    user = object["user"] | emptyString;
    password = object["password"] | emptyString;
    topic = object["topic"] | emptyString;
}

void MqttConfig::toJson(JsonObject& object) const
{
    object["enabled"].set<bool>(enabled);
    object["server"] = server;
    object["port"] = port;
    object["id"] = id;
    object["user"] = user;
    object["password"] = password;
    object["topic"] = topic;
}

bool MqttConfig::tryUpdate(const JsonObjectConst& object)
{
    if (object.isNull())
    {
        return false;
    }
    bool changed = false;
    changed |= updateField(object, "enabled", enabled);
    changed |= updateField(object, "server", server);
    changed |= updateField(object, "port", port);
    changed |= updateField(object, "id", id);
    changed |= updateField(object, "user", user);
    changed |= updateField(object, "password", password);
    changed |= updateField(object, "topic", topic);
    return changed;
}

void MqttConfig::setDefaultConfig()
{
    enabled = false;
    port = 1883;
    id = "RNGBridge";
    topic = "/rng";
}

bool PVOutputConfig::verify(const JsonObjectConst& object) const
{
    DEBUGLN(F("Verifying PVOutputConfig"));
    if (!object["enabled"].is<bool>())
    {
        DEBUGLN(F("enabled not bool"));
        return false;
    }
    if (!object["system_id"].is<unsigned int>())
    {
        DEBUGLN(F("system_id not unsigned int"));
        return false;
    }
    if (!object["api_key"].is<const char*>())
    {
        DEBUGLN(F("api_key not const char*"));
        return false;
    }
    if (!object["time_offset"].is<int>())
    {
        DEBUGLN(F("time_offset not int"));
        return false;
    }
    return true;
    // return object["enabled"].is<bool>() && object["system_id"].is<unsigned int>() && object["api_key"].is<const
    // char*>()
    //     && object["time_offset"].is<int>();
}

void PVOutputConfig::fromJson(const JsonObjectConst& object)
{
    constexpr const char* emptyString = "";
    enabled = object["enabled"];
    systemId = object["system_id"];
    apiKey = object["api_key"] | emptyString;
    timeOffset = object["time_offset"];
}

void PVOutputConfig::toJson(JsonObject& object) const
{
    object["enabled"].set<bool>(enabled);
    object["system_id"] = systemId;
    object["api_key"] = apiKey;
    object["time_offset"] = timeOffset;
}

bool PVOutputConfig::tryUpdate(const JsonObjectConst& object)
{
    if (object.isNull())
    {
        return false;
    }
    bool changed = false;
    changed |= updateField(object, "enabled", enabled);
    changed |= updateField(object, "system_id", systemId);
    changed |= updateField(object, "api_key", apiKey);
    changed |= updateField(object, "time_offset", timeOffset);
    return changed;
}

void PVOutputConfig::setDefaultConfig()
{
    enabled = false;
    systemId = 0;
    apiKey = "YourAPIKey";
    timeOffset = 0;
}
