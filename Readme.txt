
A few patches by QuDos for EGL svn revision 374, tested on Linux and FreeBSD.

-- Removed win32 code to the source tree to save space. Fortunelly, Echon will apply some of these patches soon.
-- Changes to makefile adding some extra options.
-- Posibility to write to $HOME/.quake2
-- ALSA and SDL sound added to the main egl binary (s_system <alsa>, <oss>, <sdl>).
-- SDL(*) for video and sound added with Tuxx's help.
-- Ability to load old mods (i.e eraser, lithium, gladiator, tourney, ...), big thanks to SKuller for this.
-- Clipboard pasting X11/SDL.
-- r_hwGamma variable to 1 by default.
-- Cdrom support added.
-- Added 1280x1024, mode 9, so the next modes will change too in a number.
-- OpenAL *TODO*


(*) When starting egl in SDL mode (egl-sdl), the mouse movements are slow, once in game this issue is solved.
    Toogling from fullscreen to windowed mode with ALT+RETURN keys will segfault, use menus.





