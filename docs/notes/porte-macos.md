# Plano: porte do linuxdoom-1.10 para compilar/rodar no macOS

Branch: `port/macos-build`
Status: planejamento (nenhuma mudança de código aplicada ainda)

## Objetivo

Fazer o código-fonte original (`linuxdoom-1.10`) compilar e rodar no macOS
(Apple Silicon, clang), para fins de estudo.

## Estado da investigação (concluída)

Testado compilando arquivo a arquivo com clang direto no macOS. Resumo:

- 48 de 59 arquivos do `Makefile` já compilam sem nenhuma mudança.
- `i_net.c` e `i_system.c` (rede e camada de sistema) já compilam como estão.
- `i_video.c` compila limpo desde que:
  - se use os headers X11 do Homebrew (`libx11`, `libxext`, `xorgproto`);
  - se corrija o typo `#include <errnos.h>` → `<errno.h>` (linha 49).
- `i_sound.c` só quebra por causa de `#include <linux/soundcard.h>`
  (linha 45, incondicional). O modo padrão do projeto (`SNDSERV=1` em
  `doomdef.h:84`) já usa um processo externo via pipe e nunca executa o
  código OSS direto — então o include pode ser isolado/removido sem afetar
  o caminho de execução padrão. Áudio de fato (CoreAudio ou SDL_mixer)
  fica fora do escopo inicial: o jogo pode rodar mudo.
- 7 arquivos falham por `#include <values.h>` (não existe no macOS),
  usado via `m_bbox.h:26` — trocar por `<limits.h>` + `MAXINT`→`INT_MAX`.
- `w_wad.c` falha por `#include <malloc.h>` — trocar por `<stdlib.h>`.
- `r_data.c` falha por uso de `alloca()` sem include — adicionar
  `#include <alloca.h>`.
- `am_map.c` falha por declarações estilo K&R (implicit int), que o
  clang moderno trata como erro em C99+ — resolvido só com a flag
  `-std=gnu89`, sem tocar no código.
- Faltando: instalar XQuartz de verdade para ter um X server (headers de
  dev já existem via Homebrew, mas não o app); e obter um `doom1.wad`
  (shareware) — não está neste repositório.

## Plano de execução (proposto, ainda não aplicado)

1. Ajustar `Makefile`:
   - remover `-DLINUX`, manter só `-DNORMALUNIX`;
   - adicionar `-std=gnu89` ao `CFLAGS`;
   - trocar `LDFLAGS`/`LIBS` para os paths do Homebrew, remover `-lnsl`.
2. Corrigir includes obsoletos/typos (mudança mínima, arquivo por arquivo):
   - `i_video.c`: `errnos.h` → `errno.h`.
   - `i_sound.c`: isolar `#include <linux/soundcard.h>` atrás de
     `#ifdef __linux__` (ou equivalente).
   - `m_bbox.h`: `values.h` → `limits.h`, `MAXINT`/`MININT` → `INT_MAX`/`INT_MIN`
     (conferir os outros arquivos que usam `MAXINT` diretamente).
   - `w_wad.c`: `malloc.h` → `stdlib.h`.
   - `r_data.c`: adicionar `#include <alloca.h>`.
3. Instalar XQuartz (`brew install --cask xquartz`) e validar visual
   PseudoColor 8-bit em tempo de execução (pode exigir flag/força de
   profundidade de cor, a confirmar na prática).
4. Compilar e rodar sem som primeiro; áudio fica como etapa separada.
5. Obter `doom1.wad` (shareware, distribuição livre) para testar.

## Decisões em aberto

- Áudio: aceitar rodar mudo por ora, ou já partir para um stub/rewrite
  mínimo (CoreAudio ou SDL_mixer)? — a definir antes do passo 4.

## Decisões tomadas

- **WAD fora do repositório.** `doom1.wad` (mesmo o shareware) não é
  versionado — é dado de jogo licenciado pela id Software/ZeniMax,
  separado do código GPL do engine, e não muda entre commits. Adicionado
  `*.wad` ao `.gitignore`. O usuário baixa o WAD manualmente e coloca em
  `linuxdoom-1.10/` para rodar localmente.

## Histórico de decisões / atualizações

- 2026-07-12: investigação inicial concluída (ver resumo acima). Branch e
  pasta de notas criadas. Nenhuma mudança de código ainda.
- 2026-07-12: decidido não versionar o WAD; `.gitignore` criado
  (`*.wad` + artefatos de build).
- 2026-07-12: `.gitignore` expandido para cobrir artefatos de build C
  (`*.o`, `*.a`, `*.dSYM/`, `core`), `.claude/settings.local.json`
  (config local do Claude Code), arquivos de macOS (`.DS_Store`),
  editores (`.vscode/`, `.idea/`, `*.swp`) e padrões genéricos de
  segredo (`.env`, `*.pem`, `*.key`, `*credentials*`, `*secret*`).
- 2026-07-12: assunto A*/Dijkstra segregado deste plano — passou a
  viver em `ideia-astar.md`, sem relação de dependência com o
  porte para macOS.
- 2026-07-12: adicionado `melhorias-core.md` (brainstorm de
  otimizações/melhorias no core, sem plano de execução).
- 2026-07-12: abandonado o esquema de numeração nos nomes de arquivo
  (`00-`, `01-`, `02-`) — a ordem/prioridade agora vive só em
  `INDICE.md`, e os arquivos de plano ficam nomeados só pelo assunto.
