#include <TFT_eSPI.h>
#include "driver/i2s.h"

#define BUTTON_PIN 5

#define I2S_BCLK_MIC 18
#define I2S_LRC_MIC 17
#define I2S_DIN_MIC 15

#define I2S_BCLK_SPK 3
#define I2S_LRC_SPK 8
#define I2S_DOUT_SPK 16
#define I2S_SD_SPK 46

const int SAMPLE_RATE = 16000;
const int BITS_PER_SAMPLE = 16;
const int RECORD_DURATION_SEC = 5; // Change for record Time

const int AUDIO_BUFFER_SIZE = RECORD_DURATION_SEC * SAMPLE_RATE * (BITS_PER_SAMPLE / 8);
int16_t *audio_buffer = NULL;

TFT_eSPI tft = TFT_eSPI();

void record_and_playback();

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(I2S_SD_SPK, OUTPUT);
  digitalWrite(I2S_SD_SPK, LOW);

  tft.init();
  tft.setRotation(3); // 0-3
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);
  tft.setCursor(10, 50);
  tft.println("Microphone Test");
  tft.setTextSize(1);
  tft.println("\n\nPress the button to record.");

  if (!psramFound()) { 
    tft.fillScreen(TFT_RED);
    tft.println("PSRAM ERROR!");
    while (1)
      ;
  }

  i2s_config_t i2s_rx_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 1024
  };
  i2s_pin_config_t i2s_rx_pin_config = {
    .bck_io_num = I2S_BCLK_MIC, .ws_io_num = I2S_LRC_MIC, .data_out_num = I2S_PIN_NO_CHANGE, .data_in_num = I2S_DIN_MIC
  };
  i2s_driver_install(I2S_NUM_0, &i2s_rx_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_rx_pin_config);

  i2s_config_t i2s_tx_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 1024
  };
  i2s_pin_config_t i2s_tx_pin_config = {
    .bck_io_num = I2S_BCLK_SPK, .ws_io_num = I2S_LRC_SPK, .data_out_num = I2S_DOUT_SPK, .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_NUM_1, &i2s_tx_config, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &i2s_tx_pin_config);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    while (digitalRead(BUTTON_PIN) == LOW)
      ;
    delay(100);
    record_and_playback();
  }
}

void record_and_playback() {
  tft.fillScreen(TFT_ORANGE);
  tft.setCursor(10, 50);
  tft.println("Allocating memory\nin PSRAM...");

  audio_buffer = (int16_t *)heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (audio_buffer == NULL) {
    Serial.println("ERROR: Could not allocate memory in PSRAM!");
    tft.fillScreen(TFT_RED);
    tft.println("PSRAM ERROR!");
    return;
  }
  Serial.println("Buffer created in PSRAM.");

  tft.fillScreen(TFT_RED);
  tft.setCursor(10, 50);
  tft.printf("Recording...\n(%d seconds)", RECORD_DURATION_SEC);
  Serial.println("Recording starts...");

  delay(250);

  size_t bytes_read;
  i2s_read(I2S_NUM_0, audio_buffer, AUDIO_BUFFER_SIZE, &bytes_read, portMAX_DELAY);

  Serial.printf("%d bytes of audio data recorded.\n", bytes_read);

  digitalWrite(I2S_SD_SPK, HIGH);
  tft.fillScreen(TFT_GREEN);
  tft.setCursor(10, 50);
  tft.println("Playing back...");
  Serial.println("Playback starts...");

  size_t bytes_written;
  delay(20);
  i2s_write(I2S_NUM_1, audio_buffer, bytes_read, &bytes_written, portMAX_DELAY);

  Serial.printf("%d bytes of audio data played back.\n", bytes_written);

  free(audio_buffer);
  audio_buffer = NULL;
  digitalWrite(I2S_SD_SPK, LOW);
  tft.fillScreen(TFT_BLUE);
  tft.setTextSize(2);
  tft.setCursor(10, 50);
  tft.println("Test Finished");
  tft.setTextSize(1);
  tft.println("\n\nPress the button for a new recording.");
}