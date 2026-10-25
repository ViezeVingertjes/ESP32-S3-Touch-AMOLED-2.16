#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "TouchDrvCST.hpp"
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include "HWCDC.h"
#include <ImuDrv.hpp>
#include <Wire.h>

SensorQMI8658 qmi;
AccelerometerData acc;
constexpr float kStandardGravity = 9.80665f;  // SensorLib reports m/s^2; tilt thresholds are in g
float angleX = 1;
float angleY = 0;

bool rotation = false;

HWCDC USBSerial;
#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

uint32_t screenWidth;
uint32_t screenHeight;

lv_display_t *disp;
uint8_t *rotated_buf;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS /* CS */, LCD_SCLK /* SCK */, LCD_SDIO0 /* SDIO0 */, LCD_SDIO1 /* SDIO1 */,
  LCD_SDIO2 /* SDIO2 */, LCD_SDIO3 /* SDIO3 */);

Arduino_CO5300 *gfx = new Arduino_CO5300(
  bus, LCD_RESET /* RST */, 0 /* rotation */, LCD_WIDTH /* width */, LCD_HEIGHT /* height */, 0, 0, 0, 0);

TouchDrvCST92xx touch;
volatile bool touchPending = false;

void IRAM_ATTR onTouchInterrupt() {
  touchPending = true;
}

bool takeTouchInterrupt() {
  noInterrupts();
  const bool pending = touchPending;
  touchPending = false;
  interrupts();
  return pending;
}

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
  Serial.printf(buf);
  Serial.flush();
}
#endif

void example_lvgl_rounder_cb(lv_event_t *e)
{
    lv_area_t *area = (lv_area_t *)lv_event_get_param(e);
    if(area->x1 % 2 !=0)area->x1--;
    if(area->y1 % 2 !=0)area->y1--;
    // 变为奇数(如果是偶数就加 1)
    if(area->x2 %2 ==0)area->x2++;
    if(area->y2 %2 ==0)area->y2++;
}

/* Display flushing */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  // LVGL 9 does not rotate in software by itself; rotate the rendered area before sending it.
  lv_display_rotation_t rotation = lv_display_get_rotation(disp);
  lv_area_t rotated_area;
  if (rotation != LV_DISPLAY_ROTATION_0) {
    lv_color_format_t cf = lv_display_get_color_format(disp);
    rotated_area = *area;
    lv_display_rotate_area(disp, &rotated_area);
    uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
    uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
    lv_draw_rotate(px_map, rotated_buf, lv_area_get_width(area), lv_area_get_height(area), src_stride, dest_stride, rotation, cf);
    area = &rotated_area;
    px_map = rotated_buf;
  }

  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

  lv_display_flush_ready(disp);
}

void example_increase_lvgl_tick(void *arg) {
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  if (!takeTouchInterrupt()) {
    return;
  }

  const TouchPoints &points = touch.getTouchPoints();
  if (points.hasPoints()) {
    const TouchPoint &point = points.getPoint(0);
    data->state = LV_INDEV_STATE_PRESSED;

    /*Set the coordinates*/
    data->point.x = point.x;
    data->point.y = point.y;

    USBSerial.print("Data x ");
    USBSerial.print(point.x);

    USBSerial.print("Data y ");
    USBSerial.println(point.y);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup() {
  USBSerial.begin(115200); /* prepare for possible serial debug */

  touch.setPins(TP_RST, TP_INT);
  bool result = touch.begin(Wire, CST92XX_SLAVE_ADDRESS, IIC_SDA, IIC_SCL);
  if (result == false) {
    Serial.println("touch is not online...");
    while (1) delay(1000);
  }
  Serial.print("Model :");
  Serial.println(touch.getModelName());
  touch.setCoverScreenCallback([](void *ptr) {
    Serial.print(millis());
    Serial.println(" : The screen is covered");
  },
                               NULL);
  touch.setMaxCoordinates(480, 480);
  touch.setSwapXY(true);
  touch.setMirrorXY(true, false);
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), onTouchInterrupt, FALLING);

  // SensorLib names the schematic-strapped 0x6B address as "L".
  if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Serial.println("Failed to find QMI8658 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }

  // 设置加速度计
  qmi.configAccel(AccelFullScaleRange::FS_4G, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
  qmi.enableAccel();

  gfx->begin();
  gfx->setBrightness(200);
  bus->writeC8D8(0x36, 0xA0);

  screenWidth = gfx->width();
  screenHeight = gfx->height();

  lv_init();

  const size_t buf_size = screenWidth * screenHeight / 4 * sizeof(uint16_t);
  uint8_t *buf1 = (uint8_t *)heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, buf_size, MALLOC_CAP_DMA);
  uint8_t *buf2 = (uint8_t *)heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, buf_size, MALLOC_CAP_DMA);
  rotated_buf = (uint8_t *)heap_caps_aligned_alloc(LV_DRAW_BUF_ALIGN, buf_size, MALLOC_CAP_DMA);

  String LVGL_Arduino = "Hello Arduino! ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  USBSerial.println(LVGL_Arduino);
  USBSerial.println("I am LVGL_Arduino");



#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  /*Initialize the display*/
  disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_add_event_cb(disp, example_lvgl_rounder_cb, LV_EVENT_INVALIDATE_AREA, NULL);
  lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

  /*Initialize the touch input device*/
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };

  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

  // lv_demo_widgets();
  // lv_demo_benchmark();
  // lv_demo_keypad_encoder();
  lv_demo_music();
  // lv_demo_stress();

  USBSerial.println("Setup done");
}

void loop() {
  if (qmi.isDataReady(static_cast<uint8_t>(ImuBase::DataReadyMask::ACCEL))) {
    if (qmi.readAccel(acc)) {
      angleX = acc.mps2.x / kStandardGravity;
      angleY = acc.mps2.y / kStandardGravity;
      if (angleX > 0.8 && !rotation) {
        lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
        rotation = true;
      } else if (angleX < -0.8 && !rotation) {
        lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
        rotation = true;
      } else if (angleY < -0.8 && !rotation) {
        lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_180);
        rotation = true;
      } else if (angleY > 0.8 && !rotation) {
        lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);
        rotation = true;
      }
      if ((angleX <= 0.8 && angleX >= -0.8) && (angleY <= 0.8 && angleY >= -0.8)) {
        rotation = false;  // 允许重新执行旋转
      }
    }
  }

  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}
