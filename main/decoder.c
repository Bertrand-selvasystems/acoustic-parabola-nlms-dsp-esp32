#include "decoder.h"

#include "filtre.h"

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
 * @brief Tâche d'envoi des données audio traitées au périphérique I2S.
 *
 * Cette tâche lit les données depuis la queue de traitement et les envoie directement
 * au périphérique I2S pour la sortie audio.
 *
 * @param pvParameters Paramètres de la tâche (non utilisés).
 */
void output_task(void *pvParameters) {
  size_t bytes_written;
  int32_t *buffer_to_output;  // Directement en int32_t pour correspondre aux données stéréo

  while (1) {
    // Recevoir un buffer stéréo traité de la tâche de traitement
    if (xQueueReceive(processing_to_output_queue, &buffer_to_output, portMAX_DELAY) == pdTRUE) {
      // Envoyer directement le buffer stéréo au décodeur I2S
      if (i2s_write(I2S_NUM_OUT, (void *)buffer_to_output, BLOCK_SIZE * 2 * sizeof(int32_t), &bytes_written, portMAX_DELAY) != ESP_OK) {
        ESP_LOGE("OUT_TASK", "Erreur lors de l'envoi I2S");
      }
    }
  }
}

/**
 * @brief Tâche générant un onde monochromatique de test et l'envoyant au périphérique I2S.
 *
 * Cette tâche génère une sinusoïde à une fréquence donnée, puis l'envoie continuellement
 * au périphérique I2S pour des tests audio.
 *
 * @param pvParameters Paramètres de la tâche (non utilisés).
 */
void test_signal_task(void *pvParameters) {
  const int sample_rate = I2S_SAMPLE_RATE;  // Fréquence d'échantillonnage
  const int frequency = 1000;               // Fréquence du signal (exemple : 440 Hz pour un La)
  const float amplitude = 0.05f;            // Amplitude du signal (entre 0 et 1)

  int32_t *sin_buffer = (int32_t *)malloc(BLOCK_SIZE * 2 * sizeof(int32_t));  // Stéréo
  if (sin_buffer == NULL) {
    ESP_LOGE("test_signal_task", "Failed to allocate memory for sin_buffer");
    vTaskDelete(NULL);
  }

  ESP_LOGI("test_signal_task", "Generating sine wave...");

  // Pré-calcul des échantillons pour une période de la sinusoïde
  const int samples_per_cycle = sample_rate / frequency;
  int32_t *sin_table = (int32_t *)malloc(samples_per_cycle * sizeof(int32_t));
  if (sin_table == NULL) {
    ESP_LOGE("test_signal_task", "Failed to allocate memory for sin_table");
    free(sin_buffer);
    vTaskDelete(NULL);
  }

  for (int i = 0; i < samples_per_cycle; i++) {
    float angle = 2.0f * M_PI * (float)i / (float)samples_per_cycle;  // Angle pour chaque échantillon
    sin_table[i] = (int32_t)(amplitude * INT32_MAX * sinf(angle));    // Valeurs 32 bits pour le signal
  }

  // Boucle principale
  size_t bytes_written;
  int table_index = 0;
  while (1) {
    for (int i = 0; i < BLOCK_SIZE * 2; i += 2) {
      sin_buffer[i] = sin_table[table_index];      // Canal gauche
      sin_buffer[i + 1] = sin_table[table_index];  // Canal droit (stéréo)
      table_index = (table_index + 1) % samples_per_cycle;
    }

    // Envoyer le buffer au périphérique I2S
    i2s_write(I2S_NUM_OUT, sin_buffer, BLOCK_SIZE * 2 * sizeof(int32_t), &bytes_written, portMAX_DELAY);
  }

  free(sin_table);
  free(sin_buffer);
  vTaskDelete(NULL);
}

/**
 * @brief Initialisation du périphérique I2S pour la sortie audio.
 *
 * Configure les paramètres du périphérique I2S, comme le mode, la fréquence d'échantillonnage,
 * le nombre de bits par échantillon, et les broches associées.
 */
void i2s_init_out() {
  i2s_config_t i2s_config_out = {
      .mode = I2S_MODE_MASTER | I2S_MODE_TX,
      .sample_rate = I2S_SAMPLE_RATE,
      .bits_per_sample = I2S_SAMPLE_BITS,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 1024,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config_out = {
      .bck_io_num = I2S_BCK_PIN_OUT,
      .ws_io_num = I2S_WS_PIN_OUT,
      .data_out_num = I2S_DATA_OUT_PIN,
      .data_in_num = I2S_PIN_NO_CHANGE};

  ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_OUT, &i2s_config_out, 0, NULL));
  ESP_ERROR_CHECK(i2s_set_pin(I2S_NUM_OUT, &pin_config_out));
  ESP_ERROR_CHECK(i2s_set_clk(I2S_NUM_OUT, I2S_SAMPLE_RATE, I2S_SAMPLE_BITS, I2S_CHANNEL_STEREO));
  i2s_zero_dma_buffer(I2S_NUM_OUT);
}
