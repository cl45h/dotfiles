# dotfiles

Advertencia: Esta es mi configuración privada en Slackware. Se recomienda utilizarla solo como inspiración, ya que no hay garantía de que te funcione, se compiló en mi maquina y en mi maquina funciona.

dwm + st + dwmblocks + picom + rofi + dunst

<img width="1919" height="1078" alt="imagen" src="https://github.com/user-attachments/assets/456fe771-c586-445d-b756-57799cfaf523" />

## Estructura

| Directorio | Contenido |
|---|---|
| `dwm/` | dwm parcheado a mano (vanitygaps, swallow, movestack, scratchpad, xresources, cfacts, status2d, fancybar, statuscmd) con layouts `dwindle` y `centeredmaster`. `xvfb_test.sh` prueba un build nuevo en `Xvfb :9` sin tocar la sesión. |
| `dwmblocks/` | dwmblocks con manejo de señales por self-pipe (no crashea con señales rápidas), statuscmd (clicks por bloque con `BLOCK_BUTTON`) y bloques tiempo, volumen, reloj, internet, temperatura y batería del mouse. |
| `st/` | st con el config.h propio. |
| `picom/` | `picom.conf` para el fork pijulius v13 (sintaxis nueva de `animations` y `rules`). |
| `scripts/` | van en `~/.local/bin` (`tiempo` y `dmenu.sh` en `~/scripts`). Bloques de la barra, popups yad, menús rofi, OSD de volumen, portapapeles, zoom y temas. |
| `wallfade/` | transición animada de wallpaper para X11 en C (`fade`, `outer`, `center`, `wipe`), más el conversor de temas de 43PR a colorschemes de wal. |
| `config/` | `rofi`, `dunst`, `yad` (CSS y SVG de los popups) y `wal` (templates y 27 colorschemes). Van en `~/.config/`. |
| `xinitrc` | `~/.xinitrc`: picom, dunst, dwmblocks, volume-osd, clip-daemon y dwm. |

## Dependencias

| Programa | Uso |
|---|---|
| `pactl` | control de volumen (scripts volume, volume-popup, volume-osd) |
| `sensors` | temperatura de CPU (script cputemp) |
| `nmtui` | gestión de redes (script internet) |
| `flameshot` | capturas de pantalla |
| `xwallpaper` | fondo de pantalla |
| `wal` / `pywal` | esquemas de color dinámicos (newlook, walc, apply-colors, rofi-theme) |
| `calcurse` | calendario y agenda |
| `dmenu` | lanzador de aplicaciones (Super+Espacio) |
| `rofi` | menús de energía, acciones rápidas, fondos, temas y portapapeles |
| `dunst` / `dunstctl` | notificaciones, historial y no molestar |
| `yad` | popups de calendario y volumen |
| `picom` (fork pijulius v13) | compositor (transparencias, blur, animaciones) |
| `xrandr` | configuración de monitores |
| `xdotool`, `xrdb`, `xclip` | recarga de colores, posicionamiento de popups, portapapeles |
| `loginctl` (elogind) | suspender, reiniciar y apagar desde rofi-power |
| `magick` (ImageMagick 7) | miniaturas de rofi-wall |
| `nsxiv` | ver los fondos desde walc |
| `asciilock` | bloqueo de pantalla (script lock) |
| `pavucontrol` | mixer |
| `python3` + PyGObject (GTK 3) + pycairo | tiempo, volume-osd, clip-daemon, clip-pick, zoom |

## Compilar

```sh
# dwm: el binario que corre es ~/.local/bin/dwm, se reinicia en caliente con Super+Ctrl+q
cd dwm && make && cp dwm ~/.local/bin/dwm.new && mv ~/.local/bin/dwm.new ~/.local/bin/dwm

# dwmblocks: mata, instala en ~/.local/bin y relanza (evita el "Text file busy")
cd dwmblocks && make restart

# st
cd st && sudo make install

# wallfade
cc -O2 -o ~/.local/bin/wallfade wallfade/wallfade.c $(pkg-config --cflags --libs x11 xrandr cairo) -lm
```

## Notas

- Los atajos de teclado están en `dwm/config.h`. Los combos nuevos: Super+x energía, Super+p acciones rápidas, Super+Shift+w fondos, Super+Shift+t temas, Super+v portapapeles, Super+z zoom, Super+y dwindle, Super+i centeredmaster, Super+Shift+c flotante centrada.
- Los colores de dunst entran por un drop-in que es un symlink al cache de wal (no está en el repo): `ln -s ~/.cache/wal/colors-dunst.conf ~/.config/dunst/dunstrc.d/90-wal.conf`. Los templates de `config/wal/templates` generan ese archivo y los de rofi y el OSD.
- Algunas rutas absolutas apuntan a `/home/cl45h`; ajustá a tu `$HOME` si tu usuario es otro (están en `dwm/config.h`, `dwmblocks/config.h` y `config/yad/volume.css`).
