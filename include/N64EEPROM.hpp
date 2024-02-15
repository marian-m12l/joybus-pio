#ifndef _JOYBUS_N64EEPROM_HPP
#define _JOYBUS_N64EEPROM_HPP

#include "joybus.h"
#include "n64_definitions.h"

#include <hardware/pio.h>
#include <pico/stdlib.h>


typedef struct __attribute__((packed)) {
  N64Command command;
  uint8_t page;
  uint8_t data[8];
} n64_eeprom_operation_t;

enum class N64EEPROMType {
    EEPROM_4K = 0x8000,
    EEPROM_16K = 0xC000,
};


class N64EEPROM {
  public:
    /**
     * @brief Construct a new N64EEPROM object
     *
     * @param type The type of EEPROM (4k or 16k)
     * @param pin The GPIO pin that the N64 console's data line is connected to
     * @param pio The PIO instance; either pio0 or pio1. Default is pio0.
     * @param sm The PIO state machine to run the joybus instance on. Default is to automatically
     * claim an unused one.
     * @param offset The instruction memory offset at which to load the PIO program. Default is to
     * allocate automatically.
     */
    N64EEPROM(N64EEPROMType type, uint pin, PIO pio = pio0, int sm = -1, int offset = -1);

    /**
     * @brief Cleanly terminate the joybus PIO instance, freeing the state machine, and uninstalling
     * the joybus program from the PIO instance
     */
    ~N64EEPROM();

    /**
     * @brief Block until a read or write command is received from the N64 console. Automatically responds to
     * any probe/origin commands received in the process.
     *
     * @return the operation including command, page, and optional data
     */
    n64_eeprom_operation_t WaitForCommand();

    /**
     * @brief Send data to a connected N64 console
     *
     * @param data The buffer to send
     * @param length The buffer length
     */
    void SendData(uint8_t *data, uint8_t length);

    /**
     * @brief Get the offset at which the PIO program was installed. Useful if you want to
     * communicate with multiple joybus devices without having to load multiple copies of the PIO
     * program.
     *
     * @return The offset at which the PIO program was installed
     */
    int GetOffset();

  private:
    static constexpr uint incoming_bit_length_us = 5;
    static constexpr uint max_command_bytes = 1;
    static constexpr uint receive_timeout_us = incoming_bit_length_us * 10;
    static constexpr uint reset_wait_period_us =
        (incoming_bit_length_us * 8) * (max_command_bytes - 1) + receive_timeout_us;
    static constexpr uint64_t reply_delay = incoming_bit_length_us - 1 + 2; // FIXME Delay was apparently too short

    joybus_port_t _port;
    absolute_time_t _receive_end;
    n64_status_t _eeprom_status;
    
};

#endif
