# PREFIX apunta a ~/.local porque ESE es el dwmblocks que corre:
# ~/.local/bin va antes que /usr/local/bin en el PATH. Instalar en
# /usr/local/bin deja una copia divergente que nadie usa.
PREFIX ?= $(HOME)/.local
CC ?= cc

output: dwmblocks.c config.h
	${CC} `pkg-config --cflags x11 --libs x11` dwmblocks.c -o dwmblocks
blocks.h:
	cp blocks.def.h $@


clean:
	rm -f *.o *.gch dwmblocks
install: output
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 0755 dwmblocks $(DESTDIR)$(PREFIX)/bin/dwmblocks
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks

# Reemplaza la instancia viva de forma segura: matar primero (si no, el cp
# falla con ETXTBSY), instalar, y relanzar detachado para que sobreviva al shell.
restart: output
	pkill -x dwmblocks || true
	sleep 1
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 0755 dwmblocks $(DESTDIR)$(PREFIX)/bin/dwmblocks
	setsid $(DESTDIR)$(PREFIX)/bin/dwmblocks >/dev/null 2>&1 </dev/null &
	sleep 1
	@pgrep -x dwmblocks >/dev/null && echo "dwmblocks corriendo: $$(pgrep -x dwmblocks)" || echo "ERROR: no arranco"

# Chequeo de salud: fds y zombies. Sano = pocos fds, 0 zombies.
check:
	@p=$$(pgrep -x dwmblocks | head -1); \
	if [ -z "$$p" ]; then echo "dwmblocks NO esta corriendo"; exit 1; fi; \
	echo "pid      : $$p"; \
	echo "fds      : $$(ls /proc/$$p/fd | wc -l) / $$(awk '/open files/{print $$4}' /proc/$$p/limits)"; \
	echo "zombies  : $$(ps -eo stat,ppid | awk -v q=$$p '$$1 ~ /Z/ && $$2==q' | wc -l)"; \
	echo "instancias: $$(pgrep -x dwmblocks | wc -l)"

.PHONY: clean install uninstall restart check
