#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


// -----------------------------------------------------------------------------------------
// tipo de dados para uma palavra
typedef struct {
    unsigned char com_acento[10];
    char sem_acento[5];
    bool usada;
} palavra_t;

// -----------------------------------------------------------------------------------------
// tipo de dados para o dicionário (está oculto e a implementação não funciona muito)
typedef struct dicionario dicionario_t;

struct dicionario {
    char* nome_arquivo;
    int n_palavras;
    palavra_t* palavras;
};

// Converte o segundo byte de uma caractere acentuado em UTF-8 para o caractere não acentuado correspondente
// Função BEM meia boca, só funciona para os caracteres selecionados, existentes no arquivo de palavras.
char tira_acento(unsigned char c)
{
    unsigned char ac[] = "áâãéêíóôõúç";
    char sem_acento[]  = "aaaeeiooouc";
    for (int i=0; ac[i] != '\0'; i++) {
        if (c == ac[i]) return sem_acento[i/2];
    }
    printf("Não sei converter o caractere %d (%c%c)\n", c, 195, c);
    return c;
}

bool palavra_compara(char palavra[5], char* entrada) {
    for(int i=0; i<5; i++) {
        if(palavra[i] != entrada[i]) {
            return false;
        }
    }

    return true;
}

// lê a próxima palavra do arquivo; retorna true se sucesso
// No arquivo tem uma palavra acentuada (5 caracteres, codificados em até 10 bytes),
//   opcionalmente seguido por um ponto (para indicar que já foi usada),
//   obrigatoriamente seguido por um fim de linha.
bool le_palavra(FILE *arquivo, palavra_t *palavra)
{
    int c;
    int n = 0;
    int nsa = 0;
    palavra->usada = false;
    for (;;) {
        c = fgetc(arquivo);
        if (c == '\r') continue;
        if (c == EOF) return false;
        if (c == '\n') {
            break;
        }
        if (c == '.') {
            palavra->usada = true;
            continue;
        }
        if (n >= 10) {
            return false;
        }
        palavra->com_acento[n] = c;
        n++;
        if (c >= 'a' && c <= 'z') {
            palavra->sem_acento[nsa++] = (char)c;
        } else {
            if (c != 195) {
                palavra->sem_acento[nsa++] = tira_acento(c);
            }
        }
    }
    if (n < 10) {
        palavra->com_acento[n] = '\0';
    }
    return true;
}

// aloca memória para um dicionário de palavras, inicializa ele com as palavras lidas do
// arquivo com nome "nome_arquivo"; retorna um ponteiro para o dicionário criado
dicionario_t *dicionario_cria(char *nome_arquivo)
{
    FILE *arquivo = fopen(nome_arquivo, "r");
    if(arquivo == NULL) {
        printf("Falha ao abrir o arquivo\n");
        exit(EXIT_FAILURE);
    }

    dicionario_t *dic = malloc(sizeof(dicionario_t));
    dic->nome_arquivo = strdup(nome_arquivo);

    dic->n_palavras = 0;
    char ch = '\0';
    while (ch != EOF) {
        ch = (char)fgetc(arquivo);
        if(ch == '\n') {
            dic->n_palavras++;
        }
    }

    rewind(arquivo);
    dic->palavras = malloc(sizeof(palavra_t) * dic->n_palavras);

    palavra_t palavra;
    for(int i=0; i<dic->n_palavras; i++) {
        assert(le_palavra(arquivo, &palavra));
        dic->palavras[i] = palavra;
    }
    fclose(arquivo);
    return dic;
}
// libera a memória ocupada pelo dicionário "dic"
void dicionario_destroi(dicionario_t *dic)
{
    free(dic->nome_arquivo);
    free(dic->palavras);
    free(dic);
}

// retorna o nome do arquivo usado pelo dicionário
char *dicionario_nome_arquivo(dicionario_t *dic)
{
  return dic->nome_arquivo;
}
// retorna o número de palavras existentes no dicionário "dic"
int dicionario_numero_de_palavras(dicionario_t *dic)
{
  return dic->n_palavras;
}

palavra_t *dicionario_palavra_na_posicao(dicionario_t *dic, int pos)
{
  return &dic->palavras[pos];
}

// -----------------------------------------------------------------------------------------
// tipo de dados para as dicas sobre uma letra
typedef enum {inexistente, trocado, certo} classe_letra_t;

// -----------------------------------------------------------------------------------------
// funções de E/S na tela

// sequencias ANSI para selecionar cor das letras
// ESC[ 38;2;⟨r⟩;⟨g⟩;⟨b⟩ m Select RGB foreground color
// ESC[ 48;2;⟨r⟩;⟨g⟩;⟨b⟩ m Select RGB background color
void cor_de_fundo(int vm, int vd, int az)
{
    printf("%c[48;2;%d;%d;%dm", 27, vm, vd, az);
}
void cor_da_letra(int vm, int vd, int az)
{
    printf("%c[38;2;%d;%d;%dm", 27, vm, vd, az);
}
void cor_default(void)
{
    printf("%c[m", 27);
}

// Escreve no arquivo "arq" a palavra apontada por "p", no formato esperado para o arquivo
void escreve_palavra(FILE *arq, palavra_t *p)
{
  for (int i = 0; i < 10; i++) {
    if (p->com_acento[i] == '\0') break;
    fprintf(arq, "%c", p->com_acento[i]);
  }
  if (p->usada) fprintf(arq, ".");
  fprintf(arq, "\n");
}

// escreve na tela a versão com acentos da palavra apontada por "chute", colorindo cada letra de
//   acordo com a classificação dela no vetor "cl"
void desenha_palavra(palavra_t *chute, const classe_letra_t cl[5])
{
  int jj = 0;
  for (int l=0; l<3; l++) {
    for (int j=0; j<5; j++) {
      if (cl[j] == inexistente)  cor_de_fundo(90,90,90);
      else if (cl[j] == trocado) cor_de_fundo(200,130,0);
      else                       cor_de_fundo(0,200,0);
      if (l==0 || l == 2) {
        printf("     ");
      } else {
        printf("  ");
        printf("%c", chute->com_acento[jj]);
        if (chute->com_acento[jj] == 195) {
          printf("%c", chute->com_acento[++jj]);
        }
        jj++;
        printf("  ");
      }
      if (j<4) {
        cor_de_fundo(30,30,30);
        printf(" ");
      }
    }
    cor_default();
    printf("\n");
  }
  printf("\n");
}

// lê uma palavra do usuário,
//   até que seja uma palavra pertencente ao dicionário (e retorna um ponteiro para essa palavra no dicionário)
//   ou que seja a palavra especial que indica a desistência do usuário (nesse caso, retorna NULL)
palavra_t *le_chute(dicionario_t *dic)
{
    char input[64];
    while(true) {
        printf("Digite a palavra (fim para desistir): ");
        fgets(input, sizeof(input) - 2, stdin);
        fflush(stdin);
        input[strlen(input)-1] = '\0';
        if(strcmp(input, "fim") == 0) {
            exit(EXIT_SUCCESS);
        }
        for(int i=0; i<dic->n_palavras;i++) {
            if(palavra_compara(input, dic->palavras[i].sem_acento)) {
                return dicionario_palavra_na_posicao(dic, i);
            }
        }
        printf("\nPalavra não encontrada no dicionário! Tente novamente...\n");
    }
}

// pergunta ao usuário se ele quer jogar mais uma partida e retorna true caso positivo
bool quer_jogar_de_novo(void)
{
    char input[3];
    printf("Deseja jogar novamente (S/N) ? ");
    fgets(input, sizeof(input) - 1, stdin);
    fflush(stdin);
    return(input[0] == 'S' || input[0] == 's');
}

// -----------------------------------------------------------------------------------------
// esta função tá meio peixe fora d'água (deveria ter sido incluida na interface do dicionário!)
void atualiza_arquivo(dicionario_t *palavras)
{
  FILE *arq;
  char *nome_original = dicionario_nome_arquivo(palavras);
  char nome[strlen(nome_original)+5];
  strcpy(nome, nome_original);
  strcat(nome, ".tmp");
  arq = fopen(nome, "w");
  if (arq == NULL) {
    fprintf(stderr, "Nao consegui criar arquivo\n");
    exit(1);
  }
  for (int i = 0; i < dicionario_numero_de_palavras(palavras); i++) {
    palavra_t *palavra = dicionario_palavra_na_posicao(palavras, i);
    escreve_palavra(arq, palavra);
  }
  fclose(arq);
  remove(nome);
  rename(nome, nome_original);
}



// -----------------------------------------------------------------------------------------
// funções que implementam uma jogada

// sorteia uma palavra que ainda não tenha sido usada, e retorna um ponteiro para a palavra no dicionário
// **essa função não está completa**
palavra_t *sorteio(dicionario_t *dic)
{
  return dicionario_palavra_na_posicao(dic, 0);
}

bool classifica_chute(palavra_t *sorteada, palavra_t *chute, 
                      classe_letra_t cl[5])
{
  int i;
  int certos = 0;
  for (i=0; i<5; i++) cl[i] = inexistente;
  for (i=0; i<5; i++) {
    if (sorteada->sem_acento[i] == chute->sem_acento[i]) {
      cl[i] = certo;
      certos++;
    }
  }
  for (i=0; i<5; i++) {
    if (cl[i] != certo) {
      for (int j=0; j<5; j++) {
        if (sorteada->sem_acento[i] == chute->sem_acento[j]) {
          if (cl[j] == inexistente) {
            cl[j] = trocado;
            break;
          }
        }
      }
    }
  }
  return certos == 5;
}


// implementa uma partida, retorna um bool que diz se o jogador ganhou
bool joga(dicionario_t *palavras)
{
  bool acertou = false;
  palavra_t *sorteada = sorteio(palavras);
  sorteada->usada = true;
  atualiza_arquivo(palavras);
  for (int jogada = 0; jogada < 6; jogada++) {
    palavra_t *chute = le_chute(palavras);
    if (chute == NULL) break; // amarelou!
    classe_letra_t classificacao[5];
    acertou = classifica_chute(sorteada, chute, classificacao);
    desenha_palavra(chute, classificacao);
    if (acertou) break;
  }
  return acertou;
}



// -----------------------------------------------------------------------------------------
// função principal - implementam o laço principal do programa

int main()
{
  dicionario_t *palavras;
  palavras = dicionario_cria("palavras_de_5_letras.txt");
  
  do {
    joga(palavras);
  } while (quer_jogar_de_novo());
  
  dicionario_destroi(palavras);
}
