
SOURCEDIR=.
VERSION=$(shell grep "\#define APP_VER" $(SOURCEDIR)/common/common.h | sed -e 's/.*"\([^ ]*\)"/\1/')
BINDIR=quake2

# Check OS type.
OSTYPE=$(shell uname|sed -e s/_.*//|tr '[:upper:]' '[:lower:]')
ARCH= $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc/ -e s/sparc64/sparc/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/alpha/axp/)
MACHINE= $(shell uname -m)
OP_SYSTEM= $(shell uname -sr)

VERBOSE=NO
OPTIMIZE=NO
STRIP=YES


BUILD_CLIENT=YES		# client executable
BUILD_SDL_CLIENT=YES		# SDL client executable (video & sound)
BUILD_DEDICATED=YES		# server executable
BUILD_CGAME=YES			# cgame dll
BUILD_GAME=YES			# game dll

WITH_DATADIR=NO			# Use DATADIR to read and ~/.quake2 to write.
WITH_LIBDIR=NO			# Use LIBDIR to read and ~/.quake2 to write.
WITH_XF86VM_EXT=YES
WITH_DGA_MOUSE=NO
WITH_ANTICHEAT_SUPPORT=NO	# No yet :/
WITH_CURL_SUPPORT=NO		# No yet :/

X11BASE= /usr/X11R6
LOCALBASE= /usr/local
GAMEBASE= /usr/local
SYSBINDIR=$(LOCALBASE)/bin

ifeq ($(OSTYPE),freebsd)
 DATADIR?= $(GAMEBASE)/share/quake2
 LIBDIR?= $(GAMEBASE)/lib/quake2
else
 DATADIR?= $(GAMEBASE)/games/quake2
 LIBDIR?= $(GAMEBASE)/games/quake2
endif

CC=gcc
CC_VERSION=$(shell $(CC) -dumpversion)

SHLIBEXT=so
GAME_NAME=game$(ARCH).$(SHLIBEXT)
EGLCGAME_NAME=eglcgame$(ARCH).$(SHLIBEXT)

SHARED_FLAGS+=-I$(LOCALBASE)/include \
             -I$(X11BASE)/include \
             -DGAME_NAME='\"$(GAME_NAME)\"' \
             -DEGLCGAME_NAME='\"$(EGLCGAME_NAME)\"' \
             -DOP_SYSTEM='\"$(OP_SYSTEM)\"' \
             -DCC_VERSION='\"$(CC_VERSION)\"' \
             -DC_ONLY

RELEASE_CFLAGS=$(SHARED_FLAGS) -DNDEBUG
ifeq ($(strip $(OPTIMIZE)),YES)
 RELEASE_CFLAGS+=-O3 \
                 -march=$(MACHINE) \
                 -ffast-math \
                 -funroll-loops \
                 -fstrength-reduce \
                 -fexpensive-optimizations \
                 -fno-strict-aliasing \
                 -falign-loops=2 \
                 -falign-jumps=2 \
                 -falign-functions=2
endif
ifeq ($(strip $(STRIP)),YES)
RELEASE_CFLAGS+= -s
endif

DEBUG_CFLAGS=$(SHARED_FLAGS) -ggdb -Wall

ifeq ($(ARCH),x86_64)
LIB=lib64
endif

LDFLAGS +=-L$(LOCALBASE)/lib -lm
ifeq ($(OSTYPE),linux)
LDFLAGS +=-ldl
endif
ifeq ($(OSTYPE),freebsd)
LDFLAGS += -pthread
endif
ifeq ($(strip $(WITH_ANTICHEAT_SUPPORT)),YES)
CLIENT_FLAGS +=-DCL_ANTICHEAT
endif
ifeq ($(strip $(WITH_CURL_SUPPORT)),YES)
CLIENT_FLAGS +=-DUSE_CURL
CLIENT_LDFLAGS +=$(LDFLAGS) -lcurl
endif
CLIENT_LDFLAGS +=$(LDFLAGS) -lz -ljpeg -lpng
DED_LDFLAGS +=$(LDFLAGS) -lz
MODULE_LDFLAGS +=$(LDFLAGS)
X11_LDFLAGS +=-L$(X11BASE)/lib$(LIB) -L$(LOCALBASE)/lib -lX11 -lXext

ifeq ($(OSTYPE),linux)
ALSA_LDFLAGS +=-lasound
endif

ifeq ($(strip $(WITH_DATADIR)),YES)
  SHARED_FLAGS+=-DDATADIR='\"$(DATADIR)\"'
endif

ifeq ($(strip $(WITH_LIBDIR)),YES)
  SHARED_FLAGS+=-DLIBDIR='\"$(LIBDIR)\"'
endif

SDL_CONFIG=sdl-config
SDL_FLAGS += $(shell $(SDL_CONFIG) --cflags)
SDL_LDFLAGS += $(shell $(SDL_CONFIG) --libs)
# Uncomment the next line if you get undefined symbols about Sys_GetClipboardData.
SDL_LDFLAGS +=-L$(X11BASE)/lib$(LIB) -L$(LOCALBASE)/lib -lX11

SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

ifneq ($(VERBOSE),YES)
CC_OUTPUT=echo "  Compiling >> $<" &&
endif
DO_CC=$(CC_OUTPUT) $(CC) $(CFLAGS) $(SDL_FLAGS) $(X11_FLAGS) $(DGA_FLAGS) -o $@ -c $<
DO_DED_CC=$(CC_OUTPUT) $(CC) $(CFLAGS) -DDEDICATED_ONLY -o $@ -c $<
DO_SHLIB_CC=$(CC_OUTPUT) $(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<

BUILD_DEBUG_DIR=debug$(ARCH)
BUILD_RELEASE_DIR=release$(ARCH)

ifeq ($(strip $(BUILD_CLIENT)),YES)
  TARGETS += $(BINDIR)/egl
endif

ifeq ($(strip $(BUILD_SDL_CLIENT)),YES)
  TARGETS += $(BINDIR)/egl-sdl
endif

ifeq ($(strip $(BUILD_DEDICATED)),YES)
  TARGETS += $(BINDIR)/eglded
endif

ifeq ($(strip $(BUILD_CGAME)),YES)
 TARGETS += $(BINDIR)/baseq2/eglcgame$(ARCH).$(SHLIBEXT)
endif

ifeq ($(strip $(BUILD_GAME)),YES)
 TARGETS += $(BINDIR)/baseq2/game$(ARCH).$(SHLIBEXT)
endif

ifeq ($(strip $(WITH_XF86VM_EXT)),YES)
X11_FLAGS += -DXF86VMODE
X11_LDFLAGS +=-L$(LOCALBASE)/lib -lXxf86vm
endif

ifeq ($(strip $(WITH_DGA_MOUSE)),YES)
X11_FLAGS += -DXF86DGA
X11_LDFLAGS +=-L$(LOCALBASE)/lib -lXxf86dga
endif

all: build_release #build_debug

build_debug:
	@printf "** Debug Build **\n"
	@-mkdir -p $(BUILD_DEBUG_DIR) \
		$(BINDIR)/baseq2 \
		$(BUILD_DEBUG_DIR)/client \
		$(BUILD_DEBUG_DIR)/dedicated \
		$(BUILD_DEBUG_DIR)/baseq2 \
		$(BUILD_DEBUG_DIR)/baseq2/cgame \
		$(BUILD_DEBUG_DIR)/baseq2/game
ifneq ($(VERBOSE),YES)
	@$(MAKE) --silent targets BUILDDIR=$(BUILD_DEBUG_DIR) CFLAGS="$(DEBUG_CFLAGS) -DEGL_VERSION='\"$(VERSION) Debug\"'";
else
	$(MAKE) targets BUILDDIR=$(BUILD_DEBUG_DIR) CFLAGS="$(DEBUG_CFLAGS) -DEGL_VERSION='\"$(VERSION) Debug\"'";
endif
	@printf "Done \n"

build_release:
	@printf "** Release Build **\n"
	@-mkdir -p $(BUILD_RELEASE_DIR) \
		$(BINDIR)/baseq2 \
		$(BUILD_RELEASE_DIR)/client \
		$(BUILD_RELEASE_DIR)/dedicated \
		$(BUILD_RELEASE_DIR)/baseq2 \
		$(BUILD_RELEASE_DIR)/baseq2/cgame \
		$(BUILD_RELEASE_DIR)/baseq2/game
ifneq ($(VERBOSE),YES)
	@$(MAKE) --silent targets BUILDDIR=$(BUILD_RELEASE_DIR) CFLAGS="$(RELEASE_CFLAGS) -DEGL_VERSION='\"$(VERSION)\"'";
else
	$(MAKE) targets BUILDDIR=$(BUILD_RELEASE_DIR) CFLAGS="$(RELEASE_CFLAGS) -DEGL_VERSION='\"$(VERSION)\"'";
endif
	@printf "Done \n"


targets: $(TARGETS)

#
# Client objects
#
OBJS_CLIENT=\
	$(BUILDDIR)/client/alias.o \
	$(BUILDDIR)/client/cbuf.o \
	$(BUILDDIR)/client/cm_common.o \
	$(BUILDDIR)/client/cm_q2_main.o \
	$(BUILDDIR)/client/cm_q2_trace.o \
	$(BUILDDIR)/client/cm_q3_main.o \
	$(BUILDDIR)/client/cm_q3_patch.o \
	$(BUILDDIR)/client/cm_q3_trace.o \
	$(BUILDDIR)/client/cmd.o \
	$(BUILDDIR)/client/common.o \
	$(BUILDDIR)/client/crc.o \
	$(BUILDDIR)/client/cvar.o \
	$(BUILDDIR)/client/files.o \
	$(BUILDDIR)/client/md4.o \
	$(BUILDDIR)/client/memory.o \
	$(BUILDDIR)/client/net_chan.o \
	$(BUILDDIR)/client/net_msg.o \
	$(BUILDDIR)/client/parse.o \
	\
	$(BUILDDIR)/client/sv_ccmds.o \
	$(BUILDDIR)/client/sv_ents.o \
	$(BUILDDIR)/client/sv_gameapi.o \
	$(BUILDDIR)/client/sv_init.o \
	$(BUILDDIR)/client/sv_main.o \
	$(BUILDDIR)/client/sv_pmove.o \
	$(BUILDDIR)/client/sv_send.o \
	$(BUILDDIR)/client/sv_user.o \
	$(BUILDDIR)/client/sv_world.o \
	\
	$(BUILDDIR)/client/cl_acapi.o \
	$(BUILDDIR)/client/cl_cgapi.o \
	$(BUILDDIR)/client/cl_cin.o \
	$(BUILDDIR)/client/cl_console.o \
	$(BUILDDIR)/client/cl_download.o \
	$(BUILDDIR)/client/cl_demo.o \
	$(BUILDDIR)/client/cl_input.o \
	$(BUILDDIR)/client/cl_keys.o \
	$(BUILDDIR)/client/cl_main.o \
	$(BUILDDIR)/client/cl_parse.o \
	$(BUILDDIR)/client/cl_screen.o \
	$(BUILDDIR)/client/gui_cursor.o \
	$(BUILDDIR)/client/gui_draw.o \
	$(BUILDDIR)/client/gui_events.o \
	$(BUILDDIR)/client/gui_init.o \
	$(BUILDDIR)/client/gui_items.o \
	$(BUILDDIR)/client/gui_keys.o \
	$(BUILDDIR)/client/gui_main.o \
	$(BUILDDIR)/client/gui_vars.o \
	$(BUILDDIR)/client/snd_dma.o \
	$(BUILDDIR)/client/snd_main.o \
	$(BUILDDIR)/client/snd_openal.o \
	\
	$(BUILDDIR)/client/r_math.o \
	$(BUILDDIR)/client/rb_batch.o \
	$(BUILDDIR)/client/rb_entity.o \
	$(BUILDDIR)/client/rb_light.o \
	$(BUILDDIR)/client/rb_math.o \
	$(BUILDDIR)/client/rb_qgl.o \
	$(BUILDDIR)/client/rb_render.o \
	$(BUILDDIR)/client/rb_shadow.o \
	$(BUILDDIR)/client/rb_state.o \
	$(BUILDDIR)/client/rf_2d.o \
	$(BUILDDIR)/client/rf_alias.o \
	$(BUILDDIR)/client/rf_cull.o \
	$(BUILDDIR)/client/rf_decal.o \
	$(BUILDDIR)/client/rf_font.o \
	$(BUILDDIR)/client/rf_image.o \
	$(BUILDDIR)/client/rf_init.o \
	$(BUILDDIR)/client/rf_light.o \
	$(BUILDDIR)/client/rf_main.o \
	$(BUILDDIR)/client/rf_meshbuffer.o \
	$(BUILDDIR)/client/rf_model.o \
	$(BUILDDIR)/client/rf_program.o \
	$(BUILDDIR)/client/rf_shader.o \
	$(BUILDDIR)/client/rf_skeletal.o \
	$(BUILDDIR)/client/rf_sky.o \
	$(BUILDDIR)/client/rf_sprite.o \
	$(BUILDDIR)/client/rf_world.o \
	\
	$(BUILDDIR)/client/unix_console.o \
	$(BUILDDIR)/client/unix_main.o \
	$(BUILDDIR)/client/unix_snd_cd.o \
	$(BUILDDIR)/client/unix_udp.o \
	\
	$(BUILDDIR)/client/byteswap.o \
	$(BUILDDIR)/client/infostrings.o \
	$(BUILDDIR)/client/m_angles.o \
	$(BUILDDIR)/client/m_bounds.o \
	$(BUILDDIR)/client/m_mat3.o \
	$(BUILDDIR)/client/m_mat4.o \
	$(BUILDDIR)/client/m_plane.o \
	$(BUILDDIR)/client/m_quat.o \
	$(BUILDDIR)/client/mathlib.o \
	$(BUILDDIR)/client/mersennetwister.o \
	$(BUILDDIR)/client/shared.o \
	$(BUILDDIR)/client/string.o \
	\
	$(BUILDDIR)/client/unzip.o \
	$(BUILDDIR)/client/ioapi.o
	
OBJS_GLX = \
	$(BUILDDIR)/client/unix_snd_main.o \
	$(BUILDDIR)/client/unix_snd_alsa.o \
	$(BUILDDIR)/client/unix_snd_oss.o \
	$(BUILDDIR)/client/unix_snd_sdl.o \
	$(BUILDDIR)/client/unix_glimp.o \
	$(BUILDDIR)/client/x11_main.o \
	$(BUILDDIR)/client/x11_utils.o
	
OBJS_SDL = \
	$(BUILDDIR)/client/sdl_snd.o \
	$(BUILDDIR)/client/sdl_glimp.o

$(BINDIR)/egl: $(OBJS_CLIENT) $(OBJS_GLX)
	@echo
	@echo "** Linking EGL **"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(X11_FLAGS)";
	@echo "$(SDL_FLAGS)";
	@echo "$(CLIENT_LDFLAGS)";
	@echo "$(X11_LDFLAGS)";
	@echo "$(ALSA_LDFLAGS)";
	@echo "$(SDL_LDFLAGS)";
	@echo "$(CLIENT_FLAGS)";
	$(CC) $(CFLAGS) $(CLIENT_FLAGS) $(SDL_FLAGS) -o $@ $(OBJS_CLIENT) $(OBJS_GLX) $(LDFLAGS) $(CLIENT_LDFLAGS) $(X11_LDFLAGS) $(ALSA_LDFLAGS) $(SDL_LDFLAGS)
	@echo

$(BINDIR)/egl-sdl: $(OBJS_CLIENT) $(OBJS_SDL)
	@echo
	@echo "** Linking EGL SDL **"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(SDL_FLAGS)";
	@echo "$(CLIENT_LDFLAGS)";
	@echo "$(SDL_LDFLAGS)";
	@echo "$(CLIENT_FLAGS)";
	$(CC) $(CFLAGS) $(CLIENT_FLAGS) $(SDL_FLAGS) -o $@ $(OBJS_CLIENT) $(OBJS_SDL) $(LDFLAGS) $(CLIENT_LDFLAGS) $(SDL_LDFLAGS)
	@echo

$(BUILDDIR)/client/alias.o: $(SOURCEDIR)/common/alias.c; $(DO_CC)
$(BUILDDIR)/client/cbuf.o: $(SOURCEDIR)/common/cbuf.c; $(DO_CC)
$(BUILDDIR)/client/cm_common.o: $(SOURCEDIR)/common/cm_common.c; $(DO_CC)
$(BUILDDIR)/client/cm_q2_main.o: $(SOURCEDIR)/common/cm_q2_main.c; $(DO_CC)
$(BUILDDIR)/client/cm_q2_trace.o: $(SOURCEDIR)/common/cm_q2_trace.c; $(DO_CC)
$(BUILDDIR)/client/cm_q3_main.o: $(SOURCEDIR)/common/cm_q3_main.c; $(DO_CC)
$(BUILDDIR)/client/cm_q3_patch.o: $(SOURCEDIR)/common/cm_q3_patch.c; $(DO_CC)
$(BUILDDIR)/client/cm_q3_trace.o: $(SOURCEDIR)/common/cm_q3_trace.c; $(DO_CC)
$(BUILDDIR)/client/cmd.o: $(SOURCEDIR)/common/cmd.c; $(DO_CC)
$(BUILDDIR)/client/common.o: $(SOURCEDIR)/common/common.c; $(DO_CC)
$(BUILDDIR)/client/crc.o: $(SOURCEDIR)/common/crc.c; $(DO_CC)
$(BUILDDIR)/client/cvar.o: $(SOURCEDIR)/common/cvar.c; $(DO_CC)
$(BUILDDIR)/client/files.o: $(SOURCEDIR)/common/files.c; $(DO_CC)
$(BUILDDIR)/client/md4.o: $(SOURCEDIR)/common/md4.c; $(DO_CC)
$(BUILDDIR)/client/memory.o: $(SOURCEDIR)/common/memory.c; $(DO_CC)
$(BUILDDIR)/client/net_chan.o: $(SOURCEDIR)/common/net_chan.c; $(DO_CC)
$(BUILDDIR)/client/net_msg.o: $(SOURCEDIR)/common/net_msg.c; $(DO_CC)
$(BUILDDIR)/client/parse.o: $(SOURCEDIR)/common/parse.c; $(DO_CC)

$(BUILDDIR)/client/sv_ccmds.o: $(SOURCEDIR)/server/sv_ccmds.c; $(DO_CC)
$(BUILDDIR)/client/sv_ents.o: $(SOURCEDIR)/server/sv_ents.c; $(DO_CC)
$(BUILDDIR)/client/sv_gameapi.o: $(SOURCEDIR)/server/sv_gameapi.c; $(DO_CC)
$(BUILDDIR)/client/sv_init.o: $(SOURCEDIR)/server/sv_init.c; $(DO_CC)
$(BUILDDIR)/client/sv_main.o: $(SOURCEDIR)/server/sv_main.c; $(DO_CC)
$(BUILDDIR)/client/sv_pmove.o: $(SOURCEDIR)/server/sv_pmove.c; $(DO_CC)
$(BUILDDIR)/client/sv_send.o: $(SOURCEDIR)/server/sv_send.c; $(DO_CC)
$(BUILDDIR)/client/sv_user.o: $(SOURCEDIR)/server/sv_user.c; $(DO_CC)
$(BUILDDIR)/client/sv_world.o: $(SOURCEDIR)/server/sv_world.c; $(DO_CC)

$(BUILDDIR)/client/cl_acapi.o: $(SOURCEDIR)/client/cl_acapi.c; $(DO_CC)
$(BUILDDIR)/client/cl_cgapi.o: $(SOURCEDIR)/client/cl_cgapi.c; $(DO_CC)
$(BUILDDIR)/client/cl_cin.o: $(SOURCEDIR)/client/cl_cin.c; $(DO_CC)
$(BUILDDIR)/client/cl_console.o: $(SOURCEDIR)/client/cl_console.c; $(DO_CC)
$(BUILDDIR)/client/cl_download.o: $(SOURCEDIR)/client/cl_download.c; $(DO_CC)
$(BUILDDIR)/client/cl_demo.o: $(SOURCEDIR)/client/cl_demo.c; $(DO_CC)
$(BUILDDIR)/client/cl_input.o: $(SOURCEDIR)/client/cl_input.c; $(DO_CC)
$(BUILDDIR)/client/cl_keys.o: $(SOURCEDIR)/client/cl_keys.c; $(DO_CC)
$(BUILDDIR)/client/cl_main.o: $(SOURCEDIR)/client/cl_main.c; $(DO_CC)
$(BUILDDIR)/client/cl_parse.o: $(SOURCEDIR)/client/cl_parse.c; $(DO_CC)
$(BUILDDIR)/client/cl_screen.o: $(SOURCEDIR)/client/cl_screen.c; $(DO_CC)
$(BUILDDIR)/client/gui_cursor.o: $(SOURCEDIR)/client/gui_cursor.c; $(DO_CC)
$(BUILDDIR)/client/gui_draw.o: $(SOURCEDIR)/client/gui_draw.c; $(DO_CC)
$(BUILDDIR)/client/gui_events.o: $(SOURCEDIR)/client/gui_events.c; $(DO_CC)
$(BUILDDIR)/client/gui_init.o: $(SOURCEDIR)/client/gui_init.c; $(DO_CC)
$(BUILDDIR)/client/gui_items.o: $(SOURCEDIR)/client/gui_items.c; $(DO_CC)
$(BUILDDIR)/client/gui_keys.o: $(SOURCEDIR)/client/gui_keys.c; $(DO_CC)
$(BUILDDIR)/client/gui_main.o: $(SOURCEDIR)/client/gui_main.c; $(DO_CC)
$(BUILDDIR)/client/gui_vars.o: $(SOURCEDIR)/client/gui_vars.c; $(DO_CC)
$(BUILDDIR)/client/snd_dma.o: $(SOURCEDIR)/client/snd_dma.c; $(DO_CC)
$(BUILDDIR)/client/snd_main.o: $(SOURCEDIR)/client/snd_main.c; $(DO_CC)
$(BUILDDIR)/client/snd_openal.o: $(SOURCEDIR)/client/snd_openal.c; $(DO_CC)

$(BUILDDIR)/client/r_math.o: $(SOURCEDIR)/renderer/r_math.c; $(DO_CC)
$(BUILDDIR)/client/rb_batch.o: $(SOURCEDIR)/renderer/rb_batch.c; $(DO_CC)
$(BUILDDIR)/client/rb_entity.o: $(SOURCEDIR)/renderer/rb_entity.c; $(DO_CC)
$(BUILDDIR)/client/rb_light.o: $(SOURCEDIR)/renderer/rb_light.c; $(DO_CC)
$(BUILDDIR)/client/rb_math.o: $(SOURCEDIR)/renderer/rb_math.c; $(DO_CC)
$(BUILDDIR)/client/rb_qgl.o: $(SOURCEDIR)/renderer/rb_qgl.c; $(DO_CC)
$(BUILDDIR)/client/rb_render.o: $(SOURCEDIR)/renderer/rb_render.c; $(DO_CC)
$(BUILDDIR)/client/rb_shadow.o: $(SOURCEDIR)/renderer/rb_shadow.c; $(DO_CC)
$(BUILDDIR)/client/rb_state.o: $(SOURCEDIR)/renderer/rb_state.c; $(DO_CC)
$(BUILDDIR)/client/rf_2d.o: $(SOURCEDIR)/renderer/rf_2d.c; $(DO_CC)
$(BUILDDIR)/client/rf_alias.o: $(SOURCEDIR)/renderer/rf_alias.c; $(DO_CC)
$(BUILDDIR)/client/rf_cull.o: $(SOURCEDIR)/renderer/rf_cull.c; $(DO_CC)
$(BUILDDIR)/client/rf_decal.o: $(SOURCEDIR)/renderer/rf_decal.c; $(DO_CC)
$(BUILDDIR)/client/rf_font.o: $(SOURCEDIR)/renderer/rf_font.c; $(DO_CC)
$(BUILDDIR)/client/rf_image.o: $(SOURCEDIR)/renderer/rf_image.c; $(DO_CC)
$(BUILDDIR)/client/rf_init.o: $(SOURCEDIR)/renderer/rf_init.c; $(DO_CC)
$(BUILDDIR)/client/rf_light.o: $(SOURCEDIR)/renderer/rf_light.c; $(DO_CC)
$(BUILDDIR)/client/rf_main.o: $(SOURCEDIR)/renderer/rf_main.c; $(DO_CC)
$(BUILDDIR)/client/rf_meshbuffer.o: $(SOURCEDIR)/renderer/rf_meshbuffer.c; $(DO_CC)
$(BUILDDIR)/client/rf_model.o: $(SOURCEDIR)/renderer/rf_model.c; $(DO_CC)
$(BUILDDIR)/client/rf_program.o: $(SOURCEDIR)/renderer/rf_program.c; $(DO_CC)
$(BUILDDIR)/client/rf_shader.o: $(SOURCEDIR)/renderer/rf_shader.c; $(DO_CC)
$(BUILDDIR)/client/rf_skeletal.o: $(SOURCEDIR)/renderer/rf_skeletal.c; $(DO_CC)
$(BUILDDIR)/client/rf_sky.o: $(SOURCEDIR)/renderer/rf_sky.c; $(DO_CC)
$(BUILDDIR)/client/rf_sprite.o: $(SOURCEDIR)/renderer/rf_sprite.c; $(DO_CC)
$(BUILDDIR)/client/rf_world.o: $(SOURCEDIR)/renderer/rf_world.c; $(DO_CC)

$(BUILDDIR)/client/unix_console.o: $(SOURCEDIR)/unix/unix_console.c; $(DO_CC)
$(BUILDDIR)/client/unix_glimp.o: $(SOURCEDIR)/unix/unix_glimp.c; $(DO_CC)
$(BUILDDIR)/client/sdl_glimp.o: $(SOURCEDIR)/unix/sdl_glimp.c; $(DO_CC)
$(BUILDDIR)/client/unix_main.o: $(SOURCEDIR)/unix/unix_main.c; $(DO_CC)
$(BUILDDIR)/client/unix_snd_alsa.o: $(SOURCEDIR)/unix/unix_snd_alsa.c; $(DO_CC)
$(BUILDDIR)/client/unix_snd_cd.o: $(SOURCEDIR)/unix/unix_snd_cd.c; $(DO_CC)
$(BUILDDIR)/client/unix_snd_main.o: $(SOURCEDIR)/unix/unix_snd_main.c; $(DO_CC)
$(BUILDDIR)/client/unix_snd_oss.o: $(SOURCEDIR)/unix/unix_snd_oss.c; $(DO_CC)
$(BUILDDIR)/client/unix_snd_sdl.o: $(SOURCEDIR)/unix/unix_snd_sdl.c; $(DO_CC)
$(BUILDDIR)/client/sdl_snd.o: $(SOURCEDIR)/unix/sdl_snd.c; $(DO_CC)
$(BUILDDIR)/client/unix_udp.o: $(SOURCEDIR)/unix/unix_udp.c; $(DO_CC)
$(BUILDDIR)/client/x11_main.o: $(SOURCEDIR)/unix/x11_main.c; $(DO_CC)
$(BUILDDIR)/client/x11_utils.o: $(SOURCEDIR)/unix/x11_utils.c; $(DO_CC)

$(BUILDDIR)/client/byteswap.o: $(SOURCEDIR)/shared/byteswap.c; $(DO_CC)
$(BUILDDIR)/client/infostrings.o: $(SOURCEDIR)/shared/infostrings.c; $(DO_CC)
$(BUILDDIR)/client/m_angles.o: $(SOURCEDIR)/shared/m_angles.c; $(DO_CC)
$(BUILDDIR)/client/m_bounds.o: $(SOURCEDIR)/shared/m_bounds.c; $(DO_CC)
$(BUILDDIR)/client/m_mat3.o: $(SOURCEDIR)/shared/m_mat3.c; $(DO_CC)
$(BUILDDIR)/client/m_mat4.o: $(SOURCEDIR)/shared/m_mat4.c; $(DO_CC)
$(BUILDDIR)/client/m_plane.o: $(SOURCEDIR)/shared/m_plane.c; $(DO_CC)
$(BUILDDIR)/client/m_quat.o: $(SOURCEDIR)/shared/m_quat.c; $(DO_CC)
$(BUILDDIR)/client/mathlib.o: $(SOURCEDIR)/shared/mathlib.c; $(DO_CC)
$(BUILDDIR)/client/mersennetwister.o: $(SOURCEDIR)/shared/mersennetwister.c; $(DO_CC)
$(BUILDDIR)/client/shared.o: $(SOURCEDIR)/shared/shared.c; $(DO_CC)
$(BUILDDIR)/client/string.o: $(SOURCEDIR)/shared/string.c; $(DO_CC)

$(BUILDDIR)/client/unzip.o: $(SOURCEDIR)/include/minizip/unzip.c; $(DO_CC)
$(BUILDDIR)/client/ioapi.o: $(SOURCEDIR)/include/minizip/ioapi.c; $(DO_CC)

#
# Dedicated server objects
#
OBJS_DEDICATED=\
	$(BUILDDIR)/dedicated/alias.o \
	$(BUILDDIR)/dedicated/cbuf.o \
	$(BUILDDIR)/dedicated/cm_common.o \
	$(BUILDDIR)/dedicated/cm_q2_main.o \
	$(BUILDDIR)/dedicated/cm_q2_trace.o \
	$(BUILDDIR)/dedicated/cm_q3_main.o \
	$(BUILDDIR)/dedicated/cm_q3_patch.o \
	$(BUILDDIR)/dedicated/cm_q3_trace.o \
	$(BUILDDIR)/dedicated/cmd.o \
	$(BUILDDIR)/dedicated/common.o \
	$(BUILDDIR)/dedicated/crc.o \
	$(BUILDDIR)/dedicated/cvar.o \
	$(BUILDDIR)/dedicated/files.o \
	$(BUILDDIR)/dedicated/md4.o \
	$(BUILDDIR)/dedicated/memory.o \
	$(BUILDDIR)/dedicated/net_chan.o \
	$(BUILDDIR)/dedicated/net_msg.o \
	$(BUILDDIR)/dedicated/parse.o \
	\
	$(BUILDDIR)/dedicated/sv_ccmds.o \
	$(BUILDDIR)/dedicated/sv_ents.o \
	$(BUILDDIR)/dedicated/sv_gameapi.o \
	$(BUILDDIR)/dedicated/sv_init.o \
	$(BUILDDIR)/dedicated/sv_main.o \
	$(BUILDDIR)/dedicated/sv_pmove.o \
	$(BUILDDIR)/dedicated/sv_send.o \
	$(BUILDDIR)/dedicated/sv_user.o \
	$(BUILDDIR)/dedicated/sv_world.o \
	\
	$(BUILDDIR)/dedicated/unix_console.o \
	$(BUILDDIR)/dedicated/unix_main.o \
	$(BUILDDIR)/dedicated/unix_udp.o \
	\
	$(BUILDDIR)/dedicated/byteswap.o \
	$(BUILDDIR)/dedicated/infostrings.o \
	$(BUILDDIR)/dedicated/m_angles.o \
	$(BUILDDIR)/dedicated/m_bounds.o \
	$(BUILDDIR)/dedicated/m_mat3.o \
	$(BUILDDIR)/dedicated/m_mat4.o \
	$(BUILDDIR)/dedicated/m_plane.o \
	$(BUILDDIR)/dedicated/m_quat.o \
	$(BUILDDIR)/dedicated/mathlib.o \
	$(BUILDDIR)/dedicated/mersennetwister.o \
	$(BUILDDIR)/dedicated/shared.o \
	$(BUILDDIR)/dedicated/string.o \
	\
	$(BUILDDIR)/dedicated/unzip.o \
	$(BUILDDIR)/dedicated/ioapi.o \

$(BINDIR)/eglded: $(OBJS_DEDICATED)
	@echo
	@echo "** Linking EGL Dedicated **"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(DED_LDFLAGS)";
	$(CC) $(CFLAGS) -o $@ $(OBJS_DEDICATED) $(DED_LDFLAGS)
	@echo

$(BUILDDIR)/dedicated/alias.o: $(SOURCEDIR)/common/alias.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cbuf.o: $(SOURCEDIR)/common/cbuf.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cm_common.o: $(SOURCEDIR)/common/cm_common.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cm_q2_main.o: $(SOURCEDIR)/common/cm_q2_main.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cm_q2_trace.o: $(SOURCEDIR)/common/cm_q2_trace.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cm_q3_main.o: $(SOURCEDIR)/common/cm_q3_main.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cm_q3_patch.o: $(SOURCEDIR)/common/cm_q3_patch.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cm_q3_trace.o: $(SOURCEDIR)/common/cm_q3_trace.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cmd.o: $(SOURCEDIR)/common/cmd.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/common.o: $(SOURCEDIR)/common/common.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/crc.o: $(SOURCEDIR)/common/crc.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/cvar.o: $(SOURCEDIR)/common/cvar.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/files.o: $(SOURCEDIR)/common/files.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/md4.o: $(SOURCEDIR)/common/md4.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/memory.o: $(SOURCEDIR)/common/memory.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/net_chan.o: $(SOURCEDIR)/common/net_chan.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/net_msg.o: $(SOURCEDIR)/common/net_msg.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/parse.o: $(SOURCEDIR)/common/parse.c; $(DO_DED_CC)

$(BUILDDIR)/dedicated/sv_ccmds.o: $(SOURCEDIR)/server/sv_ccmds.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/sv_ents.o: $(SOURCEDIR)/server/sv_ents.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/sv_gameapi.o: $(SOURCEDIR)/server/sv_gameapi.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/sv_init.o: $(SOURCEDIR)/server/sv_init.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/sv_main.o: $(SOURCEDIR)/server/sv_main.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/sv_pmove.o: $(SOURCEDIR)/server/sv_pmove.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/sv_send.o: $(SOURCEDIR)/server/sv_send.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/sv_user.o: $(SOURCEDIR)/server/sv_user.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/sv_world.o: $(SOURCEDIR)/server/sv_world.c; $(DO_DED_CC)

$(BUILDDIR)/dedicated/unix_console.o: $(SOURCEDIR)/unix/unix_console.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/unix_main.o: $(SOURCEDIR)/unix/unix_main.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/unix_udp.o: $(SOURCEDIR)/unix/unix_udp.c; $(DO_DED_CC)

$(BUILDDIR)/dedicated/byteswap.o: $(SOURCEDIR)/shared/byteswap.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/infostrings.o: $(SOURCEDIR)/shared/infostrings.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/m_angles.o: $(SOURCEDIR)/shared/m_angles.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/m_bounds.o: $(SOURCEDIR)/shared/m_bounds.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/m_mat3.o: $(SOURCEDIR)/shared/m_mat3.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/m_mat4.o: $(SOURCEDIR)/shared/m_mat4.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/m_plane.o: $(SOURCEDIR)/shared/m_plane.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/m_quat.o: $(SOURCEDIR)/shared/m_quat.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/mathlib.o: $(SOURCEDIR)/shared/mathlib.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/mersennetwister.o: $(SOURCEDIR)/shared/mersennetwister.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/shared.o: $(SOURCEDIR)/shared/shared.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/string.o: $(SOURCEDIR)/shared/string.c; $(DO_DED_CC)

$(BUILDDIR)/dedicated/unzip.o: $(SOURCEDIR)/include/minizip/unzip.c; $(DO_DED_CC)
$(BUILDDIR)/dedicated/ioapi.o: $(SOURCEDIR)/include/minizip/ioapi.c; $(DO_DED_CC)

#
# CGame (client-game) objects
#
OBJS_CGAME=\
	$(BUILDDIR)/baseq2/cgame/cg_api.o \
	$(BUILDDIR)/baseq2/cgame/cg_console.o \
	$(BUILDDIR)/baseq2/cgame/cg_decals.o \
	$(BUILDDIR)/baseq2/cgame/cg_draw.o \
	$(BUILDDIR)/baseq2/cgame/cg_entities.o \
	$(BUILDDIR)/baseq2/cgame/cg_hud.o \
	$(BUILDDIR)/baseq2/cgame/cg_inventory.o \
	$(BUILDDIR)/baseq2/cgame/cg_keys.o \
	$(BUILDDIR)/baseq2/cgame/cg_light.o \
	$(BUILDDIR)/baseq2/cgame/cg_loadscreen.o \
	$(BUILDDIR)/baseq2/cgame/cg_localents.o \
	$(BUILDDIR)/baseq2/cgame/cg_location.o \
	$(BUILDDIR)/baseq2/cgame/cg_main.o \
	$(BUILDDIR)/baseq2/cgame/cg_mapeffects.o \
	$(BUILDDIR)/baseq2/cgame/cg_media.o \
	$(BUILDDIR)/baseq2/cgame/cg_muzzleflash.o \
	$(BUILDDIR)/baseq2/cgame/cg_parse.o \
	$(BUILDDIR)/baseq2/cgame/cg_parteffects.o \
	$(BUILDDIR)/baseq2/cgame/cg_partgloom.o \
	$(BUILDDIR)/baseq2/cgame/cg_particles.o \
	$(BUILDDIR)/baseq2/cgame/cg_partsustain.o \
	$(BUILDDIR)/baseq2/cgame/cg_partthink.o \
	$(BUILDDIR)/baseq2/cgame/cg_parttrail.o \
	$(BUILDDIR)/baseq2/cgame/cg_players.o \
	$(BUILDDIR)/baseq2/cgame/cg_predict.o \
	$(BUILDDIR)/baseq2/cgame/cg_screen.o \
	$(BUILDDIR)/baseq2/cgame/cg_tempents.o \
	$(BUILDDIR)/baseq2/cgame/cg_view.o \
	$(BUILDDIR)/baseq2/cgame/cg_weapon.o \
	\
	$(BUILDDIR)/baseq2/cgame/m_main.o \
	$(BUILDDIR)/baseq2/cgame/m_mp.o \
	$(BUILDDIR)/baseq2/cgame/m_mp_downloading.o \
	$(BUILDDIR)/baseq2/cgame/m_mp_join.o \
	$(BUILDDIR)/baseq2/cgame/m_mp_join_addrbook.o \
	$(BUILDDIR)/baseq2/cgame/m_mp_player.o \
	$(BUILDDIR)/baseq2/cgame/m_mp_start.o \
	$(BUILDDIR)/baseq2/cgame/m_mp_start_dmflags.o \
	$(BUILDDIR)/baseq2/cgame/m_opts.o \
	$(BUILDDIR)/baseq2/cgame/m_opts_controls.o \
	$(BUILDDIR)/baseq2/cgame/m_opts_effects.o \
	$(BUILDDIR)/baseq2/cgame/m_opts_gloom.o \
	$(BUILDDIR)/baseq2/cgame/m_opts_hud.o \
	$(BUILDDIR)/baseq2/cgame/m_opts_input.o \
	$(BUILDDIR)/baseq2/cgame/m_opts_misc.o \
	$(BUILDDIR)/baseq2/cgame/m_opts_screen.o \
	$(BUILDDIR)/baseq2/cgame/m_opts_sound.o \
	$(BUILDDIR)/baseq2/cgame/m_quit.o \
	$(BUILDDIR)/baseq2/cgame/m_sp.o \
	$(BUILDDIR)/baseq2/cgame/m_sp_credits.o \
	$(BUILDDIR)/baseq2/cgame/m_sp_loadgame.o \
	$(BUILDDIR)/baseq2/cgame/m_sp_savegame.o \
	$(BUILDDIR)/baseq2/cgame/m_vid.o \
	$(BUILDDIR)/baseq2/cgame/m_vid_exts.o \
	$(BUILDDIR)/baseq2/cgame/m_vid_settings.o \
	$(BUILDDIR)/baseq2/cgame/menu.o \
	\
	$(BUILDDIR)/baseq2/cgame/ui_backend.o \
	$(BUILDDIR)/baseq2/cgame/ui_cursor.o \
	$(BUILDDIR)/baseq2/cgame/ui_draw.o \
	$(BUILDDIR)/baseq2/cgame/ui_items.o \
	$(BUILDDIR)/baseq2/cgame/ui_keys.o \
	\
	$(BUILDDIR)/baseq2/cgame/pmove.o \
	\
	$(BUILDDIR)/baseq2/cgame/byteswap.o \
	$(BUILDDIR)/baseq2/cgame/infostrings.o \
	$(BUILDDIR)/baseq2/cgame/m_angles.o \
	$(BUILDDIR)/baseq2/cgame/m_bounds.o \
	$(BUILDDIR)/baseq2/cgame/m_mat3.o \
	$(BUILDDIR)/baseq2/cgame/m_mat4.o \
	$(BUILDDIR)/baseq2/cgame/m_plane.o \
	$(BUILDDIR)/baseq2/cgame/m_quat.o \
	$(BUILDDIR)/baseq2/cgame/mathlib.o \
	$(BUILDDIR)/baseq2/cgame/mersennetwister.o \
	$(BUILDDIR)/baseq2/cgame/shared.o \
	$(BUILDDIR)/baseq2/cgame/string.o \
	$(BUILDDIR)/baseq2/cgame/m_flash.o \


$(BINDIR)/baseq2/eglcgame$(ARCH).$(SHLIBEXT): $(OBJS_CGAME)
	@echo
	@echo "** Linking cgame dll **"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(SHLIBLDFLAGS)";
	@echo "$(MODULE_LDFLAGS)";
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJS_CGAME) $(MODULE_LDFLAGS)
	@echo

$(BUILDDIR)/baseq2/cgame/cg_api.o: $(SOURCEDIR)/cgame/cg_api.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_console.o: $(SOURCEDIR)/cgame/cg_console.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_decals.o: $(SOURCEDIR)/cgame/cg_decals.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_draw.o: $(SOURCEDIR)/cgame/cg_draw.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_entities.o: $(SOURCEDIR)/cgame/cg_entities.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_hud.o: $(SOURCEDIR)/cgame/cg_hud.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_inventory.o: $(SOURCEDIR)/cgame/cg_inventory.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_keys.o: $(SOURCEDIR)/cgame/cg_keys.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_light.o: $(SOURCEDIR)/cgame/cg_light.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_loadscreen.o: $(SOURCEDIR)/cgame/cg_loadscreen.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_localents.o: $(SOURCEDIR)/cgame/cg_localents.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_location.o: $(SOURCEDIR)/cgame/cg_location.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_main.o: $(SOURCEDIR)/cgame/cg_main.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_mapeffects.o: $(SOURCEDIR)/cgame/cg_mapeffects.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_media.o: $(SOURCEDIR)/cgame/cg_media.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_muzzleflash.o: $(SOURCEDIR)/cgame/cg_muzzleflash.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_parse.o: $(SOURCEDIR)/cgame/cg_parse.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_parteffects.o: $(SOURCEDIR)/cgame/cg_parteffects.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_partgloom.o: $(SOURCEDIR)/cgame/cg_partgloom.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_particles.o: $(SOURCEDIR)/cgame/cg_particles.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_partsustain.o: $(SOURCEDIR)/cgame/cg_partsustain.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_partthink.o: $(SOURCEDIR)/cgame/cg_partthink.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_parttrail.o: $(SOURCEDIR)/cgame/cg_parttrail.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_players.o: $(SOURCEDIR)/cgame/cg_players.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_predict.o: $(SOURCEDIR)/cgame/cg_predict.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_screen.o: $(SOURCEDIR)/cgame/cg_screen.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_tempents.o: $(SOURCEDIR)/cgame/cg_tempents.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_view.o: $(SOURCEDIR)/cgame/cg_view.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/cg_weapon.o: $(SOURCEDIR)/cgame/cg_weapon.c; $(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/m_main.o: $(SOURCEDIR)/cgame/menu/m_main.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mp.o: $(SOURCEDIR)/cgame/menu/m_mp.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mp_downloading.o: $(SOURCEDIR)/cgame/menu/m_mp_downloading.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mp_join.o: $(SOURCEDIR)/cgame/menu/m_mp_join.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mp_join_addrbook.o: $(SOURCEDIR)/cgame/menu/m_mp_join_addrbook.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mp_player.o: $(SOURCEDIR)/cgame/menu/m_mp_player.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mp_start.o: $(SOURCEDIR)/cgame/menu/m_mp_start.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mp_start_dmflags.o: $(SOURCEDIR)/cgame/menu/m_mp_start_dmflags.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts.o: $(SOURCEDIR)/cgame/menu/m_opts.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts_controls.o: $(SOURCEDIR)/cgame/menu/m_opts_controls.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts_effects.o: $(SOURCEDIR)/cgame/menu/m_opts_effects.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts_gloom.o: $(SOURCEDIR)/cgame/menu/m_opts_gloom.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts_hud.o: $(SOURCEDIR)/cgame/menu/m_opts_hud.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts_input.o: $(SOURCEDIR)/cgame/menu/m_opts_input.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts_misc.o: $(SOURCEDIR)/cgame/menu/m_opts_misc.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts_screen.o: $(SOURCEDIR)/cgame/menu/m_opts_screen.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_opts_sound.o: $(SOURCEDIR)/cgame/menu/m_opts_sound.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_quit.o: $(SOURCEDIR)/cgame/menu/m_quit.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_sp.o: $(SOURCEDIR)/cgame/menu/m_sp.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_sp_credits.o: $(SOURCEDIR)/cgame/menu/m_sp_credits.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_sp_loadgame.o: $(SOURCEDIR)/cgame/menu/m_sp_loadgame.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_sp_savegame.o: $(SOURCEDIR)/cgame/menu/m_sp_savegame.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_vid.o: $(SOURCEDIR)/cgame/menu/m_vid.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_vid_exts.o: $(SOURCEDIR)/cgame/menu/m_vid_exts.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_vid_settings.o: $(SOURCEDIR)/cgame/menu/m_vid_settings.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/menu.o: $(SOURCEDIR)/cgame/menu/menu.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/ui_backend.o: $(SOURCEDIR)/cgame/ui/ui_backend.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/ui_cursor.o: $(SOURCEDIR)/cgame/ui/ui_cursor.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/ui_draw.o: $(SOURCEDIR)/cgame/ui/ui_draw.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/ui_items.o: $(SOURCEDIR)/cgame/ui/ui_items.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/ui_keys.o: $(SOURCEDIR)/cgame/ui/ui_keys.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/pmove.o: $(SOURCEDIR)/cgame/pmove.c; $(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/byteswap.o: $(SOURCEDIR)/shared/byteswap.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/infostrings.o: $(SOURCEDIR)/shared/infostrings.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_angles.o: $(SOURCEDIR)/shared/m_angles.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_bounds.o: $(SOURCEDIR)/shared/m_bounds.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mat3.o: $(SOURCEDIR)/shared/m_mat3.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_mat4.o: $(SOURCEDIR)/shared/m_mat4.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_plane.o: $(SOURCEDIR)/shared/m_plane.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_quat.o: $(SOURCEDIR)/shared/m_quat.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/mathlib.o: $(SOURCEDIR)/shared/mathlib.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/mersennetwister.o: $(SOURCEDIR)/shared/mersennetwister.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/shared.o: $(SOURCEDIR)/shared/shared.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/string.o: $(SOURCEDIR)/shared/string.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/cgame/m_flash.o: $(SOURCEDIR)/game/m_flash.c; $(DO_SHLIB_CC)

#
# Game objects
#
OBJS_GAME=\
	$(BUILDDIR)/baseq2/game/g_ai.o \
	$(BUILDDIR)/baseq2/game/g_chase.o \
	$(BUILDDIR)/baseq2/game/g_cmds.o \
	$(BUILDDIR)/baseq2/game/g_combat.o \
	$(BUILDDIR)/baseq2/game/g_func.o \
	$(BUILDDIR)/baseq2/game/g_items.o \
	$(BUILDDIR)/baseq2/game/g_main.o \
	$(BUILDDIR)/baseq2/game/g_misc.o \
	$(BUILDDIR)/baseq2/game/g_monster.o \
	$(BUILDDIR)/baseq2/game/g_phys.o \
	$(BUILDDIR)/baseq2/game/g_save.o \
	$(BUILDDIR)/baseq2/game/g_spawn.o \
	$(BUILDDIR)/baseq2/game/g_svcmds.o \
	$(BUILDDIR)/baseq2/game/g_target.o \
	$(BUILDDIR)/baseq2/game/g_trigger.o \
	$(BUILDDIR)/baseq2/game/g_turret.o \
	$(BUILDDIR)/baseq2/game/g_utils.o \
	$(BUILDDIR)/baseq2/game/g_weapon.o \
	\
	$(BUILDDIR)/baseq2/game/m_actor.o \
	$(BUILDDIR)/baseq2/game/m_berserk.o \
	$(BUILDDIR)/baseq2/game/m_boss2.o \
	$(BUILDDIR)/baseq2/game/m_boss31.o \
	$(BUILDDIR)/baseq2/game/m_boss32.o \
	$(BUILDDIR)/baseq2/game/m_boss3.o \
	$(BUILDDIR)/baseq2/game/m_brain.o \
	$(BUILDDIR)/baseq2/game/m_chick.o \
	$(BUILDDIR)/baseq2/game/m_flash.o \
	$(BUILDDIR)/baseq2/game/m_flipper.o \
	$(BUILDDIR)/baseq2/game/m_float.o \
	$(BUILDDIR)/baseq2/game/m_flyer.o \
	$(BUILDDIR)/baseq2/game/m_gladiator.o \
	$(BUILDDIR)/baseq2/game/m_gunner.o \
	$(BUILDDIR)/baseq2/game/m_hover.o \
	$(BUILDDIR)/baseq2/game/m_infantry.o \
	$(BUILDDIR)/baseq2/game/m_insane.o \
	$(BUILDDIR)/baseq2/game/m_medic.o \
	$(BUILDDIR)/baseq2/game/m_move.o \
	$(BUILDDIR)/baseq2/game/m_mutant.o \
	$(BUILDDIR)/baseq2/game/m_parasite.o \
	$(BUILDDIR)/baseq2/game/m_soldier.o \
	$(BUILDDIR)/baseq2/game/m_supertank.o \
	$(BUILDDIR)/baseq2/game/m_tank.o \
	\
	$(BUILDDIR)/baseq2/game/p_client.o \
	$(BUILDDIR)/baseq2/game/p_hud.o \
	$(BUILDDIR)/baseq2/game/p_trail.o \
	$(BUILDDIR)/baseq2/game/p_view.o \
	$(BUILDDIR)/baseq2/game/p_weapon.o \
	\
	$(BUILDDIR)/baseq2/game/byteswap.o \
	$(BUILDDIR)/baseq2/game/infostrings.o \
	$(BUILDDIR)/baseq2/game/m_angles.o \
	$(BUILDDIR)/baseq2/game/m_bounds.o \
	$(BUILDDIR)/baseq2/game/m_mat3.o \
	$(BUILDDIR)/baseq2/game/m_mat4.o \
	$(BUILDDIR)/baseq2/game/m_plane.o \
	$(BUILDDIR)/baseq2/game/m_quat.o \
	$(BUILDDIR)/baseq2/game/mathlib.o \
	$(BUILDDIR)/baseq2/game/mersennetwister.o \
	$(BUILDDIR)/baseq2/game/shared.o \
	$(BUILDDIR)/baseq2/game/string.o \


$(BINDIR)/baseq2/game$(ARCH).$(SHLIBEXT): $(OBJS_GAME)
	@echo
	@echo "** Linking game dll **"
	@echo "$(CC) $(CC_VERSION)";
	@echo "$(CFLAGS)";
	@echo "$(SHLIBLDFLAGS)";
	@echo "$(MODULE_LDFLAGS)";
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJS_GAME) $(MODULE_LDFLAGS)
	@echo

$(BUILDDIR)/baseq2/game/g_ai.o: $(SOURCEDIR)/game/g_ai.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_chase.o: $(SOURCEDIR)/game/g_chase.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_cmds.o: $(SOURCEDIR)/game/g_cmds.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_combat.o: $(SOURCEDIR)/game/g_combat.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_func.o: $(SOURCEDIR)/game/g_func.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_items.o: $(SOURCEDIR)/game/g_items.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_main.o: $(SOURCEDIR)/game/g_main.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_misc.o: $(SOURCEDIR)/game/g_misc.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_monster.o: $(SOURCEDIR)/game/g_monster.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_phys.o: $(SOURCEDIR)/game/g_phys.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_save.o: $(SOURCEDIR)/game/g_save.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_spawn.o: $(SOURCEDIR)/game/g_spawn.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_svcmds.o: $(SOURCEDIR)/game/g_svcmds.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_target.o: $(SOURCEDIR)/game/g_target.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_trigger.o: $(SOURCEDIR)/game/g_trigger.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_turret.o: $(SOURCEDIR)/game/g_turret.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_utils.o: $(SOURCEDIR)/game/g_utils.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/g_weapon.o: $(SOURCEDIR)/game/g_weapon.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_actor.o: $(SOURCEDIR)/game/m_actor.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_berserk.o: $(SOURCEDIR)/game/m_berserk.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_boss2.o: $(SOURCEDIR)/game/m_boss2.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_boss31.o: $(SOURCEDIR)/game/m_boss31.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_boss32.o: $(SOURCEDIR)/game/m_boss32.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_boss3.o: $(SOURCEDIR)/game/m_boss3.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_brain.o: $(SOURCEDIR)/game/m_brain.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_chick.o: $(SOURCEDIR)/game/m_chick.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_flash.o: $(SOURCEDIR)/game/m_flash.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_flipper.o: $(SOURCEDIR)/game/m_flipper.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_float.o: $(SOURCEDIR)/game/m_float.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_flyer.o: $(SOURCEDIR)/game/m_flyer.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_gladiator.o: $(SOURCEDIR)/game/m_gladiator.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_gunner.o: $(SOURCEDIR)/game/m_gunner.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_hover.o: $(SOURCEDIR)/game/m_hover.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_infantry.o: $(SOURCEDIR)/game/m_infantry.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_insane.o: $(SOURCEDIR)/game/m_insane.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_medic.o: $(SOURCEDIR)/game/m_medic.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_move.o: $(SOURCEDIR)/game/m_move.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_mutant.o: $(SOURCEDIR)/game/m_mutant.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_parasite.o: $(SOURCEDIR)/game/m_parasite.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_soldier.o: $(SOURCEDIR)/game/m_soldier.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_supertank.o: $(SOURCEDIR)/game/m_supertank.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_tank.o: $(SOURCEDIR)/game/m_tank.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/p_client.o: $(SOURCEDIR)/game/p_client.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/p_hud.o: $(SOURCEDIR)/game/p_hud.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/p_trail.o: $(SOURCEDIR)/game/p_trail.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/p_view.o: $(SOURCEDIR)/game/p_view.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/p_weapon.o: $(SOURCEDIR)/game/p_weapon.c; $(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/game/byteswap.o: $(SOURCEDIR)/shared/byteswap.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/infostrings.o: $(SOURCEDIR)/shared/infostrings.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_angles.o: $(SOURCEDIR)/shared/m_angles.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_bounds.o: $(SOURCEDIR)/shared/m_bounds.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_mat3.o: $(SOURCEDIR)/shared/m_mat3.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_mat4.o: $(SOURCEDIR)/shared/m_mat4.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_plane.o: $(SOURCEDIR)/shared/m_plane.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/m_quat.o: $(SOURCEDIR)/shared/m_quat.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/mathlib.o: $(SOURCEDIR)/shared/mathlib.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/mersennetwister.o: $(SOURCEDIR)/shared/mersennetwister.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/shared.o: $(SOURCEDIR)/shared/shared.c; $(DO_SHLIB_CC)
$(BUILDDIR)/baseq2/game/string.o: $(SOURCEDIR)/shared/string.c; $(DO_SHLIB_CC)

#
# Parameters
#
clean: clean-debug clean-release

clean-debug:
	$(MAKE) clean2 BUILDDIR=$(BUILD_DEBUG_DIR)

clean-release:
	$(MAKE) clean2 BUILDDIR=$(BUILD_RELEASE_DIR)

clean2:
	@-rm -f \
	$(OBJS_CLIENT) \
	$(OBJS_GLX) \
	$(OBJS_SDL) \
	$(OBJS_DEDICATED) \
	$(OBJS_CGAME) \
	$(OBJS_GAME)
	
clean_bin:
	@echo Cleaning client objects...
	@-rm -f $(BINDIR)/egl* \
	@-rm -f $(BINDIR)/baseq2/*.$(SHLIBEXT) \
	@echo ....................
	@echo .... Done

distclean:
	@printf "* Cleaning objects and binaries\n"
	@-rm -rf $(BUILD_DEBUG_DIR) $(BUILD_RELEASE_DIR) $(BINDIR)
	@-rm -f `find . \( -not -type d \) -and \
		\( -name '*~' \) -type f -print`
	
install:
	@printf "* Copying files to your home dir ...\n"
	@cp -r $(BINDIR) ~/
	@printf "* Done\n"

install_root:
	@mkdir -pv $(DATADIR)
	@printf "* Copying files to $(DATADIR) ...\n"
ifeq ($(OSTYPE),freebsd)
	@cp -rv $(BINDIR) $(LOCALBASE)/share
else
	@cp -rv $(BINDIR) $(LOCALBASE)/games
endif
	@ln -sfv $(DATADIR)/egl $(SYSBINDIR)/egl
ifeq ($(strip $(BUILD_SDL_CLIENT)),YES)
	@ln -sfv $(DATADIR)/egl-sdl $(SYSBINDIR)/egl-sdl
endif
	@printf "* Symlinking executables to $(SYSBINDIR)\n"
ifeq ($(strip $(BUILD_DEDICATED)),YES)
	@ln -sfv $(DATADIR)/eglded $(SYSBINDIR)/eglded
	@printf "* Symlinking dedicated executable to $(SYSBINDIR)\n"
endif
	@printf "* Copy or link into $(DATADIR)/baseq2 the required pak files and players folder\n" 
	@printf "  from your quake2 cdrom in order to run.\n"
	@printf "* Grab also the required file http://svn.quakedev.com/viewcvs.cgi/*checkout*/egl/trunk/assets/egl.pkz\n"
	@printf "* and optional files http://svn.quakedev.com/viewcvs.cgi/egl/trunk/assets/addons.tar.gz\n"
	@printf "  and move it to $(DATADIR)/baseq2\n"
	@printf "* Type egl as user to start the X11-OpenGL client.\n"
ifeq ($(strip $(BUILD_SDL_CLIENT)),YES)
	@printf "* Type egl-sdl as user to start the SDL-OpenGL client.\n"
endif
ifeq ($(strip $(BUILD_DEDICATED)),YES)
	@printf "* Type eglded as user to start a dedicated server.\n"
endif
	@printf "* Done\n"
	
uninstall_root:
	@printf "* Deleting files from $(DATADIR)\n"
	@rm -f $(DATADIR)/egl* $(DATADIR)/baseq2/egl*
	@printf "* Deleting symlinks executables from $(SYSBINDIR)\n"
	@rm -f $(SYSBINDIR)/egl*
	@printf "* Done\n"


