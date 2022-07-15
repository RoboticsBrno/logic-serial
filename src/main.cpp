#include <Arduino.h>
#include <string>
#include <SmartLeds.h>
#include <variant>
#include "buttons.h"
#include <map>

#define DEBUG

#ifdef DEBUG
    #define DEBUG_LINE(x) Serial.println(x)
#else
    #define DEBUG_LINE(x)
#endif

#define LOGIC_1_1

/**
 * Button pins are ordered from top to bottom, left to right,
 * e.g. top left, top right, bottom left, bottom right, arrow up,
 * arrow left, center, arrow right, arrow down.
 */

#if defined(LOGIC_1_1)
    #define LED_DATA_PIN     23
    #define LED_STATUS_PIN   21
    #define LED_POWER_PIN    16
    #define BUZZER_PIN       27
    #define BUTTON_PINS      { 18, 19, 4, 25, 14, 32, 17, 13, 26 }
    #define RX_PIN           5
    #define TX_PIN           2
#elif defined(LOGIC_1_2)
    #define LED_DATA_PIN     23
    #define LED_STATUS_PIN   21
    #define LED_POWER_PIN    26
    #define BUZZER_PIN       27
    #define BUTTON_PINS      { 18, 19, 4, 25, 14, 32, 35, 13, 36 }
    #define RX_PIN           5
    #define TX_PIN           2
#endif

const int BRIGHTNESS_DIVISOR = 2;

auto &SER = Serial2;

SmartLed leds(LED_WS2812, 100, LED_DATA_PIN, 0, SingleBuffer);
SmartLed status(LED_WS2812, 5, LED_STATUS_PIN, 1, SingleBuffer);

Buttons buttons(BUTTON_PINS);

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    pinMode(LED_POWER_PIN, OUTPUT);
    digitalWrite(LED_POWER_PIN, HIGH);
#ifdef BUZZER_PIN
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
#endif

    for (size_t i = 0; i < buttons.count(); i++) {
        buttons.set_callback(i, [](int id, bool state) {
            if (state) {
                SER.println(("pressed " + std::to_string(id)).c_str());
            } else {
                SER.println(("released " + std::to_string(id)).c_str());
            }
        });
    }
}

bool iscmdchar(char c) {
    return isalnum(c) || c == '_';
}

bool is_number(const std::string &str) {
    auto begin = str.begin();
    if (begin == str.end()) {
        return true;
    }
    if (*begin == '-') {
        begin++;
    }
    if (!std::all_of(begin, str.end(), [](char c) { return isdigit(c); })) {
        return false;
    }
    return true;
}

enum class ArgType : int {
    String = 0,
    Int = 1
};

struct Command {
    std::string name;
    std::vector<std::string> args;

    std::string to_string() const {
        std::string result = name;
        for (const auto& arg : args) {
            result += " " + arg;
        }
        return result;
    }

    bool check_args(std::vector<ArgType> &expected) const {
        if (args.size() != expected.size()) {
            return false;
        }
        for (size_t i = 0; i < args.size(); i++) {
            if (expected[i] == ArgType::Int && !is_number(args[i])) {
                return false;
            }
        }
        return true;
    }

    template<ArgType Type>
    auto arg(size_t index) const {
        if constexpr (Type == ArgType::Int) {
            return std::stoi(args[index]);
        } else {
            return args[index];
        }
    }
};

using StrIt = typename std::string::const_iterator;
using ParserState = typename std::tuple<StrIt, std::string>;

std::variant<bool, ParserState> parse_arg(StrIt begin, StrIt end) {
    while (begin != end && isspace(*begin)) {
        ++begin;
    }
    if (begin == end) {
        return true;
    }

    std::string arg;
    while (begin != end && !isspace(*begin)) {
        arg += *begin;
        ++begin;
    }

    return std::make_tuple(begin, arg);
}

std::variant<bool, Command> parse_command(const std::string& line) {
    StrIt begin = line.begin();
    auto res = parse_arg(begin, line.end());
    if (std::holds_alternative<bool>(res)) {
        return false;
    }
    std::string command;
    std::tie(begin, command) = std::get<ParserState>(res);

    if (!std::all_of(command.begin(), command.end(), iscmdchar)) {
        return false;
    }

    std::vector<std::string> args;
    while (begin != line.end()) {
        auto res = parse_arg(begin, line.end());
        if (std::holds_alternative<bool>(res)) {
            if (!std::get<bool>(res)) {
                return false;
            }
            break;
        }
        std::string arg;
        std::tie(begin, arg) = std::get<ParserState>(res);
        args.push_back(arg);
    }
    return Command{command, args};
}

constexpr ArgType Int = ArgType::Int;
constexpr ArgType String = ArgType::String;
std::map<std::string, std::pair<std::vector<ArgType>, std::function<void(Command)>>> commands = {
    { "set", { {Int, Int, Int, Int, Int}, [](Command command) {
        auto x = command.arg<Int>(0);
        auto y = command.arg<Int>(1);
        if (x < 0 || x >= 10 || y < 0 || y >= 10) {
            DEBUG_LINE("Invalid coordinates");
            return;
        }
        leds.wait();
        leds[x + y * 10] = Rgb(
            command.arg<Int>(2) / BRIGHTNESS_DIVISOR,
            command.arg<Int>(3) / BRIGHTNESS_DIVISOR,
            command.arg<Int>(4) / BRIGHTNESS_DIVISOR
        );
    }}},
    { "clear", { {}, [](Command command) {
        leds.wait();
        for (auto &led : leds) {
            led = Rgb(0, 0, 0);
        }
        leds.show();
    }}},
    { "show", { {}, [](Command command) {
        leds.wait();
        leds.show();
    }}},
    { "set_status", { {Int, Int, Int, Int}, [](Command command) {
        int index = command.arg<Int>(0);
        if (index < 0 || index >= 5) {
            DEBUG_LINE("Invalid index");
            return;
        }
        status.wait();
        status[index] = Rgb(
            command.arg<Int>(1) / BRIGHTNESS_DIVISOR,
            command.arg<Int>(2) / BRIGHTNESS_DIVISOR,
            command.arg<Int>(3) / BRIGHTNESS_DIVISOR
        );
    }}},
    { "clear_status", { {}, [](Command command) {
        status.wait();
        for (auto &led : status) {
            led = Rgb(0, 0, 0);
        }
        status.show();
    }}},
    { "show_status", { {}, [](Command command) {
        status.wait();
        status.show();
    }}},
#ifdef BUZZER_PIN
    { "set_buzzer", { {Int}, [](Command command) {
        digitalWrite(BUZZER_PIN, command.arg<Int>(0) != 0);
    }}}
#endif
};

void loop() {
    std::string line;
    while (true) {
        buttons.update();
        while (SER.available()) {
            char c = SER.read();
            if (c == '\n') {
                goto after_loop;
            } else {
                line += c;
            }
        }
        yield();
    }
after_loop:

    auto result = parse_command(line);
    if (std::holds_alternative<bool>(result)) {
        return;
    }
    Command command = std::get<Command>(result);
    DEBUG_LINE(("Command: " + command.to_string()).c_str());

    auto resp = commands.find(command.name);
    if (resp == commands.end()) {
        DEBUG_LINE(("Invalid command: " + command.to_string()).c_str());
        return;
    }
    auto [exp_args, cbk] = (*resp).second;
    if (!command.check_args(exp_args)) {
        DEBUG_LINE(("Invalid arguments: " + command.to_string()).c_str());
        return;
    }
    cbk(command);
}
