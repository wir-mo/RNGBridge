#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "Networking.h"
#include "PVOutput.h"
#include "Renogy.h"
// 60 requests per hour.
// 300 requests per hour in donation mode.

const uint32_t RENOGY_INTERVAL = 1; /// The interval in s at which the renogy data should be read

uint8_t lastSecond = 0; /// The last seconds value
uint8_t secondsPassedRenogy = 0; /// amount of seconds passed
uint16_t secondsPassedPVOutput = 0; /// amount of seconds passed

Config config;
Mqtt* mqtt;
PVOutput* pvo;
Networking networking(config);
GUI gui(networking);
Renogy renogy(Serial);

void setup()
{
    // Signal startup
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LED_BUILTIN, LOW);

    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    sniprintf(deviceMAC, sizeof(deviceMAC), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    config.initConfig();

    networking.init(renogy);

    const NetworkConfig& netwConfig = config.getNetworkConfig();
    if (netwConfig.clientEnabled)
    {
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
            // pvo->setListener([](const String& status) { gui.updatePVOutputStatus(status); });
            // pvo->connect();
        }
        else
        {
            gui.updatePVOutputStatus("Disabled");
        }
    }

    renogy.setListener([](const Renogy::Data& data) {
        if (mqtt)
        {
            mqtt->updateRenogyStatus(data);
        }
        gui.updateRenogyStatus(data);
    });

    // Signal setup done
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
    const uint32_t time = millis();
    const uint32_t timeS = time / 1000;
    const uint8_t currentSecond = timeS % 60;

    if (currentSecond != lastSecond)
    {
        // Signal start of work
        digitalWrite(LED_BUILTIN, LOW);
        lastSecond = currentSecond;

        ++secondsPassedRenogy;
        if (secondsPassedRenogy >= RENOGY_INTERVAL)
        {
            secondsPassedRenogy = 0;
            // Read and process data every 2 seconds
            renogy.readAndProcessData();
        }

        gui.updateUptime(timeS);
        gui.updateHeap(ESP.getFreeHeap());
        gui.update();

        // if (WIFI::connected)
        // {
        // if (Settings::settings.pvOutput)
        // {
        //     if (PVOutput::started)
        //     {
        //         ++secondsPassedPVOutput;
        //         if (secondsPassedPVOutput >= PVOutput::_updateInterval)
        //         {
        //             secondsPassedPVOutput = 0;
        //             PVOutput::Callback::sendData();
        //         }
        //     }
        //     else
        //     {
        //         if (secondsPassedPVOutput)
        //         {
        //             // Reset counters
        //             PVOutput::_powerGeneration = 0.0;
        //             PVOutput::_powerConsumption = 0.0;
        //             PVOutput::_voltage = 0.0;

        //             secondsPassedPVOutput = 0;
        //         }
        //         PVOutput::start();
        //     }
        // }
        // }

        if (mqtt)
        {
            mqtt->loop();
        }

        if (pvo)
        {
            pvo->loop();
        }

        networking.update();

        // Signal end of work
        digitalWrite(LED_BUILTIN, HIGH);
    }

    // handle wifi or whatever the esp is doing
    // yield();
    delay(500);
}
