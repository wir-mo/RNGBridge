#include "OutputControl.h"

OutputControl::OutputControl(Renogy& renogy, DeviceConfig& deviceConfig) : deviceConfig(deviceConfig)
{
    pinMode(PIN_OUTPUT1, OUTPUT);
    pinMode(PIN_OUTPUT2, OUTPUT);
    pinMode(PIN_OUTPUT3, OUTPUT);
    digitalWrite(PIN_OUTPUT1, LOW);
    digitalWrite(PIN_OUTPUT2, LOW);
    digitalWrite(PIN_OUTPUT3, LOW);
    handleLoad = [&](const bool enable) {
        renogy.enableLoad(enable);
        deviceConfig.load.lastState = enable;
    };
    handleOut1 = [&](const bool enable) {
        digitalWrite(PIN_OUTPUT1, enable);
        deviceConfig.out1.lastState = enable;
        _value.out1 = enable;
        notify(_value);
    };
    handleOut2 = [&](const bool enable) {
        digitalWrite(PIN_OUTPUT2, enable);
        deviceConfig.out2.lastState = enable;
        _value.out2 = enable;
        notify(_value);
    };
    handleOut3 = [&](const bool enable) {
        digitalWrite(PIN_OUTPUT3, enable);
        deviceConfig.out3.lastState = enable;
        _value.out3 = enable;
        notify(_value);
    };
}

void OutputControl::enableLoad(const bool enable)
{
    handleLoad(enable);
}

void OutputControl::enableOut1(const bool enable)
{
    handleOut1(enable);
}

void OutputControl::enableOut2(const bool enable)
{
    handleOut2(enable);
}

void OutputControl::enableOut3(const bool enable)
{
    handleOut3(enable);
}

void OutputControl::update(const Renogy::Data& data)
{
    handleOutput("Load", deviceConfig.load, data, handleLoad);
    handleOutput("Out1", deviceConfig.out1, data, handleOut1);
    handleOutput("Out2", deviceConfig.out2, data, handleOut2);
    handleOutput("Out3", deviceConfig.out3, data, handleOut3);
}

void OutputControl::handleOutput(
    const char* tag, OutputConfig& output, const Renogy::Data& data, std::function<void(const bool)> enable)
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
    case InputType::pvoltage:
        value = data.panelVoltage;
        break;
    case InputType::pcurrent:
        value = data.panelCurrent;
        break;
    }

    RNG_DEBUGF("[OutputControl][%s] min %.2f, max %.2f, value %.2f\n", tag, output.min, output.max, value);

    if (value >= output.max)
    {
        const bool newState = !output.inverted;
        if (output.lastState != newState)
        {
            // output.lastState = newState;
            enable(newState);
            RNG_DEBUGF("[OutputControl][%s] %.2f>=%.2f turned %s\n", tag, value, output.max, newState ? "on" : "off");
        }
    }
    else if (value < output.min)
    {
        const bool newState = output.inverted;
        if (output.lastState != newState)
        {
            // output.lastState = newState;
            enable(newState);
            RNG_DEBUGF("[OutputControl][%s] %.2f<%.2f turned %s\n", tag, value, output.min, newState ? "on" : "off");
        }
    }
}