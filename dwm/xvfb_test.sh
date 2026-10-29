#!/bin/sh
# dwm nuevo en Xvfb :9. REGLA: nunca pkill/killall por nombre (mata las ventanas del usuario en :0);
# Plantilla SEGURA para probar un dwm nuevo en Xvfb :9 (2026-10-27). Ver feedback_no_pkill_por_nombre en la memoria de Claude.
# Uso: S=<dir de salida> ./xvfb_test.sh  (las capturas van a $S; por defecto al directorio del script)
# solo kill por PID conocido o por procesos cuyo environ tenga DISPLAY=:9.
S=${S:-$(dirname "$0")}; export DISPLAY=:9
on9() { tr '\0' '\n' </proc/$1/environ 2>/dev/null | grep -qx 'DISPLAY=:9'; }
kill9() { # todo lo que corre en :9 salvo Xvfb
    for p in $(pgrep -u "$(id -un)"); do [ "$p" = "$$" ] && continue; on9 "$p" || continue
        case "$(tr '\0' ' ' </proc/$p/cmdline 2>/dev/null)" in Xvfb*) ;; *) kill "$p" 2>/dev/null ;; esac; done; }
setsid -f Xvfb :9 -screen 0 1280x800x24 -nolisten tcp >/dev/null 2>&1
for i in $(seq 1 12); do xdotool getdisplaygeometry >/dev/null 2>&1 && break; sleep 0.5; done
XP=""; for p in $(pgrep -x Xvfb); do tr '\0' ' ' </proc/$p/cmdline | grep -q ':9 ' && XP=$p; done
xsetroot -solid '#303030'
"$HOME/.local/src/dwm/dwm" >"$S/dwm2.log" 2>&1 & DW=$!; sleep 1
STS=""; for i in 1 2 3; do st -e sleep 100 & STS="$STS $!"; sleep 0.4; done; sleep 1
shot() { sleep 0.8; import -window root "$S/dwm2_$1.png"; }
xdotool key super+y; shot dwindle
xdotool key super+i; shot centered
xdotool key super+shift+c; shot centerfloat
xdotool key super+w; sleep 0.5; xdotool key super+space; sleep 2; import -window root "$S/dwm2_rofi.png"
for p in $(pgrep -x rofi); do on9 "$p" && kill "$p"; done
sleep 0.3; kill $STS 2>/dev/null; sleep 0.3; kill $DW 2>/dev/null; sleep 0.3; kill9; sleep 0.3
[ -n "$XP" ] && kill "$XP"; sleep 0.5
echo "dwm2.log:"; head -5 "$S/dwm2.log"
