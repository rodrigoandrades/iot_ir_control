#include <ESP8266WiFi.h>

#include <WebSockets.h>
#include <WebSocketsClient.h>
#include <WebSocketsServer.h>

#include <IRremoteESP8266.h>
#include <IRremoteInt.h>


//### RECEPTOR IR ###
int RECV_PIN = 12; //GPIO12(D6)
IRrecv irrecv(RECV_PIN);
decode_results results;

//### EMISSOR IR ###
IRsend irsend(14); //GPIO14(D5)

//### WEBSOCKET PORTA 81 ###
WebSocketsServer webSocket = WebSocketsServer(81);

//### wifi ###
const char *ssid = "iot_wifi";
const char *password = "puc#123456";

//### modo atual
const char EMISSOR = '0';
const char RECEPTOR = '1';
char modo = RECEPTOR;


void setup(){
  Serial.begin(115200);

  //inicializa wifi
  WiFi.softAP(ssid, password);

  //INICIALIZA SOCKET
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  //INICIALIZA EMISSOR IR
  irsend.begin();

  //INICIALIZA O RECEPTOR IR
  irrecv.enableIRIn();
}


void loop() {
  webSocket.loop();
  
  if(modo == RECEPTOR){
    if (irrecv.decode(&results)) {
      String message = String();
      message.concat(results.rawlen);
      message.concat("#");
      message.concat(results.bits);
      message.concat("#");

      int rawlen = results.rawlen;

      for (int i = 0; i < rawlen; i++) {
        message.concat(results.rawbuf[i]*USECPERTICK);
        message.concat(",");
      }

      char msg[message.length()];
      message.toCharArray(msg, message.length());
      Serial.println("Comando copiado:");
      Serial.println(msg);
      Serial.println();

      webSocket.broadcastTXT(msg, sizeof(msg));
      delay(300);
      irrecv.resume();
    }
  }
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if (type == WStype_TEXT){
    if((char) payload[0] == 'm'){
      modo = (char) payload[1];
    }else{
      if(modo == EMISSOR){
        String rawlen = String();
        String bits = String();
        String code = String();

        bool part1 = false;
        bool part2 = false;
        bool part3 = false;

        for(int i = 0; i < length; i++){
          char c = (char) payload[i];

          if(c == '#'){
            if(!part1){
              part1 = true;
            } else if (!part2){
              part2 = true;
            } else if (!part3){
              part3 = true;
            }
          } else {
            if(!part1){
              rawlen.concat(c);
            } else if (!part2){
              bits.concat(c);
            } else if (!part3){
              code.concat(c);
            } 
          }
        }

        unsigned int command[rawlen.toInt()];
        int count = 0;
        String part = String();
        code.concat(",");
        for(int i = 0; i < code.length(); i++){
          if(code.charAt(i) == ','){
            command[count] = part.toInt();

            part = String();
            count = count + 1;
          }else{
            part.concat(code.charAt(i));
          }
        }

        irsend.sendRaw(command, rawlen.toInt(), bits.toInt());
        Serial.println("Comando enviado:");
        Serial.println(rawlen.toInt());
        Serial.println(bits.toInt());
        Serial.println(code);
        delay(50);
      }
    }
  }
}