// Edited By: Jakub Remiśko
// Posted on Discord
// Date: 03.11.2022r
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>
#include <melody.h>
#include <rickroll.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
// Zmienna przechowująca ID skanaera
const String ScannerId = "1";
// Konfiguracja WiFi
const char *ssid = "Mi9T";
const char *password = "12345677";
// Połączenie z aktualnym czasem
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
//Timery
unsigned long startTime;
unsigned long startTime_2;
unsigned long startTime_3;
//Pin blokady zamka 
const int lock = 2;
bool lockState = false;
// Melodia
int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};
int rick = 0;
// Pin głośniczka
const int soundPin = 25;

// Konfiguracja modułu RFID
const int ssPin = 5;
const int rstPin = 15;
MFRC522::MIFARE_Key key;
MFRC522 mfrc522(ssPin, rstPin);

// Zmienna przechowująca ID karty RFID w formie tekstowej
String uidString;

// Konfiguracja klawiatury i wyświetlacza LCD
const int passwordLength = 6;
char data[passwordLength];
byte dataCount = 0;
bool passIsGood;
char customKey;
const byte rows = 4;
const byte cols = 4;
char hexaKeys[rows][cols] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
//kod serwisowy 
const String kodS = "987654";
byte rowPins[rows] = {16, 17, 27, 32};
byte colPins[cols] = {12, 26, 13, 33};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, rows, cols);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// wyczyszczenia ciągu znaków przechowywanego w zmiennej data
void clearData()
{
  while (dataCount != 0)
  {
    data[dataCount--] = 0;
  }
}
void playMelody()
{
  for (int thisNote = 0; thisNote < 8; thisNote++)
  {
    int noteDuration = 1000 / noteDurations[thisNote];

    tone(soundPin, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;

    delay(pauseBetweenNotes);

    noTone(soundPin);
  }
}
//Wysłanie żądania http w celu sprawdzenia hasła 
void CheckPassword(String type, String password)
{
  HTTPClient http;
  http.begin("https://scanner-api-esp.azurewebsites.net/api/scanner/" + ScannerId + "/" + type + "/" + password + "/");
  Serial.println(type + " , " + password);
  int httpCode = http.GET();
  lcd.clear();
  lcd.print("Wait.....");
  if (httpCode > 0)
  {
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
  }
  else
  {
    delay(100);
    lcd.clear();
    lcd.print("Wait for server");
    Serial.println("Wait for server");
  }
  if (httpCode == 200)
  {
    lcd.clear();
    lcd.print("Correct");
    playMelody();
    startTime = timeClient.getEpochTime();
    lockState = true;
    digitalWrite(lock, LOW);
    delay(1000);
  }
  else
  {
    lcd.clear();
    lcd.print("Incorrect");
    ++rick;
    if(rick==1)
    startTime_2 = timeClient.getEpochTime();
    Serial.println(startTime_2);
    Serial.println(rick);
    delay(1000);
  }
  http.end(); // zamknięcie połączenia 
}



void Keyboard()
{
  customKey = customKeypad.getKey();
  if (customKey)
  {
    // Jeśli klawisz został wciśniety, dodaj go do ciągu hasła
    data[dataCount] = customKey;
    lcd.setCursor(dataCount, 1);
    lcd.print(data[dataCount]);
    tone(soundPin, 500 + (dataCount * 50), 100);
    delay(300);
    lcd.setCursor(dataCount, 1);
    lcd.print('*');
    dataCount++;
  }
  // Jeśli ciąg hasła osiągnął długość maksymalną, porównaj go z hasłem głównym
  if (dataCount == passwordLength)
  {
    lcd.clear();
    String password = String(data);
    String type = "KOD";
    // Tryb serwisowy
    if (WiFi.status() != WL_CONNECTED)
    {
      if(password==kodS)
      {
      startTime_3 = millis();
      lcd.clear();
      lcd.print("Correct");
      lockState = true;
      digitalWrite(lock, LOW);
      delay(1000);
      }
      else
      {
      lcd.clear();
      lcd.print("Incorrect");
      delay(1000);
      }
    }
    else
    CheckPassword(type, password);
    lcd.clear();
    clearData();
  }
}
void internet()
{
  // Połączenie z siecią WiFi
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.print("Wait...");
  delay(1000);
  Serial.println("Connecting to WiFi..");
  while ((WiFi.status() != WL_CONNECTED) && !lockState)
  {
    Keyboard();
  }
  Serial.println("Connected to the WiFi network");
}
void setup()
{
  pinMode(lock, OUTPUT);
  pinMode(soundPin, OUTPUT);
  lcd.init();
  Serial.begin(115200);
  lcd.backlight();
  // Inicjalizacja modułu RFID
  SPI.begin();
  mfrc522.PCD_Init();
  internet();
}


void readRfidCard()
{
  // Odczytanie ID karty za pomocą modułu RFID
  mfrc522.PICC_ReadCardSerial();
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  // Sprawdzenie, czy karta jest kompatybilna z protokołem Classic MIFARE
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K)
  {
    Serial.println(F("Your tag is not compatible with MIFARE Classic."));
    return;
  }
  Serial.println("Scanned UID:");
  // Konwersja ID karty na tekst
  uidString = String(mfrc522.uid.uidByte[0])+ String(mfrc522.uid.uidByte[1])+ String(mfrc522.uid.uidByte[2])+ String(mfrc522.uid.uidByte[3]);
  Serial.println("Scanned UID:");
  Serial.println(uidString);
  String type = "UID";
  CheckPassword(type, uidString);
  // Hold PICC
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void play_rick() {
//zachowane zostały oryginalne komentarze 
//source : https://github.com/robsoncouto/arduino-songs/blob/master/nevergonnagiveyouup/nevergonnagiveyouup.ino
for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
  // calculates the duration of each note
  divider = music[thisNote + 1];
  if (divider > 0) {
    // regular note, just proceed
    noteDuration = (wholenote) / divider;
  } else if (divider < 0) {
    // dotted notes are represented with negative durations!!
    noteDuration = (wholenote) / abs(divider);
    noteDuration *= 1.5; // Increases the duration in half for dotted notes
  }

  // we only play the note for 90% of the duration, leaving 10% as a pause
  tone(soundPin, music[thisNote], noteDuration * 0.9);

  // Wait for the specief duration before playing the next note.
  delay(noteDuration);

  // stop the waveform generation before the next note.
  noTone(soundPin);

  // Check if a new card is present or the custom keypad has a key press.
  if ((mfrc522.PICC_IsNewCardPresent()) || customKeypad.getKey()) {
    // If either condition is true, break the loop.
    break;
  }
}
}
void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    lcd.setCursor(0, 0);
    lcd.print("Enter password:");
    // Odczytanie ID karty RFID
    if (mfrc522.PICC_IsNewCardPresent())
      readRfidCard();
    // Obsługa klawiatury
    Keyboard();
    timeClient.update();
    if (lockState && (timeClient.getEpochTime() - startTime >= 5)){
    lockState = false;
    digitalWrite(lock, HIGH);
    }
    if(timeClient.getEpochTime() - startTime_2 >= 60)
    rick==0;
    if(rick==3 && (timeClient.getEpochTime() - startTime_2 <= 60) ){
    rick=0;
    play_rick();
    }
  }
  //Powrót do wyszukiwania sieci w przypadku zaniku internetu
  if(WiFi.status() != WL_CONNECTED && !lockState)
  {
    internet();
  }
  // Zablokowanie zamka po 5s 
  if (WiFi.status() != WL_CONNECTED && lockState && (millis() - startTime_3 >= 5000)){
    lockState = false;
    digitalWrite(lock, HIGH);
    Serial.println("reset");
    delay(500);
  }
}

