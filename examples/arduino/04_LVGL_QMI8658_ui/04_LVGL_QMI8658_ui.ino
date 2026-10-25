#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "lv_conf.h"
#include <Arduino.h>
#include <Wire.h>
#include "ImuDrv.hpp"

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

alignas(LV_DRAW_BUF_ALIGN) static uint16_t buf[LCD_WIDTH * LCD_HEIGHT / 10];

SensorQMI8658 qmi;

AccelerometerData acc;
GyroscopeData gyr;
constexpr float kStandardGravity = 9.80665f;  // SensorLib reports m/s^2; the chart plots g

lv_obj_t *label;                  // Global label object
lv_obj_t *chart;                  // Global chart object
lv_chart_series_t *acc_series_x;  // Acceleration X series
lv_chart_series_t *acc_series_y;  // Acceleration Y series
lv_chart_series_t *acc_series_z;  // Acceleration Z series

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS /* CS */, LCD_SCLK /* SCK */, LCD_SDIO0 /* SDIO0 */, LCD_SDIO1 /* SDIO1 */,
  LCD_SDIO2 /* SDIO2 */, LCD_SDIO3 /* SDIO3 */);

Arduino_CO5300 *gfx = new Arduino_CO5300(
  bus, LCD_RESET /* RST */, 0 /* rotation */, LCD_WIDTH /* width */, LCD_HEIGHT /* height */, 0, 0, 0, 0);

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
  Serial.printf(buf);
  Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

  lv_display_flush_ready(disp);
}

void example_increase_lvgl_tick(void *arg) {
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */
  Wire.begin(IIC_SDA, IIC_SCL);

  gfx->begin();
  gfx->setBrightness(200);
  bus->writeC8D8(0x36, 0xA0);

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  Serial.println("I am LVGL_Arduino");

  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  /*Initialize the display*/
  lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Hello Ardino and LVGL!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };

  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

  label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "Initializing...");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  /* Create chart */
  chart = lv_chart_create(lv_screen_active());
  lv_obj_set_size(chart, 240, 280);
  lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);              /* Set the type to line */
  lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, -3, 3); /* Set the range of y axis */
  lv_chart_set_point_count(chart, 20);                       /* Set the number of data points */
  acc_series_x = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  acc_series_y = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  acc_series_z = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

  Serial.println("Setup done");

if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Serial.println("Failed to find QMI8658 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }

  qmi.configAccel(AccelFullScaleRange::FS_4G, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
  qmi.enableAccel();

  Serial.println("Read data now...");
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);

  if (qmi.isDataReady(static_cast<uint8_t>(ImuBase::DataReadyMask::ACCEL))) {
    if (qmi.readAccel(acc)) {
      const float ax = acc.mps2.x / kStandardGravity;
      const float ay = acc.mps2.y / kStandardGravity;
      const float az = acc.mps2.z / kStandardGravity;
      Serial.print("{ACCEL: ");
      Serial.print(ax);
      Serial.print(",");
      Serial.print(ay);
      Serial.print(",");
      Serial.print(az);
      Serial.println("}");

      // Update chart with new accelerometer data
      lv_chart_set_next_value(chart, acc_series_x, ax);
      lv_chart_set_next_value(chart, acc_series_y, ay);
      lv_chart_set_next_value(chart, acc_series_z, az);
    }

    if (qmi.readGyro(gyr)) {
      Serial.print("{GYRO: ");
      Serial.print(gyr.dps.x);
      Serial.print(",");
      Serial.print(gyr.dps.y);
      Serial.print(",");
      Serial.print(gyr.dps.z);
      Serial.println("}");
    }
  }
  delay(20);  // Increase the frequency of data polling
}
