#include <WiFi.h>
#include <SoftwareSerial.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#define Baudserial 9600
#define BaudSIM800L 19200
#define BaudWifi 115200
#define BOTtoken "5356568417:AAELkVZvlkk1SSEn1oe6Te_Sszk4yfpSMso"  // your Bot Token (Get from Botfather)
#define CHAT_ID "1991461800"

SoftwareSerial SIM800L(23, 22); //Tx , Rx
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };                                                     //mac para modulo wifi
WiFiClient client;
WiFiClientSecure secured;
UniversalTelegramBot bot(BOTtoken, client);

IPAddress serverip(162,243,87,153);                                                   // numeric IP for emoncms.org (no DNS) WAN                 
String apikey = "44710febd8162d15d04f55a5c4a64240";
//String message = "Alerta, movimiento detectado";
int respuesta; 
char alert5[] = "GET http://162.243.87.153/input/post.json?node=1&json={Alerta:5}&apikey=44710febd8162d15d04f55a5c4a64240\r\nHTTP/1.1\r\nHost:emoncms.org\r\nConnection: close\r\n\r\n";              
char alert0[] = "GET http://162.243.87.153/input/post.json?node=1&json={Alerta:0}&apikey=44710febd8162d15d04f55a5c4a64240\r\nHTTP/1.1\r\nHost:emoncms.org\r\nConnection: close\r\n\r\n";
int node = 1; //if 0, not used
const int pir=27;     
int pir_lectura;
boolean pir_estado = LOW;
unsigned long lastConnectionTime = 0;          // ultima conexion al servidor al servidor en milisegundos
boolean lastConnected = false;                 // ciclo principal del estado de la conexion la ultima vez
//const char* ssid     = "IoT";
//const char* password = "joselu30";

const char* ssid     = "DIGIFIBRA-C17F";
const char* password = "HCPDJQCD73";
int Conexion = false;

void setup()
{
  pinMode(22,OUTPUT);
  pinMode(23,OUTPUT);
  pinMode(pir,INPUT);
  pinMode(25, OUTPUT);
  Serial.begin(Baudserial); //Configura velocidad del puerto serie del Arduino
  Serial.begin(BaudWifi);
  SIM800L.begin(BaudSIM800L);
  Activatesensor ();
}

void loop ()
{
  pir_lectura = LOW;
  pir_lectura = digitalRead(pir); //leer el pin del sensor de movimiento PIR
    if (pir_lectura == HIGH) //SI el sensor esta en alto
    { 
      Serial.println(pir_lectura);
      Serial.println("\nEvento Detectado ");
      delay(1000);
      //envio_sms();
      //delay(5000);
      Connect();
      //ConnectWifi();
      delay(5000);
      //bot.sendMessage(CHAT_ID, "Alerta. Intrusión detectada!!", "");
    }
    Serial.println("\nSin Evento ");
    delay(1000);
}
/*
void BotStar(){
  delay(3000);
  Serial.println("\nBoot iniciado");
  bot.sendMessage(CHAT_ID, "Bot started", "");
  
}

void BotSenMessage(){
  bot.sendMessage(CHAT_ID, "Motion detected!!", "");
}*/

void Connect () 
{
  int linkwifi = 0;
  int linkgsm = 0;
  do
  {
     linkwifi = ConnectWifi();
     if (linkwifi == 1)
     {
         power_off();
         delay(1000);
         pir_lectura = LOW;
         linkwifi = 0;
         linkwifi = ConnectWifi();
     }
     else
     {
         power_on();
         linkgsm = ConnectGSM();
         if (linkgsm == 1)
         {
          pir_lectura = LOW;
          linkgsm = 0;
          linkgsm = ConnectGSM();
         }
     }
  } while (linkwifi == 0 and linkgsm == 0);
}

int ConnectWifi()
{
  long rssi;
  long aux;
  int N = 5, intento = 0;
  if(rssi == 0 && intento == 0){
    
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  secured.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conectando...");
    delay(500);
  }
  Serial.println(WiFi.localIP());
  bot.sendMessage(CHAT_ID, "Bot iniciado", "");
  rssi= WiFi.RSSI();
  Serial.println("Potencia de señal WIFI:");
  Serial.println(rssi);
  /*
  do
    {
      Serial.println("------------------------------");
      Serial.println(rssi);
      WiFi.begin(ssid, password);
      delay(1000);
      Serial.println("\nIntento de Conexión ");  
      aux = rssi;
      Serial.println("Potencia de señal: ");
      Serial.println(aux);
      intento++;
      Serial.print("Intento # : ");
      Serial.println(intento);
    }while (aux == 0 and intento < N);*/
   
    if (not (rssi == 0))
    {
      if(WiFi.status() == WL_CONNECTED)
      {
        Serial.print("Conectado a Wifi\n");
        Serial.print("IP: ");
        Serial.println((WiFi.localIP()));
        if (not (WiFi.RSSI() == 0))
        {
          if(client.connect(serverip, 80))
          {
            if(client.connected())
            {
              Send();
              delay(2000);
              Serial.println("Desconectado ...");
              client.stop();
              WiFi.disconnect();
              Conexion = true;
              return 1;
            } 
            else
            {
              Serial.println("Conexion Fallida al Servidor !!!");
              Conexion = false;
              return 0;
            }
          }
        }
        else
        {
          Serial.println("Conexion WIFI se perdió !!!");
          Conexion = false;
          return 0;
        }
      }
    }
    else
    {
      Serial.println("\nIntento de Conexión a Wifi Falló !!!");
      Conexion = false;
      return 0;
    }
    Conexion = false;
    return 0;
}

int ConnectGSM()
{
  Serial.println("Conectando...");
  if(enviarAT("AT+CGATT=1\r", "OK", 3000)==0)
  {
    enviarAT("AT+CFUN=1\r", "OK", 3000);
    enviarAT("AT+CGATT=1\r", "OK", 3000);       //Iniciamos la conexión GPRS
    enviarAT("AT+CSTT=\"web.claro.com.pa\",\"claroweb\",\"claroweb\"", "OK", 15000); //Definimos el APN, usuario y clave a utilizar
    enviarAT("AT+CIICR", "OK", 15000); //Activamos el perfil de datos inalámbrico
    enviarAT("AT+CIFSR", "", 5000); //Activamos el perfil de datos inalámbrico 
  }
  else
  {
   Serial.println("Conectado !!!");
  }
    if(pir_lectura==HIGH)
    {
      return Senddirec();
      //return 1;
    }
    else
    {
     return Senddir();
     //return 1;
    }
}

void Activatesensor ()
{                    
  delay(3000);
  Serial.print("\n Sensor Listo !!!");
  delay(1000);
  Serial.println("\n");
}

int enviarAT(String ATcommand, char* resp_correcta, unsigned int tiempo)
{
  int x = 0;
  bool correcto = 0;
  char respuesta[200];
  unsigned long anterior;
  memset(respuesta, '\0', 199); // Inicializa el string
  delay(100);
  while ( SIM800L.available() > 0) SIM800L.read(); // Limpia el buffer de entrada
  SIM800L.println(ATcommand); // Envia el comando AT
  x = 0;
  anterior = millis();
  // Espera una respuesta
  do 
  {
    if (SIM800L.available() != 0)               // si hay datos el buffer de entrada del UART lee y comprueba la respuesta
    {
        respuesta[x] = SIM800L.read();
        x++;
        if (strstr(respuesta, resp_correcta) != NULL)       // Comprueba si la respuesta es correcta
          {
            correcto = 1;
          }
    }
  }
  while ((correcto == 0) && ((millis() - anterior) < tiempo));    // Espera hasta tener una respuesta
  Serial.println(respuesta);
  
  while (SIM800L.available() > 0) SIM800L.read(); // Limpia el buffer de entrada
  
  return correcto;
}

void power_on()
{
  int respuesta = 0;
  if (enviarAT("AT", "OK", 2000) == 0)          // Comprueba que el modulo SIM900 esta arrancado
  {
      Serial.println("Encendiendo Modulo GSM...");
      pinMode(25, OUTPUT);
      digitalWrite(25, HIGH);
      delay(2000);
      while (respuesta == 0)                    // Espera la respuesta del modulo SIM900
      {                   
        respuesta = enviarAT("AT", "OK", 2000);        // Envia un comando AT cada 2 segundos y espera la respuesta
        SIM800L.println(respuesta);
      }
  }
}

void power_off()
{
  digitalWrite(25, LOW);
  delay(1000);
}

void reiniciar()
{
  Serial.println("Reiniciando...");
  power_off();
  delay (5000);
  power_on();
}

void envio_sms()                                //Funcion para el envio de mensaje de texto.
{ 
  power_on();
  while ( enviarAT("AT+CREG?", "+CREG: 0,1", 1000) == 0 )                //Espera hasta estar conectado a la red movil
  {
  }
  SIM800L.println("AT+CMGF=1"); // Se configura el modo de texto
  updateSerial();
  SIM800L.println("AT+CMGS=\"+50765907961\"");//Se agrega el numero al que se desea enviar el SMS
  updateSerial();
  Serial.println("Enviando SMS..");
  delay(5000);
  SIM800L.println("Atencion, Alerta Detectada"); //Contenido del SMS
  updateSerial();
  SIM800L.write(26);
  SIM800L.println("AT+CMGS=?"); 
  updateSerial();
  delay(3000);
  power_off();
}

void updateSerial() 
{
  delay(500);
    while (Serial.available()) 
    {
      SIM800L.write(Serial.read());//Forward what Serial received to Software Serial Port
    }
    while(SIM800L.available()) 
    {
      Serial.write(SIM800L.read());//Forward what Software Serial received to Serial Port
    }
}

int Senddirec()
{
  int dev = 0;
  char aux_str[25];
  Serial.println("Intento de conexion al Servidor .... \n"); 
  if (enviarAT("AT+CIPSTART=\"TCP\",\"162.243.87.153\",\"80\"", "CONNECT OK", 10000))   //Inicia una conexión TCP 
  {
    sprintf(aux_str, "AT+CIPSEND=%d", strlen(alert5));
    if (enviarAT(aux_str, ">", 5000) == 1)  //5000
     {
      dev = enviarAT(alert5, "SEND OK", 10000);
      enviarAT("AT+CIPCLOSE","CLOSE OK",1000); //  "AT+CIPCLOSE\"CLOSED CONNECTION\"
      return dev;
     }
     else
     {
      enviarAT("AT+CIPCLOSE","CLOSE OK",1000);
      return 0;
     }
  }
  else
  {
    enviarAT("AT+CIPCLOSE","CLOSE OK",1000); //  "AT+CIPCLOSE\"CLOSED CONNECTION\"
    power_off();
    return 0;
  }
}

int Senddir()
{
  int dev = 0;
  char aux_str[25];
  Serial.println("Intento de conexion al Servidor .... \n"); 
  if (enviarAT("AT+CIPSTART=\"TCP\",\"162.243.87.153\",\"80\"", "CONNECT OK", 10000))   //Inicia una conexión TCP    http://162.243.87.153
  {
      sprintf(aux_str, "AT+CIPSEND=%d", strlen(alert0));
      if (enviarAT(aux_str, ">", 5000) == 1)
       {
        dev = enviarAT(alert0, "SEND OK", 10000);
        enviarAT("AT+CIPCLOSE","CLOSE OK",1000);
        return dev;
       }
       else
       {
        enviarAT("AT+CIPCLOSE","CLOSE OK",1000);
        return 0;
       }
  }
  else
  {
    enviarAT("AT+CIPCLOSE","CLOSE OK",1000);
    power_off();
    return 0;
  }
}

void Send()
{   
    client.flush();
    Serial.println("\nEnviando ...");
    Serial.println(pir_lectura);
    client.print("GET /input/post.json?"); //
    client.print("node=");
    client.print(node);
    client.print("&json={Alerta:");//
    if (pir_lectura == HIGH)
    {
      client.print(5); 
    }
    else
    {
      client.print(0);
    }
    client.print("}&apikey=");
    client.println(apikey);
    client.println("HTTP/1.1");
    client.println("Host:emoncms.org");
    client.println("User-Agent: Arduino-ethernet");
    client.println("Conexion: cerrada");
    client.println();
    lastConnectionTime = millis();
    Serial.println("Envio Completo");
}
