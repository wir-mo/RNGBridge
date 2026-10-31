#pragma once

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

#include "Config.h"
#include "Constants.h"
#include "OutputControl.h"

#if defined(ESP32)
#include <Update.h>
#else
#include <ESP8266WiFi.h>
#include <Updater.h>
#include <include/WiFiState.h>
#endif

class Networking
{
public:
    typedef std::function<void()> RebootHandler;

public:
    ///@brief Static class has no constructor
    // Networking(Config& config) : config(config), mqtt(config) { }
    Networking(Config& config) : config(config)
    {
        // Needed for EventSource testing
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    }

    Networking(Networking&&) = delete;

    void initWifi();

    void init(OutputControl& outputs);

    void getStatusJsonString(JsonObject& output);

    ///@brief Handle the upload of binary program
    ///
    ///@param request Request coming from webserver
    ///@param filename Name of the uploading/uploaded file
    ///@param index Index of the raw @ref data within the whole 'file'
    ///@param data Raw data chunk
    ///@param len Size of the raw @ref data chunk
    void handleOTAUpload(
        AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final);

    ///@brief Handle the index page
    ///@param request Request coming from webserver
    void handleIndex(AsyncWebServerRequest* request);

    ///@brief Handle the config GET api
    ///
    ///@param request Request coming from webserver
    void handleConfigApiGet(AsyncWebServerRequest* request);

    ///@brief Handle the config POST api
    ///
    ///@param request Request coming from webserver
    ///@param json JSON object containing configuration of wifi, mqtt, etc.
    void handleConfigApiPost(AsyncWebServerRequest* request, JsonVariant& json);

    ///@brief Handle the renogy
    ///
    ///@param request Request coming from webserver
    ///@param json JSON object containing values of brightness, speed, effect, etc.
    ///@param outputs Control for all outputs including renogy
    void handleRenogyApi(AsyncWebServerRequest* request, JsonVariant& json, OutputControl& outputs);

    ///@brief Handle the status GET api
    ///
    ///@param request Request coming from webserver
    void handleStateApiGet(AsyncWebServerRequest* request);

    /// @brief Check if the given string is an ip address
    ///
    /// @param str String to check
    /// @return true If the string is an ip
    /// @return false If the string is not an ip
    bool isIp(const String& str);

    ///@brief Update DNS and other networking stuff
    ///
    /// Should be called once every second
    void update();

    ///@brief Set a handler for rebooting the ESP upon being called
    ///
    ///@param handler RebootHandler
    void setRebootHandler(RebootHandler handler);

private:
    /// @brief Callback used for captive portal webserver
    ///
    /// @param request Request to check and handle captive portal for
    /// @return true If not handling captive portal
    /// @return false If handling captive portal
    bool captivePortal(AsyncWebServerRequest* request);

    /// @brief Launch access point if client connection fails
    /// @return true If connected as client
    /// @return false If connection failed and access point was opened
    bool handleClientFailsafe();

    /// @brief Configure wifi for client mode
    void startClient();
    /// @brief Configure wifi for access point mode
    ///
    /// @param persistent Store the mode in flash
    /// @param keepClient Keep the station interface enabled (AP+STA) so the client can still connect later
    void startAccessPoint(bool persistent = true, bool keepClient = false);

    /// @brief Monitor the client connection, retry connecting and close the fallback AP once connected
    ///
    /// Should be called once every second
    void updateClient();

    /// @brief Initialize server and rest endpoints
    ///
    /// @param outputs Output control
    void initServer(OutputControl& outputs);

private:
    const IPAddress AP_IP = {192, 168, 4, 1};
    const IPAddress AP_NETMASK = {255, 255, 255, 0};
    DNSServer dnsServer; // DNS server for captive portal
    AsyncWebServer server {80}; /// Webserver for OTA
    AsyncEventSource es {"/events"}; /// EventSource for updating clients with live data
    bool isInitialized = false;
    bool restartESP = false; /// Restart ESP after config change
    RebootHandler _rebootHandler; /// Handler for restarting ESP and gracefully shutting down stuff
    bool fallbackAp = false; /// AP was opened because client could not connect at startup
    uint32_t lastReconnect = 0; /// Last time in ms the client was connected or a reconnect was attempted
    constexpr static const uint32_t RECONNECT_INTERVAL = 60000; /// Interval in ms between manual reconnects
    constexpr static const uint32_t FALLBACK_RECONNECT_INTERVAL = 30000; /// Same, but while fallback AP is open

    Config& config;
}; // namespace Networking
