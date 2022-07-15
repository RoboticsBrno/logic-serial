# logic-serial
An example making Logic board into a serial-connected peripheral.

Serial interface is connected to gpio 5 (rx) and 2 (tx).

## Protocol
All communication is text-based. The program expects line-separated commands and sends line-separated events. Arguments are separated by spaces.

### Commands
- `set <x> <y> <red> <green> <blue>`
    - set led at coordinates `x`, `y` to color `Rgb(red, green, blue)`
- `show`
    - show the current state of the leds
- `clear`
    - turn off all leds
- `set_status <index> <red> <green> <blue>`
    - set the status led at index `index` to color `Rgb(red, green, blue)`
- `show_status`
    - show the current state of the status leds
- `clear_status`
    - turn off all status leds
- `set_buzzer <state>`
    - set the buzzer to state `state` (0 or 1)

### Events
- `pressed <id>`
    - a button with id `id` was pressed
- `released <id>`
    - a button with id `id` was released

Button ids come in sequence starting from top to bottom, left to right,

e.g. top left (0), top right (1), bottom left (2), bottom right (3), arrow up (4), arrow left (5), center (6), arrow right (7), arrow down (8).
