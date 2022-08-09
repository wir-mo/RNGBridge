#include "OTA.h"

OTA::OTA(const char* versionTag, GUI& gui, RNGTime& time) : _versionTag(versionTag), _gui(gui), _time(time) { }

String OTA::getNewSoftwareVersion()
{
    RNG_DEBUGLN(F("[OTA] Checking for new software version"));

    // updateTime(); // Clock needs to be set to perform certificate checks

    WiFiClientSecure client;
    // client.setCertStore(_certStore);
    client.setInsecure();

    if (!client.connect(GHOTA_HOST, GHOTA_PORT))
    {
        client.stop();
        // _lastError = "Connection failed";
        RNG_DEBUGLN(F("[OTA] Connection to GitHub failed"));
        return "";
    }

    client.printf_P(PSTR("GET /repos/%s/%s/releases/latest HTTP/1.1\r\n"
                         "Host: %s\r\n"
                         "User-Agent: ESP_OTA_GitHubArduinoLibrary\r\n"
                         "Connection: close\r\n\r\n"),
        GHOTA_USER, GHOTA_REPO, GHOTA_HOST);

    // skip header
    client.find("\r\n\r\n");

    // Filter to reduce size of resulting doc
    StaticJsonDocument<32> filter;
    filter["tag_name"] = true;
    filter["prerelease"] = true;
    StaticJsonDocument<256> doc;
    const DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

    client.stop();

    if (error)
    {
        // _lastError = "Failed to parse JSON."; // Error was: " + error.c_str();
        RNG_DEBUGF("[OTA] Failed to parse JSON: %s\n", error.c_str());
        return "";
    }

    if (!doc.containsKey("tag_name"))
    {
        // _lastError = "JSON didn't match expected structure. 'tag_name' missing.";
        RNG_DEBUGLN(F("[OTA] JSON missing tag_name"));
        return "";
    }

    const char* release_tag = doc["tag_name"];
    // const char* release_name = doc["name"];
    // TODO maybe also check if new tag is larger?
    if (strcmp(release_tag, _versionTag) == 0)
    {
        // _lastError = "Already running latest release.";
        RNG_DEBUGLN(F("[OTA] Already running latest release"));
        return "";
    }

    if (!GHOTA_ACCEPT_PRERELEASE && doc["prerelease"])
    {
        // _lastError = "Latest release is a pre-release and GHOTA_ACCEPT_PRERELEASE is set to false.";
        RNG_DEBUGLN(F("[OTA] Latest release is a pre-release"));
        return "";
    }

    RNG_DEBUGF("[OTA] Found new release: %s\n", release_tag);
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

void OTA::loop()
{
    if (_state == State::CHECK_FOR_VERSION && _time.isSynced())
    {
        const String& version = getNewSoftwareVersion();
        if (!version.isEmpty())
        {
            _gui.updateOtaStatus(version);
        }
        _state = State::IDLE;
    }
}
