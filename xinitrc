#!/bin/sh

########################################################################
##  Merge in defaults and keymaps                                     ##
########################################################################

#userresources=$HOME/.Xresources
#usermodmap=$HOME/.Xmodmap
#sysresources=/etc/X11/xinit/.Xresources
#sysmodmap=/etc/X11/xinit/.Xmodmap

#if [ -f $sysresources ]; then
#    /usr/bin/xrdb -merge $sysresources
#fi
#
#if [ -f $sysmodmap ]; then
#    /usr/bin/xmodmap $sysmodmap
#fi
#
#if [ -f $userresources ]; then
#    /usr/bin/xrdb -merge $userresources
#fi

#if [ -f $usermodmap ]; then
#    /usr/bin/xmodmap $usermodmap
#fi

########################################################################
##  Start xfce Desktop Environment                                    ##
########################################################################

#xcompmgr &
picom --config ~/.config/picom/picom.conf &
pulseaudio &
~/.local/bin/newlook &
~/.local/bin/dwmblocks &
dunst &
~/.local/bin/volume-osd &   # OSD de volumen (fase 2 efectos, 2026-10-27)
~/.local/bin/clip-daemon &  # historial del portapapeles para super+v
openrazer-daemon &
##/home/r/.dwm/autostart.sh &
#exec Hyprland
#/home/r/scripts/eww.sh &
exec dwm


#/usr/bin/daemon -rB --pidfiles=~/.run --name=pipewire /usr/bin/pipewire &
#/usr/bin/daemon -rB --pidfiles=~/.run --name=pipewire-pulse /usr/bin/pipewire-pulse &
#/usr/bin/daemon -rB --pidfiles=~/.run --name=pipewire-media-session /usr/bin/pipewire-media-session &
#wireplumber &
#/home/r/scripts/wireplumber.sh &
#exec sway
#exec dwm
