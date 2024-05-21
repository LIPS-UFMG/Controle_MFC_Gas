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

const byte buffSize = 40;
char inputBuffer[buffSize];
const char startMarker = '<';
const char endMarker = '>';
byte bytesRecvd = 0;
boolean readInProgress = false;
boolean newDataFromPC = false;

char messageFromPC[buffSize] = {0};
int intFromPC = 0;
float floatFromPC = 0.0; // fraction of servo range to move

int MFC = 0;
boolean SPMFC_update = false;
boolean FluxMaxMFC_update = false;
boolean FatorMFC_update = false;
boolean FatorGas_update = false;
boolean imprimeAjuda = false;

int amostra = 0;
int intervalo = 3000;

//Configurações MFC 1
#define SPFlux1_Pin 3
#define Vopen1_Pin 2
#define Flux1_Pin A0

int SPFlux1_Out = 0;
float Flux_Max1 = 2000;
float SPFlux1 = 0;
float Flux1 = 0;
float Flux1_Val = 0;
float Fator_MFC1 = 1;
float Fator_Gas_MFC1 = 2.00;


//Configurações MFC 2
#define SPFlux2_Pin 4
#define Vopen2_Pin 5
#define Flux2_Pin A1

int SPFlux2_Out = 0;
float Flux_Max2 = 1000;
float SPFlux2 = 0;
float Flux2 = 0;
float Flux2_Val = 0;
float Fator_MFC2 = 1;
float Fator_Gas_MFC2 = 1.13;

/*
//Configurações MFC 3
#define SPFlux3_Pin 3
#define Vopen3_Pin 2
#define Flux3_Pin A0

int SPFlux3_Out = 0;
float Flux_Max3 = 500;
float SPFlux3 = 0;
float Flux3 = 0;
float Flux3_Val = 0;
float Fator_MFC3 = 1;
float Fator_Gas_MFC3 = 1;

//Configurações MFC 4
#define SPFlux4_Pin 3
#define Vopen4_Pin 2
#define Flux4_Pin A0

int SPFlux4_Out = 0;
float Flux_Max4 = 500;
float SPFlux4 = 0;
float Flux4 = 0;
float Flux4_Val = 0;
float Fator_MFC4 = 1;
float Fator_Gas_MFC4 = 1;
*/

//***************************************************************************************************************************
//Rotinas Principais
//***************************************************************************************************************************

void setup() {  
  
  //Inicia Porta Serial
  Serial.begin(9600);

  pinMode(SPFlux1_Pin,OUTPUT);
  pinMode(Vopen1_Pin,OUTPUT);
  
  digitalWrite(SPFlux1_Pin,0);
  digitalWrite(Vopen1_Pin,1);
  
  Flux_Max1 = Flux_Max1*(Fator_MFC1/Fator_Gas_MFC1);

  pinMode(SPFlux2_Pin,OUTPUT);
  pinMode(Vopen2_Pin,OUTPUT);
  
  digitalWrite(SPFlux2_Pin,0);
  digitalWrite(Vopen2_Pin,1);
  
  Flux_Max2 = Flux_Max2*(Fator_MFC2/Fator_Gas_MFC2);

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

  getDataFromPC();
  configIno();
  replyToPC();
  Controle_MFC1();
  Controle_MFC2();
  PrintFluxo();

}


//***************************************************************************************************************************
//Rotinas de Comunicação Serial
//***************************************************************************************************************************

void getDataFromPC() {

    // receive data from PC and save it into inputBuffer
    
  if(Serial.available() > 0) {

    char x = Serial.read();

      // the order of these IF clauses is significant
      
    if (x == endMarker) {
      readInProgress = false;
      newDataFromPC = true;
      inputBuffer[bytesRecvd] = 0;
      parseData();
    }
    
    if(readInProgress) {
      inputBuffer[bytesRecvd] = x;
      bytesRecvd ++;
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
 
void parseData() {

    // split the data into its parts
    
  char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(inputBuffer,",");      // get the first part - the string
  strcpy(messageFromPC, strtokIndx); // copy it to messageFromPC
  
  strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
  intFromPC = atoi(strtokIndx);     // convert this part to an integer
  
  strtokIndx = strtok(NULL, ","); 
  floatFromPC = atof(strtokIndx);     // convert this part to a float

}

//=============

void replyToPC() {

  if (newDataFromPC) {
    newDataFromPC = false;
    
    Serial.println(" ");
    
    if(SPMFC_update){
      SPMFC_update = false;
      Serial.print("SP MFC ");
      Serial.print(MFC);
      Serial.print(" Configurado: ");
      Serial.println(floatFromPC);
      }

    if(FluxMaxMFC_update){
      FluxMaxMFC_update = false;
      Serial.print("Fluxo Maximo MFC Configurado: ");
      Serial.println(floatFromPC);
      }

    if(FatorMFC_update){
      FatorMFC_update = false;
      Serial.print("Fator MFC Configurado: ");
      Serial.println(floatFromPC);
      }

    if(FatorGas_update){
      FatorGas_update = false;
      Serial.print("Fator Gas Configurado: ");
      Serial.println(floatFromPC);
      }

    if(imprimeAjuda){
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

void configIno(){

  if (strcmp(messageFromPC, "sp") == 0){
    MFC = intFromPC;
    SPMFC_update = true;
    }
    else{
      SPMFC_update = false;
    }

   if (strcmp(messageFromPC, "fluxmax") == 0){
    MFC = intFromPC;
    FluxMaxMFC_update = true;
    }
    else{
      FluxMaxMFC_update = false;
    }
    
   if (strcmp(messageFromPC, "fatormfc") == 0){
    MFC = intFromPC;
    FatorMFC_update = true;
    }
    else{
      FatorMFC_update = false;
    }
    
   if (strcmp(messageFromPC, "fatorgas") == 0){
    MFC = intFromPC;
    FatorGas_update = true;
    }
    else{
      FatorGas_update = false;
    }

    configMFC();

   if (strcmp(messageFromPC, "ajuda") == 0){
    imprimeAjuda = true;
   }
   else{
   imprimeAjuda = false;
   } 
}

//=============

void configMFC(){

  switch (MFC){

    case 1:
    if(SPMFC_update){SPFlux1 = floatFromPC*(Fator_MFC1/Fator_Gas_MFC1);} //SPFlux1 - Fluxo Real
    if(FluxMaxMFC_update){Flux_Max1 = floatFromPC;}
    if(FatorMFC_update){Fator_MFC1 = floatFromPC;}
    if(FatorGas_update){Fator_Gas_MFC1 = floatFromPC;}
    break;

    case 2:
    if(SPMFC_update){SPFlux2 = floatFromPC*(Fator_MFC2/Fator_Gas_MFC2);} //SPFlux2 - Fluxo Real
    if(FluxMaxMFC_update){Flux_Max2 = floatFromPC;}
    if(FatorMFC_update){Fator_MFC2 = floatFromPC;}
    if(FatorGas_update){Fator_Gas_MFC2 = floatFromPC;}
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

void PrintFluxo(){
  
  if (amostra >= intervalo){
    amostra = 0;
    Serial.print("SP MFC 1: ");
    Serial.print(SPFlux1);
    Serial.print(" | Fluxo MFC 1: ");
    Serial.println(Flux1*Fator_Gas_MFC1);

    amostra = 0;
    Serial.print("SP MFC 2: ");
    Serial.print(SPFlux2);
    Serial.print(" | Fluxo MFC 2: ");
    Serial.println(Flux2*Fator_Gas_MFC2);  
  }
  else{
    amostra++;
  }
}

//***************************************************************************************************************************
//Rotinas de Controle dos MFC's
//***************************************************************************************************************************

void Controle_MFC1(){

  SPFlux1_Out = (255*SPFlux1)/Flux_Max1;
  analogWrite(SPFlux1_Pin,SPFlux1_Out);
  
  Flux1_Val = analogRead(Flux1_Pin);
  Flux1 = (Flux1_Val*Flux_Max1)/1024;
}

void Controle_MFC2(){

  SPFlux2_Out = (255*SPFlux2)/Flux_Max2;
  analogWrite(SPFlux2_Pin,SPFlux2_Out);
  
  Flux2_Val = analogRead(Flux2_Pin);
  Flux2 = (Flux2_Val*Flux_Max2)/1024;
}
