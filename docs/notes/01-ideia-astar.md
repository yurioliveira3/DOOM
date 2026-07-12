# Ideia: A*/Dijkstra na IA dos monstros (exercício de aprendizado)

Status: apenas ideia/estudo — sem plano de execução, sem código, não
depende do porte para macOS (documento 00).

## Contexto

O DOOM original não faz pathfinding real. A IA de movimento dos
monstros fica em `linuxdoom-1.10/p_enemy.c`:

- `P_NewChaseDir` (linha ~363) escolhe, a cada passo, 1 de 8 direções
  fixas com base no delta (dx, dy) até o alvo — uma escolha gulosa,
  sem memória do mapa nem custo acumulado.
- `P_Move` (linha ~272) desloca o monstro na direção escolhida e chama
  `P_TryMove` (`p_map.c`) para validar colisão.
- Se a direção bloquear, tenta a segunda melhor, depois a direção
  antiga, depois varre as 8 direções em ordem aleatória até achar uma
  livre. Sem grafo, sem replanejamento — só um "seguidor de vetor" com
  fallback de força-bruta.

Isso explica comportamentos clássicos do DOOM: monstros presos em
cantos em U, voltas bobas, etc.

## Diferença conceitual A* vs Dijkstra (recapitulando)

- Dijkstra explora por custo acumulado puro, sem heurística — cresce
  como uma onda a partir da origem.
- A* usa `f(n) = g(n) + h(n)`, onde `h(n)` estima o custo restante até
  o alvo — busca dirigida, expande muito menos nós.
- Dijkstra é o caso particular de A* com `h(n) = 0`.
- Ambos garantem caminho ótimo com pesos não-negativos (Dijkstra) ou
  heurística admissível (A*).

## Pseudocódigo A* (referência)

```
function A_STAR(start, goal):
    openSet = PriorityQueue()
    openSet.push(start, f=0)
    cameFrom = {}
    gScore = { start: 0 }

    while openSet not empty:
        current = openSet.pop_lowest_f()
        if current == goal:
            return reconstruct_path(cameFrom, current)

        for neighbor in neighbors(current):
            tentative_g = gScore[current] + cost(current, neighbor)
            if tentative_g < gScore.get(neighbor, infinity):
                cameFrom[neighbor] = current
                gScore[neighbor] = tentative_g
                f = tentative_g + heuristic(neighbor, goal)
                openSet.push(neighbor, f)

    return failure
```

## Onde se encaixaria no DOOM (conceitual, não implementado)

- `neighbors(node)` → subsetores (`subsector_t`, `r_defs.h:227`)
  conectados por `linedef_t` passáveis, ou células vizinhas de um
  grid construído a partir do `blockmap` (estrutura já existente em
  `p_map.c`, hoje usada só para acelerar teste de colisão).
- `cost(a, b)` → distância entre centros dos subsetores/células, com
  penalidade para portas, degraus, etc.
- `heuristic(n, goal)` → distância octile/euclidiana até a posição do
  alvo (o motor já se move em 8 direções).
- Ponto de entrada natural: substituir a escolha gulosa de
  `P_NewChaseDir` pelo próximo nó do caminho pré-calculado, mantendo
  `P_TryMove`/`P_Move` como estão para a colisão real.
- Como o mapa é BSP (não grid), duas abordagens possíveis:
  1. Grid improvisado sobre o `blockmap` — mais simples, menos fiel
     (não sabe o que é andável, ignora alturas/portas).
  2. Grafo de navegação a partir dos `subsector_t` do BSP (nós =
     subsetores, arestas = linedefs passáveis) — mais fiel ao motor,
     mais trabalho, equivalente a um navmesh simples.
- Replanejar a cada tick seria caro; na prática se cacheia o caminho e
  só recalcula a cada N tics ou quando o alvo se move muito.

## Próximos passos (quando este exercício for retomado)

Nenhum definido ainda — este documento é só o registro da ideia e do
levantamento inicial feito em conversa. Quando decidirmos avançar,
abrir uma branch própria (não misturar com `port/macos-build`) e um
novo documento `02-plano-astar.md` com plano concreto.
