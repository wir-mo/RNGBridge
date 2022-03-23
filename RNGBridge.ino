#include <functional>

#include "Config.h"
#include "Constants.h"
#include "GUI.h"
#include "MQTT.h"
#include "Networking.h"
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

constexpr static const uint8_t LED = D1;

namespace
{
    void handleOutput(OutputControl& output, const Renogy::Data& data, std::function<void(bool)> enable)
    {
        if (output.inputType == InputType::disabled)
        {
            return;
        }

        float value = 0;
        switch (output.inputType)
        {
        case InputType::bsoc:
            value = data.batteryCharge;
            break;
        case InputType::bvoltage:
            value = data.batteryVoltage;
            break;
        }

        if (value >= output.max)
        {
            const bool newState = !output.inverted;
            if (output.lastState != newState)
            {
                output.lastState = newState;
                enable(newState);
            }
        }
        else if (value < output.min)
        {
            const bool newState = output.inverted;
            if (output.lastState != newState)
            {
                output.lastState = newState;
                enable(newState);
            }
        }
    }
} // namespace

void setup()
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.begin(115200);
    DEBUGLN();
    // Note version is made up of
    // Major Changes . New Features . Bugfixes (aka major.minor.bug)
    DEBUGLN("RNGBridge V2.2.1");
#endif
    // Signal startup
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);

    uint8_t mac[6];
    wifi_get_macaddr(STATION_IF, mac);
    sniprintf(deviceMAC, sizeof(deviceMAC), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    config.initConfig();

    networking.init(renogy);
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

    pinMode(D5, OUTPUT);
    pinMode(D6, OUTPUT);
    pinMode(D7, OUTPUT);
    digitalWrite(D5, LOW);
    digitalWrite(D6, LOW);
    digitalWrite(D7, LOW);
    auto handleLoad = [](bool enable) { renogy.enableLoad(enable); };
    auto handleOut1 = [](bool enable) { digitalWrite(D5, enable); }; // Out1 = D5;
    auto handleOut2 = [](bool enable) { digitalWrite(D6, enable); }; // Out2 = D6;
    auto handleOut3 = [](bool enable) { digitalWrite(D7, enable); }; // Out3 = D7;
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

        handleOutput(deviceConfig.load, data, handleLoad);
        handleOutput(deviceConfig.out1, data, handleOut1);
        handleOutput(deviceConfig.out2, data, handleOut2);
        handleOutput(deviceConfig.out3, data, handleOut3);

        gui.updateRenogyStatus(data);
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
