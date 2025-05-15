// #include <DoubleResetDetector.h>

#include "Config.h"
#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "Networking.h"
#include "OTA.h"
#include "OutputControl.h"
#include "PVOutput.h"
#include "RNGTime.h"

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
RNGTime _time;
Config config;
Mqtt* mqtt;
PVOutput* pvo;
OTA* ota;
std::shared_ptr<RSDevice> rsDevice;
OutputControl* outputs;
Networking networking(config);
GUI gui;

void setup()
{
#ifdef RNG_DEBUG_SERIAL
    RNG_DEBUG_SERIAL.begin(115200);
    // RNG_DEBUG_SERIAL.setDebugOutput(true);
    RNG_DEBUGLN();
    RNG_DEBUGF("%s %S (SWV%s)\n", MODEL, HARDWARE_VERSION, SOFTWARE_VERSION);
#endif
    // Signal startup
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    sniprintf(deviceMAC, sizeof(deviceMAC), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // drd = new DoubleResetDetector(0, 0);
    // RNG_DEBUGLN("[DRD] Check");
    // if (drd->detectDoubleReset())
    // {
    //     RNG_DEBUGLN("[DRD] Detected double reset, resetting config");
    //     config.initConfig();
    //     config.setDefaultConfig();
    //     config.saveConfig();
    // }
    // else
    // {
    config.initConfig();
    // }

    DeviceConfig& deviceConfig = config.getDeviceConfig();
    switch (deviceConfig.type)
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
    if (rsDevice)
    {
        outputs = new OutputControl(*rsDevice, deviceConfig);
        networking.init(*outputs);
    }
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
        ota = new OTA(SOFTWARE_VERSION, gui, _time);
        ota->checkForUpdate();

        // MQTT setup
        const MqttConfig& mqttConfig = config.getMqttConfig();
        if (mqttConfig.enabled)
        {
            mqtt = new Mqtt(mqttConfig, *outputs);
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
            pvo = new PVOutput(pvoConfig, _time);
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
            RNG_DEBUG(F("[System] Uptime: "));
            RNG_DEBUGLN(timeS);
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
