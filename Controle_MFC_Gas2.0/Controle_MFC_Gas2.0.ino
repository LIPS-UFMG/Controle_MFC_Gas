
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

int event = 6,     // Indice começa mostrando informações do fluxo
  firstPrint = 0,  // Evita delay na transição para a tela "infoFlux"
  j1 = 0, j2 = 0, minutes = 0, seconds = 0;

const int
  pc = 1,
  arduino = 2,  // Fonte dos dados inseridos
  arduinoTimer = 3,
  config = 1, qtdConfig = 2, selectMFC = 3, insertData = 4,  // Telas de interação do display
  insertTime = 5, infoFlux = 6,
  //  infoFlux2 = 8,
  infoConfigMFC1 = 7, infoConfigMFC2 = 8, infoMFC1 = 9, infoMFC2 = 10;  // Telas com informações dos MFCs
//  infoMFC3 = 10;

const char
  point = '#',
  advanceMarker = 'A', backMarker = 'B', clearMarker = 'C', doneMarker = 'D',  // Teclas especiais para teclado
  startMarker = '<', endMarker = '>';                                          // Teclas para comandos no pc

bool
  readInProgress = false,  // Indica estado da leitura
  newDataFromPC = false,   // Simboliza se houve inserção de dados pelo usuario

  timerInit1 = false,
  timerInit2 = false;

byte
  bytesRecvd = 0,    // Tamanho da mensagem recebida
  messageIndex = 0;  // Indice para leitura da mensagem

const byte
  buffSize = 40,
  buffSizeK = 8;  // Tamanho do buffer do pc e do teclado, respectivamente

char
  inputBufferK[buffSizeK],                         // Buffer para a entrada do teclado
  inputBufferS[buffSize],                          // Buffer para a entrada do pc
  messageFromK[buffSize],                          // Parametro a ser alterado recebido pelo teclado
  messageFromUser[QTD_MFC + 1][buffSize] = { 0 };  // Parametro que será alterado

int
  mfcFromK[buffSizeK] = { 0 },  // MFC selecionada pelo teclado
  quantidadeConfig = 0,         //
  configIndex = 1,              //
  timeFromUser = 0,             //
  intFromUser = 0,              // MFC selecionada pelo serial   ?
  MFC = 0,                      // MFC que será alterada

  mfc1Index = 0,  //
  mfc2Index = 0,  //
  count1 = 0,
  count2 = 0;

unsigned long
  baseTime1 = 0,  //
  baseTime2 = 0;  //

float
  floatFromK[buffSizeK] = { 0 },       //
  timeFromK[buffSizeK] = { 0 },        //
  floatFromUser[QTD_MFC + 1] = { 0 },  // Valores de configuração do MFC

  totalTime1 = 0.0,  //
  totalTime2 = 0.0;  //

bool
  printHelp = false,                                   // Estado para imprimir ajuda
  SPMFC1_update = false, FluxMaxMFC1_update = false,   //
  FatorMFC1_update = false, FatorGas1_update = false,  // Parâmetros de uma MFC

  SPMFC2_update = false, FluxMaxMFC2_update = false,   //
  FatorMFC2_update = false, FatorGas2_update = false;  // Parâmetros de uma MFC

int
  amostra = 0,
  intervalo = 1000;  // Tempo para imprimir fluxo

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
#define Flux3_Pin A2
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

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  //Inicia porta serial
  Serial.begin(9600);

  //Inicializa o LCD e o backlight
  lcd.init();
  lcd.backlight();

  //Inicialização de MFCs
  initializeMFC(3, 2, Flux_Max1, Fator_MFC1, Fator_Gas_MFC1);  // MFC1
  initializeMFC(4, 5, Flux_Max2, Fator_MFC2, Fator_Gas_MFC2);  // MFC2
  // initializeMFC(7, 8, Flux_Max2, Fator_MFC2, Fator_Gas_MFC2);  // MFC3

  // Mensagem inicial
  Serial.println("<Arduino is ready>");
  Serial.println(" ");
  Serial.println("* O R N E L A S  C O R P O R A T I O N *");
  Serial.println("Controlador de Gáses");
  Serial.println(" ");
  Serial.println("Em caso de dúvidas digite:");
  Serial.println("<ajuda>");
  Serial.println(" ");
}

void loop() {
  getDataFromKeyboard();  // Recebe dados do teclado
  getDataFromPC();        // Recebe dados do pc
  timer(1);
  timer(2);
  configIno(1);  // Configura parametro a ser atualizado
  configIno(2);  // Configura parametro a ser atualizado
  printToLcd();  // Imprime informações no display
  replyToPC();   // Imprime informações no pc
  configMFC(1);  // Atualiza valores nas MFCs
  configMFC(2);  // Atualiza valores nas MFCs
  controleMFC(1);
  controleMFC(2);
  // controleMFC(7, 8, SPFlux2, Flux2, SPFlux2_Out);
}

//***************************************************************************************************************************
//Rotinas de Comunicação
//***************************************************************************************************************************

bool validData(char* buffer) {  // Realiza validação dos dados inseridos no teclado

  float input = atof(buffer);
  bool ret = false;

  switch (event) {
    case config:
      ret = ((input >= 1) && (input <= 4));  // Só permite seleção de 1 a 4 no menu principal
      break;
    case qtdConfig:
      ret = ((input >= 1) && (input <= 4));  // maximo de configs
      break;
    case selectMFC:
      ret = (input == 1 || input == 2);  // Só permite selecionar entre MFCs 1 e 2
      break;
    case insertData:
      ret = ((input >= 0) && (input <= 2000));  // numero máximo de entrada de valor
      break;
    case insertTime:
      ret = ((input >= 0.1) && (input <= 999) && ((input - (int)input) < 0.6));
      break;
    default:
      ret = true;
      break;
  }
  return ret;
}

//=============

void parseData(int source, int mfc = 0) {  // Converte dados inseridos para controle das MFCs

  if (source == pc) {  // Trata dados se foram inseridos pelo computador
    char* strtokIndx;

    strtokIndx = strtok(inputBufferS, ",");
    strcpy(messageFromUser[intFromUser], strtokIndx);  // DEU RUIM!

    strtokIndx = strtok(NULL, ",");
    intFromUser = atoi(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    floatFromUser[intFromUser] = atof(strtokIndx);
  }

  if (source == arduino) {  // Trata dados se foram inseridos pelo arduino
    switch (event) {

      case config:
        strcpy(messageFromK, inputBufferK);
        break;

      case qtdConfig:
        quantidadeConfig = atoi(inputBufferK);
        break;

      case selectMFC:
        mfcFromK[configIndex] = atoi(inputBufferK);
        break;

      case insertData:
        floatFromK[configIndex] = atof(inputBufferK);
        if (messageFromK[0] != '1') {  // se a configuração não for setpoint
          intFromUser = mfcFromK[configIndex];
          strcpy(messageFromUser[intFromUser], messageFromK);
          floatFromUser[intFromUser] = floatFromK[configIndex];
        }
        break;
      case insertTime:


        float timeInMinutes = atof(inputBufferK);
        int minutes = (int)timeInMinutes;                      // Parte inteira dos minutos
        float seconds = (timeInMinutes - minutes);             // Parte fracionária convertida para segundos
        timeFromK[configIndex] = (minutes + (seconds / 0.6));  // Converte tudo para milissegundos

        // Serial.println("Dados: ");
        // Serial.print("A: ");
        // Serial.println(messageFromUser);
        // Serial.print("B: ");
        // Serial.println(intFromUser);
        // Serial.print("C: ");
        // Serial.println(floatFromUser);
        break;
    }
  }

  if (source == arduinoTimer) {
    strcpy(messageFromUser[mfc], messageFromK);
    intFromUser = mfc;

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

void timer(int timerNum) {  //

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
      timeFromK[mfcIndex] = totalTime;
      totalTime = 0;
      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("Processo finalizado!");
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

void getDataFromKeyboard() {  // Recebe data do teclado e salva no buffer

  char key = keypad.getKey();  // Atribui a variavel a leitura do teclado

  if (key != NO_KEY) {  // Caso tecla seja precionada
    readInProgress = true;

    if (key == advanceMarker) {
      if (event == infoMFC2) {  // Caso ultimatela
        event = infoFlux;       // volta a tela inicial
        firstPrint = 1;         // flag para primeira impressão rápida
      } else if (event >= infoFlux) {
        event++;
      }
      readInProgress = false;
      lcd.clear();
    }

    if (key == doneMarker) {  // Tecla D submete valor inserido
      if (event < infoFlux) {
        if (validData(inputBufferK)) {  // se dado inserido for válido
          parseData(arduino);
          newDataFromPC = true;

          if (event == config) {
            quantidadeConfig = 1;
          }

          if (event == config || event == insertData) {
            if (messageFromK[0] != '1') {
              event += 2;
            } else {
              event++;
            }
          } else if (event == insertTime) {
            if (configIndex < quantidadeConfig) {
              digitalWrite(BUZZER, HIGH);
              delay(200);
              digitalWrite(BUZZER, LOW);
              event = selectMFC;
              configIndex++;
            } else {
              configIndex = 1;
              timerInit1 = true;
              timerInit2 = true;
              event++;
            }
          } else {
            event++;  // Passa para próxima tela
          }
        } else {  // Se dado invalido
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("Numero invalido");
          delay(1000);
        }
        for (int i = 0; i < buffSizeK; i++) {  // Limpa buffer
          inputBufferK[i] = '\0';
        }
        messageIndex = 0;
        lcd.clear();
      } else {  // Situação contrária a anterior
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" Modo Config:");
        lcd.setCursor(0, 1);
        lcd.print(" B -> Voltar");
        lcd.setCursor(0, 2);
        lcd.print(" C -> Limpar");
        lcd.setCursor(0, 3);
        lcd.print(" D -> Submeter");
        digitalWrite(BUZZER, HIGH);
        delay(100);
        digitalWrite(BUZZER, LOW);
        delay(1000);

        for (int i = 0; i < buffSizeK; i++) {  // Limpa buffer
          timeFromK[i] = '\0';
        }
        event = config;
        count1 = 0;
        count2 = 0;
        configIndex = 1;
        messageIndex = 0;
        quantidadeConfig = 0;
      }
      readInProgress = false;
    }

    if (key == clearMarker) {  // Tecla C, apaga último caractere
      if (event < infoFlux) {
        if (messageIndex > 0) {  // caso array não nulo
          inputBufferK[--messageIndex] = '\0';
        }
        readInProgress = false;
        lcd.clear();
      }
    }

    if (key == backMarker) {  // Tecla B, volta para tela anterior

      if (event < infoFlux) {
        for (int i = 0; i < buffSizeK; i++) {  // Limpa buffer
          inputBufferK[i] = '\0';
        }
        messageIndex = 0;
      }

      if (event == infoConfigMFC1) {
        firstPrint = 1;  // evita delay para printar
      }

      if (event == config) {  // Se tela for de configuração,
        quantidadeConfig = 0;
        event = infoFlux;  // volta para a de informação de fluxo
      } else if (event == selectMFC) {
        event -= 2;
        configIndex = 1;
        quantidadeConfig = 0;
      } else if (event == infoFlux) {
        event = infoMFC2;
      } else {
        event--;
      }

      readInProgress = false;
      lcd.clear();
    }


    if (event < infoFlux) {                                  // Impede entrada no teclado e limpeza do display a cada tecla pressionada, se estiver na tela de visualização
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
      lcd.clear();  // para qualquer tecla pressionada
    }
  }
}

//=============

void getDataFromPC() {  // Recebe data do PC e salva no buffer

  if (Serial.available() > 0) {
    char x = Serial.read();

    if (x == endMarker) {
      readInProgress = false;
      newDataFromPC = true;
      inputBufferS[bytesRecvd] = 0;
      parseData(pc);
    }

    if (readInProgress) {
      inputBufferS[bytesRecvd] = x;
      bytesRecvd++;
      if (bytesRecvd == buffSize) {
        bytesRecvd = buffSize - 1;
      }
    }

    if (x == startMarker) {
      bytesRecvd = 0;
      readInProgress = true;
    }
  }
}

//=============

void configIno(int userIndex) {  // Configura parametro a ser atualizado
  // Selecionar os valores corretos com base no índice do usuário
  bool& SPMFC_update = (userIndex == 1) ? SPMFC1_update : SPMFC2_update;
  bool& FluxMaxMFC_update = (userIndex == 1) ? FluxMaxMFC1_update : FluxMaxMFC2_update;
  bool& FatorMFC_update = (userIndex == 1) ? FatorMFC1_update : FatorMFC2_update;
  bool& FatorGas_update = (userIndex == 1) ? FatorGas1_update : FatorGas2_update;
  char* message = messageFromUser[userIndex];



  // Verificar e atualizar parâmetros com base na mensagem recebida
  if (strcmp(message, "sp") == 0 || strcmp(message, "1") == 0) {
    MFC = intFromUser;
    SPMFC_update = true;
  } else {
    SPMFC_update = false;
  }

  if (strcmp(message, "fluxmax") == 0 || strcmp(message, "2") == 0) {
    MFC = intFromUser;
    FluxMaxMFC_update = true;
  } else {
    FluxMaxMFC_update = false;
  }

  if (strcmp(message, "fatormfc") == 0 || strcmp(message, "3") == 0) {
    MFC = intFromUser;
    FatorMFC_update = true;
  } else {
    FatorMFC_update = false;
  }

  if (strcmp(message, "fatorgas") == 0 || strcmp(message, "4") == 0) {
    MFC = intFromUser;
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
  switch (inputBufferK[0]) {
    case '1':
      lcd.print("->");
      break;
    case '2':
      lcd.setCursor(0, 1);
      lcd.print("->");
      break;
    case '3':
      lcd.setCursor(0, 2);
      lcd.print("->");
      break;
    case '4':
      lcd.setCursor(0, 3);
      lcd.print("->");
      break;
  }

  lcd.setCursor(2, 0);
  lcd.print("1-SetPoint");
  lcd.setCursor(2, 1);
  lcd.print("2-FluxoMax");
  lcd.setCursor(2, 2);
  lcd.print("3-FatorMFC");
  lcd.setCursor(2, 3);
  lcd.print("4-FatorGas");
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

  if ((event > qtdConfig) && (event < infoFlux)) {
    lcd.setCursor(17, 0);
    lcd.print("CFG");
    lcd.setCursor(19, 1);
    lcd.print(configIndex);
    lcd.setCursor(19, 2);
    lcd.print("/");
    lcd.setCursor(19, 3);
    lcd.print(quantidadeConfig);
  }
  lcd.setCursor(0, 0);

  switch (event) {  // Imprime mensagem de instrução correspondente a tela

    case config:
      printConfigOptions();
      break;
    case qtdConfig:
      lcd.setCursor(0, 0);
      lcd.print("*------------------*");
      lcd.setCursor(0, 1);
      lcd.print("| Qtd setpoints");
      lcd.setCursor(1, 2);
      lcd.print("->");
      lcd.print(inputBufferK);
      lcd.setCursor(19, 1);
      lcd.print("|");
      lcd.setCursor(0, 2);
      lcd.print("|");
      lcd.setCursor(19, 2);
      lcd.print("|");
      lcd.setCursor(0, 3);
      lcd.print("*------------------*");
      break;
    case selectMFC:
      lcd.setCursor(0, 1);
      lcd.print("-> ");
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
      lcd.print("MFC:");
      lcd.print(inputBufferK);
      lcd.print("");
      break;
    case insertData:
      lcd.setCursor(0, 1);
      lcd.print("-> ");
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
      lcd.setCursor(0, 2);
      lcd.print("-> ");
      lcd.print("Valor:");
      lcd.print(inputBufferK);
      break;

    case insertTime:
      lcd.setCursor(0, 1);
      lcd.print("-> ");
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
      lcd.setCursor(0, 2);
      lcd.print("-> ");
      lcd.print("Valor:");
      lcd.print(floatFromK[configIndex]);
      lcd.setCursor(0, 3);
      lcd.print("-> ");
      lcd.print("Tempo:");
      lcd.print(inputBufferK);
      break;

    case infoFlux:
      if (SPMFC1_update || SPMFC2_update || FluxMaxMFC1_update || FluxMaxMFC2_update || FatorMFC1_update || FatorMFC2_update || FatorGas1_update || FatorGas2_update) {
        for (int i = 1; i <= quantidadeConfig; i++) {
          lcd.setCursor(1, i - 1);
          lcd.print(mfcFromK[i]);
          lcd.print(",");
          switch (messageFromK[0]) {
            case '1':
              lcd.print("SP,");
              break;
            case '2':
              lcd.print("Mx,");
              break;
            case '3':
              lcd.print("FM,");
              break;
            case '4':
              lcd.print("FG,");
              break;
          }
          lcd.print(floatFromK[i], 1);
          lcd.print(",");
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
        delay(100);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        delay(2000);
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
      lcd.print("M|--| Val  | Time |");
      lcd.setCursor(0, 1);
      lcd.print("F|");
      lcd.setCursor(0, 2);
      lcd.print("C|");
      lcd.setCursor(0, 3);
      lcd.print("1|");
      j1 = 1;
      if (quantidadeConfig == 0) {
        lcd.setCursor(4, 1);
        lcd.print("|      |");
        lcd.setCursor(4, 2);
        lcd.print("|      |");
        lcd.setCursor(4, 3);
        lcd.print("|      |");
      } else {
        for (int i = 1; (i <= quantidadeConfig); i++) {
          if (mfcFromK[i] == 1) {
            lcd.setCursor(2, j1);
            switch (messageFromK[0]) {
              case '1':
                lcd.print("SP|");
                break;
              case '2':
                lcd.print("MX|");
                break;
              case '3':
                lcd.print("FM|");
                break;
              case '4':
                lcd.print("FG|");
                break;
            }
            lcd.print(floatFromK[i], 1);
            lcd.setCursor(11, j1);
            lcd.print("|");
            minutes = (int)timeFromK[i];                           // Parte inteira dos minutos
            seconds = (int)roundf((timeFromK[i] - minutes) * 60);  // Parte fracionária convertida para segundos
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

      lcd.print("M|--| Val  | Time |");
      lcd.setCursor(0, 1);
      lcd.print("F|");
      lcd.setCursor(0, 2);
      lcd.print("C|");
      lcd.setCursor(0, 3);
      lcd.print("2|");
      j2 = 1;
      if (quantidadeConfig == 0) {
        lcd.setCursor(4, 1);
        lcd.print("|      |");
        lcd.setCursor(4, 2);
        lcd.print("|      |");
        lcd.setCursor(4, 3);
        lcd.print("|      |");
      } else {
        for (int i = 1; i <= quantidadeConfig; i++) {
          if (mfcFromK[i] == 2) {
            lcd.setCursor(2, j2);
            switch (messageFromK[0]) {
              case '1':
                lcd.print("SP|");
                break;
              case '2':
                lcd.print("MX|");
                break;
              case '3':
                lcd.print("FM|");
                break;
              case '4':
                lcd.print("FG|");
                break;
            }
            lcd.print(floatFromK[i], 1);
            lcd.setCursor(11, j2);
            lcd.print("|");
            minutes = (int)timeFromK[i];                     // Parte inteira dos minutos
            seconds = (int)((timeFromK[i] - minutes) * 60);  // Parte fracionária convertida para segundos
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

void replyToPC() {  // Imprime informações no serial (pc)

  if (newDataFromPC) {
    newDataFromPC = false;

    Serial.println(" ");

    if (SPMFC1_update || SPMFC2_update) {
      Serial.println("Set Point MFC Configurado: ");
      Serial.print("1 ");
      Serial.println(floatFromUser[1]);
      Serial.print("2 ");
      Serial.println(floatFromUser[2]);
    }

    if (FluxMaxMFC1_update || FluxMaxMFC2_update) {
      Serial.println("Fluxo Maximo MFC Configurado: ");
      Serial.print(floatFromUser[1]);
      Serial.println(floatFromUser[2]);
    }

    if (FatorMFC1_update || FatorMFC2_update) {
      Serial.println("Fator MFC Configurado: ");
      Serial.print(floatFromUser[1]);
      Serial.println(floatFromUser[2]);
    }

    if (FatorGas1_update || FatorGas2_update) {
      Serial.println("Fator Gas Configurado: ");
      Serial.print(floatFromUser[1]);
      Serial.println(floatFromUser[2]);
    }

    //Serial.print(printHelp);

    if (printHelp) {
      printHelp = false;
      Serial.println("# AJUDA #");
      Serial.println("Forma de digitar os comandos: <comando,numero do MFC,valor>");
      Serial.println("Exemplo: Quero modificar o setpoit do fluxo de gás do MFC 1.");
      Serial.println("Para isso digite o seguinte comando: <sp,1,100>");
      Serial.println("Com isso o Setpoint do MFC 1 será alterado para 100 sccm.");
      Serial.println(" ");
      Serial.println("Comandos que podem ser utilizados neste software:");
      Serial.println("ajuda - Mostra os comandos que podem ser utilizados. (ex: <ajuda>)");
      Serial.println("sp - Altera o Setpoint do fluxo de gás do MFC escolhido. (ex: <sp,1,100>)");
      Serial.println("fluxmax - Altera o fluxo máximo descrito no MFC. (ex: <fluxmax,1,500>)");
      Serial.println("fatormfc - Altera o fator de correção do gás calibrado no MFC. (ex: <fatormfc,1,0.7>)");
      Serial.println("fatorgas - Altera o fator de correção do gás de trabalho. (ex: <fatorgas,1,0.1>)");
      Serial.println(" ");
      Serial.println("Desenvolvedor:");
      Serial.println("Ornelas Corporation");
    }

    Serial.println(" ");
  }

  if (amostra >= intervalo) {
    amostra = 0;
    Serial.print("SP MFC 1: ");
    Serial.print(SPFlux1);
    Serial.print(" | Fluxo MFC 1: ");
    Serial.println(Flux1 * Fator_Gas_MFC1);

    Serial.print("SP MFC 2: ");
    Serial.print(SPFlux2);
    Serial.print(" | Fluxo MFC 2: ");
    Serial.println(Flux2 * Fator_Gas_MFC2);

  } else {
    amostra++;
  }
}

//=============

void configMFC(int mfcNum) {  // Atualiza valores nas MFCs
  // Selecionar os valores corretos com base no índice do MFC
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

void controleMFC(int mfcNum) {
  // Selecionar os valores corretos com base no índice do MFC
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

/*
  void Controle_MFC3() {
    analogWrite(SPFlux3_Pin, SPFlux3_Out);
    Flux3_Val = analogRead(Flux3_Pin);
    Flux3 = (Flux3_Val * Flux_Max3) / 1024;
  }
  */