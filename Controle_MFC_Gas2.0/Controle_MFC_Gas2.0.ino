
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

int event = 6,     // Indice começa mostrando informações do fluxo
  firstPrint = 0;  // Evita delay na transição para a tela "infoFlux"

const int
  pc = 1,
  arduino = 2,                                               // Fonte dos dados inseridos
  config = 1, qtdConfig = 2, selectMFC = 3, insertData = 4,  // Telas de interação do display
  insertTime = 5, infoFlux = 6, infoConfig = 7,
  //  infoFlux2 = 8,
  infoMFC1 = 8, infoMFC2 = 9;  // Telas com informações dos MFCs
//  infoMFC3 = 10;

const char
  point = '#',
  backMarker = 'B', clearMarker = 'C', doneMarker = 'D',  // Teclas especiais para teclado
  startMarker = '<', endMarker = '>';                     // Teclas para comandos no pc

bool
  readInProgress = false,  // Indica estado da leitura
  newDataFromPC = false,   // Simboliza se houve inserção de dados pelo usuario
  timerInit = false;

byte
  bytesRecvd = 0,    // Tamanho da mensagem recebida
  messageIndex = 0;  // Indice para leitura da mensagem

const byte
  buffSize = 40,
  buffSizeK = 8;  // Tamanho do buffer do pc e do teclado, respectivamente

char
  inputBufferK[buffSizeK],            // Buffer para a entrada do teclado
  inputBufferS[buffSize],             // Buffer para a entrada do pc
  messageFromK[buffSize],             // Parametro a ser alterado recebido pelo teclado
  messageFromUser[buffSize] = { 0 };  // Parametro que será alterado

int
  mfcFromK[10] = { 0 },  // MFC selecionada pelo teclado
  quantidadeConfig = 0,  //
  configIndex = 0,       //
  timeFromUser = 0,      //
  intFromUser = 0,       // MFC selecionada pelo serial   ?
  MFC = 0;               // MFC que será alterada

unsigned long
  baseTime = 0,    //
  timePassed = 0;  //
float
  floatFromK[10] = { 0 },  //
  timeFromK[10] = { 0 },   //
  floatFromUser = 0.0,     // Valores de configuração do MFC
  totalTime = 0.0;         //

bool
  printHelp = false,                                 // Estado para imprimir ajuda
  SPMFC_update = false, FluxMaxMFC_update = false,   //
  FatorMFC_update = false, FatorGas_update = false;  // Parâmetros de uma MFC

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

//Pinagem MFC 1
#define SPFlux1_Pin 3
#define Vopen1_Pin 2
#define Flux1_Pin A0

//Pinagem MFC 2
#define SPFlux2_Pin 4
#define Vopen2_Pin 5
#define Flux2_Pin A1
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

void controleMFC(int SPFlux_Pin, int Vopen_Pin, float& SPFlux, float& Flux, int& SPFlux_Out) {
  analogWrite(SPFlux_Pin, SPFlux_Out);
  digitalWrite(Vopen_Pin, SPFlux > 0 ? HIGH : LOW);
}

void loop() {
  getDataFromKeyboard();  // Recebe dados do teclado
  getDataFromPC();        // Recebe dados do pc
  timer();
  configIno();   // Configura parametro a ser atualizado
  printToLcd();  // Imprime informações no display
  replyToPC();   // Imprime informações no pc
  configMFC();   // Atualiza valores nas MFCs
  controleMFC(3, 2, SPFlux1, Flux1, SPFlux1_Out);
  controleMFC(4, 5, SPFlux2, Flux2, SPFlux2_Out);
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
      ret = ((input >= 1) && (input <= 3));  // maximo de configs
      break;
    case selectMFC:
      ret = (input == 1 || input == 2);  // Só permite selecionar entre MFCs 1 e 2
      break;
    case insertData:
      ret = ((input >= 0) && (input <= 2000));  // numero máximo de entrada de valor
      break;
    case insertTime:
      ret = ((input >= 0.1) && (input <= 999));  // numero máximo de entrada de valor
      break;
    default:
      ret = true;
      break;
  }
  return ret;
}

//=============

void parseData(int source) {  // Converte dados inseridos para controle das MFCs

  if (source == pc) {  // Trata dados se foram inseridos pelo computador
    char* strtokIndx;

    strtokIndx = strtok(inputBufferS, ",");
    strcpy(messageFromUser, strtokIndx);

    strtokIndx = strtok(NULL, ",");
    intFromUser = atoi(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    floatFromUser = atof(strtokIndx);
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
          strcpy(messageFromUser, messageFromK);
          intFromUser = mfcFromK[configIndex];
          floatFromUser = floatFromK[configIndex];
        }
        break;
      case insertTime:
        timeFromK[configIndex] = atof(inputBufferK);


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

  if (source == timer) {
    strcpy(messageFromUser, messageFromK);
    intFromUser = mfcFromK[configIndex];
    floatFromUser = floatFromK[configIndex];
    timeFromUser = timeFromK[configIndex];
  }
}

//=============

void timer() {  //

  if (timerInit) {
    baseTime = millis();
    totalTime = timeFromK[configIndex];
    timerInit = false;
    parseData(timer);
  }

  if (totalTime > 0) {
    if (timeFromK[configIndex] > 0) {
      timePassed = millis() - baseTime;
      float minutesPassed = timePassed / 60000.0;
      float timeRemaining = totalTime - minutesPassed;
      if (timeRemaining > 0) {
        timeFromK[configIndex] = timeRemaining;
      } else {
        timeFromK[configIndex] = 0;
      }
    } else if (configIndex < quantidadeConfig) {  // acabou a temporização mais ainda há mais na fila
      switch (intFromUser) {
        case 1:
          SPFlux1 = 0;
          break;
        case 2:
          SPFlux2 = 0;
          break;
      }
      timeFromK[configIndex] = totalTime;
      configIndex++;
      timerInit = true;

    } else {  // acabou o processo
      switch (intFromUser) {
        case 1:
          SPFlux1 = 0;
          break;
        case 2:
          SPFlux2 = 0;
          break;
      }
      timeFromK[configIndex] = totalTime;
      totalTime = 0;
      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("Processo finalizado!");
      delay(5000);
      lcd.clear();
    }
  }
}

//=============

void getDataFromKeyboard() {  // Recebe data do teclado e salva no buffer

  char key = keypad.getKey();  // Atribui a variavel a leitura do teclado

  if (key != NO_KEY && key != 'A') {  // Caso tecla seja precionada
    readInProgress = true;

    if (key == doneMarker) {          // Tecla D submete valor inserido
      if (validData(inputBufferK)) {  // se dado inserido for válido
        parseData(arduino);
        newDataFromPC = true;

        if (event == config) { quantidadeConfig = 1; }

        if (event == infoMFC2) {  // Caso processo tenha sido finalizado
          event = infoFlux;       // volta a tela inicial
          firstPrint = 1;         // Seta para primeira impressão rápida
        } else if (event == config || event == insertData) {
          if (messageFromK[0] != '1') {
            event += 2;
          } else {
            event++;
          }
        } else if (event == insertTime) {
          if (configIndex < quantidadeConfig) {
            event = selectMFC;
            configIndex++;
          } else {
            configIndex = 1;
            timerInit = true;
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
      readInProgress = false;
      lcd.clear();
    }

    if (key == backMarker) {  // Tecla B, volta para tela anterior

      if (event == infoConfig) {
        firstPrint = 1;  // evita delay para printar
      }

      if (event == config) {  // Se tela for de configuração,
        quantidadeConfig = 0;
        event = infoFlux;                      // volta para a de informação de fluxo
      } else if (event == infoFlux) {          // Situação contrária a anterior
        for (int i = 0; i < buffSizeK; i++) {  // Limpa buffer
          timeFromK[i] = '\0';
        }
        event = config;
        configIndex = 1;
        quantidadeConfig = 0;
      } else if (event == selectMFC) {
        event -= 2;
      } else {
        event--;
      }

      for (int i = 0; i < buffSizeK; i++) {  // Limpa buffer
        inputBufferK[i] = '\0';
      }
      messageIndex = 0;
      readInProgress = false;
      lcd.clear();
    }

    if (event < infoFlux) {      // Impede entrada no teclado e limpeza do display a cada tecla pressionada, se estiver na tela de visualização
      if (key == clearMarker) {  // Tecla C, apaga último caractere
        if (messageIndex > 0) {  // caso array não nulo
          inputBufferK[--messageIndex] = '\0';
        }
        readInProgress = false;
      }

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

void configIno() {  // Configura parametro a ser atualizado

  if (strcmp(messageFromUser, "sp") == 0 || strcmp(messageFromUser, "1") == 0) {
    MFC = intFromUser;
    SPMFC_update = true;
  } else {
    SPMFC_update = false;
  }

  if (strcmp(messageFromUser, "fluxmax") == 0 || strcmp(messageFromUser, "2") == 0) {
    MFC = intFromUser;
    FluxMaxMFC_update = true;
  } else {
    FluxMaxMFC_update = false;
  }

  if (strcmp(messageFromUser, "fatormfc") == 0 || strcmp(messageFromUser, "3") == 0) {
    MFC = intFromUser;
    FatorMFC_update = true;
  } else {
    FatorMFC_update = false;
  }

  if (strcmp(messageFromUser, "fatorgas") == 0 || strcmp(messageFromUser, "4") == 0) {
    MFC = intFromUser;
    FatorGas_update = true;
  } else {
    FatorGas_update = false;
  }

  if (strcmp(messageFromUser, "ajuda") == 0) {
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
  lcd.print("M SetPoint:");
  lcd.print(SPFlux, 1);
  lcd.setCursor(18, 0);
  lcd.print("|");
  lcd.setCursor(0, 1);
  lcd.print("F FluxoMax:");
  lcd.print(Flux_Max, 1);
  lcd.setCursor(18, 1);
  lcd.print("|");
  lcd.print(index + 2);

  lcd.setCursor(0, 2);
  lcd.print("C FatorMFC:");
  lcd.print(Fator_MFC, 1);
  lcd.setCursor(18, 2);
  lcd.print("|/");

  lcd.setCursor(0, 3);
  lcd.print(index);
  lcd.print(" FatorGas:");
  lcd.print(Fator_Gas_MFC, 1);
  lcd.setCursor(18, 3);
  lcd.print("|4");
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
      lcd.print("| Configuracoes");
      lcd.setCursor(19, 1);
      lcd.print("|");
      lcd.setCursor(0, 2);
      lcd.print("| Qtd: ");
      lcd.print(inputBufferK);
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
      if (SPMFC_update || FluxMaxMFC_update || FatorMFC_update || FatorGas_update) {
        lcd.print("Configurado!");
        for (int i = 1; i <= quantidadeConfig; i++) {
          lcd.setCursor(1, i);
          lcd.print(mfcFromK[i]);
          lcd.print(",");
          switch (messageFromK[0]) {
            case '1':
              lcd.print("SP,");
              break;
            case '2':
              lcd.print("FM,");
              break;
            case '3':
              lcd.print("MFC,");
              break;
            case '4':
              lcd.print("FG,");
              break;
          }
          lcd.print(floatFromK[i], 1);
          lcd.print(",");
          if (timeFromK[i] != 0.0) {
            lcd.print(timeFromK[i]);
          }
        }
        delay(3000);
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
        lcd.print("==================|4");

        firstPrint = 0;
      }
      break;

    case infoConfig:
      lcd.print("M|-| Val  | Time  |");
      for (int i = 1; i <= quantidadeConfig; i++) {
        lcd.setCursor(0, i);
        lcd.print(mfcFromK[i]);
        lcd.print("|");
        switch (messageFromK[0]) {
          case '1':
            lcd.print("S|");
            break;
          case '2':
            lcd.print("F|");
            break;
          case '3':
            lcd.print("M|");
            break;
          case '4':
            lcd.print("G|");
            break;
        }
        lcd.print(floatFromK[i], 1);
        lcd.setCursor(10, i);
        lcd.print("|");
        lcd.print(timeFromK[i]);
        if (i <= configIndex) {
          lcd.setCursor(17, i);
          lcd.print("+");
        }
      }
      lcd.setCursor(18, 1);
      lcd.print("|2");
      lcd.setCursor(18, 2);
      lcd.print("|/");
      lcd.setCursor(18, 3);
      lcd.print("|4");

      break;
    /*
    case infoFlux2:
      if ((amostra == intervalo)) {

        float trueFlux3 = Flux3 * Fator_Gas_MFC3;

        lcd.print("MFC|SetPoint|Fluxo");

        lcd.setCursor(0, 1);
        lcd.print(" 3 |");
        lcd.print(SPFlux3);

        lcd.setCursor(12, 1);
        lcd.print("|");
        lcd.print("       ");
        lcd.setCursor(13, 1);
        lcd.print(trueFlux3);

        lcd.setCursor(0, 3);
        lcd.print("=================2/5");
      }
      break;
    */
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

    if (SPMFC_update) {
      Serial.print("Set Point MFC Configurado: ");
      Serial.println(floatFromUser);
    }

    if (FluxMaxMFC_update) {
      Serial.print("Fluxo Maximo MFC Configurado: ");
      Serial.println(floatFromUser);
    }

    if (FatorMFC_update) {
      Serial.print("Fator MFC Configurado: ");
      Serial.println(floatFromUser);
    }

    if (FatorGas_update) {
      Serial.print("Fator Gas Configurado: ");
      Serial.println(floatFromUser);
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

void configMFC() {  // Atualiza valores nas MFCs

  switch (MFC) {

    case 1:
      if (SPMFC_update) { SPFlux1 = floatFromUser; }
      if (FluxMaxMFC_update) { Flux_Max1 = floatFromUser; }
      if (FatorMFC_update) { Fator_MFC1 = floatFromUser; }
      if (FatorGas_update) { Fator_Gas_MFC1 = floatFromUser; }

      if (SPMFC_update || FluxMaxMFC_update || FatorMFC_update || FatorGas_update) {  //
        SPFlux1_Out = (255 * SPFlux1 * (Fator_MFC1 / Fator_Gas_MFC1)) / Flux_Max1;    // Regra de 3 simples
        SPMFC_update = false;
        FluxMaxMFC_update = false;
        FatorMFC_update = false;
        FatorGas_update = false;
      }

      break;

    case 2:
      if (SPMFC_update) { SPFlux2 = floatFromUser; }
      if (FluxMaxMFC_update) { Flux_Max2 = floatFromUser; }
      if (FatorMFC_update) { Fator_MFC2 = floatFromUser; }
      if (FatorGas_update) { Fator_Gas_MFC2 = floatFromUser; }

      if (SPMFC_update || FluxMaxMFC_update || FatorMFC_update || FatorGas_update) {  //
        SPFlux2_Out = (255 * SPFlux2 * (Fator_MFC2 / Fator_Gas_MFC2)) / Flux_Max2;    // Regra de 3 simples
        SPMFC_update = false;
        FluxMaxMFC_update = false;
        FatorMFC_update = false;
        FatorGas_update = false;
      }
      break;
      /*
    case 3:
      if (SPMFC_update) { SPFlux3 = floatFromUser; }
      if (FluxMaxMFC_update) { Flux_Max3 = floatFromUser; }
      if (FatorMFC_update) { Fator_MFC3 = floatFromUser; }
      if (FatorGas_update) { Fator_Gas_MFC3 = floatFromUser; }

      if (SPMFC_update || FluxMaxMFC_update || FatorMFC_update || FatorGas_update) {  //
        SPFlux3_Out = (255 * SPFlux3 * (Fator_MFC3 / Fator_Gas_MFC3)) / Flux_Max3;    // Regra de 3 simples
        SPMFC_update = false;
        FluxMaxMFC_update = false;
        FatorMFC_update = false;
        FatorGas_update = false;
      }
      break;
      */
    default:
      SPMFC_update = false;
      FluxMaxMFC_update = false;
      FatorMFC_update = false;
      FatorGas_update = false;
      break;
  }

  for (int i = 0; i < buffSize; i++) {  // Limpa buffer
    messageFromUser[i] = '\0';
  }
}

//***************************************************************************************************************************
//Rotinas de Controle dos MFC's
//***************************************************************************************************************************

void Controle_MFC1() {
  analogWrite(SPFlux1_Pin, SPFlux1_Out);
  Flux1_Val = analogRead(Flux1_Pin);
  Flux1 = (Flux1_Val * Flux_Max1) / 1024;
}

void Controle_MFC2() {
  analogWrite(SPFlux2_Pin, SPFlux2_Out);
  Flux2_Val = analogRead(Flux2_Pin);
  Flux2 = (Flux2_Val * Flux_Max2) / 1024;
}
/*
  void Controle_MFC3() {
    analogWrite(SPFlux3_Pin, SPFlux3_Out);
    Flux3_Val = analogRead(Flux3_Pin);
    Flux3 = (Flux3_Val * Flux_Max3) / 1024;
  }
  */