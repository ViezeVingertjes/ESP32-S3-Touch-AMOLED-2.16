#include <Arduino.h>
#include <ESP_I2S.h>
#include <esp_vad.h>
#include "es7210.h"
#include "pin_config.h"

#define VAD_SAMPLE_RATE_HZ 16000
#define VAD_FRAME_LENGTH_MS 30
#define VAD_BUFFER_LENGTH (VAD_FRAME_LENGTH_MS * VAD_SAMPLE_RATE_HZ / 1000)

I2SClass i2s;
int16_t *vad_buff;
vad_handle_t vad_inst;

void setup() {

  Serial.begin(115200);
  Serial.printf("psram size : %lu kb\r\n", (unsigned long)(ESP.getPsramSize() / 1024));
  Serial.printf("FLASH size : %lu kb\r\n", (unsigned long)(ESP.getFlashChipSize() / 1024));
  Wire.begin(15, 14);

  delay(200);

  uint32_t ret_val = ESP_OK;

  audio_hal_codec_config_t cfg = {
    .adc_input = AUDIO_HAL_ADC_INPUT_ALL,
    .codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE,
    .i2s_iface = {
      .mode = AUDIO_HAL_MODE_SLAVE,
      .fmt = AUDIO_HAL_I2S_NORMAL,
      .samples = AUDIO_HAL_16K_SAMPLES,
      .bits = AUDIO_HAL_BIT_LENGTH_16BITS,
    },
  };

  ret_val |= es7210_adc_init(&Wire, &cfg);
  ret_val |= es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
  ret_val |= es7210_adc_set_gain(
    (es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2),
    (es7210_gain_value_t)GAIN_0DB);
  ret_val |= es7210_adc_set_gain(
    (es7210_input_mics_t)(ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4),
    (es7210_gain_value_t)GAIN_37_5DB);
  ret_val |= es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);



  // Standard I2S master RX, left slot only (MIC1), 256x MCLK for the ES7210.
  i2s.setPins(PIN_ES7210_BCLK, PIN_ES7210_LRCK, -1 /* DOUT */, PIN_ES7210_DIN, PIN_ES7210_MCLK);
  if (!i2s.begin(I2S_MODE_STD, VAD_SAMPLE_RATE_HZ, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
    while (1) {
      Serial.println("Failed to initialize I2S!");
      delay(1000);
    }
  }


  vad_inst = vad_create(VAD_MODE_0);
  vad_buff = (int16_t *)malloc(VAD_BUFFER_LENGTH * sizeof(short));
  if (vad_buff == NULL) {
    while (1) {
      Serial.println("Memory allocation failed!");
      delay(1000);
    }
  }
}

void loop() {
  i2s.readBytes((char *)vad_buff, VAD_BUFFER_LENGTH * sizeof(short));
  // Feed samples to the VAD process and get the result
  vad_state_t vad_state = vad_process(vad_inst, vad_buff, VAD_SAMPLE_RATE_HZ, VAD_FRAME_LENGTH_MS);
  if (vad_state == VAD_SPEECH) {
    Serial.print(millis());
    Serial.println("Speech detected");
  }
  delay(5);
}