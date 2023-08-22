/*
28/06/23
ARDUINO MEGA
TIPO DE CONTROLADOR APLICADO: PI
FILTRO DE MÉDIA MÓVEL

*/

// BIBLIOTECAS
#include <avr/io.h>
// #include <avr/interrupt.h>

//PINOS
#define pino 7
#define botao 1
#define pinoencoderA 21
#define pinoencoderB 2

//VARIAVEIS ENCODER A
const float PERIODO_AMOSTRAGEM = 0.01; //segundos  //usar de 20 a 50 ms
const float PULSOS_VOLTA_ENCODER = 1000; //CONFORME DATASHEET DO ENCODER UTILIZADO
const float COEFICIENTE_RPM = 60 / (PERIODO_AMOSTRAGEM * PULSOS_VOLTA_ENCODER);
float RPM_ATUAL_A = 0;
long PULSOS_ENCODER_A = 0;

float RPM_ATUAL_B = 0;
long PULSOS_ENCODER_B = 0;

//PWM
float tensao = 0;
float pwm = 0;
double cont;           //conta ate onde da
double valor = 0;

//CONTROLE
float u = 0;
float u_1 = 0;
//float u_2 = 0;
float e = 0;
float e_1 = 0;
//float e_2 = 0;
float r = 0;          //referencia em RPM

//filtro encoder
float y = 0;
float y_1 = 0; //buffer leitura anterior encoder
float y_filtro = 0; //saída do filtro encoder
float y_filtro_1 = 0; //buffer leitura anterior filtro encoder

//média móvel ENCODER A
const int numLeituras = 5; //número de leituras da média móvel
int leituras_A [numLeituras]; //vetor do tamanho do número de leituras da média móvel
int readIndex_A = 0;
long total_A = 0;

//média móvel ENCODER B
const int numLeituras_B = 5; //número de leituras da média móvel
int leituras_B [numLeituras]; //vetor do tamanho do número de leituras da média móvel
int readIndex_B = 0;
long total_B = 0;

//FUNCOES
ISR(INT0_vect)        //obrigatoriamente a interrupção tem que ser no pino 2
{
   PULSOS_ENCODER_A++;
}

ISR(INT4_vect)        //obrigatoriamente a interrupção tem que ser no pino 2
{
   PULSOS_ENCODER_B++;
}

//LEITURA SERIAL MONITOR
void SerialMonitor() {
  if(Serial.available() > 0){
    String recebe = Serial.readStringUntil('\n');
    r = recebe.toInt();
  }
}

long mMovel_A(){ //FUNÇÃO FILTRO DE MÉDIA MÓVEL PARA LEITURA DO ENCODER
  long media_A;
  total_A = total_A - leituras_A[readIndex_A]; //subtrai a última leitura
  leituras_A[readIndex_A] = RPM_ATUAL_A; //lê o sensor
  total_A = total_A + leituras_A[readIndex_A];
  readIndex_A = readIndex_A + 1;
  if (readIndex_A >= numLeituras){
    readIndex_A = 0;
  }
  media_A = total_A/numLeituras;

  return media_A;
}

long mMovel_B(){ //FUNÇÃO FILTRO DE MÉDIA MÓVEL PARA LEITURA DO ENCODER
  long media_B;
  total_B = total_B - leituras_B[readIndex_B]; //subtrai a última leitura
  leituras_B[readIndex_B] = RPM_ATUAL_B; //lê o sensor
  total_B = total_B + leituras_B[readIndex_B];
  readIndex_B = readIndex_B + 1;
  if (readIndex_B >= numLeituras){
    readIndex_B = 0;
  }
  media_B = total_B/numLeituras;

  return media_B;
}

void setup() { 
  Serial.begin(9600); //também pode ser 115200
  cli(); //desabilita a interrupcao global
  pinMode (pino, OUTPUT);
  pinMode (botao, INPUT);
  pinMode (pinoencoderA, INPUT_PULLUP);
  pinMode (pinoencoderB, INPUT_PULLUP);
  pinMode (13, OUTPUT);
  digitalWrite(13, LOW);

 
///////INTERRUPT INT0///////////
   EICRA |= (1<<ISC01)|(1<<ISC00);  //borda de subida
   EIMSK |= (1<<INT0);              //habilita INT0
   EIMSK |= (1<<INT4);              //habilita INT0

   //Para o PWM
   DDRD |=(1<<PD6);
   sei();                           //habilita interrupcao global
   pwm = ((tensao*255)/5);
   analogWrite(pino, 0);
}



void loop() {
  
  y = mMovel_A();
  r = mMovel_B();
  //y = RPM_ATUAL_A;
  //r = RPM_ATUAL_B;

  //----------CONTROLADOR----------
  
  //FILTRO DE MÉDIA MÓVEL
  /*
  y_filtro = (0.003571*y)+(0.003571*y_1)+(0.4286*y_filtro_1);
  y_filtro_1 = y_filtro;  //Buffer
  y_1 = y;                //Buffer
  y_filtro = y;
  */
  
  e = r - y_filtro;
  u = (0.0424*e)-(0.0334*e_1)+u_1;

  if (r < 6){
    u = 0;
  }

  if (u > 5){
    u = 5;
  }

  if (u < 0){
    u = 0;
  }

  //u_2 = u_1;  //buffer
  u_1 = u;    //buffer
  //e_2 = e_1;  //buffer
  e_1 = e;    //buffer

  pwm = ((u*255)/5);
  analogWrite(pino, pwm);

  
  SerialMonitor(); //Função criada - verifica se tem dado na Serial

  RPM_ATUAL_A = COEFICIENTE_RPM * PULSOS_ENCODER_A;
  RPM_ATUAL_B = (COEFICIENTE_RPM * PULSOS_ENCODER_B)/100; //por causa da redução
  Serial.print(RPM_ATUAL_A);
  Serial.print(", ");
  Serial.print(RPM_ATUAL_B);
  Serial.print(", ");
  Serial.println(pwm);
  PULSOS_ENCODER_A = 0;
  PULSOS_ENCODER_B = 0;
  delay(10);
}
