#include "Networking.h"

#include "GUI.h"
#include "favicon.ico.gz.h"

#include "RNGBridgeUI/cpp/build.html.gz.h"

void Networking::initWifi()
{
    if (isInitialized)
    {
        RNG_DEBUGLN(F("[Networking] Wifi already initialized, should not be called again"));
        return;
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
        RNG_DEBUGLN(F("[Networking] Wifi auto connected"));
        isInitialized = true;
    }

    NetworkConfig& wifi = config.getNetworkConfig();

    // Does not work without issues
    // if (wifi.apEnabled && wifi.clientEnabled)
    //{
    //    WiFi.mode(WIFI_AP_STA);
    //}
    if (wifi.clientEnabled && !isInitialized)
    {
        RNG_DEBUGLN(F("[Networking] Init wifi client"));
        startClient();
    }
    else if (wifi.apEnabled)
    {
        RNG_DEBUGLN(F("[Networking] Init wifi AP"));
        startAccessPoint();
    }

    isInitialized = true;
}

void Networking::initServer(OutputControl& outputs)
{
    // Handle EventSource
    es.onConnect([this](AsyncEventSourceClient* client) {
        RNG_DEBUGF("[Networking] ES[%s] connect\n", client->client()->remoteIP().toString().c_str());

        StaticJsonDocument<1024> output;
        auto&& obj = output.to<JsonObject>();

        getStatusJsonString(obj);

        String buffer;
        buffer.reserve(measureJson(output));
        serializeJson(output, buffer);

        client->send(buffer.c_str(), "status");
    });
    server.addHandler(&es);

    // Handle binary software updates
    server.on(
        "/ota", HTTP_POST, [](AsyncWebServerRequest* request) { request->send(200); },
        [this](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len,
            bool final) { handleOTAUpload(request, filename, index, data, len, final); });

    // Handle configuration
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* r) { handleConfigApiGet(r); });
    AsyncCallbackJsonWebHandler* handlerSetConfig = new AsyncCallbackJsonWebHandler("/api/config",
        [this](AsyncWebServerRequest* request, JsonVariant& json) { handleConfigApiPost(request, json); });
    server.addHandler(handlerSetConfig);

    // Handle control
    AsyncCallbackJsonWebHandler* handlerControl = new AsyncCallbackJsonWebHandler(
        "/api/control", [this, &outputs](AsyncWebServerRequest* request, JsonVariant& json) {
            handleRenogyApi(request, json, outputs);
        });
    server.addHandler(handlerControl);

    // Handle state
    server.on("/api/state", HTTP_GET, [this](AsyncWebServerRequest* r) { handleStateApiGet(r); });

    // Serve UI
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* r) { handleIndex(r); });

    // Serve favicon
    server.on("/favicon.ico", [](AsyncWebServerRequest* r) {
        AsyncWebServerResponse* response
            = r->beginResponse_P(200, "image/x-icon", favicon_ico_gz_start, favicon_ico_gz_size);
        response->addHeader("Content-Encoding", "gzip");
        r->send(response);
    });

    // captive portal
    auto handleCaptivePortal = [this](AsyncWebServerRequest* request) { captivePortal(request); };
    // Android captive portal. Maybe not needed. Might be handled by notFound handler.
    // server.on("/generate_204", HTTP_GET, handleCaptivePortal);
    // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    // server.on("/fwlink", HTTP_GET, handleCaptivePortal);
    server.onNotFound(handleCaptivePortal);

    server.begin();

    RNG_DEBUGLN(F("[Networking] Server setup"));
}

void Networking::init(OutputControl& outputs)
{
    if (!isInitialized)
    {
        initWifi();
        initServer(outputs);
    }
}

void Networking::getStatusJsonString(JsonObject& output)
{
    auto&& networking = output.createNestedObject("network");

    bool client_enabled = config.getNetworkConfig().clientEnabled;

    auto&& wifi_client = networking.createNestedObject("wifi_client");
    wifi_client["status"] = client_enabled ? (WiFi.isConnected() ? "connected" : "enabled") : "disabled";
    wifi_client["ip"] = WiFi.localIP();
    wifi_client["netmask"] = WiFi.subnetMask();
    wifi_client["dns"] = WiFi.dnsIP();

    auto&& wifi_ap = networking.createNestedObject("wifi_ap");
    wifi_ap["status"] = client_enabled ? "disabled" : "enabled";
    wifi_ap["ip"] = WiFi.softAPIP();
}

void Networking::handleOTAUpload(
    AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final)
{
    if (!index)
    {
        RNG_DEBUGLN(F("[OTA] UploadStart"));
// calculate sketch space required for the update, for ESP32 use the max constant
#if defined(ESP32)
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
#else
        const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace))
#endif
        {
            // start with max available size
            Update.printError(RNG_DEBUG_SERIAL);
        }
#if defined(ESP8266)
        Update.runAsync(true);
#endif
    }

    if (len)
    {
        const size_t written = Update.write(data, len);
        es.send(String(index + written, 10).c_str(), "ota");
    }

    // if the final flag is set then this is the last frame of data
    if (final)
    {
        if (Update.end(true))
        {
            // true to set the size to the current progress
            RNG_DEBUGLN(F("[OTA] Update Success, \nRebooting..."));
            restartESP = true;
        }
#ifdef RNG_DEBUG_SERIAL
        else
        {

            Update.printError(RNG_DEBUG_SERIAL);
        }
#endif
    }
}

void Networking::handleIndex(AsyncWebServerRequest* request)
{
    AsyncWebServerResponse* response
        = request->beginResponse_P(200, F("text/html"), build_html_gz_start, build_html_gz_size);
    response->addHeader(F("Content-Encoding"), F("gzip"));
    request->send(response);
}

void Networking::handleConfigApiGet(AsyncWebServerRequest* request)
{
    String buffer;
    buffer.reserve(512);
    StaticJsonDocument<1024> document;
    config.createJson(document);

    auto&& wifi = document["wifi"];
    wifi.remove("client_password");
    wifi.remove("ap_password");
    wifi["client_has_password"] = config.getNetworkConfig().clientPassword.length() != 0;
    wifi["ap_has_password"] = config.getNetworkConfig().apPassword.length() != 0;
    auto&& mqtt = document["mqtt"];
    mqtt.remove("has_password");

    serializeJson(document, buffer);

    request->send(200, "application/json", buffer);
}

void Networking::handleConfigApiPost(AsyncWebServerRequest* request, JsonVariant& json)
{
    RNG_DEBUGLN(F("[Networking] Received new config"));

#ifdef DEBUG_CONFIG
    String jsonstr;
    jsonstr.reserve(measureJsonPretty(json));
    serializeJsonPretty(json, jsonstr);
    RNG_DEBUGLN(jsonstr);
#endif

    JsonObject&& data = json.as<JsonObject>();

    NetworkConfig& wifi = config.getNetworkConfig();
    bool changed = wifi.tryUpdate(data["wifi"]);

    MqttConfig& mqtt = config.getMqttConfig();
    changed |= mqtt.tryUpdate(data["mqtt"]);

    PVOutputConfig& pvo = config.getPvoutputConfig();
    changed |= pvo.tryUpdate(data["pvo"]);

    DeviceConfig& dev = config.getDeviceConfig();
    changed |= dev.tryUpdate(data["dev"]);

    if (changed)
    {
        config.saveConfig();
    }
    else
    {
        RNG_DEBUGLN(F("[Networking] Config did not change"));
    }

    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
    request->send(response);

    if (changed)
    {
        restartESP = true;
    }
}

void Networking::handleRenogyApi(AsyncWebServerRequest* request, JsonVariant& json, OutputControl& outputs)
{
    JsonObject&& data = json.as<JsonObject>();

    bool success = false;
    if (data.containsKey("load"))
    {
        const bool enabled = data["load"];
        outputs.enableLoad(enabled);
        success = true;
    }

    if (data.containsKey("out1"))
    {
        const bool enabled = data["out1"];
        outputs.enableOut1(enabled);
        success = true;
    }

    if (data.containsKey("out2"))
    {
        const bool enabled = data["out2"];
        outputs.enableOut2(enabled);
        success = true;
    }

    if (data.containsKey("out3"))
    {
        const bool enabled = data["out3"];
        outputs.enableOut3(enabled);
        success = true;
    }

    if (success)
    {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
        request->send(response);
        return;
    }

    AsyncWebServerResponse* response
        = request->beginResponse(400, "text/plain", "JSON does not contain `load`, `out1`, `out2` or `out3` key");
    request->send(response);
}

void Networking::handleStateApiGet(AsyncWebServerRequest* request)
{
    request->send(200, "application/json", GUI::status);
}

bool Networking::isIp(const String& str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9'))
        {
            return false;
        }
    }
    return true;
}

void Networking::update()
{
    // handle DNS
    dnsServer.processNextRequest();

    if (restartESP)
    {
        if (_rebootHandler)
        {
            _rebootHandler();
        }
        else
        {
            ESP.restart();
        }
    }

    if (WiFi.isConnected())
    {
        RNGBridge::rssi = RNGBridge::rssi * 0.7f + WiFi.RSSI() * 0.3f;
    }

    if (es.count())
    {
        es.send(GUI::status.c_str(), "status");
        // RNG_DEBUGF("[Networking] AVG ES packages %d\n", es.avgPacketsWaiting());
    }
}

void Networking::setRebootHandler(RebootHandler handler)
{
    _rebootHandler = handler;
}

bool Networking::captivePortal(AsyncWebServerRequest* request)
{
    if (ON_STA_FILTER(request))
    {
        // RNG_DEBUGLN(F("[Networking] Captive STA Filter"));
        return false; // only serve captive portal in AP mode
    }
    if (!request->hasHeader("Host"))
    {
        // RNG_DEBUGLN(F("[Networking] Captive Host header missing"));
        return false;
    }
    const String hostHeader = request->getHeader("Host")->value();
    if (isIp(hostHeader) || hostHeader.indexOf(HOSTNAME) >= 0)
    {
        // RNG_DEBUG(F("[Networking] Captive Host Filter: ");
        // RNG_DEBUGLN(hostHeader);
        return false;
    }

    RNG_DEBUGLN(F("[Networking] Captive portal"));
    AsyncWebServerResponse* response = request->beginResponse(302);
    response->addHeader(F("Location"), F("http://192.168.4.1"));
    request->send(response);
    return true;
}

bool Networking::handleClientFailsafe()
{
    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        RNG_DEBUG('.');
        delay(250);
        if (millis() - start > 15000)
        {
            RNG_DEBUGLN(F("\n[Networking] Failed, enabling AP"));

            startAccessPoint(false);
            return false;
        }
    }
    return true;
}

void Networking::startClient()
{
    RNG_DEBUGLN(F("[Networking] Using wifi in client mode"));

    const NetworkConfig& wifi = config.getNetworkConfig();

    if (!wifi.dhcpEnabled)
    {
        RNG_DEBUGLN(F("[Networking] Using static ip"));
        if (!WiFi.config(wifi.clientIp, wifi.clientGateway, wifi.clientMask, wifi.clientDns))
        {
            RNG_DEBUGLN(F("[Networking] STA Failed to configure"));
        }
    }
    else
    {
        RNG_DEBUGLN(F("[Networking] Using DHCP"));
    }

    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi.clientSsid, wifi.clientPassword);
    RNG_DEBUG(F("[Networking] Connecting to WiFi .."));

    if (handleClientFailsafe())
    {
        RNG_DEBUG(F("Connected: "));
        RNG_DEBUGLN(WiFi.localIP());

        WiFi.setAutoConnect(false);
        WiFi.setAutoReconnect(true);

        RNGBridge::rssi = WiFi.RSSI();
    }
}

void Networking::startAccessPoint(bool persistent)
{
    const NetworkConfig& wifi = config.getNetworkConfig();

    RNG_DEBUGLN(F("[Networking] Using wifi in ap mode"));

    WiFi.persistent(persistent);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_IP, AP_NETMASK);

    if (wifi.apPassword.length() == 0)
    {
        WiFi.softAP(wifi.apSsid);
        RNG_DEBUGLN(F("[Networking] Starting open AP"));
    }
    else
    {
        WiFi.softAP(wifi.apSsid, wifi.apPassword);
        RNG_DEBUGLN(F("[Networking] Starting protected AP"));
    }

    // captive portal
    RNG_DEBUGLN(F("[Networking] Starting DNS server"));
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
}
