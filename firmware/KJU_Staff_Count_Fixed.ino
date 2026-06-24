#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

#define RFID_SS       5
#define RFID_RST      4

#define GREEN_LED     25
#define RED_LED       26
#define BUZZER_PIN    27

#define BLOCK_ADDR    4
#define MAX_CARDS     100

#define LED_PIN 14
#define LED_COUNT 16

// =====================
// CHANGE THESE
// =====================
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* serverUrl =
"http://YOUR_IP_ADDRESS:8080/api/rfid/scan";

MFRC522 rfid(RFID_SS, RFID_RST);
MFRC522::MIFARE_Key key;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

String knownIds[MAX_CARDS];
bool cardStatus[MAX_CARDS];
int cardCount = 0;
int staffPresent = 0;


void setRingColor(uint8_t r,uint8_t g,uint8_t b){
  for(int i=0;i<LED_COUNT;i++){
    ring.setPixelColor(i, ring.Color(r,g,b));
  }
  ring.show();
}

void idleRing(){ setRingColor(0,40,0); }

void scanAnimation(){
  ring.clear();
  for(int i=0;i<LED_COUNT;i++){
    ring.setPixelColor(i, ring.Color(0,0,255));
  }
  ring.show();
  delay(20);
  ring.clear();
  ring.show();
}

void successInRing(){ setRingColor(0,255,0); }
void successOutRing(){ setRingColor(255,0,0); }
void errorRing(){ setRingColor(255,0,255); }


int findCard(String id) {
  for (int i = 0; i < cardCount; i++) {
    if (knownIds[i] == id) return i;
  }
  return -1;
}

void greenBlink() {
  digitalWrite(GREEN_LED, HIGH);
  delay(300);
  digitalWrite(GREEN_LED, LOW);
}

void redBlink() {
  digitalWrite(RED_LED, HIGH);
  delay(300);
  digitalWrite(RED_LED, LOW);
}

void inBeep() {
  tone(BUZZER_PIN, 2000);
  delay(150);
  noTone(BUZZER_PIN);
}

void outBeep() {
  tone(BUZZER_PIN, 1500);
  delay(100);
  noTone(BUZZER_PIN);

  delay(100);

  tone(BUZZER_PIN, 1500);
  delay(100);
  noTone(BUZZER_PIN);
}

void showHome()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("KJU Staff");
  lcd.setCursor(0,1);
  lcd.print("Inside:");
  lcd.print(staffPresent);
}

String getTimestamp() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "2026-01-01 00:00:00";
  }

  char buffer[25];

  strftime(
    buffer,
    sizeof(buffer),
    "%Y-%m-%d %H:%M:%S",
    &timeinfo
  );

  return String(buffer);
}

bool sendToBackend(String uid, String status) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Lost");
    return false;
  }

  HTTPClient http;
  http.setTimeout(1000);

  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  String timestamp = getTimestamp();

  String json =
    "{\"uid\":\"" + uid +
    "\",\"status\":\"" + status +
    "\",\"readerId\":\"STAFFROOM_A" +
    "\",\"timestamp\":\"" + timestamp + "\"}";

  Serial.println("Sending:");
  Serial.println(json);

  int responseCode = http.POST(json);

  Serial.print("HTTP Response: ");
  Serial.println(responseCode);

  if (responseCode > 0) {
    Serial.println(http.getString());
    http.end();
    return true;
  }

  http.end();
  return false;
}

String readCardData() {

  MFRC522::StatusCode status;

  status = rfid.PCD_Authenticate(
      MFRC522::PICC_CMD_MF_AUTH_KEY_A,
      BLOCK_ADDR,
      &key,
      &(rfid.uid)
  );

  if (status != MFRC522::STATUS_OK) {
    return "";
  }

  byte buffer[18];
  byte size = sizeof(buffer);

  status = rfid.MIFARE_Read(
      BLOCK_ADDR,
      buffer,
      &size
  );

  if (status != MFRC522::STATUS_OK) {
    return "";
  }

  String data = "";

  for (int i = 0; i < 16; i++) {

    if (buffer[i] == '#' ||
        buffer[i] == '$' ||
        buffer[i] == 0) {
      break;
    }

    if (buffer[i] >= 32 &&
        buffer[i] <= 126) {

      data += (char)buffer[i];
    }
  }

  data.trim();
  return data;
}

void setup() {

  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  ring.begin();
  ring.setBrightness(80);
  ring.show();
  idleRing();

  SPI.begin(18, 19, 23, RFID_SS);
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  lcd.clear();
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  while(!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.println("Waiting NTP...");
  }

  Serial.println("WiFi Connected");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.print("WiFi Connected");

  delay(1500);

  idleRing();
  showHome();
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {

    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }

  if (!rfid.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  scanAnimation();

  String facultyId = readCardData();

  if (facultyId == "") {

    errorRing();
    lcd.clear();
    lcd.print("READ FAILED");

    delay(300);

    showHome();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return;
  }

  int idx = findCard(facultyId);
  bool currentStatus;

  if (idx == -1) {

    knownIds[cardCount] = facultyId;
    cardStatus[cardCount] = true;

    currentStatus = true;
    cardCount++;
    staffPresent++;
  }
  else {

    cardStatus[idx] = !cardStatus[idx];
    currentStatus = cardStatus[idx];

    if(currentStatus)
      staffPresent++;
    else if(staffPresent>0)
      staffPresent--;
  }

  lcd.clear();

  if (currentStatus) {

    successInRing();
    greenBlink();
    inBeep();

    lcd.setCursor(0, 0);
    lcd.print("Faculty IN");

    lcd.setCursor(0, 1);
    lcd.print(facultyId);

    sendToBackend(facultyId, "IN");
  }
  else {

    successOutRing();
    redBlink();
    outBeep();

    lcd.setCursor(0, 0);
    lcd.print("Faculty OUT");

    lcd.setCursor(0, 1);
    lcd.print(facultyId);

    sendToBackend(facultyId, "OUT");
  }

  delay(300);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();


  idleRing();
  showHome();
}

