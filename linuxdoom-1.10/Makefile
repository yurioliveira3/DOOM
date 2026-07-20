################################################################
#
# $Id:$
#
# $Log:$
#
CC=  cc  # gcc or g++

# Nota: "brew --prefix sdl2" resolve para sdl2-compat (que sdl2 hoje
# aponta como alias no Homebrew) e não tem os headers de verdade, por
# isso aqui usamos direto o keg opt/sdl2 (o pacote sdl2 real).
CFLAGS=-g -Wall -DNORMALUNIX $(shell pkg-config --cflags x11 xext xorgproto 2>/dev/null || echo -I$(shell brew --prefix libx11)/include -I$(shell brew --prefix libxext)/include -I$(shell brew --prefix xorgproto)/include) -I$(shell brew --prefix)/opt/sdl2/include/SDL2 # -DUSEASM
LDFLAGS=$(shell pkg-config --libs-only-L x11 xext 2>/dev/null || echo -L$(shell brew --prefix libx11)/lib -L$(shell brew --prefix libxext)/lib) -L$(shell brew --prefix)/opt/sdl2/lib
LIBS=-lXext -lX11 -lm -lSDL2

# subdirectory for objects
O=linux

# not too sophisticated dependency
OBJS=				\
		$(O)/doomdef.o		\
		$(O)/doomstat.o		\
		$(O)/dstrings.o		\
		$(O)/i_system.o		\
		$(O)/i_sound.o		\
		$(O)/i_video.o		\
		$(O)/i_net.o			\
		$(O)/tables.o			\
		$(O)/f_finale.o		\
		$(O)/f_wipe.o 		\
		$(O)/d_main.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o		\
		$(O)/g_game.o			\
		$(O)/m_menu.o			\
		$(O)/m_misc.o			\
		$(O)/m_argv.o  		\
		$(O)/m_bbox.o			\
		$(O)/m_fixed.o		\
		$(O)/m_swap.o			\
		$(O)/m_cheat.o		\
		$(O)/m_random.o		\
		$(O)/am_map.o			\
		$(O)/p_ceilng.o		\
		$(O)/p_doors.o		\
		$(O)/p_enemy.o		\
		$(O)/p_floor.o		\
		$(O)/p_inter.o		\
		$(O)/p_lights.o		\
		$(O)/p_map.o			\
		$(O)/p_maputl.o		\
		$(O)/p_plats.o		\
		$(O)/p_pspr.o			\
		$(O)/p_setup.o		\
		$(O)/p_sight.o		\
		$(O)/p_spec.o			\
		$(O)/p_switch.o		\
		$(O)/p_mobj.o			\
		$(O)/p_telept.o		\
		$(O)/p_tick.o			\
		$(O)/p_saveg.o		\
		$(O)/p_user.o			\
		$(O)/r_bsp.o			\
		$(O)/r_data.o			\
		$(O)/r_draw.o			\
		$(O)/r_main.o			\
		$(O)/r_plane.o		\
		$(O)/r_segs.o			\
		$(O)/r_sky.o			\
		$(O)/r_things.o		\
		$(O)/w_wad.o			\
		$(O)/wi_stuff.o		\
		$(O)/v_video.o		\
		$(O)/st_lib.o			\
		$(O)/st_stuff.o		\
		$(O)/hu_stuff.o		\
		$(O)/hu_lib.o			\
		$(O)/s_sound.o		\
		$(O)/z_zone.o			\
		$(O)/info.o				\
		$(O)/sounds.o

WAD=doom1.wad

# Janela maior via upscale nearest-neighbor (ver i_video.c):
# 1 = 320x200 (nativo), 2 = 640x400, 3 = 960x600, 4 = 1280x800.
MULTIPLY=4

.PHONY: all clean run

all:	 $(O)/linuxxdoom

clean:
	rm -f *.o *~ *.flc
	rm -f linux/*

$(O)/linuxxdoom:	$(OBJS) $(O)/i_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/i_main.o \
	-o $(O)/linuxxdoom $(LIBS)

$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

# macOS: builda (se precisar), garante o servidor X (XQuartz) de pé e
# roda o jogo. Depende de "all" explicitamente para deixar claro que
# sempre builda antes de rodar.
# O XQuartz sobe o X server sob demanda (launchd) na primeira conexao de
# um cliente X11 de verdade -- nao da pra detectar isso de fora de forma
# confiavel, entao so damos um tempo. Se ainda assim faltar (primeira vez
# depois de reiniciar o Mac), o jogo agora sai limpo com uma mensagem de
# erro (nao trava mais) -- e so rodar 'make run' de novo.
run:	all
	@test -f $(WAD) || { \
		echo "Falta $(WAD) em $$(pwd) -- baixe o shareware original (livre) e coloque aqui."; \
		exit 1; \
	}
	open -a XQuartz
	sleep 2
	DISPLAY=:0 ./$(O)/linuxxdoom -iwad $(WAD) -$(MULTIPLY)

#############################################################
#
#############################################################