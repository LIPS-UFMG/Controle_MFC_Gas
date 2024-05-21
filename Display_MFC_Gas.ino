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
byte messageIndex = 0;
const byte buffSize = 40;
char inputBuffer2[buffSize];  // Buffer para a entrada do display

char messageFromPC[buffSize] = {0};
int intFromPC = 0;
float floatFromPC = 0.0; // fraction of servo range to move

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

Keypad teclado_personalizado = Keypad(makeKeymap(TECLAS_MATRIZ), PINOS_LINHAS, PINOS_COLUNAS, LINHAS, COLUNAS);  // Inicia teclado

void setup() {
  Serial.begin(9600);  // Inicia porta serial
  lcd.begin(16, 2);
}

void loop() {

  char leitura_teclas = teclado_personalizado.getKey();  // Atribui a variavel a leitura do teclado

  if (leitura_teclas == 'D') {
    Serial.println("Mensagem recebida:");
    Serial.println(inputBuffer2);
    inputBuffer2[messageIndex] = 0;
    messageIndex = 0;
    parseData2();
    for (int i = 0; i < buffSize; i++) {
      inputBuffer2[i] = '\0';  // Preenche o buffer com caracteres nulos
    }
    lcd.clear();
  } else if (leitura_teclas != NO_KEY) {
    if (messageIndex < buffSize) {
      inputBuffer2[messageIndex++] = leitura_teclas;
    } else {
      // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
      Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
    }
  }
  

  if (leitura_teclas) {  // Se alguma tecla foi pressionada
    lcd.setCursor(a, b);
    //Serial.println(leitura_teclas);  // Imprime a tecla pressionada na porta serial
    if (b >= 2) {
      delay(100);
      lcd.clear();
      b = 0;
    } else if (a >= 15) {
      a = 0;
      b += 1;
    } else {
      a += 1;
    }
    lcd.print(leitura_teclas);
  }
}

void parseData2() {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(inputBuffer2,"#");      // get the first part - the string
  strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC
  
  strtokIndx = strtok(NULL, "#"); // this continues where the previous call left off
  intFromPC = atoi(strtokIndx);     // convert this part to an integer
  
  strtokIndx = strtok(NULL, "#"); 
  floatFromPC = atof(strtokIndx);     // convert this part to a float

  Serial.println(messageFromPC);
  Serial.println(intFromPC);
  Serial.println(floatFromPC);

}