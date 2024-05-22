/***************************
* Teclado Matricial 16 Teclas : Primeiros Passos (v1.0)
*
* Codigo base para exibir as teclas pressionadas no monitor serial da IDE.
*
* Copyright 2020 RoboCore.
* Escrito por Matheus Cassioli (30/07/2019).
* Atualizado por Giovanni de Castro (22/01/2020).
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version (<https://www.gnu.org/licenses/>).
***************************/

#include <Keypad.h>         // Biblioteca do codigo
#include <LiquidCrystal.h>  //Biblioteca do display

const byte LINHAS = 4;   // Linhas do teclado
const byte COLUNAS = 4;  // Colunas do teclado

const int mainMenu = 1;
const int selectMFC = 2;
const int insertData = 3;
const char endMarker = 'D';

byte messageIndex = 0;
const byte buffSize = 20;
char inputBuffer2[buffSize];  // Buffer para a entrada do display

char messageFromPC[buffSize] = { 0 };
int intFromPC = 0;
float floatFromPC = 0.0;  // fraction of servo range to move

int Event = 1;

const char TECLAS_MATRIZ[LINHAS][COLUNAS] = {  // Matriz de caracteres (mapeamento do teclado)
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

byte PINOS_LINHAS[LINHAS] = { A0, A1, 7, 6 };      // Pinos de conexao com as linhas do teclado
byte PINOS_COLUNAS[COLUNAS] = { A2, A3, A4, A5 };  // Pinos de conexao com as colunas do teclado

int a = 0;
int b = 0;

Keypad teclado_personalizado =
  Keypad(makeKeymap(TECLAS_MATRIZ), PINOS_LINHAS, PINOS_COLUNAS, LINHAS, COLUNAS);  // Inicia teclado

void setup() {
  Serial.begin(9600);  // Inicia porta serial
  lcd.begin(16, 2);
}

void loop() {
  getDataFromDisplay();
  delay(30);
}


void getDataFromDisplay() {

  char leitura_teclas = teclado_personalizado.getKey();  // Atribui a variavel a leitura do teclado

  if (Event == mainMenu) {  // tela do menu principal
    lcd.setCursor(0, 0);
    lcd.print("Escolha: 1 2 3 4");
    //Serial.println("1 - SetPoint");
    //Serial.println("2 - FluxMax");
    //Serial.println("3 - FatorMFC");
    //Serial.println("4 - FatorGas");
    if (leitura_teclas == endMarker) {
      Serial.println("Mensagem recebida:");
      Serial.println(inputBuffer2);
      inputBuffer2[messageIndex] = 0;
      messageIndex = 0;
      parseData2();
      int input;
      input = atoi(inputBuffer2);
      if (input == 1 || input == 2 || input == 3 || input == 4) {  // caso entrada seja válida
        for (int i = 0; i < buffSize; i++) {                       //loop para apagar buffer
          inputBuffer2[i] = '\0';                                  // Preenche o buffer com caracteres nulos
        }
        lcd.clear();
        Event++;
      } else {  //exclui texto se entrada invalida
        for (int i = 0; i < buffSize; i++) {
          inputBuffer2[i] = '\0';  // Preenche o buffer com caracteres nulos
        }
      }
    } else if (leitura_teclas != NO_KEY) {
      if (messageIndex < buffSize) {
        inputBuffer2[messageIndex++] = leitura_teclas;
      } else {
        // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
        Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
      }
    }
  }

  else if (Event == selectMFC) {
    //Serial.println("Escolha uma opcao: ");
    lcd.setCursor(0, 0);
    lcd.println("1-MFC1 2-MFC2");

    if (leitura_teclas == endMarker) {
      Serial.println("Mensagem recebida:");
      Serial.println(inputBuffer2);
      inputBuffer2[messageIndex] = 0;
      messageIndex = 0;
      parseData2();

      int input2;
      input2 = atof(inputBuffer2);
      if (input2 == 1 || input2 == 2) {
        for (int i = 0; i < buffSize; i++) {
          inputBuffer2[i] = '\0';  // Preenche o buffer com caracteres nulos
        }
        lcd.clear();
        Event = Event + 1;
      } else {
        for (int i = 0; i < buffSize; i++) {
          inputBuffer2[i] = '\0';  // Preenche o buffer com caracteres nulos
        }
      }
    } else if (leitura_teclas != NO_KEY) {
      if (messageIndex < buffSize) {
        inputBuffer2[messageIndex++] = leitura_teclas;
      } else {
        // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
        Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
      }
    }
  }


  else if (Event == insertData) {
    lcd.setCursor(0, 0);
    lcd.println("Digite um valor:");
    if (leitura_teclas == endMarker) {
      Serial.println("Mensagem recebida:");
      Serial.println(inputBuffer2);
      inputBuffer2[messageIndex] = 0;
      messageIndex = 0;
      parseData2();

      for (int i = 0; i < buffSize; i++) {
        inputBuffer2[i] = '\0';  // Preenche o buffer com caracteres nulos
      }
      lcd.clear();
      Event = 1;
    } else if (leitura_teclas != NO_KEY) {
      if (messageIndex < buffSize) {
        Serial.print(leitura_teclas);
        if (leitura_teclas == '#') {
          inputBuffer2[messageIndex++] = '.';
        } else {
          inputBuffer2[messageIndex++] = leitura_teclas;
        }
      } else {
        // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
        Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
      }
    }
  }


  if (leitura_teclas) {  // Se alguma tecla foi pressionada
    lcd.clear();
    lcd.setCursor(0, 1);
    //Serial.println(leitura_teclas);  // Imprime a tecla pressionada na porta serial
    if (a >= 15) {
      a = 0;
    } else {
      a += 1;
    }
    lcd.print(inputBuffer2);
  }
}

void parseData2() {
  delay(50);
  if (Event == 1) {
    strcpy(messageFromPC, inputBuffer2);
  } else if (Event == 2) {
    intFromPC = atoi(inputBuffer2);
  } else if (Event == 3) {
    floatFromPC = atof(inputBuffer2);
    Serial.println("Dados: ");
    Serial.print("A: ");
    Serial.println(messageFromPC);
    Serial.print("B: ");
    Serial.println(intFromPC);
    Serial.print("C: ");
    Serial.println(floatFromPC);
  }
}