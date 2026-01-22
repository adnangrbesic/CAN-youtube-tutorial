#include <driver/twai.h>

#define TX_GPIO_NUM GPIO_NUM_21
#define RX_GPIO_NUM GPIO_NUM_22
#define LED_PIN     4  
//==================================================================================//

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 

  Serial.println("CAN RECEIVER - External LED (D4)");

  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);

  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

  // ------------------------------------------------------------
  // FILTER OPTION 1: Accept ALL messages (no filtering)
  // ------------------------------------------------------------
  // twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // ------------------------------------------------------------
  // FILTER OPTION 2: Accept ONLY standard ID 0x12
  // ------------------------------------------------------------
  twai_filter_config_t f_config = {
      .acceptance_code = (0x12 << 21), 
      .acceptance_mask = ~(0x7FF << 21), 
      .single_filter = true
  };

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    if (twai_start() == ESP_OK) {
      Serial.println("CAN Initialized");
    } else {
      Serial.println("Starting CAN failed!");
    }
  } else {
    Serial.println("Driver install failed!");
  }
}

//==================================================================================//

void loop() {
  canReceiver();
}

//==================================================================================//

void canReceiver() {
  twai_message_t message;

  if (twai_receive(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
    Serial.println("Message received! Toggling LED on D4.");
    
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    Serial.print("ID: 0x");
    Serial.println(message.identifier, HEX);
  }
}
