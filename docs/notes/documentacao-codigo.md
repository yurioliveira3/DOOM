# Ideia: documentação do código + novo README

Status: apenas ideia — sem plano de execução. Cotado como possível
próximo passo depois que `porte-macos.md` estiver funcionando (ver
`INDICE.md` para a ordem/prioridade atual).

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
