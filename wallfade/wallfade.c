/* wallfade: transición animada de wallpaper para X11 (el "awww" que no existe en X).
 *
 * uso: wallfade [-d segundos] [-t fade|outer|center|wipe] [-f fps] -- <comando que cambia el wallpaper...>
 *   ej: wallfade -t outer -d 1 -- xwallpaper --output DP-0 --zoom foto.png
 *
 * Cómo funciona: copia el pixmap de fondo actual (_XROOTPMAP_ID), corre el comando (xwallpaper
 * deja un pixmap nuevo), y durante la animación va publicando cuadros intermedios como pixmap
 * de fondo (triple buffer) cambiando _XROOTPMAP_ID / ESETROOT_PMAP_ID en cada cuadro, que es lo
 * que picom mira para redibujar el fondo. Al final deja el pixmap del comando como fondo.
 * Transiciones por monitor (XRandR): outer = círculo que se achica (como awww "outer"),
 * center = círculo que crece, wipe = barrido de izquierda a derecha, fade = fundido.
 */
#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum { FADE, OUTER, CENTER, WIPE };

static int xerr(Display *d, XErrorEvent *e) { (void)d; (void)e; return 0; }

static Pixmap root_pixmap(Display *dpy, Window root, Atom a) {
	Atom type; int fmt; unsigned long n, after; unsigned char *data = NULL; Pixmap p = None;
	if (XGetWindowProperty(dpy, root, a, 0, 1, False, XA_PIXMAP, &type, &fmt, &n, &after, &data) == Success && data) {
		if (type == XA_PIXMAP && fmt == 32 && n == 1) p = *(Pixmap *)data;
		XFree(data);
	}
	return p;
}

static int pixmap_ok(Display *dpy, Pixmap p, unsigned w, unsigned h, unsigned depth) {
	Window r; int x, y; unsigned pw, ph, bw, pd;
	if (!p || !XGetGeometry(dpy, p, &r, &x, &y, &pw, &ph, &bw, &pd)) return 0;
	return pw == w && ph == h && pd == depth;
}

static void set_root(Display *dpy, Window root, Atom a1, Atom a2, Pixmap p) {
	XChangeProperty(dpy, root, a1, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&p, 1);
	XChangeProperty(dpy, root, a2, XA_PIXMAP, 32, PropModeReplace, (unsigned char *)&p, 1);
	XSetWindowBackgroundPixmap(dpy, root, p);
	XClearWindow(dpy, root);
	XFlush(dpy);
}

static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec / 1e9; }

int main(int argc, char **argv) {
	double dur = 1.0; int fps = 60, mode = OUTER, i = 1;
	for (; i < argc; i++) {
		if (!strcmp(argv[i], "--")) { i++; break; }
		else if (!strcmp(argv[i], "-d") && i + 1 < argc) dur = atof(argv[++i]);
		else if (!strcmp(argv[i], "-f") && i + 1 < argc) fps = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
			const char *t = argv[++i];
			mode = !strcmp(t, "fade") ? FADE : !strcmp(t, "center") ? CENTER : !strcmp(t, "wipe") ? WIPE : OUTER;
		} else break;
	}
	if (i >= argc) { fprintf(stderr, "uso: wallfade [-d seg] [-t fade|outer|center|wipe] [-f fps] -- comando...\n"); return 2; }
	char **cmd = &argv[i];
	if (dur < 0.05) dur = 0.05;
	if (fps < 10) fps = 10;

	Display *dpy = XOpenDisplay(NULL);
	if (!dpy) { execvp(cmd[0], cmd); return 1; }
	XSetErrorHandler(xerr);
	int scr = DefaultScreen(dpy); Window root = RootWindow(dpy, scr);
	unsigned w = DisplayWidth(dpy, scr), h = DisplayHeight(dpy, scr), depth = DefaultDepth(dpy, scr);
	Visual *vis = DefaultVisual(dpy, scr);
	Atom a_xroot = XInternAtom(dpy, "_XROOTPMAP_ID", False), a_eset = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);

	/* 1. copia del fondo viejo (el pixmap de xwallpaper se libera al cambiarlo) */
	Pixmap oldroot = root_pixmap(dpy, root, a_xroot);
	if (!oldroot) oldroot = root_pixmap(dpy, root, a_eset);
	Pixmap old = None;
	if (pixmap_ok(dpy, oldroot, w, h, depth)) {
		old = XCreatePixmap(dpy, root, w, h, depth);
		GC gc = XCreateGC(dpy, old, 0, NULL);
		XCopyArea(dpy, oldroot, old, gc, 0, 0, w, h, 0, 0);
		XFreeGC(dpy, gc);
		XSync(dpy, False);
	}

	/* 2. el comando pone el wallpaper nuevo */
	pid_t pid = fork();
	if (pid == 0) { execvp(cmd[0], cmd); _exit(127); }
	int st = 0; if (pid > 0) waitpid(pid, &st, 0);
	XSync(dpy, False);
	Pixmap newp = root_pixmap(dpy, root, a_xroot);
	if (!newp) newp = root_pixmap(dpy, root, a_eset);
	if (!old || !pixmap_ok(dpy, newp, w, h, depth)) { XCloseDisplay(dpy); return WEXITSTATUS(st); } /* sin fondo viejo o nuevo: ya está cambiado, sin animación */

	/* 3. monitores para las transiciones circulares */
	int nmon = 0; XRRMonitorInfo *mons = XRRGetMonitors(dpy, root, True, &nmon);
	XRRMonitorInfo one = { .x = 0, .y = 0, .width = (int)w, .height = (int)h };
	if (!mons || nmon < 1) { mons = &one; nmon = 1; }

	cairo_surface_t *so = cairo_xlib_surface_create(dpy, old, vis, (int)w, (int)h);
	cairo_surface_t *sn = cairo_xlib_surface_create(dpy, newp, vis, (int)w, (int)h);
	Pixmap buf[3]; cairo_surface_t *sb[3];
	for (int k = 0; k < 3; k++) { buf[k] = XCreatePixmap(dpy, root, w, h, depth); sb[k] = cairo_xlib_surface_create(dpy, buf[k], vis, (int)w, (int)h); }

	/* 4. animación */
	double t0 = now(), frame = 1.0 / fps; int k = 0;
	for (int done = 0; !done; k = (k + 1) % 3) {
		double el = now() - t0, t = el / dur;
		if (t >= 1.0) { t = 1.0; done = 1; }
		double e = 1.0 - pow(1.0 - t, 3); /* easeOutCubic */
		cairo_t *cr = cairo_create(sb[k]);
		if (mode == FADE) {
			cairo_set_source_surface(cr, so, 0, 0); cairo_paint(cr);
			cairo_set_source_surface(cr, sn, 0, 0); cairo_paint_with_alpha(cr, e);
		} else {
			cairo_set_source_surface(cr, mode == OUTER ? sn : so, 0, 0); cairo_paint(cr);
			for (int m = 0; m < nmon; m++) {
				double cx = mons[m].x + mons[m].width / 2.0, cy = mons[m].y + mons[m].height / 2.0;
				double R = hypot(mons[m].width / 2.0, mons[m].height / 2.0) + 2;
				cairo_save(cr);
				if (mode == OUTER) cairo_arc(cr, cx, cy, R * (1.0 - e), 0, 2 * M_PI);
				else if (mode == CENTER) cairo_arc(cr, cx, cy, R * e, 0, 2 * M_PI);
				else cairo_rectangle(cr, mons[m].x, mons[m].y, mons[m].width * e, mons[m].height);
				cairo_clip(cr);
				cairo_set_source_surface(cr, mode == OUTER ? so : sn, 0, 0); cairo_paint(cr);
				cairo_restore(cr);
			}
		}
		cairo_destroy(cr);
		cairo_surface_flush(sb[k]);
		set_root(dpy, root, a_xroot, a_eset, buf[k]);
		double next = t0 + frame * floor(el / frame + 1);
		double sl = next - now(); if (sl > 0) usleep((useconds_t)(sl * 1e6));
	}

	/* 5. dejar el pixmap del comando como fondo definitivo */
	set_root(dpy, root, a_xroot, a_eset, newp);
	XSync(dpy, False);
	usleep(80000);
	for (int j = 0; j < 3; j++) { cairo_surface_destroy(sb[j]); XFreePixmap(dpy, buf[j]); }
	cairo_surface_destroy(so); cairo_surface_destroy(sn);
	XFreePixmap(dpy, old);
	if (mons != &one) XRRFreeMonitors(mons);
	XCloseDisplay(dpy);
	return 0;
}
