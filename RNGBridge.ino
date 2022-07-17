#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <time.h>

#include "Config.h"
#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "Networking.h"
#include "OutputControl.h"
#include "PVOutput.h"
#include "Renogy.h"

// 60 requests per hour.
// 300 requests per hour in donation mode.

uint8_t lastSecond = 0; /// The last seconds value
uint8_t secondsPassedRenogy = 0; /// amount of seconds passed

Config config;
Mqtt* mqtt;
PVOutput* pvo;
Networking networking(config);
GUI gui(networking);
Renogy renogy(Serial);
OutputControl outputs(renogy, config.getDeviceConfig());

constexpr static const uint8_t LED = D1;

constexpr static const char* GHOTA_NTP1 = "pool.ntp.org";
constexpr static const char* GHOTA_NTP2 = "time.nist.gov";
constexpr static const char* GHOTA_HOST = "api.github.com";
constexpr static const uint16_t GHOTA_PORT = 443;
constexpr static const char* GHOTA_CONTENT_TYPE = "application/octet-stream";

constexpr static const char* GHOTA_USER = "enwi";
constexpr static const char* GHOTA_REPO = "RNGBridgeDoc";
constexpr static const char* GHOTA_FILE = "RNGBridge.ino.bin";
constexpr static const bool GHOTA_ACCEPT_PRERELEASE = false;

// Note version is made up of
// Major Changes . New Features . Bugfixes (aka major.minor.bug)
constexpr static const char* CURRENT_VERSION_TAG = "2.6.0";

boolean networkClientEnabled = false; /// Indicates if RNGBridge should have internet access

// Set time via NTP, as required for x.509 validation
void updateTime()
{
    DEBUG("Updating time... ");
    configTime(0, 0, GHOTA_NTP1, GHOTA_NTP2); // UTC

    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2)
    {
        optimistic_yield(500000);
        now = time(nullptr);
    }

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    DEBUGLN("done");
}

String getNewSoftwareVersion()
{
    DEBUGLN("Check for new software version");

    updateTime(); // Clock needs to be set to perform certificate checks

    WiFiClientSecure client;
    // client.setCertStore(_certStore);
    client.setInsecure();

    if (!client.connect(GHOTA_HOST, GHOTA_PORT))
    {
        client.stop();
        // _lastError = "Connection failed";
        return "";
    }

    const String url = String("/repos/") + GHOTA_USER + "/" + GHOTA_REPO + "/releases/latest";

    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + GHOTA_HOST + "\r\n"
        + "User-Agent: ESP_OTA_GitHubArduinoLibrary\r\n" + "Connection: close\r\n\r\n");

    // skip header
    client.find("\r\n\r\n");

    // --- ArduinoJSON v6 --- //

    // Get from https://arduinojson.org/v6/assistant/
    const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(13) + 5 * JSON_OBJECT_SIZE(18) + 5560;
    DynamicJsonDocument doc(capacity);

    const DeserializationError error = deserializeJson(doc, client);
    client.stop();

    if (error)
    {
        // _lastError = "Failed to parse JSON."; // Error was: " + error.c_str();
        DEBUGF("Failed to parse JSON: %s\n", error.c_str());
        return "";
    }

    if (!doc.containsKey("tag_name"))
    {
        // _lastError = "JSON didn't match expected structure. 'tag_name' missing.";
        DEBUGLN("JSON missing tag_name");
        return "";
    }

    const char* release_tag = doc["tag_name"];
    const char* release_name = doc["name"];
    if (strcmp(release_tag, CURRENT_VERSION_TAG) == 0)
    {
        // _lastError = "Already running latest release.";
        DEBUGLN("Already running latest release");
        return "";
    }

    if (!GHOTA_ACCEPT_PRERELEASE && doc["prerelease"])
    {
        // _lastError = "Latest release is a pre-release and GHOTA_ACCEPT_PRERELEASE is set to false.";
        DEBUGLN("Latest release is a pre-release");
        return "";
    }

    DEBUGF("Found new release: %s\n", release_tag);
    return release_tag;

    // JsonArray assets = doc["assets"];
    // for (auto asset : assets)
    // {
    //     const char* asset_type = asset["content_type"];
    //     const char* asset_name = asset["name"];
    //     if (strcmp(asset_type, GHOTA_CONTENT_TYPE) == 0 && strcmp(asset_name, GHOTA_FILE) == 0)
    //     {
    //         // _upgradeURL = asset["browser_download_url"].as<String>();
    //         return "";
    //     }
    // }

    // // _lastError = "No valid binary found for latest release.";
    // return "";
}

void checkForUpdate()
{
    const String update = getNewSoftwareVersion();
    if (!update.isEmpty())
    {
        gui.updateOtaStatus(update);
    }
}

void setup()
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.begin(115200);
    DEBUGLN();
    DEBUGF("RNGBridge V%s\n", CURRENT_VERSION_TAG);
#endif
    // Signal startup
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    sniprintf(deviceMAC, sizeof(deviceMAC), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    config.initConfig();

    networking.init(outputs);
    // Last will of mqtt won't work this way
    // networking.setRebootHandler([]() {
    //     if (mqtt)
    //     {
    //         mqtt->disconnect();
    //     }
    //     ESP.restart();
    // });

    const NetworkConfig& netwConfig = config.getNetworkConfig();
    networkClientEnabled = netwConfig.clientEnabled;
    if (netwConfig.clientEnabled)
    {
        // Check for software update at startup
        checkForUpdate();

        // MQTT setup
        const MqttConfig& mqttConfig = config.getMqttConfig();
        if (mqttConfig.enabled)
        {
            mqtt = new Mqtt(mqttConfig);
            mqtt->setListener([](const String& status) { gui.updateMQTTStatus(status); });
            mqtt->connect();
        }
        else
        {
            gui.updateMQTTStatus("Disabled");
        }

        // PVOutput setup
        const PVOutputConfig& pvoConfig = config.getPvoutputConfig();
        if (pvoConfig.enabled)
        {
            pvo = new PVOutput(pvoConfig);
            pvo->setListener([](const String& status) { gui.updatePVOutputStatus(status); });
            pvo->start();
        }
        else
        {
            gui.updatePVOutputStatus("Disabled");
        }
    }

    DeviceConfig& deviceConfig = config.getDeviceConfig();
    renogy.setListener([&](const Renogy::Data& data) {
        if (mqtt)
        {
            mqtt->updateRenogyStatus(data);
        }
        if (pvo)
        {
            pvo->updateData(RENOGY_INTERVAL, data.panelVoltage * data.panelCurrent, data.loadVoltage * data.loadCurrent,
                data.batteryVoltage);
        }

        outputs.update(data);

        gui.updateRenogyStatus(data);
    });

    outputs.setListener([](const OutputControl::Status status) { gui.updateOutputStatus(status); });

    // Signal setup done
    digitalWrite(LED, LOW);
}

void loop()
{
    const uint32_t time = millis();
    const uint32_t timeS = time / 1000;
    const uint8_t currentSecond = timeS % 60;

    if (currentSecond != lastSecond)
    {
        // Signal start of work
        digitalWrite(LED, HIGH);
        DEBUG(F("[System] Uptime: "));
        DEBUGLN(timeS);
        lastSecond = currentSecond;

        ++secondsPassedRenogy;
        if (secondsPassedRenogy >= RENOGY_INTERVAL)
        {
            secondsPassedRenogy = 0;
            // Read and process data every 2 seconds
            renogy.readAndProcessData();
        }

        if (mqtt)
        {
            mqtt->loop();
        }

        if (pvo)
        {
            pvo->loop();
        }

        // Check for software updates every day
        if (networkClientEnabled && timeS % 86400 == 0)
        {
            checkForUpdate();
        }

        gui.updateUptime(timeS);
        gui.updateHeap(ESP.getFreeHeap());
        gui.update();

        networking.update();

        // Signal end of work
        digitalWrite(LED, LOW);
    }

    // handle wifi or whatever the esp is doing
    // yield();
    delay(500);
}
