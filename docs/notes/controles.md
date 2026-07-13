# Teclas: como funcionam e como mudar

Status: levantamento concluído, mais uma mudança pontual já aplicada
(atirar também no `Enter` — ver "Segunda tecla pra mesma ação"). Sem
plano de execução maior: mudar/adicionar tecla é edição pontual de
valor, não justifica um plano próprio.

## Como o jogo lê teclado

`i_video.c` traduz cada evento X11 (`XKeycodeToKeysym`) pra um código
interno `KEY_*` (`xlatekey()`, ver `doomdef.h` pra lista completa) e
alimenta `gamekeydown[]`. Duas categorias bem diferentes a partir daí:

1. **Teclas configuráveis** — lidas de uma tabela de nome→variável
   (`defaults[]` em `m_misc.c`) e usadas em `g_game.c`
   (`G_BuildTiccmd`) pra montar o comando do tic. São as únicas que o
   jogo permite trocar sem recompilar.
2. **Teclas fixas** — checadas direto por valor literal (`case
   KEY_F2:`, `if (gamekeydown['1'+i])`) em `m_menu.c`/`g_game.c`. O
   DOOM original **não tem menu de "customizar controles"** — essas
   teclas só mudam editando e recompilando o código.

## Teclas configuráveis (via `~/.doomrc`)

| Ação | Variável | Default |
|---|---|---|
| Andar pra frente/trás | `key_up` / `key_down` | ↑ / ↓ |
| Virar esquerda/direita | `key_left` / `key_right` | ← / → |
| Strafe esquerda/direita | `key_strafeleft` / `key_straferight` | `,` / `.` |
| Atirar | `key_fire` | Ctrl direito (+ `Enter`, fixo — ver abaixo) |
| Usar/abrir porta | `key_use` | Espaço |
| Segurar pra strafe | `key_strafe` | Alt direito |
| Segurar pra correr | `key_speed` | Shift direito |
| Botão do mouse: atirar/strafe/andar | `mouseb_fire` / `mouseb_strafe` / `mouseb_forward` | botão 1 / 2 / 3 |

## Teclas fixas (hardcoded, não configuráveis em runtime)

| Tecla | Ação | Onde |
|---|---|---|
| `1`–`7` | Trocar de arma | `g_game.c:343` |
| `Tab` | Mapa automático | `am_map.c` |
| `-` / `=` | Diminuir/aumentar tamanho da tela de jogo | `m_menu.c:1522` |
| `F1` | Tela de ajuda (ou screenshot, se `-devparm`) | `m_menu.c:1511,1536` |
| `F2` | Salvar | `m_menu.c:1548` |
| `F3` | Carregar | `m_menu.c:1554` |
| `F4` | Volume de som | `m_menu.c:1560` |
| `F5` | Alternar nível de detalhe | `m_menu.c:1567` |
| `F6` | Quicksave | `m_menu.c:1572` |
| `F7` | Encerrar partida | `m_menu.c:1577` |
| `F8` | Alternar mensagens | `m_menu.c:1582` |
| `F9` | Quickload | `m_menu.c:1587` |
| `F10` | Sair do DOOM | `m_menu.c:1592` |
| `F11` | Alternar gamma | `m_menu.c:1597` |
| `Esc` | Menu | padrão de `M_Responder` |

## Como mudar

Duas formas, dependendo da categoria:

- **Teclas configuráveis, sem recompilar**: depois de rodar o jogo pelo
  menos uma vez, edita `~/.doomrc` (texto simples, `nome<TAB>valor`) e
  reinicia o jogo. Os códigos numéricos das teclas especiais
  (`KEY_RIGHTARROW`, `KEY_RCTRL`, etc.) estão em
  `linuxdoom-1.10/doomdef.h` — pra letras/números comuns é só o
  caractere ASCII mesmo.
- **Teclas configuráveis, mudando o default de fábrica**: edita a
  tabela `defaults[]` em `linuxdoom-1.10/m_misc.c` (ex.: linha do
  `"key_fire"`) e recompila. Só afeta quem ainda não tem um
  `~/.doomrc` — se o arquivo já existe, ele sempre prevalece sobre o
  default do código (`M_LoadDefaults` aplica o default primeiro, depois
  sobrescreve com o que estiver no arquivo).
- **Teclas fixas**: só editando o `case KEY_F2:` (ou equivalente) em
  `m_menu.c`/`g_game.c` e recompilando — não tem outro jeito, essas
  nunca passam pelo arquivo de config.

## Segunda tecla pra mesma ação

O sistema de config (`defaults[]`/`~/.doomrc`) só guarda **uma** tecla
por ação — não dá pra configurar duas pelo arquivo. Mas o check de cada
ação em `G_BuildTiccmd` (`g_game.c`) já é um `||` simples entre
teclado/mouse/joystick, então adicionar uma segunda tecla **fixa** (não
configurável, mas hardcoded como alternativa) é uma linha:

```c
// g_game.c, dentro de G_BuildTiccmd
if (gamekeydown[key_fire] || gamekeydown[KEY_ENTER]
    || mousebuttons[mousebfire]
    || joybuttons[joybfire])
    cmd->buttons |= BT_ATTACK;
```

Já aplicado: **atirar também funciona no `Enter`**, além do `Ctrl`
configurável. Mesmo padrão serviria pra qualquer outra ação
(`key_use`, `key_up`, etc.) — só achar o `if (gamekeydown[key_X] ||
...)` correspondente em `G_BuildTiccmd` e adicionar o `||
gamekeydown[KEY_Y]`.

**Pegadinha menor**: `KEY_ENTER` também é a tecla de confirmar no menu
(`M_Responder`, em `m_menu.c`). Se o jogador segurar Enter pra confirmar
algo no menu e ele fechar exatamente nesse instante, o próximo tic já
em jogo pode registrar um tiro (a tecla ainda está fisicamente
pressionada). É um efeito colateral pequeno e raro — mesma categoria de
risco que já existiria pra qualquer tecla compartilhada entre menu e
jogo, não é regressão nova.
