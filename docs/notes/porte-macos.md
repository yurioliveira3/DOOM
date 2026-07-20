# Plano: porte do linuxdoom-1.10 para compilar/rodar no macOS

Branch: `port/macos-build`
Status: **concluído.** Compilação limpa, jogo testado abrindo janela via
XQuartz com cores corretas (TrueColor), janela redimensionável, sem
vazamento de shared memory ao fechar (janela nativa ou `Cmd+Q`), e com
áudio funcionando via SDL2. Ver "Comando para rodar" no fim deste
documento.

## Objetivo

Fazer o código-fonte original (`linuxdoom-1.10`) compilar e rodar no macOS
(Apple Silicon, clang), para fins de estudo.

## Estado da investigação (concluída, com correções)

Investigação inicial (compilar arquivo por arquivo) mais uma auditoria
posterior contra o estado real do código. A auditoria corrigiu alguns
pontos da investigação original e achou um bloqueio de runtime que não
tinha sido tratado como tal. Resumo final, já refletindo o que foi
implementado:

- O Makefile lista **62 arquivos** `.c` (não 59 como constava antes).
- `i_net.c`, `i_system.c`, `i_main.c` já compilam como estão — sem código
  Linux-specific escondido.
- `i_video.c` compila limpo usando os headers X11 do Homebrew (`libx11`,
  `libxext`, `xorgproto`) e corrigindo o typo `errnos.h` → `errno.h`.
- `i_sound.c`: `SNDSERV=1` (default, `doomdef.h:84`) já usa um processo
  externo via pipe e não executa o setup OSS. Único bloqueio de
  compilação: `#include <linux/soundcard.h>` incondicional — isolado
  atrás de `#ifdef __linux__`. Naquele momento o áudio ficou fora de
  escopo (jogo rodava mudo) — porte de fato feito depois, ver seção
  "Implementação — áudio via SDL2" abaixo.
- `m_bbox.h`: `values.h` (não existe no macOS) → `limits.h`;
  `m_bbox.c` (único consumidor de `MAXINT`/`MININT` vindos desse header):
  → `INT_MAX`/`INT_MIN`.
- `w_wad.c`: `malloc.h` → `stdlib.h`.
- `r_data.c`: **achado real durante a implementação** — o
  `#include <alloca.h>` já existia no arquivo, mas atrás de
  `#ifdef LINUX` (guarda que a investigação anterior não tinha notado).
  Ao remover `-DLINUX` do Makefile, o include parava de ativar e a
  compilação quebrava por `alloca` não declarado. Corrigido tornando o
  include incondicional (`<alloca.h>` existe e funciona no SDK do macOS).
- `am_map.c`: o erro de compilação é real, mas **não é K&R em parâmetro de
  função** como a investigação original supôs — são 3 declarações de
  **variável** sem tipo (`static nexttic = 0;`, dois `register outcode1/2
  = 0;`, `static fuck = 0;`), que o clang recusa em C99+. Corrigido com o
  tipo `int` explícito em cada uma; **`-std=gnu89` não foi necessário em
  nenhum arquivo** (confirmado compilando os 62 arquivos sem a flag).
- **Achado novo, fora do escopo previsto inicialmente**: `m_misc.c` tinha
  um bug real de portabilidade 64-bit na tabela `defaults[]`
  (config file). Vários campos de configuração são strings
  (`sndserver_filename`, `chatmacro0..9`), mas a tabela original guardava
  o *endereço* dessas strings dentro de um campo `int defaultvalue` via
  cast (`(int) "sndserver"`), assumindo que um ponteiro cabe em 32 bits.
  Em 64-bit isso trunca o ponteiro (corrompendo a variável) e, além
  disso, o clang recusa esse cast como "not a compile-time constant" num
  inicializador estático. Corrigido adicionando um campo `defaultstring`
  dedicado ao `default_t`, e ajustando `M_SaveDefaults`/`M_LoadDefaults`
  para decidir int-vs-string por esse campo (antes usava uma heurística
  frágil de faixa numérica do valor).
- **Bloqueio real de runtime, não só de compilação**: `i_video.c:771`
  (`I_InitGraphics`) exigia `XMatchVisualInfo(..., 8, PseudoColor, ...)`
  — servidores X modernos (XQuartz incluso) praticamente nunca oferecem
  visual PseudoColor 8-bit, só TrueColor 24/32-bit. O código compilaria
  mas travaria com `I_Error` assim que abrisse a janela. Resolvido
  implementando suporte a TrueColor (ver seção abaixo).

## Implementação — Makefile

- Removido `-DLINUX`, mantido `-DNORMALUNIX`.
- `CFLAGS`/`LDFLAGS` resolvem os paths do X11 via `pkg-config` quando
  disponível, com fallback automático para `brew --prefix libx11` /
  `libxext` / `xorgproto` (nenhum dos três tem `.pc` do `xorgproto`
  hoje, então o fallback é o caminho normalmente usado).
- Removido `-lnsl` (Solaris/glibc, não existe no macOS/BSD).
- `CC` trocado de `gcc` para `cc` (aponta pro clang do Xcode/CLT).
- **Sem `-std=gnu89`** — todos os 62 arquivos compilam com o padrão do
  clang, sem essa flag.

## Implementação — includes obsoletos

Mudança mínima, um include por arquivo: `i_video.c` (`errno.h`),
`i_sound.c` (`linux/soundcard.h` isolado por `#ifdef __linux__`),
`m_bbox.h`/`m_bbox.c` (`limits.h` + `INT_MAX`/`INT_MIN`), `w_wad.c`
(`stdlib.h`), `r_data.c` (`alloca.h` incondicional), `am_map.c` (3
declarações com tipo `int` explícito), `m_misc.c` (tabela `defaults[]`
reestruturada, ver acima).

## Implementação — TrueColor em `i_video.c` (core do porte visual)

Objetivo: rodar em qualquer visual X11 moderno (TrueColor 24/32-bit),
mantendo o caminho PseudoColor 8-bit original intacto como fallback.

- **Detecção de visual** em `I_InitGraphics`: tenta PseudoColor 8-bit
  primeiro (compat original); se não achar, tenta TrueColor 32 e depois
  24 bits. Se nenhum existir, `I_Error`.
- Quando TrueColor: força `multiply=1` (os modos `-2`/`-3`/`-4` de
  pixel-doubling continuam restritos ao caminho PseudoColor original,
  fora de escopo — documentado como limitação conhecida).
- `screens[0]` deixa de ser sempre um alias direto de `image->data`:
  em TrueColor vira um buffer indexado separado (malloc'd), já que
  `image->data` agora guarda pixels RGB reais, não índices de paleta.
- Colormap: `AllocAll` só é válido em PseudoColor; em TrueColor usa
  `AllocNone` (visual somente leitura).
- Depth da janela e da `XImage`/`XShmImage` passa a usar
  `X_visualinfo.depth` (era hardcoded em `8`).
- `UploadNewPalette` ganhou um branch para TrueColor: constrói uma LUT
  de 256 entradas (`truecolorLUT`) usando os masks reais do visual
  (`red_mask`/`green_mask`/`blue_mask`), então funciona independente de
  ordenação RGB/BGR ou de ser 24 ou 32bpp.
- `I_FinishUpdate` ganhou um loop de expansão índice→RGB
  (`screens[0][i]` → `truecolorLUT[...]` → `image->data`) antes do
  `XShmPutImage`/`XPutImage`, só quando TrueColor.

Escopo do patch: só em `i_video.c`, nenhum outro arquivo do engine
precisou mudar para isso.

## Estado do build

- 62 arquivos compilam, link gera `linux/linuxxdoom` (~649K), **zero
  erros**, com o `Makefile` e os patches acima.
- XQuartz instalado via `brew install --cask xquartz` (precisou rodar
  manualmente num terminal real — o instalador pede senha admin
  interativa via `sudo`, que não funciona a partir de shells não
  interativos). O instalador avisou que é necessário logout/login para
  o servidor X registrar corretamente os agentes do launchd — feito.
- `doom1.wad` (shareware, 4.196.020 bytes, header `IWAD` válido, 1264
  lumps) obtido manualmente e colocado em `linuxdoom-1.10/doom1.wad`
  (renomeado de `Doom1.WAD` para minúsculas).
- **Validado rodando de verdade**: o binário compilava limpo mas
  crashava (`SIGSEGV`) logo no `R_Init`, na primeira vez que tentamos
  abrir a janela de fato. A causa não tinha como aparecer só compilando
  arquivo por arquivo — só surgiu debugando com `lldb` em runtime. Ver
  seção seguinte.

## Bugs de portabilidade 64-bit achados só em runtime (via `lldb`)

O código original assume `sizeof(void*) == 4` (32-bit x86, alvo original
de 1997) em vários pontos. Compila sem erro em qualquer arquitetura, mas
corrompe memória ou lê dados errados especificamente em 64-bit (ARM64
macOS incluído). Nenhum destes apareceu na investigação nem na auditoria
por compilação — só foram achados rodando o binário sob `lldb` e lendo
o backtrace de cada crash, um de cada vez:

1. **`r_data.c`: `maptexture_t.columndirectory`** — campo `void**`
   (OBSOLETE, nunca lido/escrito em runtime) dentro de uma struct que
   faz *overlay* direto sobre os bytes crus do lump `TEXTUREx` do WAD.
   O formato binário do WAD reserva 4 bytes fixos para esse campo (era
   um ponteiro de 32-bit no x86 original); em ARM64 um `void**` ocupa 8
   bytes, deslocando os campos seguintes (`patchcount`, `patches[]`) 4
   bytes pra frente e fazendo o código ler lixo do arquivo. Corrigido
   trocando o campo pra `int` (4 bytes fixos, nunca dereferenciado —
   só precisa ocupar o espaço certo).
2. **`r_data.c` + `p_setup.c`: arrays de ponteiros alocados com
   `Z_Malloc(n*4, ...)`** — `textures` (`texture_t**`),
   `texturecolumnlump` (`short**`), `texturecolumnofs`
   (`unsigned short**`), `texturecomposite` (`byte**`) e `linebuffer`
   (`line_t**`, em `p_setup.c:536`) são todos arrays de ponteiros, mas
   alocados com tamanho fixo de 4 bytes por elemento — válido só se
   `sizeof(ponteiro) == 4`. Em ARM64 (ponteiro de 8 bytes) isso aloca
   metade da memória necessária; escrever no array corrompe a região
   vizinha no zone allocator (`Z_Malloc`), causando um crash bem depois
   e longe da causa raiz. Corrigido trocando o `4` hardcoded por
   `sizeof(*array)` em cada um. Outros arrays parecidos
   (`texturecompositesize`, `texturewidthmask`, `textureheight`,
   `texturetranslation`, `flattranslation`, `spritewidth/offset/
   topoffset`) são `int*`/`fixed_t*` de ponteiro único — `int` continua
   4 bytes em 64-bit, então esses já estavam corretos e não precisaram
   de mudança.
3. **`r_data.c` (`colormaps`) + `r_draw.c` (`translationtables`):
   alinhamento de buffer via `(int)ponteiro`** — os dois fazem
   `Z_Malloc` de um buffer um pouco maior que o necessário e arredondam
   o endereço pra um múltiplo de 256 via `(byte*)(((int)ptr + 255) &
   ~0xff)`. Nesse caso o ponteiro truncado É um endereço de heap real
   (não um índice pequeno disfarçado), então o truncamento pra 32-bit
   perde os bits altos do endereço em ARM64 e o resultado é um ponteiro
   de memória arbitrária. Corrigido trocando `(int)` por `(uintptr_t)`
   nos dois lugares (precisou `#include <stdint.h>`).
4. **`p_saveg.c`: revisado, mas não é bug real.** O arquivo tem ~12
   casts `(int)ponteiro` (save/load de jogo), que a princípio pareciam
   o mesmo problema do item 3. Só que na leitura completa do código,
   são todos parte de uma técnica de "swizzling" autoconsistente: ao
   salvar, um ponteiro real (ex: `mobj->state`) é convertido num índice
   pequeno (`ponteiro - array_base`) e esse índice pequeno é que fica
   guardado no campo, disfarçado de ponteiro; ao carregar, o mesmo
   campo é lido de volta via `(int)` e reconvertido em ponteiro real
   (`&array[indice]`). Como o valor truncado nunca foi um endereço de
   heap de verdade (é sempre um índice pequeno, bem menor que 2^31),
   truncar pra `int` não perde informação — o round-trip é correto
   mesmo em 64-bit. Trocado `(int)` por `(intptr_t)`/`(uintptr_t)`
   mesmo assim, só por precisão de tipo e pra eliminar os warnings de
   `-Wpointer-to-int-cast` do compilador — não é uma correção de bug,
   é limpeza.

## Implementação — vazamento de shared memory ao fechar

O jogo aloca um segmento de shared memory SysV (via MITSHM, usado pra
`XShmPutImage`) que só é liberado (`shmdt`/`shmctl(IPC_RMID)`) dentro do
shutdown normal (`I_Quit`). Isso é diferente de um vazamento de heap
comum (sempre reclamado pelo SO quando o processo morre) — é um recurso
de kernel que sobrevive ao processo se ele morrer sem passar por
`I_Quit`, ficando órfão (visível em `ipcs -m`).

Dois caminhos de saída não passavam por `I_Quit`, cada um corrigido em
`i_video.c`:

- **Fechar pela janela nativa (botão vermelho)**: por padrão isso só
  mata a conexão X, sem mandar nenhum evento pro processo. Corrigido
  registrando o protocolo `WM_DELETE_WINDOW` (`XSetWMProtocols`) e
  tratando o `ClientMessage` correspondente em `I_GetEvent()`, chamando
  `I_Quit()`.
- **`Cmd+Q` no XQuartz (mata o servidor X inteiro)**: nesse caso nem o
  `ClientMessage` chega — o socket X cai abruptamente (`XIO fatal
  error`). Corrigido com `XSetIOErrorHandler`, que roda mesmo com a
  conexão X já morta (faz só `shmdt`/`shmctl` via syscall direto, sem
  Xlib).

Os dois fixes foram validados ao vivo (`ipcs -m` antes/depois de fechar
por cada um dos dois caminhos) — segmento sempre limpo.

## Implementação — áudio via SDL2

O mixer em software original (`addsfx`, `I_UpdateSound` em
`i_sound.c`) já era portável — só a ponta de hardware (abrir
`/dev/dsp`, `ioctl`s OSS, `write()`) era específica de Linux. Portado
para SDL2 (já disponível via Homebrew):

- `doomdef.h`: `SNDSERV` desativado, pra cair no branch de mixagem
  síncrona interna em vez do processo externo `sndserver` (que nunca
  foi buildado no porte).
- `i_sound.c`: `open`/`ioctl`/`write`/`close` do `/dev/dsp` trocados
  por `SDL_OpenAudioDevice`/`SDL_QueueAudio`/`SDL_CloseAudioDevice`,
  isolados atrás de `#ifdef __linux__` (o caminho OSS original
  continua intacto pra quem compilar em Linux de verdade).
- Como `SDL_QueueAudio` não bloqueia (diferente do `write()` original,
  que servia de pacer natural), e o loop principal roda sem
  vsync/limite de FPS, foi adicionado um limite simples de fila em
  `I_SubmitSound` (não enfileira se já houver mais que ~3 buffers
  pendentes) — evita crescimento sem controle da latência.
- `Makefile`: `brew --prefix sdl2` resolve hoje pro alias
  `sdl2-compat` (que não tem os headers reais instalados nesse
  Homebrew) — contornado apontando direto pro keg `opt/sdl2`.
- Testado ao vivo: efeitos sonoros (tiros, portas, itens) tocando
  corretamente, sem crackling perceptível.

## Comando para rodar

```
cd linuxdoom-1.10
open -a XQuartz          # garante que o servidor X está de pé
export DISPLAY=:0
./linux/linuxxdoom -iwad doom1.wad
```

Testado: a janela abre via MITSHM, mostra a tela de créditos da id
Software com cores corretas (confirma o patch de TrueColor), o processo
roda estável, fecha sem vazar shared memory (janela nativa ou `Cmd+Q`),
e o áudio toca via SDL2. Aviso esperado no log, sem impacto: `Demo is
from a different game version!` (reclama do demo de abertura embutido
no WAD shareware, não trava nada).

## Decisões tomadas

- **SDL2 em vez de CoreAudio puro para o áudio.** O mixer em software
  original já era portável; só a saída de hardware precisava trocar.
  SDL2 cobre isso com poucas chamadas (API C simples, já disponível via
  Homebrew), evitando a complexidade de AudioQueue/AudioUnit em
  CoreAudio puro para um ganho que não justificaria o código extra.

- **WAD fora do repositório.** `doom1.wad` não é versionado — dado de
  jogo licenciado pela id Software/ZeniMax, separado do código GPL do
  engine. `*.wad` no `.gitignore`.
- **TrueColor em vez de forçar visual 8-bit.** Decisão explícita do
  usuário: implementar conversão de paleta para TrueColor em
  `i_video.c` (ver seção acima), em vez de tentar forçar um visual
  PseudoColor 8-bit (historicamente instável/improvável em XQuartz
  moderno) ou só documentar o risco sem resolver.
- **Correção do bug de 64-bit em `m_misc.c`, não só workaround.** Ao
  achar o cast de ponteiro-em-int quebrando a compilação, optou-se por
  corrigir a causa raiz (campo `defaultstring` dedicado) em vez de só
  fazer o cast compilar de outra forma — evita corrupção de ponteiro em
  runtime que existiria de qualquer forma em 64-bit.

## Histórico de decisões / atualizações

- 2026-07-12: investigação inicial concluída. Branch e pasta de notas
  criadas. Nenhuma mudança de código ainda.
- 2026-07-12: decidido não versionar o WAD; `.gitignore` criado
  (`*.wad` + artefatos de build).
- 2026-07-12: `.gitignore` expandido (artefatos de build C, config
  local do Claude Code, arquivos de macOS/editores, padrões de segredo).
- 2026-07-12: assunto A*/Dijkstra segregado para `ideia-astar.md`, sem
  relação com o porte para macOS.
- 2026-07-12: adicionado `melhorias-core.md` (brainstorm, sem plano de
  execução).
- 2026-07-12: abandonado esquema de numeração nos nomes de arquivo —
  ordem/prioridade só em `INDICE.md`.
- 2026-07-12: auditoria do plano contra o código real — corrigidas
  imprecisões (contagem de arquivos, `alloca.h`/`r_data.c`, K&R em
  `am_map.c`) e decidido implementar TrueColor em vez de aceitar o
  bloqueio de visual 8-bit. Implementação completa: Makefile, includes
  obsoletos, TrueColor em `i_video.c`, e dois bugs adicionais achados
  só durante a compilação real (`am_map.c` implicit-int em variáveis,
  `m_misc.c` ponteiro truncado em `int` na tabela de config). Build
  limpo dos 62 arquivos, sem `-std=gnu89`. XQuartz instalado
  manualmente pelo usuário (pediu senha admin fora do Claude Code).
  `doom1.wad` obtido e colocado em `linuxdoom-1.10/`.
- 2026-07-12: usuário reiniciou o Mac (necessário para o XQuartz
  registrar o servidor X). Primeira tentativa de rodar: crash
  (`SIGSEGV`) no `R_Init`. Debugado com `lldb`, achados e corrigidos 3
  bugs reais de portabilidade 64-bit não previstos por nenhuma
  investigação anterior (`columndirectory` desalinhando o parser do
  WAD, arrays de ponteiro subalocados com `*4` hardcoded, alinhamento
  de buffer truncando ponteiro via `(int)`) — ver seção "Bugs de
  portabilidade 64-bit achados só em runtime". Depois desses três
  fixes, o jogo abriu a janela, renderizou a tela de créditos com
  cores corretas via XQuartz, e ficou estável. Revisado também
  `p_saveg.c` (~12 casts `(int)ponteiro` no save/load) — concluído que
  não é bug real (swizzling autoconsistente), só trocado por
  `intptr_t`/`uintptr_t` por precisão de tipo e para eliminar warnings.
- 2026-07-19: usuário reportou vazamento real (segmento de shared
  memory SysV órfão após fechar o jogo). Corrigido em dois commits:
  `WM_DELETE_WINDOW` pro fechamento pela janela nativa, e
  `XSetIOErrorHandler` pro fechamento via `Cmd+Q`/kill do servidor X
  (ver "Implementação — vazamento de shared memory ao fechar"). Ambos
  validados ao vivo com `ipcs -m`.
- 2026-07-19: áudio portado para SDL2 (ver "Implementação — áudio via
  SDL2"). O mixer em software original já era portável; só a saída de
  hardware específica de Linux (`/dev/dsp`/OSS) precisou trocar.
  Testado ao vivo, efeitos sonoros funcionando. Porte macOS considerado
  concluído nesse ponto — sem pendências conhecidas.
