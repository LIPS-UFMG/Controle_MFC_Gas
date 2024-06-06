
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
//Declarações Iniciais
//***************************************************************************************************************************

#include <Keypad.h>             // Biblioteca do teclado
#include <LiquidCrystal_I2C.h>  //
#include <Wire.h>               // Biblioteca do adaptador i2c do display

int event = 4;  // Indice começa mostrando informações do fluxo

const int
  pc = 1,
  arduino = 2,  // Fonte dos dados inseridos

  config = 1,
  selectMFC = 2, insertData = 3,             // Telas de interação do display
  infoFlux = 4, infoMFC1 = 5, infoMFC2 = 6;  // Telas com informações dos MFCs
// infoMFC3 = 8;

const char
  point = '#',
  backMarker = 'B', clearMarker = 'C', doneMarker = 'D',  // Teclas especiais para teclado
  startMarker = '<', endMarker = '>';                     // Teclas para comandos no pc

bool
  readInProgress = false,  // Indica estado da leitura
  newDataFromPC = false;   // Simboliza se houve inserção de dados pelo usuario

byte
  bytesRecvd = 0,    // Tamanho da mensagem recebida
  messageIndex = 0;  // Indice para leitura da mensagem

const byte
  buffSize = 40,
  buffSizeK = 8;  // Tamanho do buffer do pc e do teclado, respectivamente

char
  inputBufferK[buffSizeK],            // Buffer para a entrada do teclado
  inputBufferS[buffSize],             // Buffer para a entrada do pc
  messageFromK[buffSize] = { 0 },     // Parametro a ser alterado recebido pelo teclado
  messageFromUser[buffSize] = { 0 };  // Parametro que será alterado

int
  intFromK = 0,             // MFC selecionada pelo teclado
  intFromUser = 0,          // MFC selecionada pelo serial   ?
  MFC = 0;                  // MFC que será alterada
float floatFromUser = 0.0;  // Valores de configuração do MFC

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
  { '1', '2', '3', '.' },
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
//    SPFlux3_Out = 0,

float Flux_Max1 = 2000, SPFlux1 = 0, Flux1 = 0,
      Flux1_Val = 0, Fator_MFC1 = 1, Fator_Gas_MFC1 = 2.00,

      Flux_Max2 = 1000, SPFlux2 = 0, Flux2 = 0,
      Flux2_Val = 0, Fator_MFC2 = 1, Fator_Gas_MFC2 = 1.13;

//      Flux_Max3 = 500, SPFlux3 = 0, Flux3 = 0,
//      Flux3_Val = 0, Fator_MFC3 = 1, Fator_Gas_MFC3 = 1,

//***************************************************************************************************************************
//Rotinas Principais
//***************************************************************************************************************************

void setup() {

  //Inicia porta serial
  Serial.begin(9600);

  //Inicializa o LCD e o backlight
  lcd.init();
  lcd.backlight();

  // MFC1
  pinMode(SPFlux1_Pin, OUTPUT);
  pinMode(Vopen1_Pin, OUTPUT);
  digitalWrite(SPFlux1_Pin, 0);
  digitalWrite(Vopen1_Pin, 1);
  Flux_Max1 = Flux_Max1 * (Fator_MFC1 / Fator_Gas_MFC1);

  // MFC2
  pinMode(SPFlux2_Pin, OUTPUT);
  pinMode(Vopen2_Pin, OUTPUT);
  digitalWrite(SPFlux2_Pin, 0);
  digitalWrite(Vopen2_Pin, 1);
  Flux_Max2 = Flux_Max2 * (Fator_MFC2 / Fator_Gas_MFC2);

  /* MFC3
  pinMode(SPFlux3_Pin, OUTPUT);
  pinMode(Vopen3_Pin, OUTPUT);
  digitalWrite(SPFlux3_Pin, 0);
  digitalWrite(Vopen3_Pin, 1);
  Flux_Max3 = Flux_Max3 * (Fator_MFC3 / Fator_Gas_MFC3);
  */

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
  configIno();            // Configura parametro a ser atualizado
  printToLcd();           // Imprime informações no display
  replyToPC();            // Imprime informações no pc
  configMFC();            // Atualiza valores nas MFCs
  Controle_MFC1();        // Controla MFC1
  Controle_MFC2();        // Controla MFC2
}


//***************************************************************************************************************************
//Rotinas de Comunicação
//***************************************************************************************************************************

bool validData(char* buffer) {  // Realiza validação dos dados inseridos no teclado

  int input = atoi(buffer);
  bool ret = false;

  switch (event) {
    case config:
      ret = (input == 1 || input == 2 || input == 3 || input == 4);  // Só permite seleção de 1 a 4 no menu principal
      break;
    case selectMFC:
      ret = (input == 1 || input == 2);  // Só permite selecionar entre 2 MFCs
      break;
    case insertData:
      ret = (input <= 2000);  // numero máxio de entrada de valor
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

      case selectMFC:
        intFromK = atoi(inputBufferK);
        break;

      case insertData:
        strcpy(messageFromUser, messageFromK);
        intFromUser = intFromK;
        floatFromUser = atof(inputBufferK);
        Serial.println("Dados: ");
        Serial.print("A: ");
        Serial.println(messageFromUser);
        Serial.print("B: ");
        Serial.println(intFromUser);
        Serial.print("C: ");
        Serial.println(floatFromUser);
        break;
    }
  }
}

//=============

void getDataFromKeyboard() {  // Recebe data do teclado e salva no buffer

  char key = keypad.getKey();  // Atribui a variavel a leitura do teclado

  if (key != NO_KEY) {  // Caso tecla seja precionada
    readInProgress = true;

    if (key == doneMarker) {          // Tecla D submete valor inserido
      if (validData(inputBufferK)) {  // se dado inserido for válido
        parseData(arduino);
        newDataFromPC = true;
        if (event == infoMFC2) {  // Caso processo tenha sido finalizado
          event = infoFlux;       // volta a tela inicial
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
    }

    if (key == backMarker) {  // Tecla B, volta para tela anterior

      if (event == config) {
        event = infoFlux;
      } else if (event >= infoFlux) {
        event = config;
      } else {
        event--;
      }

      for (int i = 0; i < buffSizeK; i++) {  // Limpa buffer
        inputBufferK[i] = '\0';
      }
      messageIndex = 0;
      readInProgress = false;
    }

    if (event < infoFlux) {  // Impede entrada no teclado caso esteja na ultima tela, além de doneMarker

      if (key == clearMarker) {  // Tecla C, apaga último caractere
        if (messageIndex > 0) {  // caso array não nulo
          inputBufferK[--messageIndex] = '\0';
        }
        readInProgress = false;
      }

      if (readInProgress) {                    // Se ainda estiver lendo,
        if (messageIndex < (buffSizeK - 1)) {  // Impede que a mensagem ultrapasse o limite do buffer
          if (event != insertData) {           // Nestas telas limite para 1 caractere
            inputBufferK[0] = key;             // Adiciona caracter inserido
            messageIndex = 1;
          } else {
            inputBufferK[messageIndex++] = key;  // adiciona caracter inserido
          }
        } else {  // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
          Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
        }
        readInProgress = false;
      }
    }
    lcd.clear();
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

void printToLcd() {  // Imprime no LCD

  lcd.setCursor(0, 0);
  switch (event) {  // Imprime mensagem de instrução correspondente a tela
    case config:
      lcd.setCursor(0, 0);
      lcd.print(" 1 SetPoint");
      lcd.setCursor(0, 1);
      lcd.print(" 2 FluxoMax");
      lcd.setCursor(0, 2);
      lcd.print(" 3 FatorMFC");
      lcd.setCursor(0, 3);
      lcd.print(" 4 FatorGas");

      lcd.setCursor(17, 3);
      lcd.print(inputBufferK);
      break;

    case selectMFC:
      lcd.setCursor(0, 2);
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
      lcd.setCursor(0, 2);
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
      lcd.print(intFromK);
      lcd.print(",");
      lcd.setCursor(0, 3);
      lcd.print("-> ");
      lcd.print("Valor:");
      lcd.print(inputBufferK);
      break;

    case infoFlux:
      if (SPMFC_update || FluxMaxMFC_update || FatorMFC_update || FatorGas_update) {
        if (SPMFC_update) { lcd.print("SetPoint MFC"); }
        if (FluxMaxMFC_update) { lcd.print("FluxMax MFC"); }
        if (FatorMFC_update) { lcd.print("FatorMFC MFC"); }
        if (FatorGas_update) { lcd.print("FatorGas MFC"); }
        lcd.print("Config:");

        lcd.setCursor(0, 3);
        lcd.print(messageFromUser);
        lcd.print(",");
        lcd.print(MFC);
        lcd.print(",");
        lcd.print(floatFromUser);

        delay(1000);
      }

      if ((amostra == intervalo)) {  // com o dobro da frequencia do serial
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MFC1:");
        lcd.print(SPFlux1);
        lcd.print(" | ");
        lcd.print(Flux1 * Fator_Gas_MFC1);

        lcd.setCursor(0, 2);
        lcd.print("MFC2:");
        lcd.print(SPFlux2);
        lcd.print(" | ");
        lcd.print(Flux2 * Fator_Gas_MFC2);
        /*
        lcd.print("MFC3: SP");
        lcd.print(SPFlux3);
        lcd.print(" Flux:");
        lcd.print(Flux3 * Fator_Gas_MFC3);
        */
        lcd.setCursor(17, 3);
        lcd.print("1/3");
      }
      break;

    case infoMFC1:

      lcd.print("M SetPoint:");
      lcd.print(SPFlux1);

      lcd.setCursor(0, 1);
      lcd.print("F FluxoMax:");
      lcd.print(Flux_Max1);

      lcd.setCursor(0, 2);
      lcd.print("C FatorMFC:");
      lcd.print(Fator_MFC1);

      lcd.setCursor(0, 3);
      lcd.print("1 FatorGas:");
      lcd.print(Fator_Gas_MFC1);

      lcd.setCursor(17, 3);
      lcd.print("2/3");
      break;

    case infoMFC2:
      lcd.print("M SetPoint:");
      lcd.print(SPFlux2);

      lcd.setCursor(0, 1);
      lcd.print("F FluxoMax:");
      lcd.print(Flux_Max2);

      lcd.setCursor(0, 2);
      lcd.print("C FatorMFC:");
      lcd.print(Fator_MFC2);

      lcd.setCursor(0, 3);
      lcd.print("2 FatorGas:");
      lcd.print(Fator_Gas_MFC2);

      lcd.setCursor(17, 3);
      lcd.print("3/3");
      break;

      /* case infoMFC3:
        lcd.print("MFC1: SetPoint:");
        lcd.print(SPFlux2);
        lcd.setCursor(0, 1);
        lcd.print("      FluxMax:");
        lcd.print(Flux_Max2);
        lcd.setCursor(0, 2);
        lcd.print("      FatorMFC:");
        lcd.print(Fator_MFC2);
        lcd.setCursor(0, 3);
        lcd.print("      FatorGas:");
        lcd.print(Fator_Gas_MFC2);
        lcd.setCursor(17, 3);
        lcd.print("4/4");
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
