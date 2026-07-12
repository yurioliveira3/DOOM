# Ideia: CI/CD, linter e testes (futuro)

Status: apenas ideia — sem plano de execução, sem dependência com o
porte para macOS (`00`) nem com o exercício de A*/Dijkstra (`01`).

## Contexto

Hoje o projeto não tem nenhuma automação: build é manual via `make`,
sem lint e sem testes. Faz sentido pensar nisso mais pra frente,
depois que o porte para macOS estiver estável, para não ficar
quebrando silenciosamente a cada ajuste.

## Possíveis frentes (a avaliar quando chegar a hora)

- **Build em CI**: rodar `make` (Linux original e/ou variante macOS do
  porte) em GitHub Actions a cada push/PR, pegando erro de compilação
  cedo.
- **Linter/análise estática**: algo como `cppcheck` ou `clang-tidy`
  para C — precisa calibrar bastante, já que é código C89/K&R de 1997
  cheio de padrões que ferramentas modernas marcariam como problema
  sem serem bugs de fato.
- **Testes**: o código original não tem nenhuma estrutura de teste.
  Não dá pra testar o jogo todo de forma automatizada facilmente
  (depende de vídeo/WAD), mas partes isoladas e portáveis (ex.:
  `p_maputl.c`, `m_fixed.c`, utilitários matemáticos) poderiam ganhar
  testes unitários simples, compilados à parte do engine.
- **Formatação**: provavelmente não vale a pena reformatar o código
  original (perde histórico/diff limpo com upstream), então lint de
  estilo fica de fora por ora.

## Próximos passos

Nenhum definido ainda. Retomar depois que o build no macOS estiver
funcionando (ver `porte-macos.md`); aí sim decidir prioridade
entre CI de build, lint e testes, e abrir um plano numerado próprio
se for pra frente.
