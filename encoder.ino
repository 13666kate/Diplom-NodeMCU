#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Pin definitions
const int clkPin = 14;  // D5
const int dtPin = 12;   // D6
const int swPin = 13;   // D7
const int buttonPin = 4;  // D2
const int removeButtonPin = 5;  // D1 

volatile int lastClkState;
volatile int currentLetterIndex = 0;
volatile bool forwardDirection = true;  // направление вращения
String fixedLetters = "";

const char* letters[] = {"А", "Б", "В", "Г", "Д", "Е", "Ё", "Ж", "З", "И", "Й", "К", "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я"};
//const char  letters[] ="ABCD";
ESP8266WebServer server(80);

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;  // Задержка для защиты от дребезга (в миллисекундах)

unsigned long buttonLastDebounceTime = 0;
const unsigned long buttonDebounceDelay = 50;  // Задержка для защиты от дребезга кнопки (в миллисекундах)

unsigned long removeButtonLastDebounceTime = 0;  // For the new remove button
const unsigned long removeButtonDebounceDelay = 50;  // Debounce delay for remove button

// Прототипы функций
void IRAM_ATTR updateEncoder();
void IRAM_ATTR handleButtonPress();
void IRAM_ATTR handleRemoveButtonPress(); // New function
void handleRoot();
void handleGetIP();

void setup() {
  Serial.begin(9600);
  WiFi.begin("Login", "Password");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Соединение установлено, IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);
  pinMode(swPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(removeButtonPin, INPUT_PULLUP);  // Setup for the new remove button

  lastClkState = digitalRead(clkPin);

  attachInterrupt(digitalPinToInterrupt(clkPin), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonPress, FALLING);
  attachInterrupt(digitalPinToInterrupt(removeButtonPin), handleRemoveButtonPress, FALLING);  // Interrupt for the remove button

  server.on("/", handleRoot);
  server.on("/getIP", handleGetIP);  // Новый маршрут для получения IP-адреса
  server.begin();
}

void loop() {
  server.handleClient();
}

void IRAM_ATTR updateEncoder() {
  int clkState = digitalRead(clkPin);
  unsigned long currentTime = millis();

  if ((currentTime - lastDebounceTime) > debounceDelay) {
    int dtState = digitalRead(dtPin);

    if (clkState != lastClkState && clkState == HIGH) {
      forwardDirection = (dtState != clkState);

      if (forwardDirection) {
        currentLetterIndex = (currentLetterIndex + 1) % (sizeof(letters) / sizeof(letters[0]));
      } else {
        currentLetterIndex = (currentLetterIndex - 1 + sizeof(letters) / sizeof(letters[0])) % (sizeof(letters) / sizeof(letters[0]));
      }

      Serial.print("Current letter: ");
      Serial.println(letters[currentLetterIndex]);
    }

    lastClkState = clkState;
    lastDebounceTime = currentTime;
  }
}

void IRAM_ATTR handleButtonPress() {
  unsigned long currentTime = millis();

  if ((currentTime - buttonLastDebounceTime) > buttonDebounceDelay) {
    buttonLastDebounceTime = currentTime;
    if (digitalRead(buttonPin) == LOW) {
      fixedLetters += letters[currentLetterIndex];
      Serial.print("Fixed letters: ");
      Serial.println(fixedLetters);
      // небольшая задержка для предотвращения повторного срабатывания
      delay(50);
    }
  }
}

void IRAM_ATTR handleRemoveButtonPress() {
  unsigned long currentTime = millis();

  if ((currentTime - removeButtonLastDebounceTime) > removeButtonDebounceDelay) {
    removeButtonLastDebounceTime = currentTime;
    if (digitalRead(removeButtonPin) == LOW) {
      // Remove the last letter if the string is not empty
      if (!fixedLetters.isEmpty()) {
        fixedLetters.remove(fixedLetters.length() - 1);
        Serial.print("Fixed letters after removal: ");
        Serial.println(fixedLetters);
        // небольшая задержка для предотвращения повторного срабатывания
        delay(50);
      }
    }
  }
}

void handleRoot() {
  String message = fixedLetters;
  // Разделитель между зафиксированными буквами и текущей буквой
  message += letters[currentLetterIndex];  // Добавляем текущую букву к строке
  server.send(200, "text/plain", message);
}

void handleGetIP() {
  String ip = WiFi.localIP().toString();
  server.send(200, "text/plain", ip);
}