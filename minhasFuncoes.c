#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TAM 100

//int n = 0; // quantidade de programas lidos

// FUNCOES DE FILA
//OBS: a fila armazena apenas o PID do processo para facilitar a implementaçao, os dados do processo estao no vetor programData
typedef struct fila{
  int n, ini;
  int vet[MAX_TAM];
}TF;

TF *TF_inicializa (void){
  TF *f = (TF *) malloc(sizeof (TF));
  f->n = f->ini = 0;
  return f;
}

int incr (int ind, int n){
  return (ind + 1) % n;
}

int TF_vazia (TF *f){
  return f->n == 0;
}

int TF_retira (TF *f){
  if(TF_vazia(f)) exit(1);
  int resp = f->vet[f->ini];
  f->ini = incr(f->ini, MAX_TAM);
  f->n--;
  return resp;
}

void TF_insere (TF *f, int x){
  if(f->n == MAX_TAM) exit(1);
  int fim = (f->ini + f->n++) % MAX_TAM;
  f->vet[fim] = x;
}
///FIM DA FILA

//FUNCOES DO PROCESSO
//struct do processo
struct p{
  int adm; // tempo de admissao
  char pNome[10]; //nome do programa
  int pid; //Process ID
  int prioridade;
  int burst[MAX_TAM], IO[MAX_TAM]; //vetor com tempos de CPU burst e tempos de I/O
  int quantB, quantI; //quantidade de itens no vetor de burst(quantB) e itens no vetor de I/O(quantI)
  int iB, iI; //indice do vetor de burst e IO
  int status; //se 1-> Burst, se 0 ->IO
  int terminado;
  int primeiraExec;
  int tempoInicial;
  int tempoFinal;
}typedef PD;

//Vetor de dados dos programas
struct pV{
  PD *proc;
}typedef PDVET;

//Ler arquivo de entrada
PD *lerEntradas(FILE *f, int x){
  char linha[200];
  PD *aux = (PD *) malloc(sizeof(PD));

  if (fgets (linha, 200, f) == NULL ) {// ler linha da entrada, se for vazia retona nulo
    free(aux);
    return NULL;
  }

  char *ptr;
  int tmp;
  aux->pid = x; //processos recebem um PID na ordem que foram lidos (0,1,2,...)
  int qI = 0;
  int qB = 0;
  
  //obter tempo de admissao
  ptr = strtok(linha, " ");
  tmp = atoi(ptr);
  aux->adm = tmp;
  //obter nome do programa
  ptr = strtok(NULL, " ");
  strcpy(aux->pNome, ptr);
  //obter prioridade
  ptr = strtok(NULL, " ");
  tmp = atoi(ptr);
  aux->prioridade = tmp;

  //loop para obter tempos de burst CPU e I/O
  int i = 0;
  while(ptr){
	ptr = strtok(NULL, " ");
	tmp = atoi(ptr);
	if(i % 2){ //se i for impar é tempo de IO
	  // colocar tempo no vetor de IO
	  aux->IO[qI] = tmp;
	  if (tmp > 0) qI++;
	}else{ // senao é tempo de burst
	  //colocar no vetor de burst
	  aux->burst[qB] = tmp;
	  if (tmp > 0) qB++;
	}
	i++;
  }
  
  aux->quantB = qB;
  aux->quantI = qI;
  aux->iB = 0;
  aux->iI = 0;
  aux->status = 1;
  aux->terminado = 0;
  aux->primeiraExec = 0;
  
  //retorna os dados de 1 processo
  return aux;
}

//procura no vetor de programas quem precisa ser admitido pra fila de prontos no tempo atual
void admissao(PDVET *v, int x ,int t, TF *pronto){
	for (int i = 0; i < x; i++){
		if(v[i].proc->adm == t){ //se tempo de admissao for igual ao tempo atual, colocar na fila prontos
			TF_insere(pronto, v[i].proc->pid);
      //printf("Processo PID %d admitido\n", v[i].proc->pid);
		}
	}	
}

//Consumir 1 u.t. de I/O, do processo que estiver como primeiro da fila
void verificarFilaEspera(TF *espera, TF *pronto, PDVET *v){
    //atual a fila fazer IO
    int esperaAtual;  
    esperaAtual = espera->vet[espera->ini]; //obtem indice do primeiro da fila
    PD *aux;
    aux = v[esperaAtual].proc;
    if(aux->IO[aux->iI] == 0){ //se acabar o tempo de I/O colocar na fila de prontos
      int tmp = TF_retira(espera);
      TF_insere(pronto,tmp);
      aux->iI += 1;
    }else{ // se tiver tempo de I/O reduzir 1 u.t.
      aux->IO[aux->iI] -= 1;
    }
    v[esperaAtual].proc = aux;
}

//Imprime e grava no arquivo o andamento do simulador
void escalonadorLogger(TF *prontos, TF *espera, PDVET *v, int TEMPO, FILE *out, int QUANTUM, int exec){
  PD *aux;
  if(QUANTUM > 0){
    fprintf(out, "Tempo Corrente: %d, QUANTUM = %d\n", TEMPO, QUANTUM);
    printf("Tempo Corrente: %d, QUANTUM = %d\n", TEMPO, QUANTUM);
  }else{
    fprintf(out, "Tempo Corrente: %d\n", TEMPO);
    printf("Tempo Corrente: %d\n", TEMPO);
  }
  if(exec != -1){
    aux = v[exec].proc;
    fprintf(out,"Processo em Execucao - PID: %d, Nome: %s, Prioridade: %d, Tempo Restante: %d\n",aux->pid, aux->pNome, aux->prioridade, aux->burst[aux->iB]);
    printf("Processo em Execucao - PID: %d, Nome: %s, Prioridade: %d, Tempo Restante: %d\n",aux->pid, aux->pNome, aux->prioridade, aux->burst[aux->iB]);
  }else{
    fprintf(out,"Processo em Execucao - OCIOSO\n");
    printf("Processo em Execucao - OCIOSO\n");
  }
  fprintf(out,"Fila de Prontos\n");
  printf("Fila de Prontos\n");
  if(!TF_vazia(prontos)){
    //percorrer fila
    int ind = prontos->ini, i;
    for(i = 0; i < prontos->n; i++){
      aux = v[prontos->vet[ind]].proc;
      fprintf(out, "PID: %d, Nome: %s, Tempo Admissao: %d, Prioridade: %d\n", aux->pid, aux->pNome, aux->adm ,aux->prioridade);
      printf("PID: %d, Nome: %s, Tempo Admissao: %d, Prioridade: %d\n", aux->pid, aux->pNome, aux->adm ,aux->prioridade);
      ind = incr(ind, MAX_TAM);
    }
  }
  fprintf(out,"Fila de Espera\n");
  printf("Fila de Espera\n");
  if(!TF_vazia(espera)){
    //percorrer fila
    int ind = espera->ini, i;
    for(i = 0; i < espera->n; i++){
      aux = v[espera->vet[ind]].proc;
      fprintf(out, "PID: %d, Nome: %s, Tempo Admissao: %d, Prioridade: %d, Tempo Restante IO: %d\n", aux->pid, aux->pNome, aux->adm ,aux->prioridade, aux->IO[aux->iI]);
      printf("PID: %d, Nome: %s, Tempo Admissao: %d, Prioridade: %d, Tempo Restante IO: %d\n", aux->pid, aux->pNome, aux->adm ,aux->prioridade, aux->IO[aux->iI]);
      ind = incr(ind, MAX_TAM);
    }
  }

  fprintf(out, "\n-----------------------------------------------------------------------------------------\n");
  printf("\n-----------------------------------------------------------------------------------------\n");

  fflush(out);

}

/*
void tempoMedioTurnArround(PDVET *v, int x){
  printf("\nProg   | t.espera | TA   \n");
  printf("----------------------\n");
  int soma = 0;
  for (size_t i = 0; i < x; i++){
    int espe, TA;
    TA = v[i].proc->tempoFinal - v[i].proc->tempoInicial;
    espe = TA - v[i].proc->adm;
    printf("%s|%d        |%d\n", v[i].proc->pNome, espe, TA);
    soma += espe;
  }
  printf("tempo medio dos programas: %f",soma/(1.0*x));
}
*/