# DOOM (linuxdoom-1.10) — fork de estudo, porte para macOS

Fork de estudo do código-fonte original do DOOM (`linuxdoom-1.10`, GPLv2,
id Software, dezembro de 1997), com um porte para compilar e rodar no
macOS (Apple Silicon, clang). Ver `README.TXT` para a carta original de
John Carmack liberando o código, e `LICENSE.TXT` para a licença (GPLv2).

## Rodando no macOS

### Pré-requisitos

- Xcode Command Line Tools (`clang`, `make`).
- [Homebrew](https://brew.sh) com `libx11`, `libxext` e `xorgproto`
  instalados (`brew install libx11 libxext xorgproto`).
- [XQuartz](https://www.xquartz.org/) (`brew install --cask xquartz`) —
  precisa ser instalado a partir de um terminal de verdade (o instalador
  pede senha de admin) e requer logout/login depois de instalado.
- Um `doom1.wad` (shareware, distribuição livre) — **não está neste
  repositório** (é dado de jogo licenciado separadamente pela id
  Software/ZeniMax). Baixe manualmente e coloque em `linuxdoom-1.10/`.

### Build

```
cd linuxdoom-1.10
make
```

Gera `linux/linuxxdoom`.

### Rodar

```
cd linuxdoom-1.10
make run
```

Compila se preciso, garante que o XQuartz está de pé e roda o jogo com
`doom1.wad`. Equivale a:

```
cd linuxdoom-1.10
open -a XQuartz          # garante que o servidor X está de pé
export DISPLAY=:0
./linux/linuxxdoom -iwad doom1.wad -4
```

A resolução interna do Doom é fixa em 320×200 — não dá pra pedir uma
resolução arbitrária, só escalar por um fator inteiro. `make run` usa
`MULTIPLY=4` por padrão (1280×800, upscale nearest-neighbor, o maior
suportado). Pra mudar:

```
make run MULTIPLY=2   # 640x400
make run MULTIPLY=3   # 960x600
```

Roda sem áudio (fora de escopo do porte por ora — ver
`docs/notes/porte-macos.md`).

## Mais detalhes

O levantamento técnico completo do porte (o que foi mudado e por quê,
incluindo alguns bugs reais de portabilidade 64-bit que só apareceram
rodando o binário) está em
[`docs/notes/porte-macos.md`](docs/notes/porte-macos.md). Outras
notas/ideias de estudo sobre este fork estão em
[`docs/notes/`](docs/notes/).
