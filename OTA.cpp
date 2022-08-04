#include "OTA.h"

OTA::OTA(const char* versionTag, GUI& gui) : _versionTag(versionTag), gui(gui)
{
    mState = State::IDLE;
    configTime(0, 0, GHOTA_NTP1, GHOTA_NTP2); // UTC
}

String OTA::getNewSoftwareVersion()
{
    DEBUGLN(F("[OTA] Checking for new software version"));

    // updateTime(); // Clock needs to be set to perform certificate checks

    WiFiClientSecure client;
    // client.setCertStore(_certStore);
    client.setInsecure();

    if (!client.connect(GHOTA_HOST, GHOTA_PORT))
    {
        client.stop();
        // _lastError = "Connection failed";
        DEBUGLN(F("[OTA] Connection to GitHub failed"));
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
        DEBUGF("[OTA] Failed to parse JSON: %s\n", error.c_str());
        return "";
    }

    if (!doc.containsKey("tag_name"))
    {
        // _lastError = "JSON didn't match expected structure. 'tag_name' missing.";
        DEBUGLN(F("[OTA] JSON missing tag_name"));
        return "";
    }

    const char* release_tag = doc["tag_name"];
    // const char* release_name = doc["name"];
    // TODO maybe also check if new tag is larger?
    if (strcmp(release_tag, _versionTag) == 0)
    {
        // _lastError = "Already running latest release.";
        DEBUGLN(F("[OTA] Already running latest release"));
        return "";
    }

    if (!GHOTA_ACCEPT_PRERELEASE && doc["prerelease"])
    {
        // _lastError = "Latest release is a pre-release and GHOTA_ACCEPT_PRERELEASE is set to false.";
        DEBUGLN(F("[OTA] Latest release is a pre-release"));
        return "";
    }

    DEBUGF("[OTA] Found new release: %s\n", release_tag);
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

void OTA::update()
{
    switch (mState)
    {
    case State::IDLE:
        break;

    case State::SYNC_TIME: {
        if (time(nullptr) >= 8 * 3600 * 2)
        {
            mState = State::SYNCED_TIME;
        }
    }
    break;

    case State::SYNCED_TIME: {
        DEBUGLN(F("[OTA] Synced time"));
        struct tm timeinfo;
        time_t now = time(nullptr);
        gmtime_r(&now, &timeinfo);
        mState = State::CHECK_FOR_VERSION;
    }
    break;

    case State::CHECK_FOR_VERSION: {
        const String& version = getNewSoftwareVersion();
        if (!version.isEmpty())
        {
            gui.updateOtaStatus(version);
        }
        mState = State::IDLE;
    }
    break;

    default:
        break;
    }
}

// Set time via NTP, as required for x.509 validation
void OTA::updateTime()
{
    DEBUG(F("[OTA] Updating time... "));

    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2)
    {
        yield();
        delay(500);
        now = time(nullptr);
    }

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);

    DEBUGLN("done");
}