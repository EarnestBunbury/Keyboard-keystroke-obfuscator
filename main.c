C
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"

#define BUFFER_SIZE 1024
#define SEND_BUTTON_PIN 15

static char local_buffer[BUFFER_SIZE];
static uint16_t buffer_index = 0;
static bool transmitting = false;

// Simple LCG pseudo-random generator for timing obfuscation
static uint32_t rng_state = 123456789;
static uint32_t fast_rand(void) {
    rng_state = (1103515245 * rng_state + 12345) % 2147483648;
    return rng_state;
}

// Convert HID keycode to ASCII (simplified lowercase/uppercase map)
char hid_keycode_to_ascii(uint8_t modifier, uint8_t keycode) {
    bool shift = (modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) != 0;
    
    if (keycode >= HID_KEY_A && keycode <= HID_KEY_Z) {
        return shift ? (keycode - HID_KEY_A + 'A') : (keycode - HID_KEY_A + 'a');
    }
    if (keycode >= HID_KEY_1 && keycode <= HID_KEY_9) {
        if (shift) {
            char symbols[] = { '!', '@', '#', '$', '%', '^', '&', '*', '(' };
            return symbols[keycode - HID_KEY_1];
        }
        return (keycode - HID_KEY_1 + '1');
    }
    if (keycode == HID_KEY_0) return shift ? ')' : '0';
    if (keycode == HID_KEY_SPACE) return ' ';
    if (keycode == HID_KEY_ENTER) return '\n';
    if (keycode == HID_KEY_COMMA) return shift ? '<' : ',';
    if (keycode == HID_KEY_DOT) return shift ? '>' : '.';
    if (keycode == HID_KEY_SLASH) return shift ? '?' : '/';
    
    return 0;
}

// Intercept incoming keys from physical keyboard (Host Stack Callback)
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    if (transmitting) return; // Ignore input while playing back payload

    hid_keyboard_report_t const *p_kbd_report = (hid_keyboard_report_t const *) report;
    
    static hid_keyboard_report_t prev_report = { 0 };

    for (uint8_t i = 0; i < 6; i++) {
        uint8_t keycode = p_kbd_report->keycode[i];
        if (keycode == 0) continue;

        // Check if this key was already pressed (avoid repeat flooding)
        bool exists = false;
        for (uint8_t j = 0; j < 6; j++) {
            if (prev_report.keycode[j] == keycode) {
                exists = true;
                break;
            }
        }
        if (exists) continue;

        // Handle Backspace locally
        if (keycode == HID_KEY_BACKSPACE) {
            if (buffer_index > 0) {
                buffer_index--;
                local_buffer[buffer_index] = '\0';
            }
        } 
        else {
            char c = hid_keycode_to_ascii(p_kbd_report->modifier, keycode);
            if (c != 0 && buffer_index < BUFFER_SIZE - 1) {
                local_buffer[buffer_index++] = c;
                local_buffer[buffer_index] = '\0';
            }
        }
    }
    prev_report = *p_kbd_report;
    tuh_hid_receive_report(dev_addr, instance);
}

// Transmit buffered text with randomized human-like cadence to host OS
void execute_synthetic_transmission(void) {
    if (buffer_index == 0) return;
    transmitting = true;

    // Ensure device stack is ready to output to host
    for (uint16_t i = 0; i < buffer_index; i++) {
        char c = local_buffer[i];
        
        // Map character to HID report payload (omitting full lookup array for brevity)
        uint8_t hid_key = 0;
        uint8_t mod = 0;
        
        if (c >= 'a' && c <= 'z') { hid_key = HID_KEY_A + (c - 'a'); }
        else if (c >= 'A' && c <= 'Z') { hid_key = HID_KEY_A + (c - 'A'); mod = KEYBOARD_MODIFIER_LEFTSHIFT; }
        else if (c == ' ') { hid_key = HID_KEY_SPACE; }
        else if (c == '\n') { hid_key = HID_KEY_ENTER; }
        else if (c == '.') { hid_key = HID_KEY_DOT; }

        if (hid_key != 0) {
            // Send Key Down
            uint8_t keycode[6] = { hid_key };
            tud_hid_keyboard_report(0, mod, keycode);
            sleep_ms(35 + (fast_rand() % 30); // Randomized dwell time

            // Send Key Up
            uint8_t empty[6] = { 0 };
            tud_hid_keyboard_report(0, 0, empty);
        }

        // Variable inter-character flight delay (clustering between 55ms and 175ms)
        uint32_t delay = 55 + (fast_rand() % 120);
        if (c == ' ' || c == '.' || c == '\n') {
            delay += 150; // Extra cognitive pause for spaces and punctuation
        }
        sleep_ms(delay);
    }

    // Clear buffer post-transmission
    memset(local_buffer, 0, BUFFER_SIZE);
    buffer_index = 0;
    transmitting = false;
}

int main(void) {
    board_init();
    tusb_init(); // Initialize dual host/device stack

    // Configure physical Send trigger button
    gpio_init(SEND_BUTTON_PIN);
    gpio_set_dir(SEND_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(SEND_BUTTON_PIN);

    while (true) {
        tud_task(); // TinyUSB device task (talking to computer)
        tuh_task(); // TinyUSB host task (talking to keyboard)

        // Check if the physical SEND button is pressed (active low)
        if (!transmitting && gpio_get(SEND_BUTTON_PIN) == 0) {
            sleep_ms(20); // Debounce
            if (gpio_get(SEND_BUTTON_PIN) == 0) {
                execute_synthetic_transmission();
            }
        }
    }
}
