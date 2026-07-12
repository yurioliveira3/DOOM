# Ideias: melhorias e otimizações no core (aprendizado)

Status: apenas brainstorm — sem plano de execução, sem compromisso de
implementar nada disso. Serve como catálogo de pontos interessantes
do código original para estudar/mexer depois do porte (`00`), antes
de partir para exercícios maiores como o A*/Dijkstra (`02`).

## Como usar este documento

Cada item é candidato a virar seu próprio plano numerado
(`03-...`, `04-...` etc.) quando for escolhido para ser feito de
verdade. Por enquanto é só um mapa do que existe e por que é
interessante.

## Princípio geral: performático, sem over-engineering

Ao aplicar qualquer um desses itens, manter o código enxuto e
performático — sem introduzir abstrações, camadas de configuração ou
generalidade que ninguém pediu. Isso não significa se limitar às
restrições de hardware de 1993 (não faz sentido, por exemplo, evitar
`float` só porque o código original evitava por falta de FPU) — dá
pra usar recursos modernos com naturalidade. O ponto é não trocar
"código simples e rápido de 1997" por "código genérico e
over-engineered de 2026"; trocar por "código simples e rápido,
escrito com as ferramentas de hoje" é o objetivo.

## Memória — `z_zone.c`

Alocador de memória customizado (zone allocator) com tags de
purga (`PU_STATIC`, `PU_CACHE`, etc.) — técnica clássica de engines
de 1990 para rodar em máquinas com pouca RAM, sem malloc/free
genérico o tempo todo. Bom estudo de:
- como um allocator próprio organiza blocos e evita fragmentação;
- o conceito de "cache purgável" (libera sob pressão de memória) —
  algo que hoje se resolveria de outra forma, mas o raciocínio ainda
  aparece em engines de jogo modernas (asset streaming).

## Matemática — `tables.c` / `m_fixed.c`

- `tables.c` (121KB!) é basicamente uma tabela de seno/cosseno
  pré-computada — técnica de lookup table para evitar chamadas de
  `sin()/cos()` em tempo real, essencial em 1993 e ainda relevante
  como conceito (trade-off memória vs. CPU).
- `m_fixed.c` implementa aritmética de ponto fixo (16.16). Bom
  exercício: entender por que ponto fixo era necessário (sem FPU
  rápida) e comparar com usar `float`/`double` hoje — daria pra medir
  diferença de performance real no macOS moderno.

## Portabilidade 64-bit (achado durante a investigação do porte)

Em `i_video.c`, a função `Expand4`/multiply=4 (já marcada como
"Broken" no próprio comentário original) faz cast de ponteiro para
`int` e volta (`(int)exp`, linha ~989) — quebra em qualquer sistema
64-bit, já que ponteiros não cabem em 32 bits. Bom exercício pontual:
corrigir usando `intptr_t`/`uintptr_t`, entendendo por que esse
padrão era comum em código pré-LP64.

## Robustez / segurança do parser de WAD

`w_wad.c` e `p_setup.c` fazem parsing de dados binários (lumps do
WAD) com pouca ou nenhuma validação de tamanho/limites — histórico
conhecido de forks/ports do DOOM que tiveram que adicionar bounds
checking para não estourar buffer com WADs malformados. Bom exercício
de segurança defensiva: escolher um parser (ex. leitura de
`sidedef_t`/`linedef_t` em `p_setup.c`) e adicionar validação sem
mudar o comportamento com WADs válidos.

## Renderer — `r_*.c`

O próprio `README.TXT` do John Carmack já aponta os pontos fracos:
- uso de coordenadas polares para clipping ("downright silly em
  retrospecto", nas palavras dele);
- pipeline walls → floors → sprites que poderia ser um único
  front-to-back walk da árvore BSP.

Reescrever o renderer não é razoável (major undertaking), mas dá pra
estudar essas partes lendo o código junto com o comentário do
Carmack como guia — é literalmente um mapa de "onde ele mesmo sabia
que dava pra melhorar".

## Movimento / colisão — `p_map.c`, `p_maputl.c`

Também citado no `README.TXT`: o teste de linha de visão
(`P_CheckSight`) não usa a árvore BSP (que já existe e é usada só
para renderização), quando poderia. Bom exercício: entender como um
BSP clip resolveria isso e por que é mais rápido que o método atual
de varrer linhas.

## IA de monstros

Já coberto em detalhe no documento `ideia-astar.md` — não repetir
aqui.

## Notas

- Todos esses pontos são conhecidos/documentados pelo próprio autor
  original ou pela comunidade de ports; não é preciso "descobrir"
  nada, é aplicar/entender na prática.
- Prioridade e ordem ainda não definidas — decidir quando o porte
  (`00`) estiver de pé e for hora de escolher o próximo passo.
