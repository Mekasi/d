#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <RF24/RF24.h>

#include <stdlib.h> 
#include <mysql.h>

#include <json/json.h>
using namespace std;

RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_16MHZ);

const uint8_t pipes[][7] = {"master","slave","idle"};

struct payload_request_t
{
  uint8_t number;
  uint8_t destination;
  char message[14];
};

struct payload_general_t
{
  uint8_t number;
  char message[15];
};

uint16_t timeout_ttl = 1000;

payload_request_t request;
payload_general_t answer;

void finish(bool sucess, int error, char * message)
{
	char status[6];
	if (sucess)
		strcpy(status, "true");
	else
		strcpy(status, "false");
	printf("{\"status\":\"%s\",\"code\":\"%d\",\"message\":\"%s\"}", status, error, message);
   
	if (sucess)
		exit(0);
	else
		exit(1);
}

void mysql(const char query[])
{
    MYSQL mysql; // Дескриптор соединения
   
    char host[] = "localhost"; // хост
    char user[] = "gpio"; // пользователь
    char passwd[] = "pro0713N"; // пароль
    char db[] = "status"; // название базы данных
    int port = 0; //  порт. Если порт у сервера MySQL не по умолчанию (3306), то нужно указывать конкретный номер порта
 
    mysql_init(&mysql);  // Инициализация
    mysql_real_connect(&mysql, host, user, passwd, db, port, NULL, 0); // соединение
 
    if (mysql_query(&mysql, query) > 0) // запорс. Если ошибок нет, то продолжаем работу
        {
 
            // Если была ошибка, ...
            printf("%s", mysql_error(&mysql));  // ... вывдем ее
            return; // и завершим работу
        }

    mysql_close(&mysql); // Закрываем соединение
}

int main(int argc, char** argv)
{
	
   if (argc!=2)
	finish(false, 1, (char *) "Missing JSON");

   json_object * jobj = json_tokener_parse( (char *) argv[1]);
   if (jobj==0)
	finish(false, 2, (char *) "JSON not valid");

   srand(time(NULL));
   /*
    *   Fazendo as configurações do radio
    */
   radio.begin();
   radio.setAutoAck(1); // Ensure autoACK is enabled
   radio.setRetries(15,15);
   radio.enableDynamicPayloads();
   radio.openWritingPipe(pipes[0]);
   radio.openReadingPipe(1,pipes[1]);

   // busca o parametro do destinatario
   json_object * key = json_object_object_get(jobj, "to");
   if (key == 0) // 'to' nao existe
	finish(false, 3, (char *) "Missing destination");

   request.destination = atoi( (char *) json_object_get_string(key) );

   // busca o parametro da mensagem
   key = json_object_object_get(jobj, "message");
   if (key == 0) // 'message' nao existe
	finish(false, 4, (char *) "Missing message content");

   strncpy(request.message, (char *) json_object_get_string(key), 14);
	  
   // busca a configuracao do ttl
   key = json_object_object_get(jobj, "TTL");
   if (key != 0)
	timeout_ttl = atoi((char *) json_object_get_string(key));

   request.number = (uint8_t) rand();

   radio.write(&request,sizeof(payload_request_t));
   radio.startListening();
	
   unsigned long started_waiting_at = millis();
   bool timeout = false;

   while ( !radio.available() && !timeout )
	if (millis() - started_waiting_at > timeout_ttl )
	    timeout = true;
                     
   if ( timeout ){
	finish(false, 5, (char *) "Timed-out");
	}
   radio.read( &answer, sizeof(payload_general_t));
   if (answer.number != request.number)
	finish(false, 6, (char *) "Wrong ACK");
	
   radio.stopListening();
    
	char a[] = "on";
	 if(strcmp(a,answer.message) == 0)
        mysql("UPDATE deviceStatus SET pinStatus = '19' WHERE ID = 2");
   finish(true, 0, answer.message);


}

