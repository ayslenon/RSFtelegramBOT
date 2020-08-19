/* Autor: Hilário
   Adpatado por: Ayslenon
   27/09/2019
   Bot do Telegram para controle de irrigação
*/
//==========================================================================================================================================================================
//Setar um grupo de 2-4 funções que vão ativar o(s) atuadores
//Ligar por x minutos todo dia as 07:00

//-------------------------------------------------------------------------------------------------------------------------------------
// inclusões de bibliotecas


//Bibliotecas para utilizar o bot do telegram via Wifi, uma dessas bibliotecas será utilizada

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

//bibliotecas para wifi
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

//biblioteca para ntpclient (timestamp)
#include <NTPClient.h>

//Biblioteca do bot Telegram utilizada
#include <UniversalTelegramBot.h>
//#include <EEPROM.h> 



//-------------------------------------------------------------------------------------------------------------------------------------
// defines

//SSID e senha da rede WiFi utilizada
#define SSID "EPSON_iPrint"
#define PASSWORD "aocepson"

//Variável que define a cada quantos milisegundos será verificado se teve mensagem nova
#define Intervalo_de_tempo 1000

//Token do bot do telegram
//#define BOT_TOKEN "738956687:AAEekaETTo8i-jjb1Tk11gjX2-mdsokCbzM"
#define BOT_TOKEN "649471818:AAGRmvtHP6TEqS7gjnzyjvXvDbaUIBdPlRs"
//#define BOT_TOKEN "848995162:AAEP0C4aLUjJfdQKuDtqgn0d-NzIC83mQqw"
//Variável para definir até quantos usuários podem interagir com o bot
#define Total_de_usuarios_permitidos 10



//-------------------------------------------------------------------------------------------------------------------------------------
// declarações de objetos utilizados

//função para criar client com conexões seguras
WiFiClientSecure client;

//Objeto e parâmetros para uso do Telegram
UniversalTelegramBot bot(BOT_TOKEN, client);


// Servidor NTP para pesquisar a hora
//const char* servidorNTP = "a.st1.ntp.br";
const char* servidorNTP = "time.google.com";
//Fuso horário em segundos (-03h = -10800 seg)
const int fusoHorario = -10800;

WiFiUDP ntpUDP; // Declaração do Protocolo UDP
NTPClient timeClient(ntpUDP, servidorNTP, fusoHorario, 60000);



//-------------------------------------------------------------------------------------------------------------------------------------
// declarações de variaveis

//variável para guardar timestamp
String horario, horaPrevista1 = "", horaPrevista2 = "";

uint8_t horas = 0, minutos = 0, segundos = 0;
//variavel de controle para envio ou não do timestamp
int a = 0;

//momento em que foi verificado última checagem de mensagem, mantenha sempre zero para conferir assim que inicializa o ESP32
uint32_t ultimoregistrodetempo = 0, ultimoregistrodetempo2 = 0;

//Ids dos usuários liberados para interagir com o bot (conferir a ID de comunicação via serial monitor).
String usuariosPERMITIDOS[Total_de_usuarios_permitidos] = {"588349166", "782633992", "864145848", "869546674", "123456789",
                                                           "123456789", "123456789", "123456789", "123456789", "123456789"
                                                          };
// A Primeira ID é a de Hilario no telegram

//O vetor de nomes dos usuário deve estar devidamente alinhado nas posições com as respectivas IDS dos usuário no vetor de IDS, isso é, posição a posição
String NOMEdosusuariosPERMITIDOS[Total_de_usuarios_permitidos] = {"Hilário", "Ayslenon", "Victor", "Gabriel", "123456789",
                                                                  "123456789", "123456789", "123456789", "123456789", "123456789"
                                                                 };

//Variável para Descrever em texto qual o usuário permitido na operação.
String NomedoUsuario, LastCHAT = "-347391142s"; //-347391142
//Variável de tempo do sistema
uint32_t tempo;
float tempoA1 = 0, tempoA2 = 0;
int   tempoAtuador1 = 0, tempoAtuador2 = 0;

//Variáveis que indicam quais pinos estão sendo usados no ESP
int entsensor1 = 34, entsensor2 = 35, atuador1 = 2, atuador2 = 4;

//Variáveis de identificação para saber se os atuadores estão acionados
boolean status_atuador1 = LOW,
        status_atuador2 = LOW,
        tempoDefinido1  = false,
        tempoDefinido2  = false;
//Modo_Auto    = false;

hw_timer_t *timer = NULL;


//-------------------------------------------------------------------------------------------------------------------------------------
// declarações de constantes

//Lista de comandos do bot
const String COMANDOS = "/COMANDOS";
const String ConferirHora = "/Hora";
//verificando qual a leitura das portas analógicas dos sensores de umidade  associado ao ESP server
const String conferirstatus  = "/Status",
             ligar1          = "/ligar1",
             ligar2          = "/ligar2",
             desligar1    = "/desligar1",
             desligar2    = "/desligar2",
             SETtempoA1   =  "/defineA1",
             SETtempoA2   =  "/defineA2";
             
//Variáveis com strings para textos de interação do sistema com o usuário
//Mensagem a ser exibida quando se requisita a hora



//-------------------------------------------------------------------------------------------------------------------------------------
// prototipos de funções


void IniciarWifi();
void ConferirMensagens(int totaldenovasmensagens);
boolean ConferirID(String IDdoUsuariodoCHAT);
void Iniciarcomandos(String IDdoCHAT);
String comandosdobot();
void HORAATUAL(String IDdoCHAT);
void comandoinvalido(String IDdoCHAT);
void MostrarStatus(String IDdoCHAT);
void definirTempos (String IDdoCHAT, String atuador, int tempoParaDefinir);
void atuadores (String IDdoCHAT, String text);
void IRAM_ATTR resetModule();

//-------------------------------------------------------------------------------------------------------------------------------------
// função principal

void setup()
{
  //hw_timer_t * timerBegin(uint8_t num, uint16_t divider, bool countUp)
  /*
     num: é a ordem do temporizador. Podemos ter quatro temporizadores, então a ordem pode ser [0,1,2,3].
    divider: É um prescaler (reduz a frequencia por fator). Para fazer um agendador de um segundo,
    usaremos o divider como 80 (clock principal do ESP32 é 80MHz). Cada instante será T = 1/(80) = 1us
    countUp: True o contador será progressivo
  */
  timer = timerBegin(0, 80, true); //timerID 0, div 80
  //timer, callback, interrupção de borda
  timerAttachInterrupt(timer, &resetModule, true);
  //timer, tempo (us), repetição
  timerAlarmWrite(timer, 15000000, true);
  timerAlarmEnable(timer); //habilita a interrupção

  pinMode(entsensor1, INPUT);   // Define o pino do sensor 1 como entrada
  pinMode(entsensor2, INPUT);   // Define o pino do sensor 2 como entrada
  pinMode(atuador1, OUTPUT);    // Define o pino do atuador 1 como saida
  pinMode(atuador2, OUTPUT);    // Define o pino do atuador 1 como saida

  digitalWrite(atuador1, LOW);
  digitalWrite(atuador2, LOW);
  Serial.begin(115200);         // Inicia comunicação serial com 115200 de baudrate

  //Função para iniciar a conexão com a rede Wifi local e suas configurações
  IniciarWifi();

  timeClient.begin(); //inicia o ntpclient para poder acessar o servidor e requisitar o timestamp
}

//-------------------------------------------------------------------------------------------------------------------------------------
// loop infinito

void loop()
{
  //Registro de tempo para conferir se passou o tempo necessário para verificar a atualização
  //tempo pode alcançar uma contagem até 4,294,967,295
  tempo = millis();

  //Caso tenha passado tempo superior ao definido na variável intevalo
  //(isso é, a cada quanto tempo o esp irá verificar se teve mensagem nova), será executado as ações do IF para conferir
  //a(s) mensagem(ns) enviada(s) pelo(s) usuário(s).
  if (tempo - ultimoregistrodetempo > Intervalo_de_tempo)
  {
    timerWrite(timer, 0); // reseta o counter
    //Pega as últimas mensagem enviadas e não conferidas e associa a totaldenovasmensagens
    int totaldenovasmensagens = bot.getUpdates(bot.last_message_received + 1);
    //Objeto que irá definir o que fazer com a nova mensagem ou comando
    //Conferir o objeto para implementar novas funções
    ConferirMensagens(totaldenovasmensagens);


    //Aviso a cada duas horas sobre o comando de demonstração de comandos


    //Para garantir que a variável do millis não passe de seu limite, o ESP será reiniciado caso passe de 4000000000
    //na contagem

    if (tempo > 21600000) { // reset a cada 6h

      if (status_atuador1 == HIGH)
      {
        tempoA1 = millis() - tempoA1;
        tempoA1 /= 60000; // diz o tempo em minutos
      }
      else tempoA1 = 0;

      if (status_atuador2 == HIGH)
      {
        tempoA2 = millis() - tempoA2;
        tempoA2 /= 60000; // diz o tempo em minutos
      }
      else tempoA2 = 0;

      a = 1;
      HORAATUAL(LastCHAT);
      String  mensagem_concatenada = "Usuários, no horário <b>" + horario + "</b> o ESP foi reiniciado.\n";
      mensagem_concatenada += "- O atuador 1 ficou <b>" + (String)tempoA1 + "</b> minutos ligado.\n";
      mensagem_concatenada += "- O atuador 2 ficou <b>" + (String)tempoA2 + "</b> minutos ligado.\n";
      bot.sendMessage(LastCHAT, mensagem_concatenada, "HTML");
      a = 0;
      ESP.restart();
    }
    if ((((millis() - tempoA1) / 60000) > tempoAtuador1) && (tempoDefinido1)) {
      horaPrevista1 = "";
      status_atuador1 = LOW;
      digitalWrite(atuador1, LOW);
      tempoDefinido1 = false;
      tempoAtuador1 = 0;
      tempoA1 = 0;
      bot.sendMessage(LastCHAT, "O atuador 1 desligou após o tempo definido", "HTML");
    }

    if ((((millis() - tempoA2) / 60000) > tempoAtuador2) && (tempoDefinido2)) {
      horaPrevista2 = "";
      status_atuador2 = LOW;
      digitalWrite(atuador2, LOW);
      tempoDefinido2 = false;
      tempoAtuador2 = 0;
      tempoA2 = 0;
      bot.sendMessage(LastCHAT, "O atuador 2 desligou após o tempo definido", "HTML");
    }

    if ((tempo - ultimoregistrodetempo2) > 3600000)
    {
      bot.sendMessage(LastCHAT, "- Você pode enviar /comandos para verificar o que o bot pode fazer", "HTML");
      ultimoregistrodetempo2 = tempo;
    }


    //Atualiza a variável de verificação momento de ultima checagem
    ultimoregistrodetempo = tempo;
  }
}

//-------------------------------------------------------------------------------------------------------------------------------------
// Declaração de funções


//Função para a conexão do Wifi, copiado dos exemplos da IDE e ajustados para o  exemplo
//As informações são informadas sia serial print
void IniciarWifi()
{
  Serial.print("Conectando com a rede: ");
  Serial.println(SSID);

  //Iniciando o modo station para se conectar à rede WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  //Aviso de quando não conseguir conectar a rede, enviando a mensagem sempre que não conseguir se conectar, ficando preso no loop
  //Nova tentativa a cada delay.

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(2, HIGH);
    Serial.println(" Tentando novamente a conexão...");
    delay(500);
    digitalWrite(2, LOW);
    delay(500);
    //Caso o ESP32 passe 10 minutos tentando fazer conexão e não consiga, será reiniciado
    // ---------------------------------------------------------------------------------------------------------------------
    // verificar possibilidade de não reiniciar, mas utilizar modo automatico, e de tempos em tempos tentar reconectar
    // ---------------------------------------------------------------------------------------------------------------------
    uint32_t tempo2 = millis();
    if (tempo2 > 600000) ESP.restart();
  }

  //Avisar na serial que conseguiu a conexão
  Serial.println();
  Serial.print("Conectado a rede ");
  Serial.println(SSID);
  digitalWrite(2, HIGH);
  delay(5000);
  digitalWrite(2,  LOW);
}

//Essa função Apresenta as mensagens novas, mas irá conferir quem enviou a mensagem e só irá aceitar comandos de
//usuários registrados no vetor

void ConferirMensagens(int totaldenovasmensagens)
{
  //Irá conferir cada nova mensagem enviada e de cada novo usuário
  for (int i = 0; i < totaldenovasmensagens; i++)
  {
    //Confere a ID do chat que o bot esta e associa a variável IDdoCHAT, para assim conferir se irá interagir ou não com o chat
    //cada mensagem enviada é derivada de um chat, portanto é necessário conferir cada uma das mensagens
    String IDdoCHAT = String(bot.messages[i].chat_id);
    LastCHAT = String(bot.messages[i].chat_id);
    //Realiza o mesmo com os usuários, em especial para conferir se são usuários com interações permitidas, conferidas pela ID
    String IDdoUsuariodoCHAT = String(bot.messages[i].from_id);

    //Aqui será conferido a ID do usuário, para caso não se saiba a ID do usuário para registro, ou para registrar quem esta tentando interagir sem permissão
    Serial.println("IDdoUsuariodoCHAT: " + IDdoUsuariodoCHAT); //mostra no monitor serial o id de quem mandou a mensagem

    //chamando a função que confere se o usuário pode ou não interagir com o bot (conferir função para mais detalhes)
    //A variável será utilizada para encerar o laço naquele momento ou não, assim pulando para próxima mensagem
    boolean confIDdoUsuariodoCHAT = ConferirID(IDdoUsuariodoCHAT);

    //Caso o usuário não possa interagir com o bot
    if (!confIDdoUsuariodoCHAT)
    {
      //Será enviado uma mensagem para o chat em que o usuário está informando que ele não tem permissão de interação
      //e continua as verificações das outras mensagens
      bot.sendMessage(IDdoCHAT, "Usuário " + IDdoUsuariodoCHAT + " não pode interagir com o bot por não estar na lista de usuários permitidos.", "HTML");
      continue;
    }

    //Verificando a mensagem enviadas pelo usuário permitido
    String text = bot.messages[i].text;

    //Verifica se o usuário inicializou o envio dos comandos

    //Iniciarcomandos(IDdoCHAT);
    // onde cortamos o primeiro termo "/SETtempoAn" (considerando até 10 atuadores)
    // encontramos o tamanho da variavel texto
    int tamanho = text.length();
    String auxiliar2 = text.substring(0, 9);        // cria uma variavel para o caso de definir um tempo para um atuador
    String auxiliar  = text.substring(10, tamanho); // separamos a parte dos minutos (que importam para a definição de um tempo)
    int valorDeTempo  = auxiliar.toInt();           // convertemos esse valor de string para inteiro

    if (text.equalsIgnoreCase(COMANDOS)) Iniciarcomandos(IDdoCHAT);

    //Confere se a mensagem é outra, a ideia aqui é verificar a cada novo "else if" se a mensagem enviada
    //é um comando requisitado, é bom conferir como estão os comandos e suas respectivas funções chamadas

    //comando para saber que horas são

    //Envia para o objeto o parâmetro da Id do chat para avisar no chat depois o comando, que no caso
    //desse comando é o horário

    else if (text.equalsIgnoreCase(ConferirHora)) HORAATUAL(IDdoCHAT);

    //Comando para requisição da leitura dos sensores

    //Envia para o objeto o parâmetro da Id do chat para avisar no chat depois o comando, que no caso
    //desse comando são as leituras dos sensores

    else if (text.equalsIgnoreCase(conferirstatus)) MostrarStatus(IDdoCHAT);

    else if (auxiliar2.equalsIgnoreCase(SETtempoA1))  definirTempos(IDdoCHAT, SETtempoA1, valorDeTempo);
    else if (auxiliar2.equalsIgnoreCase(SETtempoA2))  definirTempos(IDdoCHAT, SETtempoA2, valorDeTempo);

    //Verifica se algum dos atuadores precisa mudar de estado
    //else if (text.equalsIgnoreCase(ligar1)) atuadores (IDdoCHAT, text);
    //else if (text.equalsIgnoreCase(ligar2)) atuadores (IDdoCHAT, text);
    else if (text.equalsIgnoreCase(desligar1)) atuadores (IDdoCHAT, text);
    else if (text.equalsIgnoreCase(desligar2)) atuadores (IDdoCHAT, text);


    //Se não for um comando válido, chama o objeto que informará ao chat e ao usuário que o comando não é válido
    else comandoinvalido(IDdoCHAT);

    //Fim do laço de conferir todas as mensagens novas
  }
}



//A função informa por meio de uma resposta booleana (TRUE ou FALSE) se o usuário esta na lista ou não
boolean ConferirID(String IDdoUsuariodoCHAT)
{
  //Conferir se o usuário esta na lista de permitidos,
  for (int x = 0; x < Total_de_usuarios_permitidos; x++)
  {
    //Caso o usuário seja permitido, responde com TRUE
    if (IDdoUsuariodoCHAT == usuariosPERMITIDOS[x])
    {
      //Identifica o nome do usuário devido os vetores estarem relacionados
      NomedoUsuario = NOMEdosusuariosPERMITIDOS[x];
      return true;
    }
  }

  //Caso o usuário não seja permitido interagir com o sistema, a função retorna False
  return false;
}


//Quando inicializa a conexão com os usuários permitidos
void Iniciarcomandos(String IDdoCHAT)
{
  //concatena as mensagens a serem enviadas ao usuário, lembrar de conferir o comando que irá acrescentar ao sistema quais
  //comandos podem ser executados
  String mensagem_concatenada = "<b>Usuário " + NomedoUsuario + ", seguem os comandos que podem ser aplicados</b>.\n";
  mensagem_concatenada += comandosdobot();
  bot.sendMessage(IDdoCHAT, mensagem_concatenada, "HTML");
}


String comandosdobot()
{
  //String com a lista de mensagens que são válidas e explicação sobre o que faz
  String mensagem_concatenada =  "Os comandos disponíveis são:\n\n";
  mensagem_concatenada += "<b>" + ConferirHora + "</b>: Para conferir horário\n";
  mensagem_concatenada += "<b>" + conferirstatus + "</b>: Para conferir as leituras dos sensores e condições dos atuadores \n";
  mensagem_concatenada += "<b>" + desligar1 + "</b>: Para desligar o atuador 1\n";
  //mensagem_concatenada += "<b>" + ligar1 + "</b>: Para ligar o atuador 1 \n";
  mensagem_concatenada += "<b>" + desligar2 + "</b>: Para desligar o atuador 2 \n";
  //mensagem_concatenada += "<b>" + ligar2 + "</b>: Para ligar o atuador 2\n";
  mensagem_concatenada += "<b>" + SETtempoA1 + "</b>: Para definir um tempo (em minutos) para o atuador 1 ficar ligado\n";
  mensagem_concatenada += "<b>" + SETtempoA2 + "</b>: Para definir um tempo (em minutos) para o atuador 2 ficar ligado\n";
  mensagem_concatenada += "<b> Para definir um tempo, envie no segunte formato: /defineA1_tempo, por exemplo </b>\n";
  return mensagem_concatenada;
}


//Objeto que descreve a função de tempo, aqui é utilizado o NTP server para pegar o tempo real.
//Requer internet para manter o horário atualizado
void HORAATUAL(String IDdoCHAT)
{
  timeClient.update();
  horas = timeClient.getHours();
  minutos = timeClient.getMinutes();
  segundos = timeClient.getSeconds();
  horario = timeClient.getFormattedTime();

  if (a == 0) bot.sendMessage(IDdoCHAT, NomedoUsuario + ", o horário no momento da requisição é: " + horario + ". \n ", "HTML");
}


//Avisa ao chat que não existe o comando
void comandoinvalido(String IDdoCHAT)
{
  String mensagem_concatenada = "Usuário " + NomedoUsuario + ", seu comando é inexistente, confira novamente seu comando de acordo com a lista de comandos disponíveis \n";
  mensagem_concatenada += comandosdobot();
  bot.sendMessage(IDdoCHAT, mensagem_concatenada, "HTML");
}


void atuadores (String IDdoCHAT, String text)
{
  uint8_t IndicaAtuador = 0;
  String  IndicaEstado  = "";

  bool mudanca = false;
  a = 1;
  HORAATUAL(IDdoCHAT);
  if (text.equalsIgnoreCase(ligar1))
  {
    if (status_atuador1 == LOW) {            // se esta desligado
      tempoA1 = millis();
      IndicaAtuador = 1;
      IndicaEstado  = "ligado";
      status_atuador1 = HIGH;
      digitalWrite(atuador1, HIGH);
      mudanca = true;
      if (!tempoDefinido1) {
        int minutosA1 = tempoAtuador1 % 60;
        int horasA1   = tempoAtuador1 / 60;

        int minutos_prev1 = minutosA1 + minutos;
        int horas_prev1   = horasA1   +   horas;
        tempoDefinido1 = true;

        if (minutos_prev1 > 59) {
          horas_prev1 += (minutos_prev1 / 60);
          minutos_prev1 -= 60 * (minutos_prev1 / 60);
        }
        if (horas_prev1 > 23) {
          horas_prev1 = 0;
        }

        horaPrevista1 = "";
        if (horas_prev1 < 10) horaPrevista1 += "0";
        horaPrevista1  += (String)horas_prev1 + ":";
        if (minutos_prev1 < 10) horaPrevista1  += "0";
        horaPrevista1  += (String)minutos_prev1 + ":";
        if (segundos < 10) horaPrevista1  += "0";
        horaPrevista1  += (String)segundos;
      }
    }
  }

  else if ((text.equalsIgnoreCase(desligar1)) || ((((millis() - tempoA1) / 60000) > tempoAtuador1) && (tempoDefinido1))) // se ligado ou acabou o tempo que foi setado
  {
    if (status_atuador1 == HIGH)
    {
      tempoA1 = millis() - tempoA1;
      tempoA1 /= 60000; // diz o tempo em minutos
      IndicaAtuador = 1;
      IndicaEstado  = "desligado";
      status_atuador1 = LOW;
      tempoAtuador1 = 0;
      digitalWrite(atuador1, LOW);
      mudanca = true;
    }
    else tempoA1 = 0;
  }

  else if (text.equalsIgnoreCase(ligar2))
  {
    if (status_atuador2 == LOW) {
      tempoA2 = millis();
      IndicaAtuador = 2;
      IndicaEstado  = "ligado";
      status_atuador2 = HIGH;
      digitalWrite(atuador2, HIGH);
      tempoDefinido2 = false;
      mudanca = true;
      if (!tempoDefinido2) {
        int minutosA2 = tempoAtuador2 % 60;
        int horasA2   = tempoAtuador2 / 60;

        int minutos_prev2 = minutosA2 + minutos;
        int horas_prev2   = horasA2   +   horas;
        tempoDefinido2 = true;
        if (minutos_prev2 > 59) {
          horas_prev2 += (minutos_prev2 / 60);
          minutos_prev2 -= 60 * (minutos_prev2 / 60);
        }
        if (horas_prev2 > 23) {
          horas_prev2 = 0;
        }

        horaPrevista2 = "";
        if (horas_prev2 < 10) horaPrevista2 += "0";
        horaPrevista2 += (String)horas_prev2 + ":";
        if (minutos_prev2 < 10) horaPrevista2 += "0";
        horaPrevista2 += (String)minutos_prev2 + ":";
        if (segundos < 10) horaPrevista2 += "0";
        horaPrevista2 += (String)segundos;
      }
    }
  }

  else if ((text.equalsIgnoreCase(desligar2)) || ((((millis() - tempoA2) / 60000) > tempoAtuador2) && (tempoDefinido2)))
  {
    if (status_atuador2 == HIGH)
    {
      tempoA2 = millis() - tempoA2;
      tempoA2 /= 60000; // diz o tempo em minutos

      IndicaAtuador = 2;
      IndicaEstado  = "desligado";
      status_atuador2 = LOW;
      digitalWrite(atuador2, LOW);
      tempoAtuador2 = 0;
      mudanca = true;
    }
    else tempoA2 = 0;
  }

  if (mudanca) {
    if ((IndicaAtuador > 0) && (IndicaEstado != "")) {
      String  mensagem_concatenada = "Usuário <b>" + NomedoUsuario + "</b>, no horário <b>" + horario + "</b>\n";
      mensagem_concatenada += "- O atuador <b>" + (String)IndicaAtuador + "</b> mudou de estado e encontra-se <b>" + IndicaEstado + "</b>.\n";

      if ((tempoA1 > 0.00) && (status_atuador1 == LOW))
      {
        int segunds1 = (tempoA1 - ((int)tempoA1 % 60)) * 60;
        int minuts1 = (int)tempoA1;
        mensagem_concatenada += "- O atuador 1 ficou ligado por <b>" + (String)minuts1 + "</b> minutos e " + (String)segunds1 + " segundos.\n";
        if (tempoDefinido1) {
          tempoDefinido1 = false;
          mensagem_concatenada += "- O horario previsto para desligar é: " + horaPrevista1 + ".\n";
        }
        tempoA1 = 0;
      }

      if ((tempoA2 > 0.00) && (status_atuador2 == LOW))
      {
        int segunds2 = (tempoA2 - ((int)tempoA2 % 60)) * 60;
        int minuts2 = (int)tempoA2;
        mensagem_concatenada += "- O atuador 2 ficou ligado por <b>" + (String)minuts2 + "</b> minutos e " + (String)segunds2 + " segundos.\n";
        if (tempoDefinido2) {
          tempoDefinido2 = false;
          mensagem_concatenada += "- O horario previsto para desligar é: " + horaPrevista2 + ".\n";
        }
        tempoA2 = 0;
      }

      IndicaAtuador =  0;
      IndicaEstado  = "";
      bot.sendMessage(IDdoCHAT, mensagem_concatenada, "HTML");
      a = 0;
    }
  }
}


//comando para enviar alguma informação para outro ESP32
//Conferir a estrutura que será utilizada para cada novo sensor a ser adicionado
void MostrarStatus(String IDdoCHAT)
{
  String solo1,
         solo2,
         string_status_atuador1,
         string_status_atuador2;

  int umi1 = analogRead(entsensor1),
      umi2 = analogRead(entsensor2);

  int auxTempo1 = 0, auxTempo2 = 0;
  int   segundosligado1 = 0, segundosligado2 = 0;
  a = 1;

  if (umi1 > 2730) solo1 = "seco";
  else solo1 = "úmido";

  if (umi2 > 2730) solo2 = "seco";
  else solo2 = "úmido";

  if (status_atuador1 == HIGH) string_status_atuador1 = "Ligado";
  else string_status_atuador1 = "Desligado";

  if (status_atuador2 == HIGH) string_status_atuador2 = "Ligado";
  else string_status_atuador2 = "Desligado";

  if (status_atuador1 == true) {
    auxTempo1 = (millis() - tempoA1);
    segundosligado1 = auxTempo1 / 1000;
    auxTempo1 /= 60000;
    segundosligado1 -= auxTempo1 * 60;
  }

  if (status_atuador2 == true) {
    auxTempo2 = (millis() - tempoA2);
    segundosligado2 = auxTempo2 / 1000;
    auxTempo2 /= 60000;
    segundosligado2 -= auxTempo2 * 60;
  }

  int   minutosQueFaltam1 = tempoAtuador1 - auxTempo1 - 1,
        minutosQueFaltam2 = tempoAtuador2 - auxTempo2 - 1,
        segundosQueFaltam1 = 60 - segundosligado1,
        segundosQueFaltam2 = 60 - segundosligado2;

  HORAATUAL(IDdoCHAT);

  String  mensagem_concatenada = "Usuário <b>" + NomedoUsuario + "</b>, no horário <b>" + horario + "</b> os status são: \n";
  mensagem_concatenada += "- O <b>solo 1</b> encontra-se <b>" + solo1 + "</b>.\n";
  mensagem_concatenada += "- O <b>solo 2</b> encontra-se <b>" + solo2 + "</b>.\n";
  mensagem_concatenada += "- O <b>atuador 1</b> encontra-se <b>" + string_status_atuador1 + "</b>.\n";
  mensagem_concatenada += "- O <b>atuador 2</b> encontra-se <b>" + string_status_atuador2 + "</b>.\n";

  if (status_atuador1) mensagem_concatenada += "- O atuador 1 está ligado há " + (String)auxTempo1 + " minutos e " + (String)segundosligado1 + " segundos\n";
  if (status_atuador2) mensagem_concatenada += "- O atuador 2 está ligado há " + (String)auxTempo2 + " minutos e " + (String)segundosligado2 + " segundos\n";

  if (tempoDefinido1) {
    mensagem_concatenada += "- O atuador 1 deve ficar ligado por " + (String)tempoAtuador1 + " minutos\n";
    mensagem_concatenada += "- O atuador 1 vai ficar mais <b>" + (String)minutosQueFaltam1 + "</b> minutos e <b>";
    mensagem_concatenada += (String)segundosQueFaltam1 + " </b>segundos ligado\n";
    mensagem_concatenada += "- O horario previsto para desligar é: " + horaPrevista1 + ".\n";
  }
  if (tempoDefinido2) {
    mensagem_concatenada += "- O atuador 2 deve ficar ligado por " + (String)tempoAtuador2 + " minutos\n";
    mensagem_concatenada += "- O atuador 2 vai ficar mais <b>" + (String)minutosQueFaltam2 + "</b> minutos e <b>";
    mensagem_concatenada += (String)segundosQueFaltam2 + " </b>segundos ligado\n";
    mensagem_concatenada += "- O horario previsto para desligar é: " + horaPrevista2 + ".\n";
  }
  bot.sendMessage(IDdoCHAT, mensagem_concatenada, "HTML");
  a = 0;
}

void definirTempos (String IDdoCHAT, String atuador, int tempoParaDefinir) {
  if (atuador == SETtempoA1) {
    tempoAtuador1 = tempoParaDefinir;
    tempoDefinido1 = false;
    atuadores (IDdoCHAT, ligar1);
    tempoDefinido1 = true;
    bot.sendMessage(IDdoCHAT, "- O tempo foi definido", "HTML");
  }
  if (atuador == SETtempoA2) {
    tempoAtuador2 = tempoParaDefinir;
    tempoDefinido2 = false;
    atuadores (IDdoCHAT, ligar2);
    tempoDefinido2 = true;
    bot.sendMessage(IDdoCHAT, "- O tempo foi definido", "HTML");
  }
}


void IRAM_ATTR resetModule() {
  ets_printf("(watchdog) reiniciar\n"); //imprime no log
  ESP.restart(); //reinicia o chip
}
