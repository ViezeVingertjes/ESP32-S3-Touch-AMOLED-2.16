# ESP-Brookesia notes

[简体中文](brookesia_ZH.md) · [Home](../README.md)

`examples/esp-idf/03_esp-brookesia` is a first-party rich UI example built on an
embedded copy of upstream `brookesia_core` (0.6.0-beta2). That copy carries
local changes for LVGL 9.6: per-flag `lv_obj_set_<flag>()` setters instead of
the deprecated flag API (with `gui/lvgl/esp_brookesia_lv_flags.h` for runtime
flag masks), public `lvgl.h` includes, and `LV_DISPLAY_ROTATION_*` names.
Re-apply them when syncing with upstream. Its local app and assets are product
composition; they are not removed without component-equivalence
evidence. The example remains in both required ESP-IDF matrix lines.

The optional AI framework is disabled by the existing component configuration.
No compatibility claim beyond the CI matrix is made here, and no upstream tree
documentation is translated or modified.
