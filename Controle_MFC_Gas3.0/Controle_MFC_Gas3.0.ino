
/***********************************************************
 *                                                         *
 *        * O R N E L A S  C O R P O R A T I O N *         *
 *                                                         *
 ***********************************************************
 * Projeto: Controlador de Fluxo de Gás (MFC Verde)        *
 * Placa - Hardware: Arduino Uno                           *
 * Data: 08/11/2021                                        *
 * Autor: Vinícius Ornelas                                 *
 * Professor: Rodrigo Gribel                               *
 * Usuários: Laboratório de Nanomateriais - UFMG           *
 *                                                         *
 * Descrição: Projeto feito para controlar fluxo de gás    *
 * através de um MFC.                                      *
 * Este projeto, é totalmente dedicado aos alunos da UFMG, *
 * aos usuários do laboratório de nanomateriais de carbono.*                                                  *
 *                                                         *
 *            (C) Copyright 2021 Ornelas Corp.             *
 ***********************************************************
 */


//***************************************************************************************************************************
/* Para adicionar mais uma (ou mais) MFC(s)
//***************************************************************************************************************************

Descomentar linhas:
  60-62, Declaração da tela adicional "InfoFlux3" e "infoMFC3", ALTERAR os numeros das telas seguintes
  140, Pinagem
  156, Valores iniciais dos parametros de setpoint,fluxo maximo, fator gas e fator MFC
  183, Declaração I/O no setup
  210, Função de controle MFC3 no main
  606, Tela infoFlux2
  637, Tela "infoMFC3" em "printToLcd"
  752, Atualização dos parametros na saída
  796, Função de controle
Alterar:
  228, Adicionar "input==3" no if para validação de dados
  296, "infoMfc2" para "infoMfc3", simbolizando que após ultima tela deve retornar para infoFlux
  480,600, "lcd.print("X/3")" para "lcd.print("X/5")", estetica
*/

//***************************************************************************************************************************
//Declarações Iniciais
//***************************************************************************************************************************

#include <Keypad.h>             // Biblioteca do teclado
#include <LiquidCrystal_I2C.h>  //
#include <Wire.h>               // Biblioteca do adaptador i2c do display

#define QTD_MFC 2

const int

  // Fonte dos dados inseridos
  arduino = 1,
  arduinoTimer = 2,

  // Telas de adição de configurações
  config = 1, qtdConfigMFC1 = 2, qtdConfigMFC2 = 3, selectMFC = 4, insertData = 5, insertTime = 6,

  // Telas com informações dos MFCs
  infoFlux = 7, infoConfigMFC1 = 8, infoConfigMFC2 = 9, infoMFC1 = 10, infoMFC2 = 11 /*,infoMFC3 = 12;*/;


int
  event = infoFlux,          // Indice começa mostrando informações do fluxo
  firstPrint = 0,            // Evita delay na transição para a tela "infoFlux"
  j1 = 0, j2 = 0,            // Index para impressão das temporizações !!!
  minutes = 0, seconds = 0;  // Variaveis para impressão


const char
  advanceMarker = 'A',
  backMarker = 'B', clearMarker = 'C', doneMarker = 'D';  // Teclas especiais para teclado

bool
  readInProgress = false,  // Indica estado da leitura
  timerInit1 = false,      // Flag de inicialização de timer MFC1
  timerInit2 = false;      // Flag de inicialização de timer MFC2

byte
  bytesRecvd = 0,    // Tamanho da mensagem recebida
  messageIndex = 0;  // Indice para leitura da mensagem

const byte
  buffSize = 40,  //
  buffSizeK = 8;  // Tamanho do buffer e do teclado, respectivamente

char
  inputBufferK[buffSizeK],                         // Buffer para a entrada do teclado
  messageFromK[buffSize],                          // Parametro a ser alterado recebido pelo teclado
  messageFromUser[QTD_MFC + 1][buffSize] = { 0 };  // Parametro que será alterado

int
  mfcFromK[buffSizeK] = { 0 },  // MFC selecionada pelo teclado
  mfcFromUser = 0,              // MFC a ser atualizada
  MFC = 0,                      // MFC que será alterada
  timeFromUser = 0,             // tempo a ser atualizado
  quantidadeConfig = 0,         // Qtd de temporizadores
  configIndex = 1,              // index de configuração para percorrer arrays

  quantidadeConfigMFC1 = 0,  // qtd para MFC1
  quantidadeConfigMFC2 = 0,  // qtd para MFC2
  mfc1Index = 0,             // index para temporização em MFC1
  mfc2Index = 0,             // index para temporização em MFC2
  count1 = 0,                // index que contabiliza temporizações feitas
  count2 = 0;                // index que contabiliza temporizações feitas

unsigned long
  baseTime1 = 0,  // tempo para temporização MFC1
  baseTime2 = 0;  // tempo para temporização MFC1

float
  floatFromK[buffSizeK] = { 0 },       // Valor inserido pelo teclado
  timeFromK[buffSizeK] = { 0 },        // Tempo inserido pelo teclado
  floatFromUser[QTD_MFC + 1] = { 0 },  // Valores de configuração do MFC

  totalTime1 = 0.0,  // Tempo passado desde o inicio da temporização atual MFC1
  totalTime2 = 0.0;  // Tempo passado desde o inicio da temporização atual MFC2

bool
  SPMFC1_update = false,
  FluxMaxMFC1_update = false,                          //
  FatorMFC1_update = false, FatorGas1_update = false,  // Parâmetros MFC1

  SPMFC2_update = false, FluxMaxMFC2_update = false,   //
  FatorMFC2_update = false, FatorGas2_update = false;  // Parâmetros MFC2

int
  amostra = 0,       //
  intervalo = 1000;  // Variaveis para atualização da imressão de fluxo

//***************************************************************************************************************************
//Teclado e Display
//***************************************************************************************************************************

// Matriz de caracteres (mapeamento do teclado)
const byte LINHAS = 4, COLUNAS = 4;
const char TECLAS_MATRIZ[LINHAS][COLUNAS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '.', '0', '.', 'D' }
};

// Define o endereço utilizado pelo Adaptador I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Pinos de conexao com teclado
byte PINOS_LINHAS[LINHAS] = { 31, 33, 35, 37 };
byte PINOS_COLUNAS[COLUNAS] = { 39, 41, 43, 45 };

// Inicia teclado
Keypad keypad = Keypad(makeKeymap(TECLAS_MATRIZ), PINOS_LINHAS,
                       PINOS_COLUNAS, LINHAS, COLUNAS);

//***************************************************************************************************************************
//MFCs
//***************************************************************************************************************************

#define BUZZER 7
//Pinagem MFC 1
#define SPFlux1_Pin 3
#define Vopen1_Pin 2
#define Flux1_Pin A0

//Pinagem MFC 2
#define SPFlux2_Pin 4
#define Vopen2_Pin 5
#define Flux2_Pin A2
/*
//Pinagem MFC 3
#define SPFlux3_Pin 7
#define Vopen3_Pin 8
#define Flux3_Pin A3
*/
//Configurações MFCs
int SPFlux1_Out = 0,
    SPFlux2_Out = 0;
//    SPFlux3_Out = 0;

float Flux_Max1 = 2000, SPFlux1 = 0, Flux1 = 0,
      Flux1_Val = 0, Fator_MFC1 = 1, Fator_Gas_MFC1 = 2.00,

      Flux_Max2 = 1000, SPFlux2 = 0, Flux2 = 0,
      Flux2_Val = 0, Fator_MFC2 = 1, Fator_Gas_MFC2 = 1.13;

//      Flux_Max3 = 500, SPFlux3 = 0, Flux3 = 0,
//      Flux3_Val = 0, Fator_MFC3 = 1, Fator_Gas_MFC3 = 1;

//***************************************************************************************************************************
//Rotinas Principais
//***************************************************************************************************************************

void initializeMFC(int SPFlux_Pin, int Vopen_Pin, float& Flux_Max, float Fator_MFC, float Fator_Gas_MFC) {
  pinMode(SPFlux_Pin, OUTPUT);
  pinMode(Vopen_Pin, OUTPUT);
  digitalWrite(SPFlux_Pin, 0);
  digitalWrite(Vopen_Pin, 1);
  Flux_Max *= (Fator_MFC / Fator_Gas_MFC);
}

void setup() {

  //Buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  //Porta serial
  Serial.begin(9600);

  //Inicializa o LCD e o backlight
  lcd.init();
  lcd.backlight();

  //Inicialização de MFCs
  initializeMFC(3, 2, Flux_Max1, Fator_MFC1, Fator_Gas_MFC1);  // MFC1
  initializeMFC(4, 5, Flux_Max2, Fator_MFC2, Fator_Gas_MFC2);  // MFC2
  // initializeMFC(7, 8, Flux_Max2, Fator_MFC2, Fator_Gas_MFC2);  // MFC3
}

void loop() {
  getDataFromKeyboard();  // Recebe dados do teclado
  timer(1);
  timer(2);
  configIno(1);  // Configura parametro a ser atualizado
  configIno(2);  // Configura parametro a ser atualizado
  printToLcd();  // Imprime informações no display
  configMFC(1);  // Atualiza valores nas MFCs
  configMFC(2);  // Atualiza valores nas MFCs
  controleMFC(1);
  controleMFC(2);
  // controleMFC(3);
}

//***************************************************************************************************************************
//Rotinas de Comunicação
//***************************************************************************************************************************

// Impressão do erro
void showError(String messageLine1, String messageLine2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("*------------------*");
  lcd.setCursor(0, 1);
  lcd.print(messageLine1);
  lcd.setCursor(0, 2);
  lcd.print(messageLine2);
  lcd.setCursor(0, 3);
  lcd.print("*------------------*");
  delay(3000);
}

// Conversão do tempo para caso exeção 'minutos:segundos'
bool customInsertTimeValidation(float input) {
  return (input - (int)input) < 0.6;
}

// Validação da entrada
bool validateInput(float input, float min, float max, bool (*customValidation)(float) = nullptr, String errorLine1, String errorLine2) {
  if (input < min || input > max || (customValidation != nullptr && !customValidation(input))) {
    showError(errorLine1, errorLine2);
    return false;
  }
  return true;
}

// Realiza validação dos dados inseridos no teclado
bool validData(char* buffer) {
  float input = atof(buffer);
  bool ret = false;

  switch (state) {
    case config:
      ret = validateInput(input, 1, 4, "|Err: Valor Min/Max|", "|             1/4  |");
      break;
    case qtdConfigMFC1:
      ret = validateInput(input, 0, 3, "|Erro: Valor Maximo|", "|         3 por MFC|");
      break;
    case qtdConfigMFC2:
      ret = validateInput(input, 0, 3, "|Erro: Valor Maximo|", "|         3 por MFC|");
      break;
    case selectMFC:
      ret = validateInput(input, 1, 2, "|Err: MFC disponiv.|", "|      MFC1 ou MFC2|");
      break;
    case insertData:
      ret = validateInput(input, 0, 2000, "|Erro: Valor Maximo|", "|              2000|");
      break;
    case insertData:
      ret = validateInput(input, 0, 2000, "|Erro: Valor Maximo|", "|              2000|");
      break;
    case insertTime:
      ret = validateInput(input, 0.1, 999, customInsertTimeValidation, "|Err: Valor Min/Max|", "|           0.1/999|");
      break;
    default:
      ret = true;
      break;
  }
  return ret;
}

//=============
// Converte dados inseridos para futuro controle das MFCs
void parseData(int source, int mfc = 0) {

  if (source == arduino) {  // Trata dados iniciais inseridos pelo usuário
    switch (event) {
      case config:
        strcpy(messageFromK, inputBufferK);
        break;
      case qtdConfigMFC1:
        quantidadeConfigMFC1 = atoi(inputBufferK);
        break;
      case qtdConfigMFC2:
        quantidadeConfigMFC2 = atoi(inputBufferK);
        quantidadeConfig = quantidadeConfigMFC1 + quantidadeConfigMFC2;
        for (int i = 1; i <= quantidadeConfigMFC1; i++) { mfcFromK[i] = 1; }
        for (int i = 1 + quantidadeConfigMFC1; i <= quantidadeConfig; i++) { mfcFromK[i] = 2; }
        break;
      case selectMFC:
        mfcFromK[configIndex] = atoi(inputBufferK);
        break;
      case insertData:
        floatFromK[configIndex] = atof(inputBufferK);
        if (messageFromK[0] != '1') {  // se a configuração não for setpoint
          mfcFromUser = mfcFromK[configIndex];
          strcpy(messageFromUser[mfcFromUser], messageFromK);
          floatFromUser[mfcFromUser] = floatFromK[configIndex];
        }
        break;
      case insertTime:
        float timeInMinutes = atof(inputBufferK);
        int minutes = (int)timeInMinutes;                      // Parte inteira dos minutos
        float seconds = (timeInMinutes - minutes);             // Parte fracionária convertida para segundos
        timeFromK[configIndex] = (minutes + (seconds / 0.6));  // Converte tudo para milissegundos
        break;
    }
  }

  if (source == arduinoTimer) {  // Trata dados temporizados, controlados pelo timer
    strcpy(messageFromUser[mfc], messageFromK);
    mfcFromUser = mfc;
    if (mfc == 1) {
      floatFromUser[mfc] = floatFromK[mfc1Index];
      timeFromUser = timeFromK[mfc1Index];
    } else if (mfc == 2) {
      floatFromUser[mfc] = floatFromK[mfc2Index];
      timeFromUser = timeFromK[mfc2Index];
    }
  }
}

//=============
// Função de temporização
void timer(int timerNum) {
  unsigned long& baseTime = (timerNum == 1) ? baseTime1 : baseTime2;
  bool& timerInit = (timerNum == 1) ? timerInit1 : timerInit2;
  int& mfcIndex = (timerNum == 1) ? mfc1Index : mfc2Index;
  int& count = (timerNum == 1) ? count1 : count2;
  float& totalTime = (timerNum == 1) ? totalTime1 : totalTime2;
  float& SPFlux = (timerNum == 1) ? SPFlux1 : SPFlux2;


  if (timerInit) {
    baseTime = millis();
    mfcIndex = 0;
    int aux = count;

    for (int i = 1; i <= quantidadeConfig; i++) {
      if (mfcFromK[i] == timerNum) {
        if (aux == 0) {
          mfcIndex = i;
          break;
        } else {
          aux--;
        }
      }
    }

    if (mfcIndex > 0) {
      totalTime = timeFromK[mfcIndex];
      parseData(arduinoTimer, timerNum);
    } else {
      totalTime = 0;
    }

    timerInit = false;
  } else if (totalTime > 0) {
    if (timeFromK[mfcIndex] > 0) {
      unsigned long timePassed = millis() - baseTime;
      float minutesPassed = timePassed / 60000.0;
      float timeRemaining = totalTime - minutesPassed;
      if (timeRemaining > 0) {
        timeFromK[mfcIndex] = timeRemaining;
      } else {
        timeFromK[mfcIndex] = 0;
      }
    } else if (configIndex < quantidadeConfig) {  // acabou a temporização mas ainda há mais na fila
      timeFromK[mfcIndex] = totalTime;
      configIndex++;
      count++;
      SPFlux = 0;
      timerInit = true;
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      delay(100);
    } else {  // acabou o processo
      SPFlux = 0;
      lcd.clear();
      lcd.setCursor(1, 1);
      if (quantidadeConfig != 0) {
        timeFromK[mfcIndex] = totalTime;
        lcd.print("Processo finalizado!");
      } else {
        lcd.print("Processo cancelado");
        lcd.setCursor(1, 2);
        lcd.print("MFC");
        lcd.print(timerNum);
      }
      totalTime = 0;
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      delay(200);
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      delay(200);
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      delay(2000);
      lcd.clear();
    }
  }
}



//=============
// Recebe data do teclado e salva no buffer, toda tecla pressionada limpa display
void getDataFromKeyboard() {
  char key = keypad.getKey();  // Atribui a variavel a leitura do teclado
  if (key != NO_KEY) {         // Caso alguma tecla seja precionada
    readInProgress = true;

    // Tecla "A", avança
    if (key == advanceMarker) {
      if (event == infoMFC2) {         // Caso ultima tela
        event = infoFlux;              // Volta a tela inicial
        firstPrint = 1;                // Flag para evitar delay na primeira impressão
      } else if (event >= infoFlux) {  // Se tela de visualização de dados,
        event++;                       // avança para próxima
      }
      readInProgress = false;
      lcd.clear();
    }

    // Tecla B, volta para tela anterior
    if (key == backMarker) {
      if (event < infoFlux) {                                            // Se tela de inserção de dados,
        for (int i = 0; i < buffSizeK; i++) { inputBufferK[i] = '\0'; }  // limpa buffer e
        messageIndex = 0;                                                // reseta indice
      }
      if (event == infoConfigMFC1) { firstPrint = 1; }  // evita delay para printar
      if (event == config) {                            // Se tela for de configuração,
        quantidadeConfig = 0;                           //
        quantidadeConfigMFC1 = 0;                       //
        quantidadeConfigMFC2 = 0;                       // reseta quantidade de configurações e
        event = infoFlux;                               // volta para a de informação de fluxo
      } else if (event == selectMFC) {
        event = config;                  // Se tela de mfc, volta duas
      } else if (event == insertData) {  // Se tela insert data e
        if (messageFromK[0] == '1') {    // estiver setando setpoint, reseta tudo e vai para modo config
          event = config;
          configIndex = 1;
          quantidadeConfig = 0;
          quantidadeConfigMFC1 = 0;
          quantidadeConfigMFC2 = 0;
        } else {
          event--;  // se não estiver setando setpoint, volta para tela anterior
        }
      } else if (event == infoFlux) {  // Caso esteja na primeira tela de visualização,
        event = infoMFC2;              // volta para última
      } else {
        event--;  // Caso nenhuma das exeções anteriores
      }
      readInProgress = false;
      lcd.clear();
    }

    // Tecla C, apaga último caractere
    if (key == clearMarker) {
      if (event < infoFlux) {
        if (messageIndex > 0) {  // caso array não nulo
          inputBufferK[--messageIndex] = '\0';
        }
        readInProgress = false;
        lcd.clear();
      }
    }

    // Tecla D, submete valor
    if (key == doneMarker) {
      if (event < infoFlux) {           // Se estiver no modo config
        if (validData(inputBufferK)) {  // Se dado inserido for válido
          parseData(arduino);           // Envia para função de conversão

          if (event == qtdConfigMFC2) {   // Na ultima tela de qtd de config,
            if (quantidadeConfig == 0) {  // caso usuario nao tenha posto nenhum valor de config, não aceita
              lcd.clear();
              lcd.setCursor(0, 1);
              lcd.print("Insira valor nao nulo");
              delay(1000);
              event--;
            }
            delay(200);  // Se não, passa pra próxima tela
            event = insertData;
          } else if (event == config || event == insertData) {  // Exeções caso
            if (messageFromK[0] != '1') {                       // tiver temporizador (config ser setpoint)
              if (event == config) {
                quantidadeConfig = 1;
                event = selectMFC;
              } else if (event == insertData) {
                event = infoFlux;
              }
            } else {  // Se não for setpoint, nada incomum acontece
              event++;
            }
          } else if (event == insertTime) {
            if (configIndex < quantidadeConfig) {  // Se tiver concluido uma inserção de configuração
              digitalWrite(BUZZER, HIGH);          // aciona buzina
              delay(200);                          //
              digitalWrite(BUZZER, LOW);           //
              event = insertData;                  // passa pra próxima tela e incrementa indice
              configIndex++;
            } else {  // Caso ultima config
              configIndex = 1;
              timerInit1 = true;
              timerInit2 = true;
              event++;
            }
          } else {    // Se não for nenhuma exeção,
            event++;  // passa para próxima tela
          }
        }
        for (int i = 0; i < buffSizeK; i++) {  // Limpa buffer sempre que estiver no modo config
          inputBufferK[i] = '\0';
        }
        messageIndex = 0;
        lcd.clear();
      } else {        // Se estiver em telas de visualização, vai para modo config
        lcd.clear();  // Tela de instrução
        lcd.setCursor(0, 0);
        lcd.print("||     Config     ||");
        lcd.setCursor(0, 1);
        lcd.print("||  B -> Voltar   ||");
        lcd.setCursor(0, 2);
        lcd.print("||  C -> Limpar   ||");
        lcd.setCursor(0, 3);
        lcd.print("||  D -> Submeter ||");
        digitalWrite(BUZZER, HIGH);
        delay(100);
        digitalWrite(BUZZER, LOW);
        delay(4000);

        for (int i = 0; i < buffSizeK; i++) { timeFromK[i] = '\0'; }  // Limpa buffer
      }
      event = config;  // Reinicializa variáveis
      count1 = 0;
      count2 = 0;
      configIndex = 1;
      messageIndex = 0;
      quantidadeConfig = 0;
      quantidadeConfigMFC1 = 0;
      quantidadeConfigMFC2 = 0;
    }
    readInProgress = false;
  }

  if (event < infoFlux) {                                  // Caso estiver modo config
    if (readInProgress) {                                  // Se ainda estiver lendo,
      if (messageIndex < (buffSizeK - 1)) {                // Impede que a mensagem ultrapasse o limite do buffer
        if (event != insertData && event != insertTime) {  // Nestas telas limite para 1 caractere
          inputBufferK[0] = key;                           // Adiciona caracter inserido
          messageIndex = 1;
        } else {
          inputBufferK[messageIndex++] = key;  // Adiciona caracter inserido
        }
      } else {  // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
        Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
      }
      readInProgress = false;
    }
    lcd.clear();  // Para toda tecla pressionada
  }
}
}

//=============

void configIno(int userIndex) {  // Configura parametro a ser atualizado
  bool& SPMFC_update = (userIndex == 1) ? SPMFC1_update : SPMFC2_update;
  bool& FluxMaxMFC_update = (userIndex == 1) ? FluxMaxMFC1_update : FluxMaxMFC2_update;
  bool& FatorMFC_update = (userIndex == 1) ? FatorMFC1_update : FatorMFC2_update;
  bool& FatorGas_update = (userIndex == 1) ? FatorGas1_update : FatorGas2_update;
  char* message = messageFromUser[userIndex];

  // Verificar e atualizar parâmetros com base na mensagem recebida
  if (strcmp(message, "sp") == 0 || strcmp(message, "1") == 0) {
    MFC = mfcFromUser;
    SPMFC_update = true;
  } else {
    SPMFC_update = false;
  }

  if (strcmp(message, "fluxmax") == 0 || strcmp(message, "2") == 0) {
    MFC = mfcFromUser;
    FluxMaxMFC_update = true;
  } else {
    FluxMaxMFC_update = false;
  }

  if (strcmp(message, "fatormfc") == 0 || strcmp(message, "3") == 0) {
    MFC = mfcFromUser;
    FatorMFC_update = true;
  } else {
    FatorMFC_update = false;
  }

  if (strcmp(message, "fatorgas") == 0 || strcmp(message, "4") == 0) {
    MFC = mfcFromUser;
    FatorGas_update = true;
  } else {
    FatorGas_update = false;
  }

  if (strcmp(message, "ajuda") == 0) {  // DEU RUIM!
    printHelp = true;
  } else {
    printHelp = false;
  }
}


//=============

//Impressão da tela de configuração
void printConfigOptions() {

  for (int i = 0; i <= 3; i++) {
    lcd.setCursor(0, i);
    lcd.print("||");
  }

  lcd.setCursor(5, 0);
  lcd.print("1-SetPoint   ||");
  lcd.setCursor(5, 1);
  lcd.print("2-FluxoMax   ||");
  lcd.setCursor(5, 2);
  lcd.print("3-FatorMFC   ||");
  lcd.setCursor(5, 3);
  lcd.print("4-FatorGas   ||");

  switch (inputBufferK[0]) {
    case '1':
      lcd.setCursor(3, 0);
      lcd.print("->");
      break;
    case '2':
      lcd.setCursor(3, 1);
      lcd.print("->");
      break;
    case '3':
      lcd.setCursor(3, 2);
      lcd.print("->");
      break;
    case '4':
      lcd.setCursor(3, 3);
      lcd.print("->");
      break;
  }
}

// Impressão da tela de informações dos MFC's
void printMFCInfo(int index, float SPFlux, float Flux_Max, float Fator_MFC, float Fator_Gas_MFC) {
  lcd.print("M|SetPoint:");
  lcd.print(SPFlux, 1);
  lcd.setCursor(18, 0);
  lcd.print("|");
  lcd.setCursor(0, 1);
  lcd.print("F|FluxoMax:");
  lcd.print(Flux_Max, 1);
  lcd.setCursor(18, 1);
  lcd.print("|");
  lcd.print(index + 3);

  lcd.setCursor(0, 2);
  lcd.print("C|FatorMFC:");
  lcd.print(Fator_MFC, 1);
  lcd.setCursor(18, 2);
  lcd.print("|/");

  lcd.setCursor(0, 3);
  lcd.print(index);
  lcd.print("|FatorGas:");
  lcd.print(Fator_Gas_MFC, 1);
  lcd.setCursor(18, 3);
  lcd.print("|5");
}

void printToLcd() {  // Imprime no LCD

  if ((event > qtdConfigMFC2) && (event < infoFlux)) {
    lcd.setCursor(17, 0);
    lcd.print("CFG");
    lcd.setCursor(18, 1);
    lcd.print("|");
    lcd.print(configIndex);
    lcd.setCursor(18, 2);
    lcd.print("|/");
    lcd.setCursor(18, 3);
    lcd.print("|");
    lcd.print(quantidadeConfig);
  }
  lcd.setCursor(0, 0);

  switch (event) {  // Imprime mensagem de instrução correspondente a tela

    case config:
      printConfigOptions();
      break;
    case qtdConfigMFC1:

      for (int i = 0; i <= 3; i++) {
        lcd.setCursor(0, i);
        lcd.print("||");
        lcd.setCursor(18, i);
        lcd.print("||");
      }
      lcd.setCursor(3, 0);
      lcd.print("Qtd. Setpoints");
      lcd.setCursor(3, 2);
      lcd.print("MFC1:");
      lcd.print(inputBufferK);
      lcd.setCursor(11, 2);
      lcd.print("MFC2:");
      lcd.setCursor(2, 3);
      break;

    case qtdConfigMFC2:

      for (int i = 0; i <= 3; i++) {
        lcd.setCursor(0, i);
        lcd.print("||");
        lcd.setCursor(18, i);
        lcd.print("||");
      }
      lcd.setCursor(3, 0);
      lcd.print("Qtd. Setpoints");
      lcd.setCursor(3, 2);
      lcd.print("MFC1:");
      lcd.print(quantidadeConfigMFC1);
      lcd.setCursor(11, 2);
      lcd.print("MFC2:");
      lcd.print(inputBufferK);
      lcd.setCursor(2, 3);
      break;

    case selectMFC:
      for (int i = 0; i <= 3; i++) {
        lcd.setCursor(0, i);
        lcd.print("||");
      }
      lcd.setCursor(3, 1);
      switch (messageFromK[0]) {
        case '1':
          lcd.print("SetPoint");
          break;
        case '2':
          lcd.print("FluxoMax");
          break;
        case '3':
          lcd.print("FatorMFC");
          break;
        case '4':
          lcd.print("FatorGas");
          break;
      }
      lcd.print(",");
      lcd.print("MFC");
      lcd.print(inputBufferK);
      break;

    case insertData:
      for (int i = 0; i <= 3; i++) {
        lcd.setCursor(0, i);
        lcd.print("||");
      }
      lcd.setCursor(3, 1);
      switch (messageFromK[0]) {
        case '1':
          lcd.print("SetPoint");
          break;
        case '2':
          lcd.print("FluxoMax");
          break;
        case '3':
          lcd.print("FatorMFC");
          break;
        case '4':
          lcd.print("FatorGas");
          break;
      }
      lcd.print(",");
      lcd.print("MFC");
      lcd.print(mfcFromK[configIndex]);
      lcd.print(",");
      lcd.setCursor(3, 2);
      lcd.print("Valor:");
      lcd.print(inputBufferK);
      break;

    case insertTime:
      for (int i = 0; i <= 3; i++) {
        lcd.setCursor(0, i);
        lcd.print("||");
      }
      lcd.setCursor(3, 1);
      switch (messageFromK[0]) {
        case '1':
          lcd.print("SetPoint");
          break;
        case '2':
          lcd.print("FluxoMax");
          break;
        case '3':
          lcd.print("FatorMFC");
          break;
        case '4':
          lcd.print("FatorGas");
          break;
      }
      lcd.print(",");
      lcd.print("MFC");
      lcd.print(mfcFromK[configIndex]);
      lcd.print(",");
      lcd.setCursor(3, 2);
      lcd.print("Valor:");
      lcd.print(floatFromK[configIndex]);
      lcd.setCursor(3, 3);
      lcd.print("Tempo:");
      if (inputBufferK[0] == '\0') {
        lcd.print("'mm.ss'");
      } else {
        lcd.print(inputBufferK);
      }

      break;

    case infoFlux:
      if (SPMFC1_update || FluxMaxMFC1_update || FatorMFC1_update || FatorGas1_update) {
        lcd.print("MFC1| Val  | Time ");
        for (int i = 1; i <= (quantidadeConfig - quantidadeConfigMFC2); i++) {
          lcd.setCursor(2, i);
          switch (messageFromK[0]) {
            case '1':
              lcd.print("SP");
              break;
            case '2':
              lcd.print("Mx");
              break;
            case '3':
              lcd.print("FM");
              break;
            case '4':
              lcd.print("FG");
              break;
          }
          lcd.print("|");
          lcd.print(floatFromK[i], 1);
          lcd.setCursor(11, i);
          lcd.print("|");
          minutes = (int)timeFromK[i];                           // Parte inteira dos minutos
          seconds = (int)roundf((timeFromK[i] - minutes) * 60);  // Parte fracionária convertida para segundos
          lcd.print(minutes);
          lcd.print(":");
          if (seconds < 10) {
            lcd.print("0");  // Para garantir que sempre mostre dois dígitos para segundos
          }
          lcd.print(seconds);
        }
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        delay(4000);
        lcd.clear();
      }
      if (SPMFC2_update || FluxMaxMFC2_update || FatorMFC2_update || FatorGas2_update) {
        lcd.print("MFC2| Val  | Time ");
        for (int i = 1; i <= (quantidadeConfig - quantidadeConfigMFC1); i++) {
          lcd.setCursor(2, i);
          switch (messageFromK[0]) {
            case '1':
              lcd.print("SP");
              break;
            case '2':
              lcd.print("Mx");
              break;
            case '3':
              lcd.print("FM");
              break;
            case '4':
              lcd.print("FG");
              break;
          }
          lcd.print("|");
          lcd.print(floatFromK[i + quantidadeConfigMFC1], 1);
          lcd.setCursor(11, i);
          lcd.print("|");
          minutes = (int)timeFromK[i + quantidadeConfigMFC1];                           // Parte inteira dos minutos
          seconds = (int)roundf((timeFromK[i + quantidadeConfigMFC1] - minutes) * 60);  // Parte fracionária convertida para segundos
          lcd.print(minutes);
          lcd.print(":");
          if (seconds < 10) {
            lcd.print("0");  // Para garantir que sempre mostre dois dígitos para segundos
          }
          lcd.print(seconds);
        }
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        delay(4000);
        lcd.clear();
      }

      if (firstPrint || (amostra == intervalo)) {
        float trueFlux1 = Flux1 * Fator_Gas_MFC1;  // Variavel temporaria que calcula o fluxo real que esta saindo
        float trueFlux2 = Flux2 * Fator_Gas_MFC2;
        lcd.print("MFC| SetP | Fluxo |");

        lcd.setCursor(0, 1);
        lcd.print(" 1 |");
        lcd.print(SPFlux1, 1);

        lcd.setCursor(10, 1);
        lcd.print("|");
        lcd.print("       ");
        lcd.setCursor(11, 1);
        lcd.print(trueFlux1);
        lcd.setCursor(18, 1);
        lcd.print("|1");

        lcd.setCursor(0, 2);
        lcd.print(" 2 |");
        lcd.print(SPFlux2, 1);

        lcd.setCursor(10, 2);
        lcd.print("|");
        lcd.print("       ");
        lcd.setCursor(11, 2);
        lcd.print(trueFlux2);
        lcd.setCursor(18, 2);
        lcd.print("|/");


        lcd.setCursor(0, 3);
        lcd.print("==================|5");

        firstPrint = 0;
      }
      break;

    case infoConfigMFC1:
      lcd.print("M|-| Val  | Time  |");
      lcd.setCursor(0, 1);
      lcd.print("F|");
      lcd.setCursor(0, 2);
      lcd.print("C|");
      lcd.setCursor(0, 3);
      lcd.print("1|");
      j1 = 1;
      if (quantidadeConfig == 0) {
        lcd.setCursor(3, 1);
        lcd.print("|      |");
        lcd.setCursor(3, 2);
        lcd.print("|      |");
        lcd.setCursor(3, 3);
        lcd.print("|      |");
      } else {
        for (int i = 1; (i <= quantidadeConfig); i++) {
          if (mfcFromK[i] == 1) {
            lcd.setCursor(2, j1);
            switch (messageFromK[0]) {
              case '1':
                lcd.print("S|");
                break;
              case '2':
                lcd.print("X|");
                break;
              case '3':
                lcd.print("M|");
                break;
              case '4':
                lcd.print("G|");
                break;
            }
            lcd.print(floatFromK[i], 1);
            lcd.setCursor(10, j1);
            lcd.print("|");
            minutes = (int)timeFromK[i];  // Parte inteira dos minutos
            if (j1 - count1 == 1) {
              seconds = (int)((timeFromK[i] - minutes) * 60);  // Parte fracionária convertida para segundos
            } else {
              seconds = (int)roundf((timeFromK[i] - minutes) * 60);  // Parte fracionária convertida para segundos
            }
            lcd.print(minutes);
            lcd.print(":");
            if (seconds < 10) {
              lcd.print("0");  // Para garantir que sempre mostre dois dígitos para segundos
            }
            lcd.print(seconds);
            if (j1 <= count1 || ((totalTime1 == 0) && (quantidadeConfig > 0))) {
              lcd.setCursor(17, j1);
              lcd.print("+");
            }
            j1++;
          }
        }
      }
      lcd.setCursor(18, 1);
      lcd.print("|2");
      lcd.setCursor(18, 2);
      lcd.print("|/");
      lcd.setCursor(18, 3);
      lcd.print("|5");
      break;

    case infoConfigMFC2:

      lcd.print("M|-| Val  | Time  |");
      lcd.setCursor(0, 1);
      lcd.print("F|");
      lcd.setCursor(0, 2);
      lcd.print("C|");
      lcd.setCursor(0, 3);
      lcd.print("2|");
      j2 = 1;
      if (quantidadeConfig == 0) {
        lcd.setCursor(3, 1);
        lcd.print("|      |");
        lcd.setCursor(3, 2);
        lcd.print("|      |");
        lcd.setCursor(3, 3);
        lcd.print("|      |");
      } else {
        for (int i = 1; i <= quantidadeConfig; i++) {
          if (mfcFromK[i] == 2) {
            lcd.setCursor(2, j2);
            switch (messageFromK[0]) {
              case '1':
                lcd.print("S|");
                break;
              case '2':
                lcd.print("X|");
                break;
              case '3':
                lcd.print("M|");
                break;
              case '4':
                lcd.print("G|");
                break;
            }
            lcd.print(floatFromK[i], 1);
            lcd.setCursor(10, j2);
            lcd.print("|");
            minutes = (int)timeFromK[i];  // Parte inteira dos minutos
            if (j2 - count2 == 1) {
              seconds = (int)((timeFromK[i] - minutes) * 60);  // Parte fracionária convertida para segundos
            } else {
              seconds = (int)roundf((timeFromK[i] - minutes) * 60);  // Parte fracionária convertida para segundos
            }
            lcd.print(minutes);
            lcd.print(":");
            if (seconds < 10) {
              lcd.print("0");  // Para garantir que sempre mostre dois dígitos para segundos
            }
            lcd.print(seconds);

            if (j2 <= count2) {
              lcd.setCursor(17, j2);
              lcd.print("+");
            }
            j2++;
          }
        }
      }
      lcd.setCursor(18, 1);
      lcd.print("|3");
      lcd.setCursor(18, 2);
      lcd.print("|/");
      lcd.setCursor(18, 3);
      lcd.print("|5");
      break;

    case infoMFC1:
      printMFCInfo(1, SPFlux1, Flux_Max1, Fator_MFC1, Fator_Gas_MFC1);
      break;

    case infoMFC2:
      printMFCInfo(2, SPFlux2, Flux_Max2, Fator_MFC2, Fator_Gas_MFC2);
      break;
      /*
    case infoMFC3:
      printMFCInfo(3, SPFlux2, Flux_Max2, Fator_MFC2, Fator_Gas_MFC2);
      break;
    */
  }
}


//=============

void configMFC(int mfcNum) {  // Atualiza valores nas MFCs
  bool& SPMFC_update = (mfcNum == 1) ? SPMFC1_update : SPMFC2_update;
  bool& FluxMaxMFC_update = (mfcNum == 1) ? FluxMaxMFC1_update : FluxMaxMFC2_update;
  bool& FatorMFC_update = (mfcNum == 1) ? FatorMFC1_update : FatorMFC2_update;
  bool& FatorGas_update = (mfcNum == 1) ? FatorGas1_update : FatorGas2_update;
  float& SPFlux = (mfcNum == 1) ? SPFlux1 : SPFlux2;
  float& Flux_Max = (mfcNum == 1) ? Flux_Max1 : Flux_Max2;
  float& Fator_MFC = (mfcNum == 1) ? Fator_MFC1 : Fator_MFC2;
  float& Fator_Gas_MFC = (mfcNum == 1) ? Fator_Gas_MFC1 : Fator_Gas_MFC2;
  float* SPFlux_Out = (mfcNum == 1) ? SPFlux1_Out : SPFlux2_Out;
  char* message = messageFromUser[mfcNum];

  if (SPMFC_update) { SPFlux = floatFromUser[mfcNum]; }
  if (FluxMaxMFC_update) { Flux_Max = floatFromUser[mfcNum]; }
  if (FatorMFC_update) { Fator_MFC = floatFromUser[mfcNum]; }
  if (FatorGas_update) { Fator_Gas_MFC = floatFromUser[mfcNum]; }

  if (SPMFC_update || FluxMaxMFC_update || FatorMFC_update || FatorGas_update) {  //
    *SPFlux_Out = (255 * SPFlux * (Fator_MFC / Fator_Gas_MFC)) / Flux_Max;        // Regra de 3 simples
    SPMFC_update = false;
    FluxMaxMFC_update = false;
    FatorMFC_update = false;
    FatorGas_update = false;
    for (int i = 0; i < buffSize; i++) {  // Limpa buffer
      message[i] = '\0';
    }
  }
}




//***************************************************************************************************************************
//Rotinas de Controle dos MFC's
//***************************************************************************************************************************

void controleMFC(int mfcNum) {  // Controla MFCs
  float& SPFlux = (mfcNum == 1) ? SPFlux1 : SPFlux2;
  int& SPFlux_Out = (mfcNum == 1) ? SPFlux1_Out : SPFlux2_Out;
  int SPFlux_Pin = (mfcNum == 1) ? SPFlux1_Pin : SPFlux2_Pin;
  int Flux_Pin = (mfcNum == 1) ? Flux1_Pin : Flux2_Pin;
  float& Flux_Val = (mfcNum == 1) ? Flux1_Val : Flux2_Val;
  float& Flux = (mfcNum == 1) ? Flux1 : Flux2;
  float& Flux_Max = (mfcNum == 1) ? Flux_Max1 : Flux_Max2;

  SPFlux_Out = (255 * SPFlux) / Flux_Max;
  analogWrite(SPFlux_Pin, SPFlux_Out);
  Flux_Val = analogRead(Flux_Pin);
  Flux = (Flux_Val * Flux_Max) / 1024;
}