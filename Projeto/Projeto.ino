#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <TimerOne.h>

bool flagGoertzel = false;
bool flagSinal = false;
byte vetor[150];
byte indiceAtual = 0;
char tecla = '0';
bool senhaCorreta = false;
byte estado = 0;
int tempo10 = 0;
bool alarme = false;


// Portas
char portaSinal = 'A0';
char portaTemperatura = 'A1';

byte portaRele = 13;
byte sirene = 8;

//chaves

byte chaveJanela = 9;
byte chavePorta = 10;

// =============================== TECLADO
char teclado[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

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

// Define as tarefas
void TaskPrincipal(void *pvParameters);
void TaskGoertzel(void *pvParameters);
void TaskLerChave(void *pvParameters);

// Especifica o tipo para o semáforo da interface serial
SemaphoreHandle_t mtxSerial;

void setup() {
  pinMode(sirene, OUTPUT);
  pinMode(portaRele, OUTPUT);
  
  // Interrupções do Timer 1 a cada 278 useg -> T/2 de 1,8KHz
  Timer1.initialize(125);
  Timer1.attachInterrupt(LerSinalDTMF);

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

  digitalWrite(sirene, HIGH);
}

// Deixa vazio
void loop() {
}

void TaskPrincipal(void *pvParameters) {
  (void) pvParameters;

  for (;;)
  {
    if(tecla == '0') {
      if(estado != 0) {
        tempo10++;
  
        if(tempo10 == 50) {
          estado = 0;
          tempo10 = 0;
        }
      }
    } else {
      tempo10 = 0;
      if(estado == 0) {
        estado = tecla == '#' ? 1 : 4;
      } else if(estado == 1) {
        estado = tecla == '4' ? 2 : 4;
      } else if(estado == 2) {
        estado = tecla == '2' ? 3 : 4;
      } else {
        if(estado == 3) {
          if (xSemaphoreTake(mtxSerial,(TickType_t)5) == pdTRUE) {
            switch(tecla) {
              case '2':
                Serial.println('LAMPADA LIGADA');
                break;
              case '3':
                // Desligar lâmpada
                Serial.println('LAMPADA DESLIGADA');
                break;
              case '4':
                // Acionar o led
                Serial.println('RELÉ LIGADO');
                digitalWrite(portaRele,HIGH);
                break;
              case '5':
                // Desligar o led
                Serial.println('RELÉ DESLIGADO');
                digitalWrite(portaRele,LOW);
                break;
              case '6':
                // Armar alarme
                Serial.println('ALARME LIGADO');
                alarme = true;
                break;
              case '7':
                // Desarmar alarme
                Serial.println('ALARME DESLIGADO');
                alarme = false;
                digitalWrite(sirene,HIGH);
                break;
              case '8':
                digitalWrite(sirene,LOW);
                Serial.println('SIRENE LIGADA');
                break;
              case '9':
                //Desligar sirene
                digitalWrite(sirene,HIGH);
                Serial.println('SIRENE DESLIGADA');
                break;
              default:
                break; 
            }
            xSemaphoreGive(mtxSerial); // Após utilizar, libera semáforo
          }
        }
      }
    }
 
    tecla = '0';
    vTaskDelay(200/portTICK_PERIOD_MS); // Tarefa instanciada a cada 200 mseg
  }
}


void TaskGoertzel(void *pvParameters) {
  (void) pvParameters;

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

      // Pegar os maiores
      long listaLinha[4] = {p1, p2, p3, p4};
      long listaColuna[3] = {p5, p6, p7};
      
      long maiorLinhaIndice = 0;
      byte i;
      for(i = 1; i < 4; i++) {
        if(listaLinha[i] > listaLinha[maiorLinhaIndice])
          maiorLinhaIndice = i;
      }

      long maiorColunaIndice = 0;
      for(i = 1; i < 3; i++) {
        if(listaColuna[i] > listaColuna[maiorColunaIndice])
          maiorColunaIndice = i;
      }

      tecla = teclado[maiorLinhaIndice][maiorColunaIndice];
      flagGoertzel = false;
    }
    
    vTaskDelay(200/portTICK_PERIOD_MS); // Tarefa instanciada a cada 200 mseg
  }
}

void TaskLerChave(void *pvParameters) {
  (void) pvParameters;

  int temperatura;
  bool sinalChaveJanela;
  bool sinalChavePorta;
  byte contador = 1;
  
  for (;;)
  {
    sinalChaveJanela = (digitalRead(chaveJanela) == 0) ;
    sinalChavePorta = (digitalRead(chavePorta) == 0);

    if(contador == 8) {
      temperatura = (analogRead(chavePorta)/16)-16;
      contador = 1;
    }    

    if((sinalChaveJanela || sinalChavePorta) && alarme) {
      digitalWrite(sirene,LOW);
    }

    if (xSemaphoreTake(mtxSerial,(TickType_t)5) == pdTRUE) {
      Serial.print("Temperatura: ");
      Serial.println(temperatura);
      xSemaphoreGive(mtxSerial); // Após utilizar, libera semáforo
    }

    contador++;
    
    vTaskDelay(150/portTICK_PERIOD_MS); // Tarefa instanciada a cada 200 mseg
  }
}

// Tarefa de leitura do sinal DTMF
void LerSinalDTMF() {
  byte sinal = (byte) (analogRead(portaSinal) / 4);

  if(flagSinal) {
    if(indiceAtual < 150) {
      vetor[indiceAtual] = sinal;
      indiceAtual++;

      if(indiceAtual == 150)
        flagGoertzel = true;
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

