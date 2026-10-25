/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "lvgl.h"

/* LVGL 9.6 deprecates the OR-ed lv_obj_add/remove/has_flag() API. These helpers keep
 * runtime flag masks working by dispatching each bit to its dedicated setter/getter. */

#define ESP_BROOKESIA_LV_FLAG_TABLE(X) \
    X(LV_OBJ_FLAG_HIDDEN, lv_obj_set_hidden, lv_obj_is_hidden) \
    X(LV_OBJ_FLAG_CLICKABLE, lv_obj_set_clickable, lv_obj_is_clickable) \
    X(LV_OBJ_FLAG_CLICK_FOCUSABLE, lv_obj_set_click_focusable, lv_obj_is_click_focusable) \
    X(LV_OBJ_FLAG_CHECKABLE, lv_obj_set_checkable, lv_obj_is_checkable) \
    X(LV_OBJ_FLAG_SCROLLABLE, lv_obj_set_scrollable, lv_obj_is_scrollable) \
    X(LV_OBJ_FLAG_SCROLL_ELASTIC, lv_obj_set_scroll_elastic, lv_obj_is_scroll_elastic) \
    X(LV_OBJ_FLAG_SCROLL_MOMENTUM, lv_obj_set_scroll_momentum, lv_obj_is_scroll_momentum) \
    X(LV_OBJ_FLAG_SCROLL_ONE, lv_obj_set_scroll_one, lv_obj_is_scroll_one) \
    X(LV_OBJ_FLAG_SCROLL_CHAIN_HOR, lv_obj_set_scroll_chain_hor, lv_obj_is_scroll_chain_hor) \
    X(LV_OBJ_FLAG_SCROLL_CHAIN_VER, lv_obj_set_scroll_chain_ver, lv_obj_is_scroll_chain_ver) \
    X(LV_OBJ_FLAG_SCROLL_ON_FOCUS, lv_obj_set_scroll_on_focus, lv_obj_is_scroll_on_focus) \
    X(LV_OBJ_FLAG_SCROLL_WITH_ARROW, lv_obj_set_scroll_with_arrow, lv_obj_is_scroll_with_arrow) \
    X(LV_OBJ_FLAG_SNAPPABLE, lv_obj_set_snappable, lv_obj_is_snappable) \
    X(LV_OBJ_FLAG_PRESS_LOCK, lv_obj_set_press_lock, lv_obj_is_press_lock) \
    X(LV_OBJ_FLAG_EVENT_BUBBLE, lv_obj_set_event_bubble, lv_obj_is_event_bubble) \
    X(LV_OBJ_FLAG_GESTURE_BUBBLE, lv_obj_set_gesture_bubble, lv_obj_is_gesture_bubble) \
    X(LV_OBJ_FLAG_ADV_HITTEST, lv_obj_set_adv_hittest, lv_obj_is_adv_hittest) \
    X(LV_OBJ_FLAG_IGNORE_LAYOUT, lv_obj_set_ignore_layout, lv_obj_is_ignore_layout) \
    X(LV_OBJ_FLAG_FLOATING, lv_obj_set_floating, lv_obj_is_floating) \
    X(LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS, lv_obj_set_send_draw_task_events, lv_obj_is_send_draw_task_events) \
    X(LV_OBJ_FLAG_OVERFLOW_VISIBLE, lv_obj_set_overflow_visible, lv_obj_is_overflow_visible) \
    X(LV_OBJ_FLAG_EVENT_TRICKLE, lv_obj_set_event_trickle, lv_obj_is_event_trickle) \
    X(LV_OBJ_FLAG_STATE_TRICKLE, lv_obj_set_state_trickle, lv_obj_is_state_trickle) \
    X(LV_OBJ_FLAG_FLEX_IN_NEW_TRACK, lv_obj_set_flex_in_new_track, lv_obj_is_flex_in_new_track)

#define ESP_BROOKESIA_LV_USER_FLAGS \
    (LV_OBJ_FLAG_USER_1 | LV_OBJ_FLAG_USER_2 | LV_OBJ_FLAG_USER_3 | LV_OBJ_FLAG_USER_4)

/** Set (en = true) or clear (en = false) every flag in `flags`. */
static inline void esp_brookesia_lv_obj_set_flags(lv_obj_t *obj, uint32_t flags, bool en)
{
#define ESP_BROOKESIA_LV_SET_FLAG(flag, setter, getter) \
    if (flags & (uint32_t)(flag)) { setter(obj, en); flags &= ~(uint32_t)(flag); }
    ESP_BROOKESIA_LV_FLAG_TABLE(ESP_BROOKESIA_LV_SET_FLAG)
#undef ESP_BROOKESIA_LV_SET_FLAG
    for (uint32_t bit = 0; bit < 4; bit++) {
        if (flags & ((uint32_t)LV_OBJ_FLAG_USER_1 << bit)) {
            lv_obj_set_user_flag(obj, bit, en);
        }
    }
    flags &= ~(uint32_t)ESP_BROOKESIA_LV_USER_FLAGS;
    LV_ASSERT_MSG(flags == 0, "LVGL flag without a dedicated setter");
}

/** Return true if every flag in `flags` is set. */
static inline bool esp_brookesia_lv_obj_has_flags(const lv_obj_t *obj, uint32_t flags)
{
    bool result = true;
#define ESP_BROOKESIA_LV_HAS_FLAG(flag, setter, getter) \
    if (flags & (uint32_t)(flag)) { result = result && getter(obj); flags &= ~(uint32_t)(flag); }
    ESP_BROOKESIA_LV_FLAG_TABLE(ESP_BROOKESIA_LV_HAS_FLAG)
#undef ESP_BROOKESIA_LV_HAS_FLAG
    for (uint32_t bit = 0; bit < 4; bit++) {
        if (flags & ((uint32_t)LV_OBJ_FLAG_USER_1 << bit)) {
            result = result && lv_obj_get_user_flag((lv_obj_t *)obj, bit);
        }
    }
    flags &= ~(uint32_t)ESP_BROOKESIA_LV_USER_FLAGS;
    LV_ASSERT_MSG(flags == 0, "LVGL flag without a dedicated getter");
    return result;
}
