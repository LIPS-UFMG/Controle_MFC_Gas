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

#include <Keypad.h>         // Biblioteca do teclado
#include <LiquidCrystal.h>  // Biblioteca do display

const byte buffSize = 20;
char inputBufferS[buffSize];                                                    // Buffer para a entrada do pc
char inputBufferK[buffSize];                                                    // Buffer para a entrada do display
const int mainMenu = 1, selectMFC = 2, insertData = 3, working = 4;             // Telas de interação do display
int event = 1;                                                                  // Indice para telas de interação
const int pc = 1, arduino = 2;                                                  // Fonte dos dados inseridos
const char point = '#', backMarker = 'B', clearMarker = 'C', doneMarker = 'D',  // Teclas especiais para teclado
  startMarker = '<', endMarker = '>';                                           // Teclas para comandos no pc
byte bytesRecvd = 0;
byte messageIndex = 0;
boolean readInProgress = false;
boolean newDataFromPC = false;

char messageFromPC[buffSize] = { 0 };
char messageFromPCAux[buffSize] = { 0 };

int intFromPCAux = 0;
int intFromPC = 0;
float floatFromPC = 0.0;

int MFC = 0;
boolean SPMFC_update = false;
boolean FluxMaxMFC_update = false;
boolean FatorMFC_update = false;
boolean FatorGas_update = false;
boolean imprimeAjuda = false;

int amostra = 0;
int intervalo = 3000;

//***************************************************************************************************************************
//Teclado e Display
//***************************************************************************************************************************

const byte LINHAS = 4;   // Linhas do teclado
const byte COLUNAS = 4;  // Colunas do teclado

const char TECLAS_MATRIZ[LINHAS][COLUNAS] = {  // Matriz de caracteres (mapeamento do teclado)
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

byte PINOS_LINHAS[LINHAS] = { A0, A1, 7, 6 };      // Pinos de conexao com as linhas do teclado
byte PINOS_COLUNAS[COLUNAS] = { A2, A3, A4, A5 };  // Pinos de conexao com as colunas do teclado


Keypad keypad = Keypad(makeKeymap(TECLAS_MATRIZ), PINOS_LINHAS,
                       PINOS_COLUNAS, LINHAS, COLUNAS);  // Inicia teclado

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
#define SPFlux3_Pin 3
#define Vopen3_Pin 2
#define Flux3_Pin A0

//Pinagem MFC 4
#define SPFlux4_Pin 3
#define Vopen4_Pin 2
#define Flux4_Pin A0
*/

//Configurações MFCs
int SPFlux1_Out = 0,
    SPFlux2_Out = 0;
//    SPFlux3_Out = 0,
//    SPFlux4_Out = 0;

float Flux_Max1 = 2000, SPFlux1 = 0, Flux1 = 0,
      Flux1_Val = 0, Fator_MFC1 = 1, Fator_Gas_MFC1 = 2.00,

      Flux_Max2 = 1000, SPFlux2 = 0, Flux2 = 0,
      Flux2_Val = 0, Fator_MFC2 = 1, Fator_Gas_MFC2 = 1.13;

//      Flux_Max3 = 500, SPFlux3 = 0, Flux3 = 0,
//      Flux3_Val = 0, Fator_MFC3 = 1, Fator_Gas_MFC3 = 1,

//      Flux_Max4 = 500, SPFlux4 = 0, Flux4 = 0, Flux4_Val = 0,
//      Fator_MFC4 = 1, Fator_Gas_MFC4 = 1;


//***************************************************************************************************************************
//Rotinas Principais
//***************************************************************************************************************************

void setup() {


  Serial.begin(9600);  //Inicia Porta Serial
  lcd.begin(16, 2);    //Inicia o lcd

  pinMode(SPFlux1_Pin, OUTPUT);
  pinMode(Vopen1_Pin, OUTPUT);

  digitalWrite(SPFlux1_Pin, 0);
  digitalWrite(Vopen1_Pin, 1);

  Flux_Max1 = Flux_Max1 * (Fator_MFC1 / Fator_Gas_MFC1);

  pinMode(SPFlux2_Pin, OUTPUT);
  pinMode(Vopen2_Pin, OUTPUT);

  digitalWrite(SPFlux2_Pin, 0);
  digitalWrite(Vopen2_Pin, 1);

  Flux_Max2 = Flux_Max2 * (Fator_MFC2 / Fator_Gas_MFC2);

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
  getDataFromPC();        // Recebe dados do serial
  getDataFromKeyboard();  // Recebe dados do teclado
  configIno();            // Configura MFCs
  replyToPC();            // Envia dados para o serial
  Controle_MFC1();        // Controla MFC1
  Controle_MFC2();        // Controla MFC2
  printToLcd();           // Imprime no LCD
  PrintFluxo();           // Imprime Fluxo no serial
}


//***************************************************************************************************************************
//Rotinas de Comunicação Serial
//***************************************************************************************************************************

void getDataFromPC() {  // Recebe data no PC e salva no buffer

  if (Serial.available() > 0) {
    char x = Serial.read();

    // the order of these IF clauses is significant

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

void getDataFromKeyboard() {  // Recebe data do teclado e salva no buffer

  char key = keypad.getKey();  // Atribui a variavel a leitura do teclado

  if (key != NO_KEY) {              // Caso tecla seja precionada
    if (messageIndex < buffSize) {  // Impede que a mensagem ultrapasse o limite

      readInProgress = true;

      if (key == point) {                    // Tecla #
        inputBufferK[messageIndex++] = '.';  // substituida por ponto
        readInProgress = false;
      }

      if (key == backMarker) {                // Tecla B, volta para tela anterior
        for (int i = 0; i < buffSize; i++) {  // Limpa buffer
          inputBufferK[i] = '\0';
        }
        messageIndex = 0;
        readInProgress = false;
        if (event != mainMenu) {
          event--;
        }
      }

      if (key == clearMarker) {                          // Tecla C, apaga último caractere
        if (messageIndex != "\0" && messageIndex > 0) {  // caso array não nulo
          inputBufferK[--messageIndex] = '\0';
        }
        readInProgress = false;
      }

      if (key == doneMarker) {          // Tecla D submete valor inserido
        if (validData(inputBufferK)) {  // se dado inserido for válido
          parseData(arduino);
          if (event == working) {  // Caso processo tenha sido finalizado
            event = 0;             // volta a tela inicial
          }
          messageIndex = 0;
          event++;  // Passa para próxima tela

        } else {  // Se dado invalido
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("Numero invalido");
          delay(1000);
        }
        for (int i = 0; i < buffSize; i++) {  // Limpa buffer
          inputBufferK[i] = '\0';
        }
        readInProgress = false;
      }

      if (readInProgress) {                  // Se ainda estiver lendo,
        inputBufferK[messageIndex++] = key;  // adiciona caracter inserido
        readInProgress = false;
      }

    } else {  // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
      Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
    }
    lcd.clear();
  }
}

void printToLcd() {  //imprime instrução no LCD
  lcd.setCursor(0, 1);
  lcd.print(inputBufferK);
  lcd.setCursor(0, 0);
  switch (event) {  // Imprime mensagem correspondente a tela
    case mainMenu:
      lcd.print("Escolha: 1 2 3 4");
      break;
    case selectMFC:
      lcd.println("1-MFC1 2-MFC2");
      break;
    case insertData:
      lcd.println("Digite um valor:");
      break;
    case working:
      lcd.println("Em funcionamento");
      lcd.setCursor(0, 1);
      lcd.print("A: ");
      lcd.print(messageFromPC);
      lcd.print("B: ");
      lcd.print(intFromPC);
      lcd.print("C: ");
      lcd.print(floatFromPC);
      break;
  }
}

bool validData(char* buffer) {  // Realiza validação dos dados

  int input = atoi(buffer);
  bool ret = false;

  switch (event) {
    case mainMenu:
      ret = (input == 1 || input == 2 || input == 3 || input == 4);
      break;
    case selectMFC:
      ret = (input == 1 || input == 2);
      break;
    default:
      ret = true;
  }

  return ret;
}

//=============

void parseData(int source) {  // Atualiza dados inseridos


  if (source == pc) {
    char* strtokIndx;

    strtokIndx = strtok(inputBufferS, ",");
    strcpy(messageFromPC, strtokIndx);

    strtokIndx = strtok(NULL, ",");
    intFromPC = atoi(strtokIndx);

    strtokIndx = strtok(NULL, ",");
    floatFromPC = atof(strtokIndx);
  }

  if (source == arduino) {
    switch (event) {
      case mainMenu:
        strcpy(messageFromPCAux, inputBufferK);
        break;

      case selectMFC:
        intFromPCAux = atoi(inputBufferK);
        break;

      case insertData:
        newDataFromPC = true;
        strcpy(messageFromPC, messageFromPCAux);
        intFromPC = intFromPCAux;
        floatFromPC = atof(inputBufferK);
        Serial.println("Dados: ");
        Serial.print("A: ");
        Serial.println(messageFromPC);
        Serial.print("B: ");
        Serial.println(intFromPC);
        Serial.print("C: ");
        Serial.println(floatFromPC);
        break;
    }
  }
}



//=============

void replyToPC() {

  if (newDataFromPC) {
    newDataFromPC = false;

    Serial.println(" ");

    if (SPMFC_update) {
      SPMFC_update = false;
      Serial.print("SP MFC ");
      Serial.print(MFC);
      Serial.print(" Configurado: ");
      Serial.println(floatFromPC);
    }

    if (FluxMaxMFC_update) {
      FluxMaxMFC_update = false;
      Serial.print("Fluxo Maximo MFC Configurado: ");
      Serial.println(floatFromPC);
    }

    if (FatorMFC_update) {
      FatorMFC_update = false;
      Serial.print("Fator MFC Configurado: ");
      Serial.println(floatFromPC);
    }

    if (FatorGas_update) {
      FatorGas_update = false;
      Serial.print("Fator Gas Configurado: ");
      Serial.println(floatFromPC);
    }

    if (imprimeAjuda) {
      imprimeAjuda = false;
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
}

//=============

void configIno() {

  if (strcmp(messageFromPC, "sp") == 0 || strcmp(messageFromPC, "1") == 0) {
    MFC = intFromPC;
    SPMFC_update = true;
  } else {
    SPMFC_update = false;
  }

  if (strcmp(messageFromPC, "fluxmax") == 0 || strcmp(messageFromPC, "2") == 0) {
    MFC = intFromPC;
    FluxMaxMFC_update = true;
  } else {
    FluxMaxMFC_update = false;
  }

  if (strcmp(messageFromPC, "fatormfc") == 0 || strcmp(messageFromPC, "3") == 0) {
    MFC = intFromPC;
    FatorMFC_update = true;
  } else {
    FatorMFC_update = false;
  }

  if (strcmp(messageFromPC, "fatorgas") == 0 || strcmp(messageFromPC, "4") == 0) {
    MFC = intFromPC;
    FatorGas_update = true;
  } else {
    FatorGas_update = false;
  }

  configMFC();

  if (strcmp(messageFromPC, "ajuda") == 0) {
    imprimeAjuda = true;
  } else {
    imprimeAjuda = false;
  }
}

//=============

void configMFC() {

  switch (MFC) {

    case 1:
      if (SPMFC_update) { SPFlux1 = floatFromPC * (Fator_MFC1 / Fator_Gas_MFC1); }  //SPFlux1 - Fluxo Real
      if (FluxMaxMFC_update) { Flux_Max1 = floatFromPC; }
      if (FatorMFC_update) { Fator_MFC1 = floatFromPC; }
      if (FatorGas_update) { Fator_Gas_MFC1 = floatFromPC; }
      break;

    case 2:
      if (SPMFC_update) { SPFlux2 = floatFromPC * (Fator_MFC2 / Fator_Gas_MFC2); }  //SPFlux2 - Fluxo Real
      if (FluxMaxMFC_update) { Flux_Max2 = floatFromPC; }
      if (FatorMFC_update) { Fator_MFC2 = floatFromPC; }
      if (FatorGas_update) { Fator_Gas_MFC2 = floatFromPC; }
      break;

    default:
      SPMFC_update = false;
      FluxMaxMFC_update = false;
      FatorMFC_update = false;
      FatorGas_update = false;
      break;
  }
}

//=============

void PrintFluxo() {

  if (amostra >= intervalo) {
    amostra = 0;
    Serial.print("SP MFC 1: ");
    Serial.print(SPFlux1);
    Serial.print(" | Fluxo MFC 1: ");
    Serial.println(Flux1 * Fator_Gas_MFC1);

    amostra = 0;
    Serial.print("SP MFC 2: ");
    Serial.print(SPFlux2);
    Serial.print(" | Fluxo MFC 2: ");
    Serial.println(Flux2 * Fator_Gas_MFC2);
  } else {
    amostra++;
  }
}

//***************************************************************************************************************************
//Rotinas de Controle dos MFC's
//***************************************************************************************************************************

void Controle_MFC1() {

  SPFlux1_Out = (255 * SPFlux1) / Flux_Max1;
  analogWrite(SPFlux1_Pin, SPFlux1_Out);

  Flux1_Val = analogRead(Flux1_Pin);
  Flux1 = (Flux1_Val * Flux_Max1) / 1024;
}

void Controle_MFC2() {

  SPFlux2_Out = (255 * SPFlux2) / Flux_Max2;
  analogWrite(SPFlux2_Pin, SPFlux2_Out);

  Flux2_Val = analogRead(Flux2_Pin);
  Flux2 = (Flux2_Val * Flux_Max2) / 1024;
}
