#include <driver/twai.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// ================= CAN =================
#define TX_GPIO_NUM GPIO_NUM_21
#define RX_GPIO_NUM GPIO_NUM_22

// ================= BUTTON ==============
#define BUTTON_PIN 4
bool lastButtonState = HIGH;

// ================= LCD =================
#define I2C_SDA 17
#define I2C_SCL 18
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= KEYPAD ==============
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ================= RFID =================
byte allowedUIDs[][4] = {
  {0x41, 0x5F, 0x29, 0x16},
  {0x63, 0x27, 0xBE, 0x2C}
};

const byte NUM_UIDS = sizeof(allowedUIDs) / sizeof(allowedUIDs[0]);

// ================= INPUT ================
char digitBuffer[3] = {0};
int digitIndex = 0;

char wordBuffer[9] = {0};
int wordIndex = 0;

bool messageSent = false;

//==================================================================================//

void setup() {
  Serial.begin(115200);
  Serial.println("=== CAN SENDER START ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("INPUT:");

  twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  twai_driver_install(&g_config, &t_config, &f_config);
  twai_start();
}

//==================================================================================//

void loop() {
  handleKeypad();
  handleButton();
  handleCANRx();
  delay(10);
}

//==================================================================================//

void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  if (messageSent) {
    resetInput();
    messageSent = false;
  }

  if (key >= '0' && key <= '9') {
    digitBuffer[digitIndex++] = key;

    if (digitIndex == 2) {
      digitBuffer[2] = '\0';
      int asciiVal = atoi(digitBuffer);

      if (asciiVal >= 32 && asciiVal <= 126 && wordIndex < 8) {
        wordBuffer[wordIndex++] = (char)asciiVal;
        wordBuffer[wordIndex] = '\0';
        updateLCDInput();
      }
      digitIndex = 0;
    }
  }
}

//==================================================================================//

void handleButton() {
  bool currentState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && currentState == LOW) {
    if (wordIndex > 0) {
      sendCANMessage();
      messageSent = true;
    }
  }

  lastButtonState = currentState;
}

//==================================================================================//

void sendCANMessage() {
  twai_message_t msg;
  msg.identifier = 0x12;
  msg.extd = 0;
  msg.rtr = 0;
  msg.data_length_code = wordIndex;

  for (int i = 0; i < wordIndex; i++) {
    msg.data[i] = wordBuffer[i];
  }

  if (twai_transmit(&msg, pdMS_TO_TICKS(1000)) == ESP_OK) {

    Serial.print("[TX] ");
    Serial.println(wordBuffer);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SENT:");
    lcd.setCursor(0,1);
    lcd.print(wordBuffer);
  
  }
}

//==================================================================================//

void handleCANRx() {
  twai_message_t msg;

  while (twai_receive(&msg, 0) == ESP_OK) {

    // -------- GENERIC RESPONSE --------
    Serial.print("[RX] ID=0x");
    Serial.println(msg.identifier, HEX);

    

    // -------- RFID RESPONSE --------
    if (msg.identifier == 0x13 && msg.data_length_code == 4) {

      bool authorized = false;

      for (byte i = 0; i < NUM_UIDS; i++) {
        bool match = true;
        for (byte j = 0; j < 4; j++) {
          if (msg.data[j] != allowedUIDs[i][j]) {
            match = false;
            break;
          }
        }
        if (match) {
          authorized = true;
          break;
        }
      }

      lcd.clear();
      lcd.print(authorized ? "OTKLJUCANO" : "ODBIJENO");
      delay(1500);
      updateLCDInput();
    }
  }
}

//==================================================================================//

void updateLCDInput() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("INPUT:");
  lcd.setCursor(0,1);
  lcd.print(wordBuffer);
}

void resetInput() {
  memset(digitBuffer, 0, sizeof(digitBuffer));
  memset(wordBuffer, 0, sizeof(wordBuffer));
  digitIndex = 0;
  wordIndex = 0;
  updateLCDInput();
}
