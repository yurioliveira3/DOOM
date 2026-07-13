# Ideia: CI/CD, linter e testes

Status: ideia com evidência concreta por trás — ainda sem plano de
execução formal, mas os pontos abaixo não são mais especulação: cada um
foi motivado por um bug real encontrado durante o porte para macOS (ver
`porte-macos.md`). O pré-requisito prático (porte funcionando) já foi
cumprido — o jogo compila e roda. Sem dependência com o exercício de
A*/Dijkstra (`ideia-astar.md`).

## Contexto

Hoje o projeto não tem nenhuma automação: build é manual via `make`,
sem lint e sem testes. Durante o porte para macOS, vários bugs reais só
apareceram rodando o binário de verdade (via `lldb`), não na compilação
— mas pelo menos três deles **teriam sido pegos por um lint mais
rigoroso, antes de chegar a esse ponto**. Isso muda o cálculo de
prioridade: não é só "boa prática genérica", é uma lição concreta desta
sessão de trabalho.

## Catálogo de bugs desta sessão × o que teria pegado cada um

| Bug | Onde | Como foi achado | O que teria pegado antes |
|---|---|---|---|
| `maptexture_t.columndirectory` como `void**` (8 bytes em vez de 4 no formato binário do WAD), desalinhando `patchcount`/`patches[]` | `r_data.c` | `lldb` + backtrace, `SIGSEGV` em `R_InitTextures` | Nenhuma ferramenta automática pega isso — é um erro de *layout* contra um formato de arquivo externo, não um erro de tipo em si. Só teste de integração (rodar o binário de verdade) pega. |
| `Z_Malloc(n*4, ...)` para arrays de ponteiro (`texture_t**`, `short**`, `unsigned short**`, `byte**`), assumindo ponteiro de 32-bit | `r_data.c`, `p_setup.c` | `lldb`, corrupção de heap manifestando um crash bem depois da causa raiz | `cppcheck`/`clang-tidy` têm checks de `sizeof` suspeito (`sizeofwithsilentarraypointer`, `-Wsizeof-pointer-div` e afins); um grep-lint simples por `Z_Malloc(.*\*4` também pegaria, já que usamos exatamente essa busca manualmente para achar todos os casos de uma vez |
| `(int)ptr` truncando ponteiro real de heap para alinhamento (`colormaps`, `translationtables`) | `r_data.c`, `r_draw.c` | Warning do compilador (`-Wpointer-to-int-cast`) **já existia**, só não travava o build | **Já temos essa rede de segurança pronta — só falta ativar.** `-Werror=pointer-to-int-cast -Werror=int-to-pointer-cast` no `CFLAGS` do CI transforma isso em erro de build, sem precisar de ferramenta nova |
| `shmat()` cujo retorno de falha (`(void*)-1`) era comparado com `NULL` (nunca dispara) | `i_video.c` | Revisão manual ao mexer no código, não por ferramenta | `clang-tidy` tem checks de uso de API POSIX (`bugprone-*`, `unix.*` no `scan-build`/Clang Static Analyzer) que reconhecem esse padrão específico de `shmat`/`mmap` |
| Declaração implícita de tipo (`static nexttic = 0;`, K&R) | `am_map.c` | Erro de compilação direto (`-Wimplicit-int`, já é erro por padrão em C99+) | **Já pego automaticamente** — clang moderno já trata isso como erro, não precisa de nada extra |
| Cast de ponteiro de string pra `int` em inicializador estático (`(int) "sndserver"`) | `m_misc.c` | Erro de compilação direto ("not a compile-time constant") | **Já pego automaticamente** — o build já falha nisso |
| `alloca()` sem include ativo (guarda `#ifdef LINUX` escondendo o include) | `r_data.c` | Erro de compilação direto (`-Wimplicit-function-declaration`) | **Já pego automaticamente** |

Conclusão prática: metade dos bugs desta sessão **já são pegos pelo
build normal** (implicit-int, implicit-function-declaration, inicializador
não-constante) — a rede de segurança do compilador já existe e já
funcionou. A parte que faltava é **promover os warnings de cast de
ponteiro para erro**, que é a mudança de menor esforço e maior retorno
identificada aqui.

## Possíveis frentes (atualizado com base no que vimos)

- **Build em CI (prioridade alta, baixo esforço)**: rodar `make` no
  macOS (`macos-latest` no GitHub Actions, com `brew install libx11
  libxext xorgproto` antes) a cada push/PR. Pega qualquer regressão de
  compilação — inclusive as que já são erro hoje (implicit-int,
  inicializador não-constante), então qualquer PR que reintroduza um
  padrão parecido já quebra o CI sem esforço extra.
- **Warnings de ponteiro como erro (prioridade alta, esforço mínimo)**:
  adicionar ao `CFLAGS` do `Makefile` (ou só no job de CI, pra não travar
  o dia a dia de quem ainda está explorando código legado):
  `-Werror=pointer-to-int-cast -Werror=int-to-pointer-cast`. Isso sozinho
  teria pego o bug do `colormaps`/`translationtables` em tempo de
  compilação em vez de precisar de uma sessão de debug com `lldb`.
- **Linter/análise estática**: `cppcheck` ou `clang-tidy` para C — like
  já era esperado, precisa calibrar bastante contra falso-positivo em
  código C89/K&R de 1997. Com base nos bugs reais encontrados, os checks
  que valem a pena habilitar primeiro (maior retorno, menor ruído):
  - `clang-tidy` `bugprone-sizeof-expression` — pega `sizeof` suspeito
    (ajudaria a flagar os `Z_Malloc(n*4, ...)` de arrays de ponteiro).
  - Clang Static Analyzer (`scan-build`) — tem checks específicos de API
    Unix (`unix.API`) que cobrem uso incorreto de `shmat`/`shmget`/`mmap`.
  - Não vale a pena, por ora: checks de estilo/formatação (mudaria
    diff/histórico do código original sem ganho real) e a maioria dos
    checks "modernização" do `clang-tidy` (o código é C89 de propósito).
- **Testes**: o código original não tem nenhuma estrutura de teste. Duas
  categorias distintas, com trade-offs diferentes:
  - **Testes unitários de utilitários portáveis** (`m_fixed.c`,
    `p_maputl.c`, matemática de ponto fixo): baixo esforço, alto valor —
    não dependem de vídeo/WAD, compilam isolados do engine.
  - **Smoke test de integração** (novo, motivado pelo que vimos aqui): os
    bugs mais graves desta sessão (crash em `R_InitTextures`,
    `R_GenerateLookup`, `I_ShutdownGraphics` chamado com display nulo)
    **só apareceram rodando o binário de verdade** — nenhum teste
    unitário isolado pegaria. Um job de CI que sobe o binário headless
    via `Xvfb` (Linux) e roda alguns tics (`-timedemo` com um demo
    embutido no WAD, ou checando só que o processo não crasha nos
    primeiros N frames) teria pego pelo menos 3 dos bugs reais desta
    sessão automaticamente. Bloqueio em aberto: precisa de um `doom1.wad`
    disponível no ambiente de CI, e o WAD não é versionado (decisão já
    tomada, ver `porte-macos.md`) — teria que vir de um secret/cache do
    CI ou de uma URL de download, não resolvido ainda.
- **Formatação**: mantido fora de escopo — não vale reformatar o código
  original (perde histórico/diff limpo com upstream).

## Próximos passos

Pré-requisito (porte funcionando) já cumprido. Se/quando isso virar
prioridade de verdade, a ordem de menor-esforço-maior-retorno sugerida
pelo que vimos nesta sessão seria:

1. CI de build simples no macOS (só `make`).
2. `-Werror` nos dois warnings de cast de ponteiro (achado concreto desta
   sessão, esforço de uma linha no `Makefile`/job de CI).
3. `scan-build`/Clang Static Analyzer como job separado, não bloqueante
   no início (calibrar ruído antes de travar PR nele).
4. Testes unitários dos utilitários matemáticos portáveis.
5. Smoke test com `Xvfb` — depende de resolver o acesso ao `doom1.wad`
   em CI primeiro.

Ainda não teve um plano numerado próprio aberto pra isso — abrir quando
alguma dessas frentes virar trabalho de verdade.
