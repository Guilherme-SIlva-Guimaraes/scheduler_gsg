# Scheduler

Scheduler é um simulador de escalonamento de tarefas de tempo real feito em C. Ele lê um conjunto de tarefas periódicas (período, deadline, burst) e simula a execução delas ao longo do tempo, comparando dois algoritmos clássicos: rate-monotonic (rate) e earliest-deadline-first (edf).

## Funcionalidades

* Leitura de um arquivo de entrada com o tempo total de simulação e uma tarefa por linha.
* Validação completa do arquivo de entrada (argumentos, algoritmo, arquivo, campos malformados, violação de C <= D <= P).
* Simulação por evento discreto, unidade de tempo a unidade de tempo.
* Escalonamento preemptivo, com desempate pela ordem de chegada no arquivo de entrada.
* Detecção de perda de deadline no instante exato.
* Geração de arquivo de saída com a trilha de execução e as seções LOST DEADLINES, COMPLETE EXECUTION e KILLED.

## Pré-requisitos

* Linux ou WSL no Windows (o projeto foi desenvolvido e testado no Windows com WSL2, distribuição Ubuntu).
* gcc.
* make.

## Como compilar

Use o make:

```
make
```

Isso gera o executável `scheduler`.

Também é possível compilar diretamente com:

```
gcc -Wall -Wextra -std=c11 -O2 scheduler.c -o scheduler
```

Para limpar o executável gerado:

```
make clean
```

## Como executar

```
./scheduler rate arquivo_entrada.txt
./scheduler edf arquivo_entrada.txt
```

O resultado é gravado em `rate_gsg4.out` ou `edf_gsg4.out`, conforme o algoritmo escolhido.

## Formato do arquivo de entrada

```
TEMPO_TOTAL
NOME PERIODO DEADLINE BURST
NOME PERIODO DEADLINE BURST
```

Exemplo (`voo.txt`):

```
100
ATT 20 12 8
NAV 50 30 15
```

## Como testar

Para rodar a bateria completa de testes (casos de erro obrigatórios e casos de sucesso em rate e edf):

```
bash testebloco6.sh
```

As sessões de teste reais, incluindo os casos de sucesso e de erro, ficam registradas em `evidencias.log`, gravado com `script -a evidencias.log`.

## Estrutura do projeto

```
scheduler.c
makefile
testebloco6.sh
evidencias.log
README.md
```

* scheduler.c: código-fonte único do simulador (parsing, motor de simulação, geração de saída).
* makefile: automatiza a compilação e a limpeza.
* testebloco6.sh: script com a bateria de testes.
* evidencias.log: registro dos testes feitos no terminal.