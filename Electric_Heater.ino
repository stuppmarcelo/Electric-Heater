/******************************************
** 09/02/2024
** Electric Heater
** V1.0
***********************************/
#include <DHT.h>
#include <Ticker.h>
#include <Wire.h> 
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Time.h>

//**************** Macros **************** 
#define CLK 1  // Interrupt time in ms

#define DHTTYPE DHT21
#define DHTPIN 5
#define OUTLOAD 4
#define MANUALINP 35

#define EEPROM_SIZE 12
#define SETPOINTADDRESS 0

#define MAXTIMERFUNCTIONS 50
#define MAXTIMERSENSOR 30
#define MAXTIMERDISPLAY 15
#define MAXTIMERCONTROL 1000
#define MAXTIMERANALOG 1000
#define MAXTIMERINFORMATION 5000
#define MAXTIMERERROR 10000
#define MAXTIMEREFFECT 3000
#define MAXTIMERBLINK 500
#define TIMEBLINK 350

#define KP 0.5
#define KI 0.005
#define MAXI 0.40
#define SECURETEMP 30

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

//***************** Variáveis *********************
float temp = 24.00;
float humidity = 50.00;

float P = 0.00;
float I = 0.00;
float PID = 0.00;
float oldError = 0.00;
float error = 0.00;

uint16_t timerFunctions = 0;
uint16_t timerDisplay = 0;
uint16_t timerSensor = 0;
uint16_t timerControl = 0;
uint16_t timerAnalog = 0;
uint16_t timerInformation = 0;
uint16_t timerError = 0;
uint16_t timerEffect = 0;
uint16_t timerBlink = 0;
uint16_t milisegundos = 0;

uint8_t setpoint = 22;
uint8_t bestHumidity = 50;
uint8_t horas = 0;
uint8_t minutos = 0;
uint8_t segundos = 0;
uint8_t input_start_horas = 22;
uint8_t input_start_minutos = 0;
uint8_t input_end_horas = 8;
uint8_t input_end_minutos = 0;
uint8_t ErrorCounter = 0;

bool Error = false;
bool starting = true;
bool information = true;
bool effect = false;
bool nightMode = false;

//**************** Display parameters **************** 
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//**************** Sensor parameters **************** 
DHT dht(DHTPIN, DHTTYPE);

//**************** Timer parameters **************** 
hw_timer_t *MyTimer = NULL;


//*************** Web Parameters ***************
const char* ssid = "SSID";
const char* password = "Password";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

WebServer server(80);

//*************** Function prototipes ***************
void functions();
void readSensor();
void readAnalog();
void display();
void control();
void handleRoot();
void handleUpdateInformations();
bool getLocalTime();
void interruptAction();


void setup() 
{
  //**************** OTA Configuration *****************
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while ((WiFi.waitForConnectResult() != WL_CONNECTED) && (ErrorCounter < 10)) {
    Serial.println("Connection Failed! Rebooting...");
    delay(3000);
    ErrorCounter++;
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("SMART-HEATER");

  // No authentication by default
  ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.on("/", handleRoot);
  server.on("/updateInformations", HTTP_POST, handleUpdateInformations);

  server.begin();

  //*************** END OTA Configuration ****************



  //***************** Timer Setup ***********************
  MyTimer = timerBegin(0, 160, true);
  timerAttachInterrupt(MyTimer, &interruptAction, true);
  timerAlarmWrite(MyTimer, 1000, true);
  timerAlarmEnable(MyTimer); //Just Enable


  //*************** EEPROM Configuration ******************
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(SETPOINTADDRESS, setpoint);
  


  dht.begin();

  pinMode(OUTLOAD, OUTPUT);

  //set ADC resolution to 12 bits (0-4096)
  analogReadResolution(12);

  delay(10);

  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  delay(100);
  
  while(starting) display();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  do {
      getLocalTime();
      delay(100);
      ErrorCounter++;
  } while(getLocalTime() && ErrorCounter < 10);


  if (horas >= input_start_horas && minutos >= input_start_minutos && horas <= input_end_horas && minutos <= input_end_minutos) nightMode = true;
}

void loop() 
{
  ArduinoOTA.handle();
  server.handleClient();
  
  if (!timerFunctions) functions();
  if (!timerAnalog) readAnalog();
  if (!timerSensor) readSensor();
  if (!timerDisplay) display();
  if (!timerControl) control();

  if (timerError > MAXTIMERERROR) ESP.restart();

}

void functions()
{
  if (horas == input_start_horas && minutos == input_start_minutos) nightMode = true;
  else if (horas == input_end_horas && minutos == input_end_minutos) nightMode = false;
  

  timerFunctions = MAXTIMERFUNCTIONS;
}

void readSensor()
{
  float input = 0.00;
  const float factorExternalTemp = 0.05;
  static double oldExternalTemp = 25.00;

  input = (dht.readTemperature()) - 1.8;

  temp = (input * factorExternalTemp) + (oldExternalTemp * (1 - factorExternalTemp));

  oldExternalTemp = temp;

  
  humidity = dht.readHumidity();
  

  if (isnan(temp) || isnan(humidity)) Error = true;

  else if (temp > SECURETEMP) Error = true;

  else Error = false;
  
  timerSensor = MAXTIMERSENSOR;
}

void readAnalog ()
{
  uint16_t input = 0;
  float variation = 0.00;
  static uint16_t oldInput = 5000;

  input = analogRead(MANUALINP);

  if (oldInput == 5000) oldInput = input;
  
  variation = (float) input / oldInput;
  
  input = (uint16_t) input / 341;

  
  if (variation > 1.04 || variation < 0.96) 
  {
    if (setpoint > 30) setpoint = 30;
    if (setpoint < 18) setpoint = 18;

    oldInput = analogRead(MANUALINP);
    effect = true;
    timerEffect = 0;
    setpoint = (uint8_t) input + 18;

    EEPROM.put(SETPOINTADDRESS, setpoint);
    EEPROM.commit();
  }

 timerAnalog = MAXTIMERANALOG;
}

void display()
{
  uint8_t Temp = temp;
  uint8_t Humidity = humidity;
  uint8_t TracePoint = (uint8_t) PID * 100;

  oled.clearDisplay();
  oled.setTextSize(3);
  oled.setTextColor(WHITE);

  
  if (starting)
  {
    static int y = 128;

    oled.setCursor(y, 8);
    oled.println("SMART HEATER");
    delay(5);
    
    if (y > -95) y--;
    else 
    {
      delay(500);
      starting = false;
    }
  }
  else if (effect)
  {
    if (timerBlink < TIMEBLINK)
    {
      oled.setCursor(22, 8);
      oled.println("S");
      oled.setCursor(48, 8);
      oled.println(setpoint);
      oled.setCursor(82, 8);
      oled.println(char(248));
      oled.setCursor(98, 8);
      oled.println("C");
    }
  }
  else if (nightMode)
  {
    oled.setCursor(TracePoint, 8);
    oled.println(char(248));
  }
  else
  {
    if (information)
    {
      oled.setCursor(24, 8);
      oled.println(Temp);
      oled.setCursor(58, 8);
      oled.println(char(248));
      oled.setCursor(74, 8);
      oled.println("C");
    }
    else
    {
      oled.setCursor(28, 8);
      oled.println(Humidity);
      oled.setCursor(74, 8);
      oled.println("%");
    }
  }
  oled.display();

  timerDisplay = MAXTIMERDISPLAY;
}

void control()
{
  //Controle de aquecimento
  if (!Error)
  {
    error = setpoint - temp;

    P = error * KP;

    I += error * KI;

    oldError = error;

    if (I > MAXI) I = MAXI;

    if (I < 0.00) I = 0.00;

    PID = P + I;

    if (PID > 0.5) digitalWrite(OUTLOAD, HIGH);

    else digitalWrite(OUTLOAD, LOW);
  }

  else digitalWrite(OUTLOAD, LOW);


  timerControl = MAXTIMERCONTROL;
}

void handleRoot() 
{
  String html = "<html><body>";
  html += "<meta http-equiv='refresh' content='30'>";
  html += "<h1>SMART HEATER</h1>";

  // Adiciona a informação de horas, minutos e segundos
  html += "<h2>Hora atual: " + String(horas) + ":" + String(minutos) + ":" + String(segundos) + "</h2>";

  // Adiciona os campos de entrada para as horas e minutos de início e fim
  html += "<form action='/updateInformations' method='post'>";
  html += "Hora de inicio (HH:MM): <input type='text' name='input_start_horas' value='" + String(input_start_horas) + "'>:<input type='text' name='input_start_minutos' value='" + String(input_start_minutos) + "'><br>";
  html += "Hora de fim (HH:MM): <input type='text' name='input_end_horas' value='" + String(input_end_horas) + "'>:<input type='text' name='input_end_minutos' value='" + String(input_end_minutos) + "'><br>";

  // Adiciona o botão toggle para a variável nightMode
  html += "<h2>Modo Noturno: <input type='checkbox' name='nightMode' " + String(nightMode ? "checked" : "") + "></h2>";

  // Adiciona os dados atuais
  html += "<h2>Setpoint: " + String(setpoint) + "</h2>";
  html += "<h2>Temperatura: " + String(temp) + "</h2>";
  html += "<h2>Umidade: " + String(humidity) + "</h2>";

  // Adiciona o formulário para atualizar o setpoint
  html += "Novo Setpoint: <input type='text' name='setpoint' value='" + String(setpoint) + "'><br><br>";
  html += "<div style='width: 100%; '><input type='submit' value='Salvar' style='width: 200px;'></div>";
  html += "</form>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleUpdateInformations() 
{
  if (server.hasArg("setpoint")) {
    setpoint = server.arg("setpoint").toInt();
    if (setpoint > 30) setpoint = 30;
    if (setpoint < 18) setpoint = 18;
    EEPROM.put(SETPOINTADDRESS, setpoint);
    EEPROM.commit();
  }

  // Atualiza os valores de input_start_horas, input_start_minutos, input_end_horas e input_end_minutos
  if (server.hasArg("input_start_horas")) {
    input_start_horas = server.arg("input_start_horas").toInt();
  }

  if (server.hasArg("input_start_minutos")) {
    input_start_minutos = server.arg("input_start_minutos").toInt();
  }

  if (server.hasArg("input_end_horas")) {
    input_end_horas = server.arg("input_end_horas").toInt();
  }

  if (server.hasArg("input_end_minutos")) {
    input_end_minutos = server.arg("input_end_minutos").toInt();
  }

  // Atualiza a variável nightMode
  if (server.hasArg("nightMode")) {
    nightMode = !nightMode; // Inverte o valor do nightMode
  }

  server.sendHeader("Location", "/");
  server.send(303);
}

bool getLocalTime()
{
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo)) return true;

  horas = timeinfo.tm_hour;
  minutos = timeinfo.tm_min;
  segundos = timeinfo.tm_sec;

  return false;
}

void IRAM_ATTR interruptAction()
{
  if (timerSensor) timerSensor--;
  if (timerFunctions) timerFunctions--;
  if (timerAnalog) timerAnalog--;
  if (timerDisplay) timerDisplay--;
  if (timerControl) timerControl--;
  if (Error) timerError++;
  else timerError = 0;
  if (timerInformation) timerInformation--;
  else
  {
    information = !information;
    timerInformation = MAXTIMERINFORMATION;
  }
  if (effect)
  {
    timerEffect++;

    if (timerEffect > MAXTIMEREFFECT) effect = false;
  
    if (timerBlink) timerBlink--;
    else timerBlink = MAXTIMERBLINK;
  }

  if (milisegundos < 999) milisegundos++;
  else
  {
    milisegundos = 0;
    segundos++;

    if (segundos < 59) segundos++;
    else
    {
      segundos = 0;
      minutos++;

      if (minutos < 59) minutos++;
      else
      {
        minutos = 0;
        horas++;

        if (horas < 23) horas++;
        else
        {
          horas = 0;
          getLocalTime();
        }
      }
        
    }
  }

}