// #include <DoubleResetDetector.h>

#include "Config.h"
#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "Networking.h"
#include "OTA.h"
#include "OutputControl.h"
#include "PVOutput.h"
#include "RSTime.h"

#include "device/Dummy.h"
#include "device/Epever.h"
#include "device/RSDevice.h"
#include "device/Renogy.h"
#include "device/RenogyBattery.h"

// 60 requests per hour.
// 300 requests per hour in donation mode.

// pinout
// D0 = LED
// D1 = SCL (QWIIC)
// D2 = SDA (QWIIC)
// D4 = Debug Serial
// D5 = Out1
// D6 = Out2
// D7 = Out3
// D8 = RS485 DE/!RE (direction)
// RX = RS232 RX = RS485 RO = RNG TX
// TX = RS232 TX = RS485 DI = RNG RX

constexpr static const uint8_t LED = D0;

uint8_t lastSecond = 0; /// The last seconds value
uint8_t secondsPassedRenogy = 0; /// amount of seconds passed

// DoubleResetDetector* drd;
RSTime _time;
Config config;
std::unique_ptr<Mqtt> mqtt;
std::unique_ptr<PVOutput> pvo;
std::unique_ptr<OTA> ota;
std::shared_ptr<RSDevice> rsDevice;
std::unique_ptr<OutputControl> outputs;
Networking networking(config);
GUI gui;

void changeDeviceType(const DeviceType type)
{
    // signal device type change start
    digitalWrite(LED, HIGH);

    DeviceConfig& deviceConfig = config.getDeviceConfig();

    // Delete shared pointer to device
    rsDevice = nullptr;
    // Clear the whole gui json
    gui.json.clear();

    switch (type)
    {
    case DeviceType::dummy:
        rsDevice = std::make_shared<Dummy>();
        break;
    case DeviceType::renogy:
        rsDevice = std::make_shared<Renogy>(Serial, deviceConfig.address);
        break;
    case DeviceType::renogyBattery:
        rsDevice = std::make_shared<RenogyBattery>(Serial, deviceConfig.address);
        break;
    case DeviceType::epever:
        rsDevice = std::make_shared<Epever>(Serial, deviceConfig.address);
        break;

    default:
        break;
    }
    gui.json["t"] = DeviceTypeToString(type);

    if (!rsDevice)
    {
        return;
    }

    outputs = std::make_unique<OutputControl>(*rsDevice, deviceConfig);
    networking.init(*outputs);

    // Last will of mqtt won't work this way
    // networking.setRebootHandler([]() {
    //     if (mqtt)
    //     {
    //         mqtt->disconnect();
    //     }
    //     ESP.restart();
    // });

    const NetworkConfig& netwConfig = config.getNetworkConfig();
    if (netwConfig.clientEnabled)
    {
        // Check for software update at startup
        if (!ota)
        {
            ota = std::make_unique<OTA>(SOFTWARE_VERSION, gui, _time);
            ota->checkForUpdate();
        }
        const MqttConfig& mqttConfig = config.getMqttConfig();
        if (mqttConfig.enabled)
        {
            mqtt = std::make_unique<Mqtt>(mqttConfig, *outputs);
            // mqtt->observe([](const String& status) { gui.updateMQTTStatus(status); });
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
            pvo = std::make_unique<PVOutput>(pvoConfig, _time);
            pvo->observe([](const String& status) { gui.updatePVOutputStatus(status); });
            pvo->start();
        }
        else
        {
            gui.updatePVOutputStatus("Disabled");
        }
    }

    rsDevice->setListener([&]() {
        if (pvo)
        {
            // TODO where do we get the data from?
            // pvo->updateData(data);
        }

        outputs->update(*rsDevice);

        rsDevice->updateUI(gui.json);

        if (mqtt)
        {
            mqtt->publishData();
            if (mqtt->splitData())
            {
                rsDevice->publishIndividualMqttData(*mqtt);
            }
        }
    });

    outputs->observe([](const OutputStatus status) { gui.updateOutputStatus(status); });

    // Signal device type change done
    digitalWrite(LED, LOW);
}

void setup()
{
#ifdef RS_DEBUG_SERIAL
    RS_DEBUG_SERIAL.begin(115200);
    // RS_DEBUG_SERIAL.setDebugOutput(true);
    RS_DEBUGLN();
    RS_DEBUGF("%s (SWV%s)\n", MODEL, SOFTWARE_VERSION);
#endif
    // Signal startup
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    sniprintf(deviceMAC, sizeof(deviceMAC), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // drd = new DoubleResetDetector(0, 0);
    // RS_DEBUGLN("[DRD] Check");
    // if (drd->detectDoubleReset())
    // {
    //     RS_DEBUGLN("[DRD] Detected double reset, resetting config");
    //     config.initConfig();
    //     config.setDefaultConfig();
    //     config.saveConfig();
    // }
    // else
    // {
    config.initConfig();
    // }

    changeDeviceType(config.getDeviceConfig().type);

    // drd->stop();
    // delete drd;

    // Signal setup done
    digitalWrite(LED, LOW);
}

void loop()
{
    const uint32_t timeS = millis() / 1000;
    const uint8_t currentSecond = timeS % 60;

    if (currentSecond != lastSecond)
    {
        // Signal start of work
        digitalWrite(LED, HIGH);
        lastSecond = currentSecond;

        _time.loop();

        if (currentSecond % 5 == 0)
        {
            RS_DEBUG(F("[System] Uptime: "));
            RS_DEBUGLN(timeS);
        }

        ++secondsPassedRenogy;
        if (secondsPassedRenogy >= RENOGY_INTERVAL)
        {
            secondsPassedRenogy = 0;
            // Read and process data every 2 seconds
            rsDevice->readAndProcessData();
        }

        if (mqtt)
        {
            mqtt->loop();
        }

        if (pvo)
        {
            pvo->loop();
        }

        if (ota)
        {
            ota->loop();

            // Check for software updates every day at midnight
            if (_time.getEpochTime() % 86400 == 0)
            {
                ota->checkForUpdate();
            }
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
    delay(0);
}
