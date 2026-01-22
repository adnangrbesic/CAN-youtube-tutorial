#include <driver/twai.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= CAN PINOVI =================
#define TX_GPIO_NUM GPIO_NUM_21
#define RX_GPIO_NUM GPIO_NUM_22

// ================= LED ========================
#define LED_PIN 4  

// ================= RFID (RC522) ===============
#define SS_PIN   5
#define RST_PIN  27
MFRC522 rfid(SS_PIN, RST_PIN);

// ================= LCD (I2C) ==================
#define I2C_SDA 25
#define I2C_SCL 26
LiquidCrystal_I2C lcd(0x27, 16, 2);

//==================================================================================//

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("CAN RECEIVER + RFID SENDER + LCD");

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("BOOTING...");

  twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    if (twai_start() == ESP_OK) {
      Serial.println("CAN Initialized");
      lcd.setCursor(0,1);
      lcd.print("CAN OK");
    } else {
      Serial.println("Starting CAN failed!");
      lcd.setCursor(0,1);
      lcd.print("CAN FAIL");
    }
  } else {
    Serial.println("Driver install failed!");
    lcd.setCursor(0,1);
    lcd.print("DRV FAIL");
  }

  delay(1000);

  SPI.begin();       
  rfid.PCD_Init();
  Serial.println("RFID Ready");
  lcd.clear();
  lcd.print("RFID READY");
}

//==================================================================================//

void loop() {
  canReceiver();     
  checkRFID();      
}

//==================================================================================//
// ================= CAN RECEIVER + LCD ===================
//==================================================================================//

void canReceiver() {
  twai_message_t message;

  if (twai_receive(&message, pdMS_TO_TICKS(10)) == ESP_OK) {

    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    Serial.print("[CAN RX] ID: 0x");
    Serial.println(message.identifier, HEX);

    lcd.clear();
    lcd.setCursor(0,0);

    lcd.print("0x");
    lcd.print(message.identifier, HEX);
    lcd.print(" | ");

    for (int i = 0; i < message.data_length_code; i++) {
      char c = (char)message.data[i];
      if (c >= 32 && c <= 126) {
        lcd.print(c);
      }
    }
  }
}

//==================================================================================//
// ================= RFID SEND LOGIKA ====================
//==================================================================================//

void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print("[RFID] UID: ");
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  sendUIDOverCAN(rfid.uid.uidByte);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1000); 
}

//==================================================================================//

void sendUIDOverCAN(byte *uid) {
  twai_message_t msg;
  msg.identifier = 0x13;      
  msg.extd = 0;
  msg.rtr = 0;
  msg.data_length_code = 4;

  for (int i = 0; i < 4; i++) {
    msg.data[i] = uid[i];
  }

  if (twai_transmit(&msg, pdMS_TO_TICKS(100)) == ESP_OK) {
    Serial.println("[CAN TX] RFID UID sent");
  } else {
    Serial.println("[CAN TX] FAILED");
  }
}
