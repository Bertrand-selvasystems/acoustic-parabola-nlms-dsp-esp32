/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DEFINITION DES LIBRAIRIES
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
#include "initialisation.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DECLARATION DES VARIABLES GLOBALES
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

/** @brief Queue entre la tâche d'acquisition et la tâche de traitement. */
QueueHandle_t acquisition_to_processing_queue;

/** @brief Queue entre la tâche de traitement et la tâche de sortie. */
QueueHandle_t processing_to_output_queue;

/** @brief Queue pour transmettre le rapport signal/bruit (SNR) en dB. */
QueueHandle_t snr_db_queue;

/** @brief Buffer pour stocker les données audio du signal. */
int32_t buffer0[BLOCK_SIZE * 2];

/** @brief Buffer alternatif pour stocker les données stéréo du signal. */
int32_t buffer1[BLOCK_SIZE * 2];

/** @brief Tableau des buffers utilisés en alternance pour le stockage des données audio. */
int32_t *buffers[2] = {buffer0, buffer1};

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DEFINITION DES FONCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Fonction d'initialisation du système.
 *
 * Cette fonction initialise les queues FreeRTOS nécessaires à la communication entre les tâches,
 * configure les périphériques I2S pour l'acquisition et la sortie audio, et initialise les LEDs.
 */
void initialisation() {
  // Création des queues
  acquisition_to_processing_queue = xQueueCreate(2, sizeof(int32_t *));
  processing_to_output_queue = xQueueCreate(2, sizeof(int32_t *));
  snr_db_queue = xQueueCreate(4, sizeof(float *));
  if (acquisition_to_processing_queue == NULL || processing_to_output_queue == NULL || snr_db_queue == NULL) {
    ESP_LOGE("MAIN", "Erreur lors de la création des queues");
    return;
  }

  // Initialisation I2S
  i2s_init_in();
  i2s_init_out();

  // Initialisation LED
  queue_init();
  led_init();
  led_send_command(BLINK_MODE_SLOW_GREEN);
}
