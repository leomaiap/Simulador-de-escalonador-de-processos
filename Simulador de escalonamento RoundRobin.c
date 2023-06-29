//Leonardo Maia Pereira - Grupo 15
#include <stdio.h>
#include <stdlib.h>
#include "minhasFuncoes.c" //impotar bibliotecas das funcoes usadas

int TEMPO = 0;
int QUANTUM = 10;
char in[] = "processos.in"; //Nome arquivo de entrada
char out[] = "processosRR.out"; //Nome arquivo de saida

int main(int argc, char const *argv[]){
    FILE *IN = fopen(in, "r");
    PDVET *programData = (PDVET*) malloc(sizeof(PDVET)*MAX_TAM);//Vetor onde armazena os dados dos programas lidos do arquivo
    PD *tmp;
    int qProg = 0;
    int qTerminados = 0;
    //ler arquivo e por no vetor programas
    while(1){
        tmp = lerEntradas(IN, qProg);
        if(tmp == NULL) break;
        programData[qProg].proc = tmp;
        qProg++;
    }
    fclose(IN);

    //Criar arquivo de saida
    FILE *OUT = fopen(out, "w");

    //simular escalonador Round_Robin
    TF *prontos = TF_inicializa(); //fila de prontos
    TF *espera = TF_inicializa(); //fila de espera
    int emExec = -1; //processo atual em execução, -1 == a nenhum processo executando (ocioso)
    int Q; //armazena o quantum atual da execucao
    int troca = 0; //flag para saber se precisa trocar o processo executando pelo proxima da fila, quando o quantum chega a zero

    while(qTerminados != qProg){
        admissao(programData, qProg, TEMPO, prontos); // se programa tiver no tempo de admissao é posto na fila
        if(emExec == -1){ //se nenhum processo estiver executando, consumir o primeiro fila de prontos
            if(!TF_vazia(prontos)){
                emExec = TF_retira(prontos);
                Q = QUANTUM;
                if(programData[emExec].proc->primeiraExec == 0){ 
                    programData[emExec].proc->tempoInicial = TEMPO;
                    programData[emExec].proc->primeiraExec = 1;
                }
            }
        }else{ // se processo corrente tiver executando, executar-lo
            if(programData[emExec].proc->burst[programData[emExec].proc->iB] > 0){ //se ainda tiver tempo de burst
                if(Q > 0){//se quantum >= 0 executa...
                    programData[emExec].proc->burst[programData[emExec].proc->iB] -= 1;
                }else{ 
                    TF_insere(prontos, emExec);
                    troca = 1;
                }
            }else{// se acabar o tempo de burst
                //adicionar a fila de espera, se tiver tempo de IO para fazer
                if((programData[emExec].proc->iI == programData[emExec].proc->quantI)){ //se nao houver I/O, encerrar o processo
                    programData[emExec].proc->tempoFinal = TEMPO;
                    emExec = -1;
                    qTerminados++; //incrementar a quantidade de processos que ja terminaram
                }else{// se existir i/o colocar na fila de espera, e parar de excutar
                    //printf("ESPERA\n");
                    TF_insere(espera, emExec);
                    emExec = -1;
                }
            }    
        }
        //Verificar fila de espera
        if(!TF_vazia(espera)){//se houver processo na fila de espera, executar tempo de I/O
            verificarFilaEspera(espera, prontos, programData);
        }
        //Imprime e salva no arquivo o andamento do simulador
        escalonadorLogger(prontos, espera, programData, TEMPO, OUT, Q, emExec);
        TEMPO++; //Tempo atual é incrementado
        if(emExec != -1) Q--; //Decremento do tempo de quantum, se houver algum processo executando
        if(troca){ //Se o quantum do processo atual for 0, ele é trocado pelo proximo da fila, se houver
            emExec = -1; 
            troca = 0;
        }
    }
    fclose(OUT);

    //calcular tempo medio e tru arround
    //tempoMedioTurnArround(programData, qProg);

    return 0;
}