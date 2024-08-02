#pragma once

/// @brief Class that can be listened to for status updates
template <typename T>
class Observerable
{
public:
    typedef std::function<void(const T&)> Observer; /// Observer callback

public:
    Observerable() { }

    ///@brief Observe this listenable
    ///
    ///@param observer Observer or null
    void observe(Observer observer)
    {
        _observer = observer;
        if (_observer)
        {
            _observer(_value);
        }
    }

protected:
    ///@brief Notify observer and update internal value
    ///
    ///@param value New value
    void notify(const T& value)
    {
        _value = value;
        if (_observer)
        {
            _observer(value);
        }
    }

protected:
    Observer _observer; /// Observer or null
    T _value; /// Current value
};
