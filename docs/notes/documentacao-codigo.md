# Ideia: documentação do código + novo README

Status: em andamento. `porte-macos.md` já está funcionando, então esse
passo foi liberado (ver `INDICE.md`).

## Contexto

Hoje a única documentação é o `README.TXT` original do Carmack (mais
histórico/notas de release do que guia de código) e os `FILES`/`FILES2`
dentro de `linuxdoom-1.10/`, que listam os arquivos sem muito contexto
de arquitetura. Não existe nada que explique, de forma organizada, como
as peças do engine se encaixam.

## Ideia

- **Documentação por pasta/arquivo principal**: um guia curto explicando
  o papel de cada área do código — ex. `r_*.c` (renderer), `p_*.c`
  (lógica de jogo/física), `i_*.c` (camada de plataforma), `s_*`/`i_sound.c`
  (som), `w_wad.c` (carregamento de dados), `z_zone.c` (memória) — sem
  reescrever o que já vimos em `melhorias-core.md`, só linkar pra lá
  onde fizer sentido.
- **README com nova cara**: manter o conteúdo original do Carmack (é
  histórico, não deve ser apagado — talvez mover para
  `docs/README-original.txt` ou similar) e escrever um novo README no
  topo do repo com:
  - visão geral do projeto e do que este fork/estudo está fazendo
    (porte macOS, exercícios de aprendizado);
  - diagrama(s) simples da arquitetura (ex.: fluxo de um frame, camadas
    do engine);
  - instruções de build atualizadas pro macOS, uma vez que o porte
    estiver pronto;
  - espaço reservado para imagens de gameplay, uma vez que o build
    estiver rodando de fato (depende do `doom1.wad` e do porte).

## Notas

- Depende do porte estar funcionando antes de fazer sentido gerar
  imagens de gameplay reais (não faz sentido documentar/printar algo
  que ainda não roda).
- A documentação por arquivo pode começar antes disso, só lendo o
  código — não depende do build.

## Decisões tomadas

- **`README.TXT` fica onde está**, na raiz — não foi movido pra
  `docs/`. É a carta original do Carmack, curta, e continuar na raiz
  mantém o contexto de "isto é o release original" sem exigir que
  quem clona o repo abra uma subpasta pra achar. O novo `README.md`
  referencia ele claramente no fim, em vez de escondê-lo.

## Histórico de decisões / atualizações

- 2026-07-12: escrito `docs/arquitetura.md` (camadas do engine,
  diagrama de um frame em Mermaid, papel de cada grupo de arquivo
  `p_*`/`r_*`/`i_*`/etc.) e reescrito `README.md` da raiz com visão
  geral do fork, instruções de build/rodar já atualizadas do porte,
  link pra `docs/arquitetura.md` e pra `docs/notes/`, e seção final
  apontando pro `README.TXT`/`LICENSE.TXT` originais. Falta ainda:
  espaço de imagens de gameplay (depende de rodar o jogo manualmente
  e capturar prints, não é algo que dá pra gerar sem interação).
