#include "OutputControl.h"

OutputControl::OutputControl(Renogy& renogy, DeviceConfig& deviceConfig) : deviceConfig(deviceConfig)
{
    pinMode(PIN_OUTPUT1, OUTPUT);
    pinMode(PIN_OUTPUT2, OUTPUT);
    pinMode(PIN_OUTPUT3, OUTPUT);
    digitalWrite(PIN_OUTPUT1, LOW);
    digitalWrite(PIN_OUTPUT2, LOW);
    digitalWrite(PIN_OUTPUT3, LOW);
    handleLoad = [&](bool enable) { renogy.enableLoad(enable); };
    handleOut1 = [&](bool enable) {
        digitalWrite(PIN_OUTPUT1, enable);
        _status.out1 = enable;
        if (_listener)
        {
            _listener(_status);
        }
    };
    handleOut2 = [&](bool enable) {
        digitalWrite(PIN_OUTPUT2, enable);
        _status.out2 = enable;
        if (_listener)
        {
            _listener(_status);
        }
    };
    handleOut3 = [&](bool enable) {
        digitalWrite(PIN_OUTPUT3, enable);
        _status.out3 = enable;
        if (_listener)
        {
            _listener(_status);
        }
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
    handleOutput(deviceConfig.load, data, handleLoad);
    handleOutput(deviceConfig.out1, data, handleOut1);
    handleOutput(deviceConfig.out2, data, handleOut2);
    handleOutput(deviceConfig.out3, data, handleOut3);
}

void OutputControl::setListener(StatusListener listener)
{
    _listener = listener;
    if (_listener)
    {
        _listener(_status);
    }
}

void OutputControl::handleOutput(OutputConfig& output, const Renogy::Data& data, std::function<void(bool)> enable)
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