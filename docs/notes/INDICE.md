# Índice dos planos/ideias

Ordem e prioridade atual dos documentos desta pasta. Ver
[README.md](README.md) para a convenção de como a pasta funciona.

1. **[porte-macos.md](porte-macos.md)** — concluído. `linuxdoom-1.10`
   compila e roda no macOS (`make run`), incluindo janela
   redimensionável e vários bugs reais de portabilidade 64-bit
   corrigidos. Áudio segue fora de escopo.
2. **[documentacao-codigo.md](documentacao-codigo.md)** — em
   andamento. Documentação de arquitetura em
   [`docs/arquitetura.md`](../arquitetura.md) e README novo já
   escritos; falta o espaço de imagens de gameplay.
3. **[melhorias-core.md](melhorias-core.md)** — ideia/brainstorm.
   Catálogo de pontos do core interessantes pra otimizar/estudar
   (zone allocator, tabelas trigonométricas, portabilidade 64-bit,
   robustez do parser de WAD, renderer, colisão). Cada item pode virar
   um plano próprio quando escolhido.
4. **[ci-cd-lint-testes.md](ci-cd-lint-testes.md)** — ideia com
   evidência concreta: catálogo dos bugs reais do porte cruzado com o
   que cada tipo de ferramenta (warnings de compilador, `clang-tidy`,
   testes de integração) teria pego. Vem antes do A* de propósito: dá
   uma rede de segurança (pega regressão de build) antes de partir pra
   uma mudança mais experimental na IA dos monstros.
5. **[ideia-astar.md](ideia-astar.md)** — ideia. Exercício de
   aprendizado: A*/Dijkstra na IA dos monstros, no lugar da escolha
   gulosa de direção que o DOOM original usa.
6. **[controles.md](controles.md)** — concluído (levantamento). Como o
   jogo lê teclado, quais teclas são configuráveis via `~/.doomrc` vs.
   fixas no código, e como mudar cada uma. Os controles essenciais
   (andar, atirar, usar) também estão resumidos no `README.md`
   principal.
7. **[modificar-game.md](modificar-game.md)** — ideia, ainda não
   detalhada. Possibilidade de adicionar armas/inimigos novos.
