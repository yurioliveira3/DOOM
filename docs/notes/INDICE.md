# Índice dos planos/ideias

Ordem e prioridade atual dos documentos desta pasta. Ver
[README.md](README.md) para a convenção de como a pasta funciona.

1. **[porte-macos.md](porte-macos.md)** — em andamento. Fazer o
   `linuxdoom-1.10` compilar e rodar no macOS. Pré-requisito prático
   pra quase tudo abaixo (sem build funcionando, não dá pra testar
   nada na prática).
2. **[documentacao-codigo.md](documentacao-codigo.md)** — ideia,
   próximo cotado. Documentar o código por pasta/arquivo e renovar o
   README (mantendo o original do Carmack como histórico), com
   diagramas e, mais pra frente, imagens de gameplay.
3. **[melhorias-core.md](melhorias-core.md)** — ideia/brainstorm.
   Catálogo de pontos do core interessantes pra otimizar/estudar
   (zone allocator, tabelas trigonométricas, portabilidade 64-bit,
   robustez do parser de WAD, renderer, colisão). Cada item pode virar
   um plano próprio quando escolhido.
4. **[ci-cd-lint-testes.md](ci-cd-lint-testes.md)** — ideia. CI de
   build, linter e testes — vem antes do A* de propósito: dá uma rede
   de segurança (pega regressão de build) antes de partir pra uma
   mudança mais experimental na IA dos monstros.
5. **[ideia-astar.md](ideia-astar.md)** — ideia. Exercício de
   aprendizado: A*/Dijkstra na IA dos monstros, no lugar da escolha
   gulosa de direção que o DOOM original usa.
