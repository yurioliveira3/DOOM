################################################################
# Wrapper de conveniência: delega pro Makefile de verdade em
# linuxdoom-1.10/, pra funcionar mesmo rodando make a partir da
# raiz do repositório.
################################################################

.PHONY: all clean run

all clean run:
	$(MAKE) -C linuxdoom-1.10 $@
