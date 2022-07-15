#include "buttons.h"

#include <Arduino.h>

Buttons::Buttons(std::vector<uint8_t> pins):
        pins(pins),
        states(pins.size(), false),
        changes(pins.size(), false),
        callbacks(pins.size(), nullptr)
{
    for (auto pin : pins) {
        pinMode(pin, INPUT_PULLUP);
    }
}

void Buttons::update() {
    for (int i = 0; i < pins.size(); i++) {
        bool newState = digitalRead(pins[i]) == LOW;
        if (newState != states[i]) {
            changes[i] = true;
            states[i] = newState;
            if (callbacks[i]) {
                callbacks[i](i, newState);
            }
        }
    }
}

void Buttons::set_callback(int id, std::function<void(int, bool)> callback) {
    callbacks[id] = callback;
}
