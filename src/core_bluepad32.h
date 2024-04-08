// SPDX-License-Identifier: EPL-2.0
// Copyright 2024 Ricardo Quesada

#ifndef CORE_BLUEPAD32_H
#define CORE_BLUEPAD32_H

#include <stdint.h>

typedef enum {
    BP32_REPORT_TYPE_MOUSE,
    BP32_REPORT_TYPE_KEYBOARD,
} bp32_report_type_t;

// Mimics TinyUSB hid_keyboard_report_t and hid_mouse_report_t;
// Cannot include "class/class/hid.h" because some definitions from TinyUSB collide
// with definitions from BTstack. E.g.: HID_REPORT_TYPE_INPUT will collide with
typedef struct __attribute((packed)) {
    uint8_t modifier;   /**< Keyboard modifier (KEYBOARD_MODIFIER_* masks). */
    uint8_t reserved;   /**< Reserved for OEM use, always set to 0. */
    uint8_t keycode[6]; /**< Key codes of the currently pressed keys. */
} bp32_queue_kb_entry_t;

typedef struct __attribute((packed)) {
    uint8_t buttons; /**< buttons mask for currently pressed buttons in the mouse. */
    int8_t  x;       /**< Current delta x movement of the mouse. */
    int8_t  y;       /**< Current delta y movement on the mouse. */
    int8_t  wheel;   /**< Current delta wheel movement on the mouse. */
    int8_t  pan;     // using AC Pan
} bp32_queue_mouse_entry_t;

typedef struct __attribute((packed))
{
    bp32_report_type_t type;   // kb or mouse
    union {
        bp32_queue_kb_entry_t kb;
        bp32_queue_mouse_entry_t mouse;
    };
} bp32_queue_entry_t;

void bp32_init(void);
void bp32_task(void);

#endif // CORE_BLUEPAD32_H
