#include "micros.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DECLARATION DES VARIABLES STATIC
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DECLARATION DES FONCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Tâche d'acquisition des données audio depuis le périphérique I2S.
 *
 * Cette tâche lit les données I2S dans des buffers cycliques, puis les envoie
 * à la tâche de traitement via une queue.
 *
 * @param pvParameters Paramètres de la tâche (non utilisés).
 */
void acquisition_task(void *pvParameters) {
  size_t bytes_read;
  int write_index = 0;  // Index du buffer actif

  while (1) {
    int32_t *current_buffer = buffers[write_index];

    // Lecture des données I2S
    if (i2s_read(I2S_NUM, (void *)current_buffer, BLOCK_SIZE * 2 * sizeof(int32_t), &bytes_read, portMAX_DELAY) == ESP_OK) {
      // Transmettre le buffer rempli à la tâche de traitement
      if (xQueueSend(acquisition_to_processing_queue, &current_buffer, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW("ACQ_TASK", "Queue pleine! Données perdues.");
      }

      // Passer au buffer suivant
      write_index = (write_index + 1) % 2;
    } else {
      ESP_LOGE("ACQ_TASK", "Erreur lors de la lecture I2S");
    }
  }
}

/**
 * @brief Initialisation du périphérique I2S pour l'entrée audio.
 *
 * Configure les paramètres du périphérique I2S pour l'acquisition audio,
 * comme le mode, la fréquence d'échantillonnage, le nombre de bits par échantillon,
 * et les broches associées.
 */
void i2s_init_in() {
  i2s_config_t i2s_config = {
      .mode = I2S_MODE_MASTER | I2S_MODE_RX,
      .sample_rate = I2S_SAMPLE_RATE,
      .bits_per_sample = I2S_SAMPLE_BITS,
      .channel_format = I2S_FORMAT,
      .communication_format = I2S_COMM_FORMAT,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 64,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCK_PIN,
      .ws_io_num = I2S_WS_PIN,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_DATA_IN_PIN};

  ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL));
  ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM, &pin_config));
  ESP_ERROR_CHECK(i2s_set_clk(I2S_NUM, I2S_SAMPLE_RATE, I2S_SAMPLE_BITS, I2S_CHANNEL_STEREO));
  i2s_zero_dma_buffer(I2S_NUM);
}
