// SPDX-License-Identifier: EPL-2.0
// Copyright 2024 Ricardo Quesada

#ifndef CORE_BLUEPAD32_H
#define CORE_BLUEPAD32_H

#include <stdint.h>

// Mimics TinyUSB hid_keyboard_report_t;
// Cannot include "class/class/hid.h" because some definitions from TinyUSB collide
// with definitions from BTstack. E.g.: HID_REPORT_TYPE_INPUT will collide with
typedef struct __attribute((packed))
{
    uint8_t modifier;   /**< Keyboard modifier (KEYBOARD_MODIFIER_* masks). */
    uint8_t reserved;   /**< Reserved for OEM use, always set to 0. */
    uint8_t keycode[6]; /**< Key codes of the currently pressed keys. */
} bp32_queue_entry_t;

void bp32_init(void);
void bp32_task(void);

#endif // CORE_BLUEPAD32_H
