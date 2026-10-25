# ESP-Brookesia 说明

[English](brookesia.md) · [首页](../README_ZH.md)

`examples/esp-idf/03_esp-brookesia` 是第一方富 UI 示例，基于嵌入的上游
`brookesia_core`（0.6.0-beta2）副本。该副本包含针对 LVGL 9.6 的本地修改：用逐标志的
`lv_obj_set_<flag>()` 代替已弃用的标志 API（运行时标志掩码通过
`gui/lvgl/esp_brookesia_lv_flags.h` 处理）、改用公共 `lvgl.h` 头文件，以及
`LV_DISPLAY_ROTATION_*` 命名。与上游同步时需重新应用这些修改。本地 app 和资源属于产品组合；没有组件等价证据时不会删除。
该示例仍在两条要求的 ESP-IDF 矩阵线中。

现有组件配置已禁用可选 AI 框架。本文不做超出 CI 矩阵的兼容性承诺，也不翻译或修改上游树文档。
