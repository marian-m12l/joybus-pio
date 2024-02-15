#include "N64EEPROM4k.hpp"

#include "joybus.h"
#include "n64_definitions.h"

#include <hardware/pio.h>
#include <hardware/timer.h>
#include <pico/stdlib.h>
#include <pico/time.h>

N64EEPROM4k::N64EEPROM4k(uint pin, PIO pio, int sm, int offset) {
    joybus_port_init(&_port, pin, pio, sm, offset);
}

N64EEPROM4k::~N64EEPROM4k() {
    joybus_port_terminate(&_port);
}

n64_eeprom_operation_t __no_inline_not_in_flash_func(N64EEPROM4k::WaitForCommand)() {
    n64_eeprom_operation_t operation;

    while (true) {
        joybus_receive_bytes(&_port, (uint8_t*) &operation.command, 1, receive_timeout_us, false);

        switch (operation.command) {
            case N64Command::RESET:
            case N64Command::PROBE:
                // Wait for stop bit before responding.
                busy_wait_us(reply_delay);
                joybus_send_bytes(&_port, (uint8_t *)&default_eeprom4k_status, sizeof(n64_status_t));
                break;
            case N64Command::READ_EEPROM:
                joybus_receive_bytes(&_port, &operation.page, 1, receive_timeout_us, false);
                // Set timeout for how long to wait until we can send response because we don't
                // want to reply before the console is done sending.
                _receive_end = make_timeout_time_us(reply_delay);
                return operation;
            case N64Command::WRITE_EEPROM:
                joybus_receive_bytes(&_port, &operation.page, 1, receive_timeout_us, false);
                joybus_receive_bytes(&_port, operation.data, 8, receive_timeout_us, false);
                // Set timeout for how long to wait until we can send response because we don't
                // want to reply before the console is done sending.
                _receive_end = make_timeout_time_us(reply_delay);
                return operation;
            default:
                // If we received an invalid command, wait long enough for command
                // to finish, then reset receiving.
                busy_wait_us(reset_wait_period_us);
                joybus_port_reset(&_port);
        }
    }
}

void __no_inline_not_in_flash_func(N64EEPROM4k::SendData)(uint8_t *data, uint8_t length) {
    // Wait for receive timeout to end before responding.
    while (!time_reached(_receive_end)) {
        tight_loop_contents();
    }

    joybus_send_bytes(&_port, data, length);
}

int N64EEPROM4k::GetOffset() {
    return _port.offset;
}
