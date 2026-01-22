#include <driver/twai.h>

#define TX_GPIO_NUM GPIO_NUM_21
#define RX_GPIO_NUM GPIO_NUM_22
#define BUTTON_PIN  4  

//==================================================================================//

void setup() {
  Serial.begin(115200);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("CAN SENDER - Button Triggered");

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

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
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); 
    if(digitalRead(BUTTON_PIN) == LOW) { 
        canSender();
        while(digitalRead(BUTTON_PIN) == LOW); 
        delay(50);
    }
  }
}

//==================================================================================//

void canSender() {
  Serial.print("Sending packet ... ");

  twai_message_t message;
  message.identifier = 0x12;     
  message.extd = 0;              
  message.rtr = 0;               
  message.data_length_code = 8;  
  message.data[0] = 'H';
  message.data[1] = 'e';
  message.data[2] = 'l';
  message.data[3] = 'l';
  message.data[4] = 'o';
  message.data[5] = '!';
  message.data[6] = 0;
  message.data[7] = 0;

  if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("done");
  } else {
    Serial.println("failed");
  }
}