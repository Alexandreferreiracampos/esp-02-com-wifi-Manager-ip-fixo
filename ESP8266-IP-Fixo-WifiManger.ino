#include <FS.h>                         
#include <ESP8266WiFi.h>                  
#include <ESP8266WebServer.h>
#include <WiFiManager.h>                 
#include <ArduinoJson.h>       
#include <FastLED.h>           
#include <EEPROM.h>

#define EEPROM_SIZE 2

char static_ip[16] = "192.168.0.218";         
char static_gw[16] = "192.168.0.1";
char static_sn[16] = "255.255.255.0";
bool shouldSaveConfig = false;            

int rele1 = 14; 
int rele2 = 5; 
#define REDPIN   15
#define GREENPIN 12
#define BLUEPIN  13
int bt = 4;
int btBoot = 0;
boolean buttonPress = false;                      

ESP8266WebServer server(80);                                
//  Strings com HTML
String Q_1 = "<!DOCTYPE HTML><html><head></head><h1><center>AlexandreDev Automacao / Device-Quarto</center>";
String Q_2 = "</p></center><h3><BR></h3><html>\r\n";
String Ql = "";                                             // Quarto ligado
String Qd = "";                                             // Quarto desligado
String Qil = "";
String Q2 = "<p><center></p><p><h1>Equipamento resetado!</h1></p><p><center></p><p><h2>Conectar-se a rede wifi ''AlexandreDev-Device-Quarto'' e acesse http://192.168.4.1/<h2></p>";    
String Qid = "";
String Rele1Quarto;  
String Rele2Quarto;
int rele1State;
int rele2State ;
String rgb;
String rgbBrilho;
String ResetEsp; // String para controle

void saveConfigCallback ()              
{
  shouldSaveConfig = true;
}
//-------------------------------
void setup()
{
  Serial.begin(9600);
  Ql += Q_1;                                                // Monta tela pra informar que a luz
  Ql += "<p><center></p><p><a href=\"/Controle?Rele1Quarto=on \"><button style=\"background-color: rgb(123,104,238);height: 100px; width: 200px; border-radius: 25px;\"><h1>Rele1</h1></button></a><p><center></p><p><a href=\"/Controle?Rele2Quarto=on \"><button style=\"background-color: rgb(123,104,238);height: 100px; width: 200px; border-radius: 25px;\"><h1>Rele2</h1></button></a><p><center></p><p><a href=\"/Controle?ResetEsp=on \"><button style=\"background-color: rgb(255,0,0);height: 100px; width: 200px; border-radius: 25px;\"><h1>Reset</h1></button></a>";
  Qd += Q_1;                                                

  EEPROM.begin(EEPROM_SIZE);
  rele1State = EEPROM.read(0);
  rele2State = EEPROM.read(1);
  
int fitaSpeed = 20;
pinMode(rele1, OUTPUT);
pinMode(rele2, OUTPUT);
pinMode(REDPIN,   OUTPUT);
pinMode(GREENPIN, OUTPUT);
pinMode(BLUEPIN,  OUTPUT);
pinMode(bt, INPUT_PULLUP);
pinMode(btBoot, INPUT_PULLUP);
digitalWrite(rele1, rele1State);
digitalWrite(rele2, rele2State);

colorBars();

  if (SPIFFS.begin())
  {
  
    if (SPIFFS.exists("/config.json"))                      // carregar arquivo caso exista
    {
      
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);       
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          
          if (json["ip"])
          {
            
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
           
          }
          
        }
        
      }
    }
  }
  
  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);      //salva a configuração recebida
  IPAddress _ip, _gw, _sn;                                    //salvar IP
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  // wifiManager.resetSettings();                               //reset para apagar senha wifi

  wifiManager.setMinimumSignalQuality();                        //confiura qualidade minima de sinal do wifi padrão 8%
  
  if (!wifiManager.autoConnect("AlexandreDev-Device-Quarto", "91906245")) //verififcar se o wifi se conectou
  {
    delay(3000);
    ESP.reset();                                                //reinicia o ESP caso não conecte em nenhum rede
    delay(5000);
  }
                      
  if (shouldSaveConfig)                                         //salva as configuração recebidas
  {
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["ip"] = WiFi.localIP().toString();
    json["gateway"] = WiFi.gatewayIP().toString();
    json["subnet"] = WiFi.subnetMask().toString();
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
     // verifica se ouve erro na leitura dos arquivos
    }
    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();    //fechar e salvar a configuração
  }
  
  //Serial.println("local ip");
  //Serial.println(WiFi.localIP());
  //Serial.println(WiFi.gatewayIP());
 // Serial.println(WiFi.subnetMask());

  server.on("/", []()                                       // Ao request
  {
    server.send(200, "text/html", Ql);                      // Executa o HTML Ql (Quarto ligado)
  });
  
  server.on("/status",[](){
    if (digitalRead(rele1) == HIGH)                        // Se a saida esta ligada, carrega a pagina "ligada"
    {
      server.send(200, "text/json", "{\"Rele 1\": \"Ligado\"}");                                   
    }
    if (digitalRead(rele2) == HIGH)                        // Se a saida esta ligada, carrega a pagina "ligada"
    {
      server.send(200, "text/json", "{\"Rele 2\": \"Ligado\"}");                                    
    }
    if (digitalRead(rele1) == LOW)                         // Se a saida esta desligada, carrega a pagina "desligada"
    {
     server.send(200, "text/json", "{\"Rele 1\": \"Desligado\"}");                                         
    }
    if (digitalRead(rele2) == LOW)                         // Se a saida esta desligada, carrega a pagina "desligada"
    {
     server.send(200, "text/json", "{\"Rele 2\": \"Desligado\"}");                                        
    }
       
  });
  server.on("/Controle", []()                               // Ao requeste
  {
    WiFiManager wifiManager;
    Rele1Quarto = server.arg("Rele1Quarto"); 
    Rele2Quarto = server.arg("Rele2Quarto"); 
    rgb = server.arg("rgb");
    ResetEsp = server.arg("ResetEsp");
    rgbBrilho = server.arg("rgbBrilho");

    int finalRGB = rgb.indexOf("a");
    String valorReq = rgb.substring(0, finalRGB);
    String valorBri = rgb.substring(finalRGB + 1);

    if(Rele1Quarto == "on") {
      digitalWrite(rele1, !digitalRead(rele1));
       EEPROM.write(0, digitalRead(rele1));
       EEPROM.commit();
      
    }
    if(Rele2Quarto == "on"){
      digitalWrite(rele2, !digitalRead(rele2));
      EEPROM.write(1, digitalRead(rele2));
      EEPROM.commit(); 
    }

    if(rgb == "off"){
      showAnalogRGB( CRGB::Black ); 
    }
    
    if(valorReq == "white"){
       showAnalogRGB( CHSV( 255, 120, valorBri.toInt()));
    }

    if(rgb != "" && rgb != "off" && valorReq != "white" ){
      showAnalogRGB( CHSV( valorReq.toInt(), 255, valorBri.toInt()) );
    }
 
    if(ResetEsp == "on"){
      server.send(200, "text/html", Q2);
      delay(2000);
      wifiManager.resetSettings();    
      delay(8000);
      ESP.reset();   
    }

    server.send(200, "text/html", Ql);  


   /* if (digitalRead(rele1) == HIGH)                        // Se a saida esta ligada, carrega a pagina "ligada"
    {
      Qil += Ql;                                            // Monta tela nova quarto ligado
      Qil +=  "<p><center>Rele1 Ligado</p>";
      server.send(200, "text/html", Qil);                   // Mostra Quarto ligado
      Qil = "";                                             // Limpa valor de temperatura e umidade
    }    if (digitalRead(rele2) == HIGH)                        // Se a saida esta ligada, carrega a pagina "ligada"
    {
      Qid += Ql;                                            // Monta tela nova quarto ligado
      Qid +=  "<p><center>Rele2 Ligado</p>";
      server.send(200, "text/html", Qid);                   // Mostra Quarto ligado
      Qil = "";                                             // Limpa valor de temperatura e umidade
    }*/
    
    delay(100);                                             // Delay
  });
  server.begin();                                           // Inicaliza servidor
  
}
//-------------------------------

void showAnalogRGB( const CRGB& rgb)
{
  analogWrite(REDPIN,   rgb.r );
  analogWrite(GREENPIN, rgb.g );
  analogWrite(BLUEPIN,  rgb.b );
}

void colorBars()
{
  showAnalogRGB( CRGB::Red );   delay(500);
  showAnalogRGB( CRGB::Green ); delay(500);
  showAnalogRGB( CRGB::Blue );  delay(500);
  showAnalogRGB( CRGB::Black ); delay(500);
}

void loop()
{
  WiFiManager wifiManager;
  server.handleClient();   

  digitalRead(btBoot) == HIGH;
  digitalRead(bt) == HIGH;

  if(digitalRead(btBoot) == LOW){
    delay(50);
    digitalWrite(rele2, !digitalRead(rele2));
    digitalRead(btBoot) == HIGH;
    delay(2000);
    if(digitalRead(btBoot) == LOW){
      ESP.reset();
    }
    
    }
  
   if(digitalRead(bt) == LOW && buttonPress == false){
    delay(50);
      if(digitalRead(bt) == LOW){
        digitalWrite(rele1, !digitalRead(rele1));
        buttonPress = true;
        delay(1000);
        buttonPress = false;
        }
      }

      if (digitalRead(bt) == LOW)                           // Se reset foi pressionado
     {
         delay(5000);                                              // Aguarda 3 minutos segundos
         if (digitalRead(bt) == LOW)                         // Se reset continua pressionado
    {
      server.send(200, "text/html", Q2);
      delay(2000);
      wifiManager.resetSettings();    
      delay(8000);
      ESP.reset();                       
    }      
    }

    if( rgb == "blinkNormal"){
        static uint8_t hue;
        hue = hue + 1;
        showAnalogRGB( CHSV( hue, 255, 255) );
        delay(20);
    }
    if( rgb == "blinkRapido"){
        static uint8_t hue;
        hue = hue + 1;
        showAnalogRGB( CHSV( hue, 255, 255) );
    }
    if( rgb == "blinkLento"){
        static uint8_t hue;
        hue = hue + 1;
        showAnalogRGB( CHSV( hue, 255, 255) );
        delay(300);   
    }
    

}
     
 

//http://192.168.0.106/Controle?FanQuarto=on
//http://192.168.0.106/controle?rgb=140a200
//http://192.168.0.106/controle?rgb=blinkNormal
//http://192.168.0.106/controle?rgb=255a155
//http://192.168.0.106/controle?
