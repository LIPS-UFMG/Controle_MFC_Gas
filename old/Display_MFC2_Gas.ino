/***************************

***************************/

#include <Keypad.h>         // Biblioteca do codigo
#include <LiquidCrystal.h>  // Biblioteca do display

const byte LINHAS = 4;   // Linhas do teclado
const byte COLUNAS = 4;  // Colunas do teclado

//-----------------------------------------PINAGEM----------------------------------------//

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);             // Pinos do lcd
byte PINOS_LINHAS[LINHAS] = { A0, A1, 7, 6 };      // Pinos de conexao com as linhas do teclado
byte PINOS_COLUNAS[COLUNAS] = { A2, A3, A4, A5 };  // Pinos de conexao com as colunas do teclado

const char TECLAS_MATRIZ[LINHAS][COLUNAS] = {  // Matriz de caracteres (mapeamento do teclado)
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

Keypad teclado_personalizado =
  Keypad(makeKeymap(TECLAS_MATRIZ), PINOS_LINHAS, PINOS_COLUNAS, LINHAS, COLUNAS);  // Inicia teclado

//----------------------------------------VARIÁVEIS---------------------------------------//

int event = 1;                                                                  // Indice para telas de interação, comecando na primeira
const int mainMenu = 1, selectMFC = 2, insertData = 3;                          // Telas de interação
const char comma = '#', backMarker = 'B', clearMarker = 'C', doneMarker = 'D';  // Teclas especiais

const byte buffSize = 20;     // Tamanho máximo de inserçao
char inputBuffer2[buffSize];  // Buffer para a entrada do display
byte messageIndex = 0;        // Indice para manipular array

char messageFromPC[buffSize] = { 0 };  // Variável para configuracao(?)
int intFromPC = 0;                     // MFC selecionada
float floatFromPC = 0.0;               // fraction of servo range to move(?)

void setup() {
  Serial.begin(9600);  // Inicia porta serial
  lcd.begin(16, 2);    // Inicializa lcd
}

void loop() {
  printToLcd();           // Impressão de instruções e valores inseridos
  getDataFromKeyboard();  // Pega informações do teclado
  delay(50);
}


void getDataFromKeyboard() {
  lcd.setCursor(0, 0);
  char leitura_teclas = teclado_personalizado.getKey();  // Atribui a variavel a leitura do teclado

  if (leitura_teclas != NO_KEY) {   // Caso tecla não nula
    if (messageIndex < buffSize) {  // Impede que a mensagem ultrapasse o limite
      switch (leitura_teclas) {
        case (comma):  // Tecla #, substituida por ponto
          inputBuffer2[messageIndex] = '.';
          messageIndex++;
          break;

        case (backMarker):  // Tecla B, volta para tela anterior
          for (int i = 0; i < buffSize; i++) {
            inputBuffer2[i] = '\0';  // Preenche o buffer com caracteres nulos
          }
          messageIndex = 0;
          event--;
          break;

        case (clearMarker):                                // Tecla C, apaga último caractere
          if (messageIndex != "\0" && messageIndex > 0) {  // Caso array não nulo
            messageIndex--;
            inputBuffer2[messageIndex] = '\0';
          }
          break;

        case (doneMarker):  // Tecla D, submete valor inserido
          Serial.println("Mensagem recebida:");
          Serial.println(inputBuffer2);

          messageIndex = 0;                // Zera indice do array
          int input = atoi(inputBuffer2);  // Converte entrada em string para double
          Serial.println("input");
          Serial.println(input);
          if (validData(input)) {  // Se dado inserido for válido
            parseData2();          // Envia dados
            if (event == 3) {      // Caso tenha terminado o processo
              event = 0;
              lcd.clear();
              lcd.print("Confirmado");
              delay(1000);
            }
            event++;  // Passa para próxima tela

          } else {  // Se dado invalido
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("Numero invalido");
            delay(1000);
          }
          lcd.clear();

          for (int i = 0; i < buffSize; i++) {  // Limpa buffer
            inputBuffer2[i] = '\0';             // Preenche o buffer com caracteres nulos
          }
          break;
        default:
          inputBuffer2[messageIndex++] = leitura_teclas;
      }
      lcd.clear();
    } else {
      // Caso contrário, mensagem cheia, não podemos adicionar mais caracteres
      Serial.println("Buffer cheio! Não é possível adicionar mais caracteres.");
    }
  }
}

void printToLcd() {
  lcd.setCursor(0, 1);
  lcd.print(inputBuffer2);
  lcd.setCursor(0, 0);
  switch (event) {  // Printa mensagem correspondente a tela
    case mainMenu:
      lcd.print("Escolha: 1 2 3 4");
      break;
    case selectMFC:
      lcd.println("1-MFC1 2-MFC2");
      break;
    case insertData:
      lcd.println("Digite um valor:");
      break;
  }
}

bool validData(int input) {  // Realiza validação dos dados para cada tela

  bool ret = false;

  switch (event) {
    case mainMenu:
      ret = (input == 1 || input == 2 || input == 3 || input == 4);
      break;
    case selectMFC:
      ret = (input == 1 || input == 2);
      break;
    case insertData:
      ret = true;
      break;
  }

  return ret;
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
