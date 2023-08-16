//Librerias
#include <DHT.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <addons/TokenHelper.h>
#include <Adafruit_NeoPixel.h>

//Sensor de infrarrojo
#define InfraSensor 18

//Foto resistencia
#define FotoRes 4

//Sensor de temperatura
#define DhtType DHT11
#define DhtPin 15
DHT dht (DhtPin, DhtType);

//Detector de corriente
#define Corriente 14

//Iman puerta
#define Iman 5

//LED PROCESO
#define Led 27

//Luces Led
#define BLEDs 13
#define NumPixeles 125
Adafruit_NeoPixel pixeles(NumPixeles, BLEDs, NEO_GRB + NEO_KHZ800);

//Firestore
#define FIREBASE_PROJECT_ID "aslogic-monitor"
#define API_KEY 
#define USER_EMAIL "aslogicempresa@gmail.com"
#define USER_PASSWORD "$CCAI&2022#"

//Variables
uint32_t TiempoLed = 0, TiempoTemp = 0;
int Temp, Hume, TempAux, HumeAux;
bool BanderaEnergia = true, BanderaRack = true, BanderaLuz = true;
//WiFi
const char* ssid = "Wikipy Note20" ; //Nombre de la red
const char* pass = "SoyUnaContraseñaDeVerdad"; //Contraseña de la red
String NumeroRack = "1", NumeroSite = "1", documentPath = "Site1/Rack1";
//Firestore objetos
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson content;

HTTPClient http;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(InfraSensor, INPUT);
  pinMode(FotoRes, INPUT);
  pinMode(Corriente, INPUT);
  pinMode(Iman, INPUT_PULLUP);
  pinMode(Led, OUTPUT);
  dht.begin();
  WiFi.begin(ssid, pass);
  Serial.print("Se está conectado a la red WiFi denominada: ");
  Serial.print(ssid);
  Serial.println();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if(millis() > 5000){
      ESP.restart();
    }
  }
  Serial.println("WiFi connected");
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Conectado");
  content.set("fields/Energia/booleanValue", true);
  content.set("fields/Humedad/integerValue", String(0));
  content.set("fields/Luz/booleanValue", true);
  content.set("fields/Puertas/booleanValue", true);
  content.set("fields/Temperatura/integerValue", String(0));
  SubirDatos();
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite (Led, HIGH);
  if (millis() - TiempoLed >= 500)
  {
    TiempoLed = millis();
    digitalWrite (Led, LOW);
  }
  if (digitalRead(Corriente) == LOW)
  {
    if (BanderaEnergia) {
      Notificacion("No hay energia");
      content.set("fields/Energia/booleanValue", false);
      SubirDatos();
      BanderaEnergia = false;
    }
  }
  else
  {
    if (!BanderaEnergia) {
      Notificacion("Regreso la energia");
      content.set("fields/Energia/booleanValue", true);
      SubirDatos();
      BanderaEnergia = true;
    }
  }
  RegistrarTH();
  if (Temp != TempAux && Hume != HumeAux) {
    content.set("fields/Humedad/integerValue", String(Hume));
    content.set("fields/Temperatura/integerValue", String(Temp));
    SubirDatos();
    TempAux = Temp;
    HumeAux = Hume;
  }
  if (Temp >= 26 && (millis() - TiempoTemp) >= 30000) {
    Notificacion("La temperatura ha sobrepasado el rango de seguridad " + (String)Temp + "°C");
    TiempoTemp = millis();
  }

  if (digitalRead(FotoRes) == LOW)
  {
    ControlLuces();
    if (!BanderaLuz) {
      content.set("fields/Luz/booleanValue", true);
      SubirDatos();
      BanderaLuz = true;
    }
  }
  else
  {
    if (BanderaLuz) {
      content.set("fields/Luz/booleanValue", false);
      SubirDatos();
      BanderaLuz = false;
    }
    LucesLed(0, 0, 0, true);
  }
}

void RegistrarTH() {
  Temp = (int)dht.readTemperature();
  Hume = (int)dht.readHumidity();
  if (isnan(Temp) || isnan(Hume)) {
    RegistrarTH();
  }
}

void ControlLuces() {
  if (digitalRead(Iman) == HIGH) {
    if (BanderaRack) {
      Notificacion("La puerta fue abierta");
      content.set("fields/Puertas/booleanValue", true);
      SubirDatos();
      BanderaRack = false;
    }
    LucesLed(0, 255, 0, true);
  } else {
    if (!BanderaRack) {
      Notificacion("Se cerro la puerta");
      content.set("fields/Puertas/booleanValue", false);
      SubirDatos();
      BanderaRack = true;
    }
    if (digitalRead(InfraSensor) == LOW) {
      LucesLed(255, 0, 0, true);
    } else {
      LucesLed(0, 0, 255, false);
    }

  }
}
void Notificacion(String Mensaje) {
  http.begin("https://fcm.googleapis.com/fcm/send");
  String data = "{";
  data = data + "\"to\":\"/topics/ASLOGIC\",";
  data = data + "\"notification\": {";
  data = data + "\"body\": \"SITE " + NumeroSite + ": " + Mensaje + " en el rack " + NumeroRack + "\",";
  data = data + "\"title\":\"ASLOGIC\"";
  data = data + "} }";
  http.addHeader("Authorization", "key=AAAA5NiGT1w:APA91bGyh700jtbM5YEaeF1anEf_YlUxuz6iWajyjl8zdxHTAan3UGCHLfhvgINLOZZ8Vbx3VUtZrDEug5-iVetXdLNyIHqtgV3n38R_7pFMHqd0a7z0eT2o0S0E7l3gFP_-bYtO2n6a");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Content-Length", (String)data.length());
  int httpResponseCode = http.POST(data);
}
void LucesLed(int R, int G, int B, bool Sentido) {
  if (Sentido) {
    for (int i = 0; i < NumPixeles; i++) {
      pixeles.setPixelColor(i, pixeles.Color(R, G, B));
      pixeles.show();
    }
  } else {
    for (int i = NumPixeles; i >= 0; i--) {
      pixeles.setPixelColor(i, pixeles.Color(R, G, B));
      pixeles.show();
    }
  }
}
void SubirDatos() {
  struct fb_esp_firestore_document_write_t update_write;
  std::vector<struct fb_esp_firestore_document_write_t> writes;
  update_write.type = fb_esp_firestore_document_write_type_update;
  update_write.update_document_content = content.raw();
  update_write.update_document_path = documentPath.c_str();
  writes.push_back(update_write);
  if (Firebase.Firestore.commitDocument(&fbdo, FIREBASE_PROJECT_ID, "", writes, "" )) {
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
  }
  else {
    SubirDatos();
  }
}
