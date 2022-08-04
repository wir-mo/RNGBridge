#include "Config.h"
#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "Networking.h"
#include "OTA.h"
#include "OutputControl.h"
#include "PVOutput.h"
#include "Renogy.h"

// 60 requests per hour.
// 300 requests per hour in donation mode.

constexpr static const uint8_t LED = D1;

uint8_t lastSecond = 0; /// The last seconds value
uint8_t secondsPassedRenogy = 0; /// amount of seconds passed

Config config;
Mqtt* mqtt;
PVOutput* pvo;
OTA* ota;
Networking networking(config);
GUI gui(networking);
Renogy renogy(Serial);
OutputControl outputs(renogy, config.getDeviceConfig());

void setup()
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.begin(115200);
    DEBUGLN();
    DEBUGF("%s %S (SWV%s)\n", MODEL, HARDWARE_VERSION, SOFTWARE_VERSION);
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
    if (netwConfig.clientEnabled)
    {
        // Check for software update at startup
        ota = new OTA(SOFTWARE_VERSION, gui);
        ota->checkForUpdate();

        // MQTT setup
        const MqttConfig& mqttConfig = config.getMqttConfig();
        if (mqttConfig.enabled)
        {
            mqtt = new Mqtt(mqttConfig, outputs);
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

    outputs.setListener([](const OutputControl::Status status) {
        gui.updateOutputStatus(status);
        if (mqtt)
        {
            mqtt->updateOutputStatus(status);
        }
    });

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
        lastSecond = currentSecond;

        if (currentSecond % 5 == 0)
        {
            DEBUG(F("[System] Uptime: "));
            DEBUGLN(timeS);
        }

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
        if (ota)
        {
            ota->update();

            if (timeS % 86400 == 0)
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
    delay(500);
}
