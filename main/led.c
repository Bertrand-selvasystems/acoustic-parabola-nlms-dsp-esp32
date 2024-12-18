/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DECLARATION DES BIBLIOTHEQUES
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
#include "led.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DECLARATION DES VARIABLES
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Tableau des stratégies de clignotement disponibles.
 *
 * Contient des paires de modes de clignotement et de fonctions associées.
 */
BlinkStrategy blink_strategies[] = {
    {BLINK_MODE_SLOW_GREEN, slow_green_blink},
    {BLINK_MODE_SLOW_YELLOW, slow_yellow_blink},
    {BLINK_MODE_FAST_YELLOW, fast_yellow_blink},
    {BLINK_MODE_FAST_RED, fast_red_blink},
    {NO_BLINK, no_blink}};

/**
 * @brief Nombre de stratégies disponibles dans le tableau.
 */
const int NUM_STRATEGIES = sizeof(blink_strategies) / sizeof(BlinkStrategy);

/**
 * @brief Stratégie de clignotement actuelle.
 */
BlinkStrategy current_strategy;

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DECLARATION DES VARIABLES STATIQUES
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Etat actuel du blink mode
 */
static BlinkMode current_blink_mode = NO_BLINK;

/**
 * @brief Etat précédent du blink mode
 */
static BlinkMode previous_BlinkMode;  // Valeur par défaut au démarrage

/**
 * @brief Gestionnaire pour la bande LED.
 */
static led_strip_handle_t led_strip;

/**
 * @brief File d'attente pour les commandes LED.
 */
static QueueHandle_t led_queue = NULL;

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// DEFINITION DES FONCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Tâche pour initialiser en fonction de la valeur initiale l'état de la led
 *

 */
void led_init_previous_mode() {
  previous_BlinkMode = current_blink_mode;
}

/**
 * @brief Tâche pour contrôler la bande LED.
 *
 * Initialise et gère la bande LED, appliquant des changements de couleur et d'effets périodiquement.
 *
 * @param pvParameter Pointeur de paramètre pour la tâche (utilisé par FreeRTOS).
 */

void led_task(void *pvParameter) {
  float current_snr;
  uint8_t rouge, vert, bleu;
  const float snr_min = 0.0f;
  const float snr_max = 7.0f;
  while (1) {
    // Recevoir un buffer stéréo traité de la tâche de traitement
    if (xQueueReceive(snr_db_queue, &current_snr, portMAX_DELAY) == pdTRUE) {
      // calcul de rouge, vert et bleu en fonction de current_snr
      float ratio = (current_snr - snr_min) / (snr_max - snr_min);
      if (ratio < 0) ratio = 0;
      if (ratio > 1) ratio = 1;
      // On fait un mélange entre rouge (0, ratio=0) et vert (ratio=1)
      // Rouge pur: (rouge=255, vert=0)
      // Vert pur : (rouge=0, vert=255)
      rouge = (uint8_t)(255 * (1.0f - ratio));
      vert = (uint8_t)(255 * ratio);
      bleu = 0;  // pas nécessaire, on peut le laisser à 0
      // reglage de l'intention
      rouge = rouge >> 3;
      vert = vert >> 3;
      led_set_color(0, rouge, vert, bleu);
      led_refresh();
    }
  }
}

// void led_task(void *pvParameter) {
//   while (1) {
//     if (xQueueReceive(led_queue, &current_blink_mode, 0) == pdPASS) {
//       current_strategy = blink_strategies[current_blink_mode];
//       ESP_LOGI("LED_Task", "Mode de clignotement changé: %d", current_blink_mode);
//     }
//     esp_err_t ret = current_strategy.blink_function();
//     if (ret != ESP_OK) {
//       ESP_LOGE("LED_BLINK_TASK", "Erreur lors de l'exécution de la stratégie de clignotement");
//     }
//     vTaskDelay(pdMS_TO_TICKS(10));
//   }
// }

/**
 * @brief Envoie une commande de clignotement à la bande LED.
 *
 * @param mode Le mode de clignotement à appliquer.
 * @return `ESP_OK` si la commande a été envoyée avec succès, sinon un code d'erreur esp_err_t.
 */
esp_err_t led_send_command(BlinkMode mode) {
  // Mémoriser l'état actuel avant de le modifier
  if (previous_BlinkMode != current_blink_mode)
    previous_BlinkMode = current_blink_mode;
  current_blink_mode = mode;
  if (xQueueSend(led_queue, &mode, (TickType_t)10) != pdPASS) {
    ESP_LOGE("LED_Module", "Échec de l'envoi de la commande LED à la file d'attente");
    return ESP_FAIL;
  }
  ESP_LOGI("LED_Task", "envoi à la queue de commande LED le mode : %d", mode);
  return ESP_OK;
}

/**
 * @brief Initialise la bande LED.
 *
 * Configure le matériel et crée la file d'attente pour les commandes LED.
 *
 * @return `ESP_OK` si l'initialisation est réussie, sinon un code d'erreur esp_err_t.
 */
esp_err_t led_init(void) {
  ESP_LOGI("LED", "Initialisation de la bande LED...");

  led_strip_config_t strip_config = {
      .strip_gpio_num = LED_STRIP_GPIO,
      .max_leds = NUM_LEDS,
      .led_pixel_format = LED_PIXEL_FORMAT_GRB,
      .led_model = LED_MODEL_WS2812,
      .flags = {.invert_out = false}};

  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10 * 1000 * 1000,
      .flags = {.with_dma = false}};

  // Vérification de l'erreur lors de la création du périphérique LED
  esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
  if (ret != ESP_OK) {
    ESP_LOGE("LED", "Erreur lors de la création de l'instance LED : %s", esp_err_to_name(ret));
    return ret;  // Retourne l'erreur si la création échoue
  }

  ESP_LOGI("LED", "Bande LED initialisée avec succès");
  return ESP_OK;  // Retourne ESP_OK si tout s'est bien passé
}

/**
 * @brief Initialise la queue pour les commandes
 *
 * @return `ESP_OK` si l'initialisation est réussie, sinon un code d'erreur esp_err_t.
 */
esp_err_t queue_init(void) {
  led_queue = xQueueCreate(10, sizeof(BlinkMode));
  if (led_queue == NULL) {
    ESP_LOGE("LED_Module", "Échec de la création de la file d'attente LED");
    return ESP_FAIL;
  }

  return ESP_OK;
}

/**
 * @brief Définit la couleur RGB pour une LED spécifique.
 *
 * @param index Index de la LED dans la bande.
 * @param r Intensité de la couleur rouge.
 * @param g Intensité de la couleur verte.
 * @param b Intensité de la couleur bleue.
 * @return `ESP_OK` si la couleur a été définie avec succès, sinon un code d'erreur esp_err_t.
 */
esp_err_t led_set_color(uint32_t index, uint8_t r, uint8_t g, uint8_t b) {
  ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, index, r, g, b));
  return ESP_OK;
}

/**
 * @brief Rafraîchit la bande LED pour appliquer les changements.
 *
 * @return `ESP_OK` si le rafraîchissement est réussi, sinon un code d'erreur esp_err_t.
 */
esp_err_t led_refresh(void) {
  ESP_ERROR_CHECK(led_strip_refresh(led_strip));
  return ESP_OK;
}

/**
 * @brief Efface toutes les LEDs de la bande.
 *
 * @return `ESP_OK` si l'effacement est réussi, sinon un code d'erreur esp_err_t.
 */
esp_err_t led_clear(void) {
  ESP_ERROR_CHECK(led_strip_clear(led_strip));
  return led_refresh();
}

/**
 * @brief Récupère l'état précédent de la LED.
 *
 * Cette fonction retourne l'état de clignotement de la LED avant le dernier
 * changement de mode. Utile pour restaurer l'état antérieur après une tâche temporaire.
 *
 * @return L'état précédent de la LED (type BlinkMode).
 */
BlinkMode get_previous_BlinkMode() {
  return previous_BlinkMode;
}

/**
 * @brief Stratégie de clignotement sans effet de clignotement (LED éteinte).
 *
 * @return `ESP_OK` si l'opération est réussie, sinon un code d'erreur esp_err_t.
 */
esp_err_t no_blink(void) {
  esp_err_t ret = led_set_color(0, 0, 0, 0);
  if (ret != ESP_OK) return ret;
  ret = led_refresh();
  if (ret != ESP_OK) return ret;
  vTaskDelay(pdMS_TO_TICKS(1000));
  return ESP_OK;
}

/**
 * @brief Stratégie de clignotement lent en vert.
 *
 * @return `ESP_OK` si l'opération est réussie, sinon un code d'erreur esp_err_t.
 */
esp_err_t slow_green_blink(void) {
  esp_err_t ret = led_set_color(0, 0, 255, 0);
  if (ret != ESP_OK) return ret;
  ret = led_refresh();
  if (ret != ESP_OK) return ret;
  vTaskDelay(pdMS_TO_TICKS(500));
  led_set_color(0, 0, 0, 0);
  led_refresh();
  vTaskDelay(pdMS_TO_TICKS(500));
  return ESP_OK;
}

/**
 * @brief Stratégie de clignotement lent en vert.
 *
 * @return `ESP_OK` si l'opération est réussie, sinon un code d'erreur esp_err_t.
 */
esp_err_t slow_yellow_blink(void) {
  esp_err_t ret = led_set_color(0, 255, 255, 0);
  if (ret != ESP_OK) return ret;
  ret = led_refresh();
  if (ret != ESP_OK) return ret;
  vTaskDelay(pdMS_TO_TICKS(500));
  led_set_color(0, 0, 0, 0);
  led_refresh();
  vTaskDelay(pdMS_TO_TICKS(500));
  return ESP_OK;
}

/**
 * @brief Stratégie de clignotement rapide en jaune.
 *
 * @return `ESP_OK` si l'opération est réussie, sinon un code d'erreur esp_err_t.
 */
esp_err_t fast_yellow_blink(void) {
  esp_err_t ret = led_set_color(0, 255, 255, 0);
  if (ret != ESP_OK) return ret;
  ret = led_refresh();
  if (ret != ESP_OK) return ret;
  vTaskDelay(pdMS_TO_TICKS(200));
  led_set_color(0, 0, 0, 0);
  led_refresh();
  vTaskDelay(pdMS_TO_TICKS(200));
  return ESP_OK;
}

/**
 * @brief Stratégie de clignotement rapide en rouge.
 *
 * @return `ESP_OK` si l'opération est réussie, sinon un code d'erreur esp_err_t.
 */
esp_err_t fast_red_blink(void) {
  esp_err_t ret = led_set_color(0, 255, 0, 0);
  if (ret != ESP_OK) return ret;
  ret = led_refresh();
  if (ret != ESP_OK) return ret;
  vTaskDelay(pdMS_TO_TICKS(200));
  led_set_color(0, 0, 0, 0);
  led_refresh();
  vTaskDelay(pdMS_TO_TICKS(200));
  return ESP_OK;
}
