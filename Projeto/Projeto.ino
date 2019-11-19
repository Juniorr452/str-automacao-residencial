#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <TimerOne.h>

bool flagGoertzel = false;
bool flagSinal = false;
byte vetor[150];
byte indiceAtual = 0;
byte tecla = 0;
bool senhaCorreta = false;

// Portas
char portaSinal = A0;

// =============================== GOERTZEL


// Define as tarefas
void TaskPrincipal(void *pvParameters);
void TaskGoertzel(void *pvParameters);
void TaskLerChave(void *pvParameters);

// Especifica o tipo para o semáforo da interface serial
SemaphoreHandle_t mtxSerial;

void setup() {
  // Interrupções do Timer 1 a cada 278 useg -> T/2 de 1,8KHz
  Timer1.initialize(125);
  Timer1.attachInterrupt(LerSinal);
  // Interrupção externa da Chave1, rotina AcessoCPD, borda de descida
  attachInterrupt(digitalPinToInterrupt(Chave1),AcessoCPD,FALLING);

  // Cria as tarefas do sistema
  xTaskCreate(TaskPrincipal,(const portCHAR *) "",128,NULL,1,NULL); // Máquina de estado (SWITCH CASE)
  xTaskCreate(TaskLerChave,(const portCHAR *) "",128,NULL,2,NULL); // Menor prioridade
  xTaskCreate(TaskGoertzel,(const portCHAR *) "",256,NULL,3,NULL); // Algoritmo de Goertzel

  // Cria mutex para controlar acesso à interface serial
  if (mtxSerial == NULL) {
    mtxSerial = xSemaphoreCreateMutex(); // Cria o semáforo
    if (mtxSerial != NULL) {
      xSemaphoreGive(mtxSerial); // Semáforo livre
    }
  }
}

// Deixa vazio
void loop() {
}

void TaskPrincipal(void *pvParameters) {
  (void) pvParameters;

  for (;;)
  {
    if(senhaCorreta) { // Senha correta
      switch(tecla) {
        case 2:
          // Ligar lâmpada
          break;W
        case 3:
          // Desligar lâmpada
          break;
        case 4:
          // Acionar o relé
          break;
        case 5:
          // Desligar o relé
          break;
        case 6:
          // Armar alarme
          break;
        case 7:
          // Desarmar alarme
          break;
        case 8:
          // Ligar sirene
          break;
        case 9:
          //Desligar sirene
          break;
        default:
          break; 
      }
    }
    
    vTaskDelay(200/portTICK_PERIOD_MS); // Tarefa instanciada a cada 200 mseg
  }
}


void TaskGoertzel(void *pvParameters) {
  (void) pvParameters;

  // Coeficientes do Algoritmo de Goertzel para cada uma das
  // 7 frequências do sinal DTMF, com uma
  // frequência de amostragem fs = 8000Hz
  byte c1 = 219; // 2*cos(2*pi*697/fs)*128
  byte c2 = 211; // 2*cos(2*pi*770/fs)*128
  byte c3 = 201; // 2*cos(2*pi*852/fs)*128
  byte c4 = 189; // 2*cos(2*pi*941/fs)*128
  byte c5 = 149; // 2*cos(2*pi*1209/fs)*128
  byte c6 = 128; // 2*cos(2*pi*1336/fs)*128
  byte c7 = 102; // 2*cos(2*pi*1477/fs)*128
  
  // Variáveis das iterações
  int v10; //v(i) p/ f1 = 697Hz
  int v11; //v(i-1) p/ f1 = 697Hz
  int v12; //v(i-2) p/ f1 = 697Hz
  
  int v20; //v(i) p/ f2 = 770Hz
  int v21; //v(i-1) p/ f2 = 770Hz
  int v22; //v(i-2) p/ f2 = 770Hz

  int v30; //v(i) p/ f3 = 852Hz
  int v31; //v(i-1) p/ f3 = 852Hz
  int v32; //v(i-2) p/ f3 = 852Hz
  
  int v40; //v(i) p/ f4 = 941Hz
  int v41; //v(i-1) p/ f4 = 941Hz
  int v42; //v(i-2) p/ f4 = 941Hz

  int v50; //v(i) p/ f5 = 1209Hz
  int v51; //v(i-1) p/ f5 = 1209Hz
  int v52; //v(i-2) p/ f5 = 1209Hz

  int v60; //v(i) p/ f6 = 1336Hz
  int v61; //v(i-1) p/ f6 = 1336Hz
  int v62; //v(i-2) p/ f6 = 1336Hz
  
  int v70; //v(i) p/ f7 = 1477Hz
  int v71; //v(i-1) p/ f7 = 1477Hz
  int v72; //v(i-2) p/ f7 = 1477Hz

  // Variáveis auxiliares para as iterações
  byte i;
  int j;
  long k;
  long l;
  long m;

  for (;;)
  {
    if(flagGoertzel) {
      v11 = 0;
      v12 = 0;
      v21 = 0;
      v22 = 0;
      v31 = 0;
      v32 = 0;
      v41 = 0;
      v42 = 0;
      v51 = 0;
      v52 = 0;
      v61 = 0;
      v62 = 0;
      v71 = 0;
      v72 = 0;

      // Etapa 1
      for(i=0; i < 150; i++) {
        j = int(vetor[i]) - 128;

        // Cálculos para f1
        k = long(c1)*long(v11)/128;
        v10 = j + int(k) - v12;
        v12 = v11;
        v11 = v10;

        // Cálculos para f2
        k = long(c2)*long(v21)/128;
        v20 = j + int(k) - v22;
        v22 = v21;
        v21 = v20;

        // Cálculos para f3
        k = long(c3)*long(v31)/128;
        v30 = j + int(k) - v32;
        v32 = v31;
        v31 = v30;

        // Cálculos para f4
        k = long(c4)*long(v41)/128;
        v40 = j + int(k) - v42;
        v42 = v41;
        v41 = v40;

        // Cálculos para f5
        k = long(c5)*long(v51)/128;
        v50 = j + int(k) - v52;
        v52 = v51;
        v51 = v50;

        // Cálculos para f6
        k = long(c6)*long(v61)/128;
        v60 = j + int(k) - v62;
        v62 = v61;
        v61 = v60;

        // Cálculos para f7
        k = long(c7)*long(v71)/128;
        v70 = j + int(k) - v72;
        v72 = v71;
        v71 = v70;
      }

      // Etapa 2
      // Energia f1
      k = long(v11);
      l = long(v12);
      m = long(c1)*k/128;
      long p1 = k*k + l*l - m*l;

      // Energia f2
      k = long(v21);
      l = long(v22);
      m = long(c2)*k/128;
      long p2 = k*k + l*l - m*l;

      // Energia f3
      k = long(v31);
      l = long(v32);
      m = long(c3)*k/128;
      long p3 = k*k + l*l - m*l;

      // Energia f4
      k = long(v41);
      l = long(v42);
      m = long(c4)*k/128;
      long p4 = k*k + l*l - m*l;

      // Energia f5
      k = long(v51);
      l = long(v52);
      m = long(c5)*k/128;
      long p5 = k*k + l*l - m*l;

      // Energia f6
      k = long(v61);
      l = long(v62);
      m = long(c6)*k/128;
      long p6 = k*k + l*l - m*l;

      // Energia f7
      k = long(v71);
      l = long(v72);
      m = long(c7)*k/128;
      long p7 = k*k + l*l - m*l;
    }
    vTaskDelay(200/portTICK_PERIOD_MS); // Tarefa instanciada a cada 200 mseg
  }
}


void TaskLerChave(void *pvParameters) {
  (void) pvParameters;

  for (;;)
  {
    vTaskDelay(200/portTICK_PERIOD_MS); // Tarefa instanciada a cada 200 mseg
  }
}


void LerSinal() {
  int sinal = analogRead(portaSinal);

  if(flagSinal) {
    if(vetor[indiceAtual] < 150) {
      vetor[indiceAtual] = sinal;
      indiceAtual++;
    } else {
      if(!flagGoertzel) {
        flagGoertzel = true;
      }

      flagSinal = sinal >= 5;
    }
  } else if(sinal >= 128) {
    flagSinal = true;
    indiceAtual = 0;
  }
}

