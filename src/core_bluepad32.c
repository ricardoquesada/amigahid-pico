// SPDX-License-Identifier: EPL-2.0
// Copyright 2024 Ricardo Quesada

#include "core_bluepad32.h"

#include <stddef.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <pico/util/queue.h>

#include <uni.h>

#include "sdkconfig.h"

// Bluepad32 / TinyUSB queue
static queue_t bp32_tuh_queue;

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t *d);
// Defined in usb_hid.c
void handle_bp32_keyboard_report(const bp32_queue_entry_t *entry);

//
// Platform Overrides
//
static void my_platform_init(int argc, const char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("my_platform: init()\n");

#if 0
    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // Inverted axis with inverted Y in RY.
    mappings.axis_x = UNI_GAMEPAD_MAPPINGS_AXIS_RX;
    mappings.axis_y = UNI_GAMEPAD_MAPPINGS_AXIS_RY;
    mappings.axis_ry_inverted = true;
    mappings.axis_rx = UNI_GAMEPAD_MAPPINGS_AXIS_X;
    mappings.axis_ry = UNI_GAMEPAD_MAPPINGS_AXIS_Y;

    // Invert A & B
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;

    uni_gamepad_set_mappings(&mappings);
#endif
}

static void my_platform_on_init_complete(void)
{
    logi("my_platform: on_init_complete()\n");

    // Safe to call "unsafe" functions since they are called from BT thread

    // Start scanning
    uni_bt_enable_new_connections_unsafe(true);

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (1)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();

    // Turn off LED once init is done.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    //    uni_bt_service_set_enabled(true);

    uni_property_dump_all();
}

static void my_platform_on_device_connected(uni_hid_device_t *d)
{
    logi("my_platform: device connected: %p\n", d);
}

static void my_platform_on_device_disconnected(uni_hid_device_t *d)
{
    logi("my_platform: device disconnected: %p\n", d);
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t *d)
{
    logi("my_platform: device ready: %p\n", d);

    // You can reject the connection by returning an error.
    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_controller_data(uni_hid_device_t *d, uni_controller_t *ctl)
{
    static uni_controller_t prev = {0};
    bp32_queue_entry_t entry;

    if (memcmp(&prev, ctl, sizeof(*ctl)) == 0) {
        return;
    }
    prev = *ctl;
    // Print device Id before dumping gamepad.
//    logi("(%p) id=%d ", d, uni_hid_device_get_idx_for_instance(d));
//    uni_controller_dump(ctl);

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            // Do something
            break;
        case UNI_CONTROLLER_CLASS_BALANCE_BOARD:
            // Do something
            break;
        case UNI_CONTROLLER_CLASS_MOUSE:
            // Do something
            break;
        case UNI_CONTROLLER_CLASS_KEYBOARD:
            // Do something
            entry.modifier = d->controller.keyboard.modifiers;
            // TinyUSB has 6 entries, while BP32 supports 10. Use the TinyUSB.
            memcpy(entry.keycode, d->controller.keyboard.pressed_keys, sizeof(entry.keycode));
            queue_add_blocking(&bp32_tuh_queue, &entry);
            break;
        default:
            loge("Unsupported controller class: %d\n", ctl->klass);
            break;
    }
}

static const uni_property_t *my_platform_get_property(uni_property_idx_t idx)
{
    ARG_UNUSED(idx);
    return NULL;
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void *data)
{
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON:
            // Optional: do something when "system" button gets pressed.
            trigger_event_on_gamepad((uni_hid_device_t *)data);
            break;

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            // When the "bt scanning" is on / off. Could be triggered by different events
            // Useful to notify the user
            logi("my_platform_on_oob_event: Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("my_platform_on_oob_event: unsupported event: 0x%04x\n", event);
    }
}

//
// Helpers
//
static void trigger_event_on_gamepad(uni_hid_device_t *d)
{
    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 50 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        static uint8_t led = 0;
        led += 1;
        led &= 0xf;
        d->report_parser.set_player_leds(d, led);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        static uint8_t red = 0x10;
        static uint8_t green = 0x20;
        static uint8_t blue = 0x40;

        red += 0x10;
        green -= 0x20;
        blue += 0x40;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

//
// Entry Point
//
struct uni_platform *get_my_platform(void)
{
    static struct uni_platform plat = {
        .name = "My Platform",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_oob_event = my_platform_on_oob_event,
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
    };

    return &plat;
}

// Runs on Core 1
static void bp32_main_core1(void)
{
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return;
    }

    // Must be called before uni_init()
    uni_platform_set_custom(get_my_platform());

    // Initialize BP32
    uni_init(0, NULL);

    // Does not return.
    btstack_run_loop_execute();
}

// Runs on Core 0
void bp32_init(void)
{
    stdio_init_all();
    queue_init(&bp32_tuh_queue, sizeof(bp32_queue_entry_t), 10);

    multicore_launch_core1(bp32_main_core1);
}

//
// Communication with TinyUSB / AmigaHid
//

// Runs on Core0
void bp32_task(void)
{
    bp32_queue_entry_t entry;

    while (!queue_is_empty(&bp32_tuh_queue)) {
        queue_remove_blocking(&bp32_tuh_queue, &entry);
        handle_bp32_keyboard_report(&entry);
    }
}
