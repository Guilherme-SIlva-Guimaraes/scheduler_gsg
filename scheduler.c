#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define LOGIN "gsg4"
#define MAX_NOME_TAREFA 32
#define MAX_LINHA 256

typedef struct {
    char nome[MAX_NOME_TAREFA];
    int periodo;
    int deadline;
    int burst;
} Tarefa;

void erro_fatal(const char *msg) {
    fprintf(stderr, "erro: %s\n", msg);
    exit(1);
}

void validar_argumentos(int argc, char **argv) {
    if (argc != 3) {
        erro_fatal("uso: ./scheduler <rate|edf> <arquivo_entrada>");
    }
    if (strcmp(argv[1], "rate") != 0 && strcmp(argv[1], "edf") != 0) {
        erro_fatal("algoritmo invalido: use 'rate' ou 'edf'");
    }
}

FILE *abrir_arquivo(const char *caminho) {
    FILE *f = fopen(caminho, "r");
    if (f == NULL) {
        erro_fatal("nao foi possivel abrir o arquivo de entrada");
    }
    return f;
}

static int parse_inteiro_positivo(const char *str, int *out) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    char *endptr;
    long val = strtol(str, &endptr, 10);

    while (*endptr == ' ' || *endptr == '\t') {
        endptr++;
    }

    if (*endptr != '\0') {
        return 0;
    }
    if (val <= 0 || val > INT_MAX) {
        return 0;
    }

    *out = (int) val;
    return 1;
}

int ler_tempo_total(FILE *f) {
    char linha[MAX_LINHA];

    if (fgets(linha, sizeof(linha), f) == NULL) {
        erro_fatal("arquivo vazio ou sem tempo total de simulacao");
    }
    linha[strcspn(linha, "\r\n")] = '\0';

    int tempo_total;
    if (!parse_inteiro_positivo(linha, &tempo_total)) {
        erro_fatal("tempo total de simulacao invalido");
    }

    return tempo_total;
}

int parsear_linha_tarefa(char *linha, Tarefa *t) {
    char *tokens[5];
    int n = 0;

    char *tok = strtok(linha, " \t");
    while (tok != NULL && n < 5) {
        tokens[n++] = tok;
        tok = strtok(NULL, " \t");
    }

    if (n != 4) {
        return 0;
    }
    if (strlen(tokens[0]) >= MAX_NOME_TAREFA) {
        return 0;
    }

    strcpy(t->nome, tokens[0]);

    if (!parse_inteiro_positivo(tokens[1], &t->periodo)) {
        return 0;
    }
    if (!parse_inteiro_positivo(tokens[2], &t->deadline)) {
        return 0;
    }
    if (!parse_inteiro_positivo(tokens[3], &t->burst)) {
        return 0;
    }

    return 1;
}

Tarefa *ler_tarefas(FILE *f, int *quantidade) {
    int capacidade = 8;
    Tarefa *tarefas = malloc(capacidade * sizeof(Tarefa));
    if (tarefas == NULL) {
        erro_fatal("falha ao alocar memoria");
    }

    int n = 0;
    char linha[MAX_LINHA];

    while (fgets(linha, sizeof(linha), f) != NULL) {
        linha[strcspn(linha, "\r\n")] = '\0';

        int vazio = 1;
        for (char *p = linha; *p; p++) {
            if (*p != ' ' && *p != '\t') {
                vazio = 0;
                break;
            }
        }
        if (vazio) {
            continue;
        }

        if (n == capacidade) {
            capacidade *= 2;
            Tarefa *tmp = realloc(tarefas, capacidade * sizeof(Tarefa));
            if (tmp == NULL) {
                free(tarefas);
                erro_fatal("falha ao realocar memoria");
            }
            tarefas = tmp;
        }

        if (!parsear_linha_tarefa(linha, &tarefas[n])) {
            free(tarefas);
            erro_fatal("linha de tarefa malformada no arquivo de entrada");
        }

        Tarefa *t = &tarefas[n];
        if (t->burst > t->deadline || t->deadline > t->periodo) {
            free(tarefas);
            erro_fatal("tarefa viola a restricao C <= D <= P");
        }

        n++;
    }

    if (n == 0) {
        free(tarefas);
        erro_fatal("arquivo de entrada nao contem nenhuma tarefa");
    }

    *quantidade = n;
    return tarefas;
}

int main(int argc, char **argv) {
    validar_argumentos(argc, argv);

    FILE *f = abrir_arquivo(argv[2]);
    int tempo_total = ler_tempo_total(f);

    int quantidade;
    Tarefa *tarefas = ler_tarefas(f, &quantidade);

    fclose(f);

    (void) tempo_total;

    for (int i = 0; i < quantidade; i++) {
        printf("%s periodo=%d deadline=%d burst=%d\n",
               tarefas[i].nome, tarefas[i].periodo,
               tarefas[i].deadline, tarefas[i].burst);
    }

    free(tarefas);
    return 0;
}