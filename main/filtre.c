#include "filtre.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DECLARATION DES VARIABLES STATIC
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
static float left_channel[BLOCK_SIZE] __attribute__((aligned(16)));
static float right_channel[BLOCK_SIZE] __attribute__((aligned(16)));
static float filtered_output[BLOCK_SIZE] __attribute__((aligned(16)));
// Initialisation des coefficients du filtre NLMS à zéro
static float filter_coefficients[FILTER_LENGTH] __attribute__((aligned(16))) = {0.0f};
// Buffer circulaire pour stocker les derniers N échantillons du canal gauche
static float x_history[FILTER_LENGTH] __attribute__((aligned(16))) = {0.0f};

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DECLARATION DES FONCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

// Fonction de traitement de l'algorithme NLMS (Normalized Least Mean Squares)
// Cette fonction est exécutée en tant que tâche FreeRTOS et traite les données audio
// reçues via une queue d'acquisition, applique le filtre NLMS, et envoie les
// données filtrées à une queue de restitution.
/**
 * @brief Tache pour l'application du traitement du signal
 *
 */
void processing_task(void *pvParameters) {
  float gain = 1.0f;
  float gain_lisse = 1.0f;
  int32_t *buffer_to_process;  // Pointeur vers le buffer de données audio à traiter
  int history_index = 0;       // Index pour le buffer circulaire de l'historique des échantillons
  int compteur = 0;
  // Initialisation de la norme utilisée dans l'algorithme NLMS
  float norm = 0.0f;
  float rms_signal_bruit = 0.0f;
  float rms_signal_debruite = 0.0f;
  float snr_db_lisse = 0.0f;
  float snr_db = 0.0f;

  // Initialiser la norme
  norm = dsps_dotprod_f32(x_history, x_history, &norm, FILTER_LENGTH);
  norm += EPSILON;  // Ajout d'une petite constante pour éviter la division par zéro

  // Boucle principale de la tâche, exécutée indéfiniment
  while (1) {
    uint64_t start_time = esp_timer_get_time();  // Démarrer le chronomètre

    // Recevoir un buffer rempli de la tâche d'acquisition via la queue
    if (xQueueReceive(acquisition_to_processing_queue, &buffer_to_process, portMAX_DELAY) == pdTRUE) {
      // Séparer les canaux gauche et droit avec normalisation
      for (int i = 0; i < BLOCK_SIZE; i++) {
        // Extraction et normalisation du canal gauche : bruit
        left_channel[i] = (float)(buffer_to_process[2 * i] >> 8) * NORMALIZE_FACTOR;
        // Extraction et normalisation du canal droit : signal + bruit
        right_channel[i] = (float)(buffer_to_process[2 * i + 1] >> 8) * NORMALIZE_FACTOR;
      }

      // Calcul de la puissance du signal bruité
      dsps_dotprod_f32(right_channel, right_channel, &rms_signal_bruit, BLOCK_SIZE);
      rms_signal_bruit = sqrtf(rms_signal_bruit / (float)BLOCK_SIZE);

      // Appliquer l'algorithme NLMS pour chaque échantillon du bloc
      for (int i = 0; i < BLOCK_SIZE; i++) {
        // Échantillon sortant (ancien) du buffer historique
        float exiting_sample = x_history[history_index];

        // Mettre à jour la norme incrémentale
        norm -= exiting_sample * exiting_sample;    // Soustraire le carré de l'échantillon sortant
        norm += left_channel[i] * left_channel[i];  // Ajouter le carré du nouvel échantillon

        // Mettre à jour le buffer historique avec le nouvel échantillon du canal gauche
        x_history[history_index] = left_channel[i];

        // Calculer la sortie du filtre y(n) = somme des (coefficients * échantillons historiques)
        float y = 0.0f;
        for (int k = 0; k < FILTER_LENGTH; k++) {
          int idx = (history_index + FILTER_LENGTH - k) % FILTER_LENGTH;
          y += filter_coefficients[k] * x_history[idx];
        }

        // Calculer l'erreur e(n) = signal désiré (d(n)) - signal estimé (y(n))
        float e = right_channel[i] - y;

        // Pré-calcul du facteur d'échelle pour éviter les divisions répétées
        float norm_inv = MU / (norm + EPSILON);
        float scale_factor = e * norm_inv;

        // Mettre à jour les coefficients du filtre en fonction de l'erreur et du signal d'entrée
        for (int k = 0; k < FILTER_LENGTH; k++) {
          int idx = (history_index + FILTER_LENGTH - k) % FILTER_LENGTH;
          filter_coefficients[k] += scale_factor * x_history[idx];
        }

        // Stocker l'erreur comme sortie filtrée pour ce bloc d'échantillons
        filtered_output[i] = e;

        // Mettre à jour l'index du buffer historique pour le prochain échantillon
        history_index = (history_index + 1) % FILTER_LENGTH;

        // Ajouter un point de céderie pour permettre au système de gestion de tâches de fonctionner
        if (i == (BLOCK_SIZE / 2)) {  // Par exemple, toutes les 512 itérations si BLOCK_SIZE = 1024
          taskYIELD();                // Céder le contrôle à d'autres tâches pour éviter le watchdog timeout
        }
      }

      // Calcul de la puissance du signal débruité
      dsps_dotprod_f32(filtered_output, filtered_output, &rms_signal_debruite, BLOCK_SIZE);
      rms_signal_debruite = sqrtf(rms_signal_debruite / (float)BLOCK_SIZE);

      // Calcul du rapport signal débruité / signal bruité (SNR) en dB
      snr_db = 20.0f * log10f(rms_signal_bruit / (rms_signal_debruite + EPSILON));

      // Lissage exponentiel du SNR
      snr_db_lisse = ALPHA_SNR * snr_db_lisse + (1.0f - ALPHA_SNR) * snr_db;

      // Envoi du SNR dans la queue snr_db_queue périodiquement
      compteur++;
      if (compteur == COMPTEUR_LED) {  // Conditionne la fréquence de réactualisation de la LED
        compteur = 0;
        // Envoyer le SNR lissé à la queue snr_db_queue pour actualisation de la LED
        if (xQueueSend(snr_db_queue, &snr_db_lisse, portMAX_DELAY) != pdTRUE) {
          ESP_LOGW("PROC_TASK", "Queue pleine! Données perdues.");  // Log en cas d'échec de l'envoi à la queue
        }
      }

      // Lissage exponentiel du gain
      // Application du gain adaptatif et dénormalisation
      gain = COEFF_GAIN / (rms_signal_debruite + EPSILON);
      gain_lisse = ALPHA_GAIN * gain_lisse + (1.0f - ALPHA_GAIN) * gain;
      if (gain_lisse > GAIN_MAX) {  // Clamping du gain
        gain_lisse = GAIN_MAX;
      }

      // Appliquer le gain adaptatif directement en flottant
      dsps_mulc_f32(filtered_output, filtered_output, BLOCK_SIZE, gain_lisse, 1, 1);

      // Clamp à la plage [-1.0, 1.0] pour éviter le dépassement
      for (int i = 0; i < BLOCK_SIZE; i++) {
        if (filtered_output[i] > 1.0f) {
          filtered_output[i] = 1.0f;
        }
        if (filtered_output[i] < -1.0f) {
          filtered_output[i] = -1.0f;
        }
      }

      // Conversion Float → Int32. On prend 70 pourcent de la valeur max en 32 bits signés
      dsps_mulc_f32(filtered_output, filtered_output, BLOCK_SIZE, 0.7 * (1 << 31), 1, 1);

      // Assignation au canal droit et gauche du signal debruité et amplifié
      for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer_to_process[2 * i + 1] = (int32_t)(filtered_output[i]);
        buffer_to_process[2 * i] = (int32_t)(filtered_output[i]);
      }

      // Envoyer le buffer filtré à la tâche de restitution via la queue
      if (xQueueSend(processing_to_output_queue, &buffer_to_process, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW("PROC_TASK", "Queue pleine! Données perdues.");  // Log en cas d'échec de l'envoi à la queue
      }

      // // Optionnel : Affichage périodique pour le débogage ;
      // attention l'affichage est trop lent sur le serial et ca rend le filtre instable et fait des atefacts audio très désagréable
      // if ((compteur % 1000) == 0) {
      //   uint64_t end_time = esp_timer_get_time();  // Arrêter le chronomètre
      //   uint64_t elapsed = end_time - start_time;
      //   ESP_LOGI("PROC_TASK", "Temps de traitement du bloc: %llu microsecondes", elapsed);
      //   ESP_LOGI("PROC_TASK", "RMS signal debruite : %f", rms_signal_debruite);
      //   for (int k = 0; k < 10; k++) {
      //     ESP_LOGI("PROC_TASK", "w[%d]: %.6f", k, filter_coefficients[k]);
      //   }
      // }
    }
  }
}
