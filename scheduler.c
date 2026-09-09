#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#define LOGIN "gsg4"
#define MAX_NOME_TAREFA 32
#define MAX_LINHA 256

typedef struct {
    char nome[MAX_NOME_TAREFA];
    int periodo;
    int deadline;
    int burst;

    int proxima_chegada;
    int prazo_absoluto;
    int burst_restante;
    int ativa;

    int concluidas;
    int perdidas;
    int morta;
} Tarefa;

typedef struct {
    int indice_tarefa;
    int duracao;
    char razao;
} Segmento;

typedef struct {
    Segmento *segmentos;
    int quantidade;
    int capacidade;
} Historico;

typedef int (*FuncaoPrioridade)(const Tarefa *tarefas, int indice);

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

void inicializar_estado_simulacao(Tarefa *tarefas, int quantidade) {
    for (int i = 0; i < quantidade; i++) {
        tarefas[i].proxima_chegada = 0;
        tarefas[i].prazo_absoluto = 0;
        tarefas[i].burst_restante = 0;
        tarefas[i].ativa = 0;
        tarefas[i].concluidas = 0;
        tarefas[i].perdidas = 0;
        tarefas[i].morta = 0;
    }
}

int obter_prioridade_rate(const Tarefa *tarefas, int indice) {
    return tarefas[indice].periodo;
}

int obter_prioridade_edf(const Tarefa *tarefas, int indice) {
    return tarefas[indice].prazo_absoluto;
}

int selecionar_tarefa(Tarefa *tarefas, int quantidade, FuncaoPrioridade prioridade) {
    int escolhida = -1;
    int melhor_chave = 0;

    for (int i = 0; i < quantidade; i++) {
        if (!tarefas[i].ativa) {
            continue;
        }
        int chave = prioridade(tarefas, i);
        if (escolhida == -1 || chave < melhor_chave) {
            escolhida = i;
            melhor_chave = chave;
        }
    }

    return escolhida;
}

void adicionar_segmento(Historico *h, int indice_tarefa, int duracao, char razao) {
    if (h->quantidade == h->capacidade) {
        h->capacidade = (h->capacidade == 0) ? 8 : h->capacidade * 2;
        Segmento *tmp = realloc(h->segmentos, h->capacidade * sizeof(Segmento));
        if (tmp == NULL) {
            erro_fatal("falha ao realocar memoria de historico");
        }
        h->segmentos = tmp;
    }

    h->segmentos[h->quantidade].indice_tarefa = indice_tarefa;
    h->segmentos[h->quantidade].duracao = duracao;
    h->segmentos[h->quantidade].razao = razao;
    h->quantidade++;
}

void simular(Tarefa *tarefas, int quantidade, int tempo_total,
             FuncaoPrioridade prioridade, Historico *historico) {
    int segmento_tarefa = -1;
    int segmento_duracao = 0;

    int *perdeu_agora = calloc((size_t) quantidade, sizeof(int));
    int *acabou_de_terminar = calloc((size_t) quantidade, sizeof(int));
    if (perdeu_agora == NULL || acabou_de_terminar == NULL) {
        erro_fatal("falha ao alocar memoria de simulacao");
    }

    for (int t = 0; t < tempo_total; t++) {
        for (int i = 0; i < quantidade; i++) {
            perdeu_agora[i] = 0;
        }

        for (int i = 0; i < quantidade; i++) {
            if (tarefas[i].ativa && tarefas[i].prazo_absoluto == t && tarefas[i].burst_restante > 0) {
                tarefas[i].ativa = 0;
                tarefas[i].perdidas++;
                perdeu_agora[i] = 1;
            }
        }

        for (int i = 0; i < quantidade; i++) {
            if (t == tarefas[i].proxima_chegada) {
                tarefas[i].ativa = 1;
                tarefas[i].burst_restante = tarefas[i].burst;
                tarefas[i].prazo_absoluto = t + tarefas[i].deadline;
                tarefas[i].proxima_chegada += tarefas[i].periodo;
            }
        }

        int quem = selecionar_tarefa(tarefas, quantidade, prioridade);

        int houve_transicao = (segmento_tarefa != quem);
        if (segmento_tarefa != -1 && (acabou_de_terminar[segmento_tarefa] || perdeu_agora[segmento_tarefa])) {
            houve_transicao = 1;
        }

        if (houve_transicao) {
            if (segmento_tarefa != -1) {
                char razao;
                if (acabou_de_terminar[segmento_tarefa]) {
                    razao = 'F';
                } else if (perdeu_agora[segmento_tarefa]) {
                    razao = 'L';
                } else {
                    razao = 'H';
                }
                adicionar_segmento(historico, segmento_tarefa, segmento_duracao, razao);
            } else if (segmento_duracao > 0) {
                adicionar_segmento(historico, -1, segmento_duracao, '\0');
            }
            segmento_tarefa = quem;
            segmento_duracao = 0;
        }

        for (int i = 0; i < quantidade; i++) {
            acabou_de_terminar[i] = 0;
        }

        if (quem != -1) {
            tarefas[quem].burst_restante--;
            segmento_duracao++;
            if (tarefas[quem].burst_restante == 0) {
                tarefas[quem].ativa = 0;
                tarefas[quem].concluidas++;
                acabou_de_terminar[quem] = 1;
            }
        } else {
            segmento_duracao++;
        }
    }

    if (segmento_tarefa != -1) {
        char razao;
        if (acabou_de_terminar[segmento_tarefa]) {
            razao = 'F';
        } else {
            razao = 'K';
            tarefas[segmento_tarefa].morta = 1;
        }
        adicionar_segmento(historico, segmento_tarefa, segmento_duracao, razao);
    } else if (segmento_duracao > 0) {
        adicionar_segmento(historico, -1, segmento_duracao, '\0');
    }

    for (int i = 0; i < quantidade; i++) {
        if (tarefas[i].ativa) {
            tarefas[i].morta = 1;
        }
    }

    free(perdeu_agora);
    free(acabou_de_terminar);
}

void gerar_arquivo_saida(const char *algoritmo, const Tarefa *tarefas, int quantidade, const Historico *h) {
    char nome_arquivo[64];
    snprintf(nome_arquivo, sizeof(nome_arquivo), "%s_%s.out", algoritmo, LOGIN);

    FILE *out = fopen(nome_arquivo, "w");
    if (out == NULL) {
        erro_fatal("nao foi possivel criar o arquivo de saida");
    }

    char algoritmo_maiusculo[16];
    int i;
    for (i = 0; algoritmo[i] != '\0' && i < (int) sizeof(algoritmo_maiusculo) - 1; i++) {
        algoritmo_maiusculo[i] = (char) toupper((unsigned char) algoritmo[i]);
    }
    algoritmo_maiusculo[i] = '\0';

    fprintf(out, "EXECUTION BY %s\n", algoritmo_maiusculo);
    for (int s = 0; s < h->quantidade; s++) {
        Segmento seg = h->segmentos[s];
        if (seg.indice_tarefa == -1) {
            fprintf(out, "idle for %d units\n", seg.duracao);
        } else {
            fprintf(out, "[%s] for %d units - %c\n",
                    tarefas[seg.indice_tarefa].nome, seg.duracao, seg.razao);
        }
    }

    fprintf(out, "\nLOST DEADLINES\n");
    for (int t = 0; t < quantidade; t++) {
        fprintf(out, "[%s] %d\n", tarefas[t].nome, tarefas[t].perdidas);
    }

    fprintf(out, "\nCOMPLETE EXECUTION\n");
    for (int t = 0; t < quantidade; t++) {
        fprintf(out, "[%s] %d\n", tarefas[t].nome, tarefas[t].concluidas);
    }

    fprintf(out, "\nKILLED\n");
    for (int t = 0; t < quantidade; t++) {
        fprintf(out, "[%s] %d\n", tarefas[t].nome, tarefas[t].morta);
    }

    fclose(out);
}

int main(int argc, char **argv) {
    validar_argumentos(argc, argv);

    FILE *f = abrir_arquivo(argv[2]);
    int tempo_total = ler_tempo_total(f);

    int quantidade;
    Tarefa *tarefas = ler_tarefas(f, &quantidade);

    fclose(f);

    inicializar_estado_simulacao(tarefas, quantidade);

    FuncaoPrioridade prioridade;
    if (strcmp(argv[1], "rate") == 0) {
        prioridade = obter_prioridade_rate;
    } else {
        prioridade = obter_prioridade_edf;
    }

    Historico historico = { NULL, 0, 0 };
    simular(tarefas, quantidade, tempo_total, prioridade, &historico);

    gerar_arquivo_saida(argv[1], tarefas, quantidade, &historico);

    free(historico.segmentos);
    free(tarefas);
    return 0;
}