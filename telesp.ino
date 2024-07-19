#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define TELE_BOT_TOKEN ""
#define TELE_CHAT_ID ""
#define FIREBASE_API_KEY ""
#define FIREBASE_URL ""
#define FIREBASE_EMAIL ""
#define FIREBASE_PASS """

const unsigned long BOT_MTBS = 1000;
unsigned long botLastTime;
const int ledPin = 2;
char chat_id[20];
String opsi = "NONE";
String idupdate;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String uid;
WiFiClientSecure client;
UniversalTelegramBot bot(TELE_BOT_TOKEN, client);

struct Database {
  char name[30];
  char age[4];
  char position[30];
  char status[30];
};
Database data;

void handleNewMessages(int numNewMessages, const char* chat_id);
void setupWiFi();
void setupFirebase();
void handleCommands(const char* text, const char* chat_id);
void handleAddData(const char* chat_id);
void handleUpdateData(const char* chat_id);
void handleDeleteData(const char* chat_id);
void handleListData(const char* chat_id);
void handleOptions(const char* chat_id);
void handleStart(const char* chat_id);

void setup(){
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, 0);
  setupWiFi();
  setupFirebase();
  botLastTime = millis();
}

void loop(){
  if (millis() > botLastTime + BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    if (numNewMessages > 0) {
      handleNewMessages(numNewMessages, TELE_CHAT_ID);
    } 
    botLastTime = millis();
  }
}

void handleNewMessages(int numNewMessages, const char* chat_id) {
  for (int i = 0; i < numNewMessages; i++) {
    const char* cid = bot.messages[i].chat_id.c_str();
    if (strcmp(cid, chat_id) != 0) {
      bot.sendMessage(chat_id, "Unautorized user", "");
      continue;
    }
    const char* text = bot.messages[i].text.c_str();
    handleCommands(text, chat_id);
  }
}

void handleCommands(const char* text, const char* chat_id) {
  Serial.print("Perintah diterima: ");
  Serial.println(text);

  if (opsi == "NONE") {
    if (strcmp(text, "/start") == 0) {
      ledBlink();
      handleStart(chat_id);
    } else if (strcmp(text, "/addData") == 0) {
      ledBlink();
      handleAddData(chat_id);
    } else if (strcmp(text, "/updateData") == 0) {
      ledBlink();
      handleUpdateData(chat_id);
    } else if (strcmp(text, "/deleteData") == 0) {
      ledBlink();
      handleDeleteData(chat_id);
    } else if (strcmp(text, "/listData") == 0) {
      ledBlink();
      handleListData(chat_id);
    } else if (strcmp(text, "/options") == 0) {
      ledBlink();
      handleOptions(chat_id);
    }
  } else {
    handleStateCommands(text, chat_id);
  }
}

void handleStateCommands(const char* text, const char* chat_id) {
  if (opsi == "ADD_NAME") {
    strncpy(data.name, text, sizeof(data.name) - 1);
    data.name[sizeof(data.name) - 1] = '\0';
    bot.sendMessage(chat_id, F("Masukkan umur:"), "");
    opsi = "ADD_AGE";
  } else if (opsi == "ADD_AGE") {
    strncpy(data.age, text, sizeof(data.age) - 1);
    data.age[sizeof(data.age) - 1] = '\0';
    bot.sendMessage(chat_id, F("Masukkan posisi:"), "");
    opsi = "ADD_POSITION";
  } else if (opsi == "ADD_POSITION") {
    strncpy(data.position, text, sizeof(data.position) - 1);
    data.position[sizeof(data.position) - 1] = '\0';
    bot.sendMessage(chat_id, F("Masukkan status:"), "");
    opsi = "ADD_STATUS";
  } else if (opsi == "ADD_STATUS") {
    strncpy(data.status, text, sizeof(data.status) - 1);
    data.status[sizeof(data.status) - 1] = '\0';
    addData(chat_id);
    opsi = "NONE";
  } else if (opsi == "UPDATE_ID") {
    idupdate = String(text);
    bot.sendMessage(chat_id, F("Masukkan nama baru:"), "");
    opsi = "UPDATE_NAME";
  } else if (opsi == "UPDATE_NAME") {
    strncpy(data.name, text, sizeof(data.name) - 1);
    data.name[sizeof(data.name) - 1] = '\0';
    bot.sendMessage(chat_id, F("Masukkan umur baru:"), "");
    opsi = "UPDATE_AGE";
  } else if (opsi == "UPDATE_AGE") {
    strncpy(data.age, text, sizeof(data.age) - 1);
    data.age[sizeof(data.age) - 1] = '\0';
    bot.sendMessage(chat_id, F("Masukkan posisi baru:"), "");
    opsi = "UPDATE_POSITION";
  } else if (opsi == "UPDATE_POSITION") {
    strncpy(data.position, text, sizeof(data.position) - 1);
    data.position[sizeof(data.position) - 1] = '\0';
    bot.sendMessage(chat_id, F("Masukkan status baru:"), "");
    opsi = "UPDATE_STATUS";
  } else if (opsi == "UPDATE_STATUS") {
    strncpy(data.status, text, sizeof(data.status) - 1);
    data.status[sizeof(data.status) - 1] = '\0';
    updateData(chat_id, idupdate);
    opsi = "NONE";
  } else if (opsi == "DELETE_ID") {
    String iddelete = String(text);
    deleteData(chat_id, iddelete);
    opsi = "NONE";
  }
}

void handleAddData(const char* chat_id) {
  bot.sendMessage(chat_id, F("Masukkan nama:"), "");
  opsi = "ADD_NAME";
}

void handleUpdateData(const char* chat_id) {
  bot.sendMessage(chat_id, F("Masukkan Id karyawan yang akan diperbarui:"), "");
  opsi = "UPDATE_ID";
}

void handleDeleteData(const char* chat_id) {
  bot.sendMessage(chat_id, F("Masukkan Id karyawan yang akan dihapus:"), "");
  opsi = "DELETE_ID";
}

void handleListData(const char* chat_id) {
  String Msg = F("Data karyawan:\n\n");
  if (Firebase.RTDB.getJSON(&fbdo, "/data")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, fbdo.jsonString());
    JsonObject obj = doc.as<JsonObject>();
    for (JsonPair pair : obj) {
      const char* id = pair.key().c_str();
      JsonObject fbData = pair.value().as<JsonObject>();
      const char* name = fbData["name"];
      const char* age = fbData["age"];
      const char* position = fbData["position"];
      const char* status = fbData["status"];
      Msg += F("Id: ");
      Msg += String(id);
      Msg += F("\nNama: ");
      Msg += String(name);
      Msg += F("\nUmur: ");
      Msg += String(age);
      Msg += F("\nPosisi: ");
      Msg += String(position);
      Msg += F("\nStatus: ");
      Msg += String(status);
      Msg += F("\n\n");
    }
    bot.sendMessage(chat_id, Msg, "");
  } else {
    bot.sendMessage(chat_id, String(F("Gagal mendapatkan data karyawan: ")) + fbdo.errorReason(), "");
  }
}

void handleOptions(const char* chat_id) {
  const char keyboardJson[] PROGMEM = "[[\"/addData\", \"/updateData\", \"/deleteData\", \"/listData\"]]";
  bot.sendMessageWithReplyKeyboard(chat_id, F("Pilih salah satu opsi berikut"), "", keyboardJson, true);
}

void handleStart(const char* chat_id) {
  const char Msg[] PROGMEM = "Selamat datang di Bot Manajemen Karyawan.\n"
                             "Gunakan perintah berikut untuk mengelola karyawan:\n"
                             "/addData    - Tambah karyawan baru\n"
                             "/updateData - Perbarui data karyawan\n"
                             "/deleteData - Hapus data karyawan\n"
                             "/listData   - Tampilkan daftar semua karyawan\n"
                             "/options    - Tampilkan opsi perintah\n";
  bot.sendMessage(chat_id, Msg, "Markdown");
}

void setupWiFi() {
  Serial.print(F("Menyambungkan WiFi"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(F("Menyambungkan WiFi.."));
  }
  Serial.println(F("WiFi telah tersambung"));
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP());
}

void setupFirebase() {
  auth.user.email = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASS;
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_URL;
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);
  while ((auth.token.uid) == "") {
    Serial.print(F('.'));
    delay(1000);
  }
  uid = auth.token.uid.c_str();
  Serial.print(F("User UID: "));
  Serial.println(uid);
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  Serial.println(F("Berhasil tersambung Firebase"));
}

void ledBlink() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
  delay(100);
}

void addData(const char* chat_id) {
  FirebaseJson json;
  json.add(F("name"), data.name);
  json.add(F("age"), data.age);
  json.add(F("position"), data.position);
  json.add(F("status"), data.status);

  if (Firebase.RTDB.push(&fbdo, "/data", &json)) {
    bot.sendMessage(chat_id, String(F("Data karyawan ditambahkan: ")) + data.name + F(", ") + data.age + F(", ") + data.position + F(", ") + data.status);
  } else {
    bot.sendMessage(chat_id, String(F("Gagal menambahkan karyawan")) + fbdo.errorReason(), "");
  }
}

void updateData(const char* chat_id, String id) {
  FirebaseJson json;
  json.add(F("name"), data.name);
  json.add(F("age"), data.age);
  json.add(F("position"), data.position);
  json.add(F("status"), data.status);

  if (Firebase.RTDB.updateNode(&fbdo, String(F("/data/")) + String(id), &json)) {
    bot.sendMessage(chat_id, String(F("Karyawan diperbarui: ")) + data.name + F(", ") + data.age + F(", ") + data.position + F(", ") + data.status);
  } else {
    bot.sendMessage(chat_id, String(F("Gagal memperbarui karyawan")) + fbdo.errorReason(), "");
  }
}

void deleteData(const char* chat_id, String id) {
  if (Firebase.RTDB.deleteNode(&fbdo, String("/data/") + id)) {
    bot.sendMessage(chat_id, F("Karyawan dihapus"), "");
  } else {
    bot.sendMessage(chat_id, String(F("Gagal menghapus karyawan")) + fbdo.errorReason(), "");
  }
}