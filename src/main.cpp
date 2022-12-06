#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

#define MAX_CLIENT 16
#define TOKEN_LENGTH 16
#define LED 16
#define FAN 5
#define LOCK 4
#define DHTPIN 0
#define WIFI_PIN 2

AsyncWebServer server(80); 
AsyncWebSocket ws("/ws");
DHT dht(DHTPIN, DHT22);
String client_token[MAX_CLIENT];
int client_timeout[MAX_CLIENT];
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
const unsigned long interval1 = 5000;
const unsigned long interval2 = 3000;
float hum, tem, old_temp, old_hum;
String ssid, pass, ip, gateway, uname, password;
IPAddress localIP, localGateway, subnet(255, 255, 255, 0);
bool restart = false;

bool initWifi() {
    if(ssid=="" || pass=="" || ip =="" || gateway==""){
      return false;
    }
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    localIP.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());

    if (!WiFi.config(localIP, localGateway, subnet)){
        return false;
    }

    WiFi.begin(ssid.c_str(), pass.c_str());
    int count = 0;
    digitalWrite(WIFI_PIN, LOW);
    while(WiFi.status() != WL_CONNECTED) {
        if (count >= 40) {
            return false;
        }
        delay(500);
        digitalWrite(WIFI_PIN, !(digitalRead(WIFI_PIN)));
        count++;    
    }
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
}
String create_token(int len) {
   String res;
   char sample[60] = {'a','b','c','d','e','f','g','h','i','k','l','m','n','o','p','q','w','r','t','y','u','j','z','x','v',
                     'A','B','C','D','E','F','G','H','I','K','L','M','N','O','P','Q','W','R','T','Y','U','J','Z','X','V',
                     '1','2','3','4','5','6','7','8','9','0'};
   for (uint8_t i = 0; i < len; i++) {
      res += sample[random(0,60)];
   }
   return res;
}
void client_init() {
    for (int i = 0; i < MAX_CLIENT; i++) {
        client_token[i] = "/";
        client_timeout[i] = -1;
    }
}
void add_client(String token) { 
  for (int i = 0; i < MAX_CLIENT; i++) {
    if (client_token[i] == "/") {
      client_token[i] = token;
      client_timeout[i] = 0;
      return;
    }
  }
}
bool check_token(String token) {
  for (int i = 0; i < MAX_CLIENT; i++) {
    if (token == client_token[i] && token != "/") {
      return true;
    }
  }
  return false;
}
void update_client() {
  for (int i = 0; i < MAX_CLIENT; i++) {
    if (client_timeout[i] >= 6) {
      Serial.print("Remove client ");
      Serial.println(client_token[i]);
      client_token[i] = "/";
      client_timeout[i] = -1;
    }
    if (client_token[i] != "/") {
      client_timeout[i]++; 
    }
  }
}
boolean isNumeric(String str) {
    unsigned int stringLength = str.length();
    if (stringLength == 0) {
        return false;
    }
    boolean seenDecimal = false;
    for(unsigned int i = 0; i < stringLength; ++i) {
        if (isDigit(str.charAt(i))) {
            continue;
        }
        if (str.charAt(i) == '.') {
            if (seenDecimal) {
                return false;
            }
            seenDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}
String readFile(fs::FS &fs, const char * path) {
    File file = fs.open(path, "r");
    String fileContent;
    fileContent = file.readStringUntil('\n');
    file.close();
    return fileContent;
}
void writeFile(fs::FS &fs, const char * path, const char * message){ 
  File file = fs.open(path, "w");
  file.print(message);
  file.close();
}
void fileInit() {
    ssid = readFile(LittleFS, "/wifi_data/ssid.txt");
    pass = readFile(LittleFS, "/wifi_data/password.txt");
    ip = readFile(LittleFS, "/wifi_data/ip.txt");
    gateway = readFile(LittleFS, "/wifi_data/gateway.txt");
    uname = readFile(LittleFS, "/login_data/username.txt");
    password = readFile(LittleFS, "/login_data/password.txt");
}

void login_page(AsyncWebServerRequest *request) {
    request->send(LittleFS, "/page/login.html", "text/html");
}
void dashboard_page(AsyncWebServerRequest *request) {
    request->send(LittleFS, "/page/dashboard.html", "text/html");
}
void setting_page(AsyncWebServerRequest *request) {
    request->send(LittleFS, "/page/setting.html", "text/html");
}
void wifimanager_page(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/page/wifimanager.html", "text/html");
}
void wifimanager_post(AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      if (p->name() == "ssid") {
        ssid = p->value().c_str();
        writeFile(LittleFS, "/wifi_data/ssid.txt", ssid.c_str());
      }
      if (p->name() == "pass") {
        pass = p->value().c_str();
        writeFile(LittleFS, "/wifi_data/password.txt", pass.c_str());
      }
      if (p->name() == "ip") {
        ip = p->value().c_str();
        writeFile(LittleFS, "/wifi_data/ip.txt", ip.c_str());
      }
      if (p->name() == "gateway") {
        gateway = p->value().c_str();
        writeFile(LittleFS, "/wifi_data/gateway.txt", gateway.c_str());
      }
    }
  }
  restart = true;
  request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
}
void login(AsyncWebServerRequest *request) {
    bool isUname = false;
    bool isPass = false;
    int params = request->params();
    for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            if (p->name() == "uname") {
                if (String(p->value().c_str()) == uname) {
                    isUname = true;
                }
            }
            if (p->name() == "pass") {
                if (String(p->value().c_str()) == password) {
                    isPass = true;
                }
            }
        }
    }
    if (isUname && isPass) {
        String token = create_token(TOKEN_LENGTH);
        add_client(token);
        request->send(200, "text/plain", token);
    } else {
        request->send(200, "text/plain", "!");
    }
}
void check_Token(AsyncWebServerRequest *request) {
    bool isAutho = false;
    int params = request->params();
    for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            if (p->name() == "token") {
                if (check_token(p->value().c_str())) {
                    isAutho = true;
                } 
            }
        }
    }
    if (isAutho) {
        request->send(200, "text/plain", "@");
    } else {
        request->send(200, "text/plain", "!");
    }
}
void get_state(AsyncWebServerRequest *request) {
    bool isAutho = false;
    int params = request->params();
    for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            if (p->name() == "token") {
                if (check_token(p->value().c_str())) {
                    isAutho = true;
                } 
            }
        }
    }
    if (isAutho) {
        String payload;
        payload += String(digitalRead(LED));
        payload += String(digitalRead(FAN));
        payload += String(digitalRead(LOCK));
        request->send(200, "text/plain", payload);
    } else {
        request->send(200, "text/plain", "!");
    }
}
void toggle(AsyncWebServerRequest *request) {
    bool isAutho = false;
    String dev, state;
    int params = request->params();
    for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            if (p->name() == "token") {
                if (check_token(p->value().c_str())) {
                    isAutho = true;
                }    
            }
            if (p->name() == "dev") {
                dev = String(p->value().c_str());
            }
            if (p->name() == "state") {
                state = String(p->value().c_str());
            }
        }
    }
    
    if (!isAutho) {
        request->send(200, "text/plain", "!");
        return;
    }
    if (dev != "led" && dev != "fan" && dev != "lock") {
        request->send(200, "text/plain", "!");
        return;
    }
    if (!isNumeric(state)) {
        request->send(200, "text/plain", "!");
        return;
    }
    String wsPayload;
    if (dev == "led") {
        digitalWrite(LED, state.toInt());
        wsPayload = String("TOGGLE led ") + String(digitalRead(LED));
        ws.textAll(wsPayload);
        request->send(200, "text/plain", "@");
    }
    else if (dev == "fan") {
        digitalWrite(FAN, state.toInt());
        wsPayload = String("TOGGLE fan ") + String(digitalRead(FAN));
        ws.textAll(wsPayload);
        request->send(200, "text/plain", "@");
    }
    else if (dev == "lock") {
        digitalWrite(LOCK, state.toInt());
        wsPayload = String("TOGGLE lock ") + String(digitalRead(LOCK));
        ws.textAll(wsPayload);
        request->send(200, "text/plain", "@");
    }
}
void change_pw(AsyncWebServerRequest *request) {
    bool isAutho = false;
    bool isCorrect = false;
    String new_pw;
    int params = request->params();
    for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            if (p->name() == "token") {
                if (check_token(p->value().c_str())) {
                    isAutho = true;
                }    
            }
            if (p->name() == "old_pw") {
                if (String(p->value().c_str()) == password) {
                    isCorrect = true;
                }    
            }
            if (p->name() == "new_pw") {
                new_pw = String(p->value().c_str());
            }
        }
    }
    if (isAutho && isCorrect) {
        writeFile(LittleFS, "/login_data/password.txt", new_pw.c_str());
        password = readFile(LittleFS, "/login_data/password.txt");
        request->send(200, "text/plain", "@");
    } else {
        request->send(200, "text/plain", "!");
    }
}

void resetWiFi(AsyncWebServerRequest *request) {
    bool isAutho = false;
    bool isCorrect = false;
    int params = request->params();
    for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            if (p->name() == "token") {
                if (check_token(p->value().c_str())) {
                    isAutho = true;
                }    
            }
            if (p->name() == "pass") {
                if (String(p->value().c_str()) == password) {
                    isCorrect = true;
                }    
            }
        }
    }
    if (isAutho && isCorrect) {
        writeFile(LittleFS, "/wifi_data/ssid.txt", "");
        writeFile(LittleFS, "/wifi_data/password.txt", "");
        writeFile(LittleFS, "/wifi_data/ip.txt", "");
        writeFile(LittleFS, "/wifi_data/gateway.txt", "");
        restart = true;
        request->send(200, "text/plain", "@");
    } else {
        request->send(200, "text/plain", "!");
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    
    if (String((char*)data).length() != TOKEN_LENGTH) {
      return;
    }

    for (uint8_t i=0;i < MAX_CLIENT; i++) {
      if (strcmp((char*)data, client_token[i].c_str()) == 0) {
        client_timeout[i] = 0;
        return;
      }
    }
    
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        break;
      case WS_EVT_DISCONNECT:
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}


void setup() {
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    pinMode(FAN, OUTPUT);
    pinMode(LOCK, OUTPUT);
    pinMode(DHTPIN, INPUT);
    pinMode(WIFI_PIN, OUTPUT);
    LittleFS.begin();
    fileInit();
    dht.begin();
    if (initWifi()) {
        digitalWrite(WIFI_PIN, HIGH);
        server.on("/", HTTP_GET, login_page);
        server.on("/dashboard", HTTP_GET, dashboard_page);
        server.on("/setting", HTTP_GET, setting_page);
        server.on("/login", HTTP_POST, login);
        server.on("/check_token", HTTP_POST, check_Token);
        server.on("/state", HTTP_POST, get_state);
        server.on("/toggle", HTTP_POST, toggle);
        server.on("/change_pw", HTTP_POST, change_pw);
        server.on("/reset_wifi", HTTP_POST, resetWiFi);
        ws.onEvent(onEvent);
        server.addHandler(&ws);
        client_init();
        old_temp = dht.readTemperature();
        old_hum = dht.readHumidity();
    } else {
        digitalWrite(WIFI_PIN, LOW);
        WiFi.softAP("ESP8266-WIFI-MANAGER");
        server.on("/", HTTP_GET, wifimanager_page);
        server.on("/", HTTP_POST, wifimanager_post);
    }
    
    server.begin();
}


void loop() {
    ws.cleanupClients();

    tem = dht.readTemperature();
    hum = dht.readHumidity();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis1 >= interval1) {
        previousMillis1 = currentMillis;
        update_client();
    }
    if (currentMillis - previousMillis2 >= interval2) {
        previousMillis2 = currentMillis;
        if (!isnan(tem) && !isnan(hum)) {
            if (tem != old_temp || hum != old_hum) { 
                String payload = String("SENSOR ") + String(tem) + ' ' + String(hum);
                ws.textAll(payload);
                old_temp = tem;
                old_hum = hum;
            }
        }
    }
    if (restart){
        delay(5000);
        ESP.restart();
    }
    delay(100);
}