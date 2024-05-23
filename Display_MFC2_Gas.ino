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
const char backMarker = 'B';
const char endMarker = 'D';
int event = 1;

byte messageIndex = 0;
const byte buffSize = 20;
char inputBuffer2[buffSize];  // Buffer para a entrada do display

char messageFromPC[buffSize] = { 0 };
int intFromPC = 0;
float floatFromPC = 0.0;  // fraction of servo range to move


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

  lcd.setCursor(0, 0);
  if (event == mainMenu) {  //imprime mensagem na tela
    lcd.print("Escolha: 1 2 3 4");
  } else if (event == selectMFC) {
    lcd.println("1-MFC1 2-MFC2");
  } else if (event == insertData) {
    lcd.println("Digite um valor:");
  }
  lcd.setCursor(0, 1);
  lcd.print(inputBuffer2);

  char leitura_teclas = teclado_personalizado.getKey();  // Atribui a variavel a leitura do teclado

  if (leitura_teclas == endMarker) {  // se dado foi inserido
    bool validData = false;           // variavel para indicar se dado inserido é valido
    Serial.println("Mensagem recebida:");
    Serial.println(inputBuffer2);
    messageIndex = 0;
    int input = atof(inputBuffer2);  // converte entrada para numero para comparar

    if (event == mainMenu) {  // confere parametros
      if (input == 1 || input == 2 || input == 3 || input == 4) {
        validData = true;
      }
    } else if (event == selectMFC) {
      if (input == 1 || input == 2) {
        validData = true;
      }

    } else if (event == insertData) {
      validData = true;
    }

    if (validData) {  // se dado for válido chama parse 2 e passa pro proximo evento
      parseData2();
      if (event == 3) {
        event = 0;
      }
      event++;

    } else {  // se dado invalido limpa buffer

      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Numero invalido");
      delay(1000);
    }
    lcd.clear();

    for (int i = 0; i < buffSize; i++) {  //limpa buffer
      inputBuffer2[i] = '\0';             // Preenche o buffer com caracteres nulos
    }

  } else if (leitura_teclas != NO_KEY) {  //processo para ler teclas
    if (messageIndex < buffSize) {        //caso mensagem seja menor que limite do buffer
      //Serial.print(leitura_teclas);
      switch (leitura_teclas) {
        case '#':
          inputBuffer2[messageIndex] = '.';
          messageIndex++;
          break;
        case 'B':
          for (int i = 0; i < buffSize; i++) {
            inputBuffer2[i] = '\0';  // Preenche o buffer com caracteres nulos
          }
          messageIndex = 0;
          event--;
          break;
        case 'C':
          messageIndex--;
          inputBuffer2[messageIndex] = '\0';
          break;
        default:
          inputBuffer2[messageIndex++] = leitura_teclas;
          break;
      }
      lcd.clear();
    } else {
      // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
      Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
    }
  }
}

void parseData2() {
  delay(50);
  if (event == 1) {
    strcpy(messageFromPC, inputBuffer2);
  } else if (event == 2) {
    intFromPC = atoi(inputBuffer2);
  } else if (event == 3) {
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




// void clearBuffer(**inputBuffer) {
// }

// void receiveData(int e, char *inputBuffer, char p1, char p2, char p3, char p4) {


//   }
