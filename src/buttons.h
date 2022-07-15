#pragma once

#include <functional>
#include <cstdint>
#include <vector>

class Buttons {
    std::vector<uint8_t> pins;
    std::vector<bool> states;
    std::vector<bool> changes;
    std::vector<std::function<void(int, bool)>> callbacks;
public:
    Buttons(std::vector<uint8_t> pins);
    bool is_pressed(int id) const { return states[id]; }
    bool changed(int id) const { return changes[id]; }
    void update();
    void set_callback(int id, std::function<void(int, bool)> callback);
    size_t count() const { return pins.size(); }
};
