BUILD_CLIENT=YES	# client executable
BUILD_CGAME=YES		# cgame dll
BUILD_UI=YES		# ui dll


VERSION=0.0.6


CC=gcc
EGL_MAKEFILE=makefile
RELEASE_CFLAGS=-Isource/ -I./ -I../ -DXF86 -O2 -fno-strict-aliasing -ffast-math -fexpensive-optimizations
DEBUG_CFLAGS=-g -Isource/ -I./ -I../ -DXF86 -DC_ONLY
LDFLAGS=-L/usr/local/lib -ldl -lm -ljpeg -lz -lpng
MODULE_LDFLAGS=-L/usr/local/lib -ldl -lm
X11_LDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86dga -lXxf86vm


SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared


DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<


# this nice line comes from the linux kernel makefile
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc/ -e s/sparc64/sparc/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/alpha/axp/)
SHLIBEXT =so

BUILD_DEBUG_DIR=debug$(ARCH)
BUILD_RELEASE_DIR=release$(ARCH)

ifeq ($(strip $(BUILD_CLIENT)),YES)
  TARGETS += $(BUILDDIR)/egl
endif

ifeq ($(strip $(BUILD_CGAME)),YES)
 TARGETS += $(BUILDDIR)/baseq2/cgame_$(ARCH).$(SHLIBEXT)
endif

ifeq ($(strip $(BUILD_UI)),YES)
 TARGETS += $(BUILDDIR)/baseq2/ui_$(ARCH).$(SHLIBEXT)
endif



#all: build_debug build_release
#all: build_debug
all: build_release


build_debug:
	@-mkdir -p $(BUILD_DEBUG_DIR) \
		$(BUILD_DEBUG_DIR)/client \
		$(BUILD_DEBUG_DIR)/baseq2 \
		$(BUILD_DEBUG_DIR)/baseq2/cgame \
		$(BUILD_DEBUG_DIR)/baseq2/ui
	$(MAKE) -f $(EGL_MAKEFILE) targets BUILDDIR=$(BUILD_DEBUG_DIR) SOURCEDIR=. CFLAGS="$(DEBUG_CFLAGS) -DLINUX_VERSION='\"$(VERSION) Debug\"'"

build_release:
	@-mkdir -p $(BUILD_RELEASE_DIR) \
		$(BUILD_RELEASE_DIR)/client \
		$(BUILD_RELEASE_DIR)/baseq2 \
		$(BUILD_RELEASE_DIR)/baseq2/cgame \
		$(BUILD_RELEASE_DIR)/baseq2/ui
	$(MAKE) -f $(EGL_MAKEFILE) targets BUILDDIR=$(BUILD_RELEASE_DIR) SOURCEDIR=. CFLAGS="$(RELEASE_CFLAGS) -DLINUX_VERSION='\"$(VERSION)\"'"


targets: $(TARGETS)


OBJS_CLIENT=\
	$(BUILDDIR)/client/alias.o \
	$(BUILDDIR)/client/cbuf.o \
	$(BUILDDIR)/client/cmd.o \
	$(BUILDDIR)/client/cmodel.o \
	$(BUILDDIR)/client/common.o \
	$(BUILDDIR)/client/crc.o \
	$(BUILDDIR)/client/cvar.o \
	$(BUILDDIR)/client/files.o \
	$(BUILDDIR)/client/md4.o \
	$(BUILDDIR)/client/memory.o \
	$(BUILDDIR)/client/net_chan.o \
	$(BUILDDIR)/client/net_msg.o \
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
	$(BUILDDIR)/client/cl_cgapi.o \
	$(BUILDDIR)/client/cl_cin.o \
	$(BUILDDIR)/client/cl_demo.o \
	$(BUILDDIR)/client/cl_input.o \
	$(BUILDDIR)/client/cl_loc.o \
	$(BUILDDIR)/client/cl_main.o \
	$(BUILDDIR)/client/cl_parse.o \
	$(BUILDDIR)/client/cl_screen.o \
	$(BUILDDIR)/client/cl_uiapi.o \
	$(BUILDDIR)/client/console.o \
	$(BUILDDIR)/client/keys.o \
	\
	$(BUILDDIR)/client/snd_dma.o \
	$(BUILDDIR)/client/snd_mix.o \
	\
	$(BUILDDIR)/client/r_alias.o \
	$(BUILDDIR)/client/r_backend.o \
	$(BUILDDIR)/client/r_cin.o \
	$(BUILDDIR)/client/r_cull.o \
	$(BUILDDIR)/client/r_draw.o \
	$(BUILDDIR)/client/r_entity.o \
	$(BUILDDIR)/client/r_glstate.o \
	$(BUILDDIR)/client/r_image.o \
	$(BUILDDIR)/client/r_light.o \
	$(BUILDDIR)/client/r_main.o \
	$(BUILDDIR)/client/r_math.o \
	$(BUILDDIR)/client/r_meshbuffer.o \
	$(BUILDDIR)/client/r_model.o \
	$(BUILDDIR)/client/r_poly.o \
	$(BUILDDIR)/client/r_program.o \
	$(BUILDDIR)/client/r_shader.o \
	$(BUILDDIR)/client/r_shadow.o \
	$(BUILDDIR)/client/r_skeletal.o \
	$(BUILDDIR)/client/r_sprite.o \
	$(BUILDDIR)/client/r_world.o \
	\
	$(BUILDDIR)/client/cd_linux.o \
	$(BUILDDIR)/client/glimp_linux.o \
	$(BUILDDIR)/client/in_x11.o \
	$(BUILDDIR)/client/net_udp.o \
        $(BUILDDIR)/client/qgl_linux.o \
	$(BUILDDIR)/client/snd_linux.o \
	$(BUILDDIR)/client/sys_linux.o \
	$(BUILDDIR)/client/vid_linux.o \
	\
	$(BUILDDIR)/client/infostrings.o \
	$(BUILDDIR)/client/mathlib.o \
	$(BUILDDIR)/client/mersennetwister.o \
	$(BUILDDIR)/client/shared.o \
	$(BUILDDIR)/client/string.o \


$(BUILDDIR)/egl: $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $@ $(OBJS_CLIENT) $(LDFLAGS) $(X11_LDFLAGS)

$(BUILDDIR)/client/alias.o: $(SOURCEDIR)/common/alias.c
	$(DO_CC)

$(BUILDDIR)/client/cbuf.o: $(SOURCEDIR)/common/cbuf.c
	$(DO_CC)

$(BUILDDIR)/client/cmd.o: $(SOURCEDIR)/common/cmd.c
	$(DO_CC)

$(BUILDDIR)/client/cmodel.o: $(SOURCEDIR)/common/cmodel.c
	$(DO_CC)

$(BUILDDIR)/client/common.o: $(SOURCEDIR)/common/common.c
	$(DO_CC)

$(BUILDDIR)/client/crc.o: $(SOURCEDIR)/common/crc.c
	$(DO_CC)

$(BUILDDIR)/client/cvar.o: $(SOURCEDIR)/common/cvar.c
	$(DO_CC)

$(BUILDDIR)/client/files.o: $(SOURCEDIR)/common/files.c
	$(DO_CC)

$(BUILDDIR)/client/md4.o: $(SOURCEDIR)/common/md4.c
	$(DO_CC)

$(BUILDDIR)/client/memory.o: $(SOURCEDIR)/common/memory.c
	$(DO_CC)

$(BUILDDIR)/client/net_chan.o: $(SOURCEDIR)/common/net_chan.c
	$(DO_CC)

$(BUILDDIR)/client/net_msg.o: $(SOURCEDIR)/common/net_msg.c
	$(DO_CC)

$(BUILDDIR)/client/sv_ccmds.o: $(SOURCEDIR)/server/sv_ccmds.c
	$(DO_CC)

$(BUILDDIR)/client/sv_ents.o: $(SOURCEDIR)/server/sv_ents.c
	$(DO_CC)

$(BUILDDIR)/client/sv_gameapi.o: $(SOURCEDIR)/server/sv_gameapi.c
	$(DO_CC)

$(BUILDDIR)/client/sv_init.o: $(SOURCEDIR)/server/sv_init.c
	$(DO_CC)

$(BUILDDIR)/client/sv_main.o: $(SOURCEDIR)/server/sv_main.c
	$(DO_CC)

$(BUILDDIR)/client/sv_pmove.o: $(SOURCEDIR)/server/sv_pmove.c
	$(DO_CC)

$(BUILDDIR)/client/sv_send.o: $(SOURCEDIR)/server/sv_send.c
	$(DO_CC)

$(BUILDDIR)/client/sv_user.o: $(SOURCEDIR)/server/sv_user.c
	$(DO_CC)

$(BUILDDIR)/client/sv_world.o: $(SOURCEDIR)/server/sv_world.c
	$(DO_CC)

$(BUILDDIR)/client/cl_cgapi.o: $(SOURCEDIR)/client/cl_cgapi.c
	$(DO_CC)

$(BUILDDIR)/client/cl_cin.o: $(SOURCEDIR)/client/cl_cin.c
	$(DO_CC)

$(BUILDDIR)/client/cl_demo.o: $(SOURCEDIR)/client/cl_demo.c
	$(DO_CC)

$(BUILDDIR)/client/cl_input.o: $(SOURCEDIR)/client/cl_input.c
	$(DO_CC)

$(BUILDDIR)/client/cl_loc.o: $(SOURCEDIR)/client/cl_loc.c
	$(DO_CC)

$(BUILDDIR)/client/cl_main.o: $(SOURCEDIR)/client/cl_main.c
	$(DO_CC)

$(BUILDDIR)/client/cl_parse.o: $(SOURCEDIR)/client/cl_parse.c
	$(DO_CC)

$(BUILDDIR)/client/cl_screen.o: $(SOURCEDIR)/client/cl_screen.c
	$(DO_CC)

$(BUILDDIR)/client/cl_uiapi.o: $(SOURCEDIR)/client/cl_uiapi.c
	$(DO_CC)

$(BUILDDIR)/client/console.o: $(SOURCEDIR)/client/console.c
	$(DO_CC)

$(BUILDDIR)/client/keys.o: $(SOURCEDIR)/client/keys.c
	$(DO_CC)

$(BUILDDIR)/client/snd_dma.o: $(SOURCEDIR)/sound/snd_dma.c
	$(DO_CC)

$(BUILDDIR)/client/snd_mix.o: $(SOURCEDIR)/sound/snd_mix.c
	$(DO_CC)

$(BUILDDIR)/client/r_alias.o: $(SOURCEDIR)/renderer/r_alias.c
	$(DO_CC)

$(BUILDDIR)/client/r_backend.o: $(SOURCEDIR)/renderer/r_backend.c
	$(DO_CC)

$(BUILDDIR)/client/r_cin.o: $(SOURCEDIR)/renderer/r_cin.c
	$(DO_CC)

$(BUILDDIR)/client/r_cull.o: $(SOURCEDIR)/renderer/r_cull.c
	$(DO_CC)

$(BUILDDIR)/client/r_draw.o: $(SOURCEDIR)/renderer/r_draw.c
	$(DO_CC)

$(BUILDDIR)/client/r_entity.o: $(SOURCEDIR)/renderer/r_entity.c
	$(DO_CC)

$(BUILDDIR)/client/r_glstate.o: $(SOURCEDIR)/renderer/r_glstate.c
	$(DO_CC)

$(BUILDDIR)/client/r_image.o: $(SOURCEDIR)/renderer/r_image.c
	$(DO_CC)

$(BUILDDIR)/client/r_light.o: $(SOURCEDIR)/renderer/r_light.c
	$(DO_CC)

$(BUILDDIR)/client/r_main.o: $(SOURCEDIR)/renderer/r_main.c
	$(DO_CC)

$(BUILDDIR)/client/r_math.o: $(SOURCEDIR)/renderer/r_math.c
	$(DO_CC)

$(BUILDDIR)/client/r_meshbuffer.o: $(SOURCEDIR)/renderer/r_meshbuffer.c
	$(DO_CC)

$(BUILDDIR)/client/r_model.o: $(SOURCEDIR)/renderer/r_model.c
	$(DO_CC)

$(BUILDDIR)/client/r_poly.o: $(SOURCEDIR)/renderer/r_poly.c
	$(DO_CC)

$(BUILDDIR)/client/r_program.o: $(SOURCEDIR)/renderer/r_program.c
	$(DO_CC)

$(BUILDDIR)/client/r_shader.o: $(SOURCEDIR)/renderer/r_shader.c
	$(DO_CC)

$(BUILDDIR)/client/r_shadow.o: $(SOURCEDIR)/renderer/r_shadow.c
	$(DO_CC)

$(BUILDDIR)/client/r_skeletal.o: $(SOURCEDIR)/renderer/r_skeletal.c
	$(DO_CC)

$(BUILDDIR)/client/r_sprite.o: $(SOURCEDIR)/renderer/r_sprite.c
	$(DO_CC)

$(BUILDDIR)/client/r_world.o: $(SOURCEDIR)/renderer/r_world.c
	$(DO_CC)

$(BUILDDIR)/client/cd_linux.o: $(SOURCEDIR)/linux/cd_linux.c
	$(DO_CC)

$(BUILDDIR)/client/glimp_linux.o: $(SOURCEDIR)/linux/glimp_linux.c
	$(DO_CC)

$(BUILDDIR)/client/in_x11.o: $(SOURCEDIR)/linux/in_x11.c
	$(DO_CC)

$(BUILDDIR)/client/net_udp.o: $(SOURCEDIR)/linux/net_udp.c
	$(DO_CC)

$(BUILDDIR)/client/qgl_linux.o: $(SOURCEDIR)/linux/qgl_linux.c
	$(DO_CC)

$(BUILDDIR)/client/snd_linux.o: $(SOURCEDIR)/linux/snd_linux.c
	$(DO_CC)

$(BUILDDIR)/client/sys_linux.o: $(SOURCEDIR)/linux/sys_linux.c
	$(DO_CC)

$(BUILDDIR)/client/vid_linux.o: $(SOURCEDIR)/linux/vid_linux.c
	$(DO_CC)

$(BUILDDIR)/client/infostrings.o: $(SOURCEDIR)/shared/infostrings.c
	$(DO_CC)

$(BUILDDIR)/client/mathlib.o: $(SOURCEDIR)/shared/mathlib.c
	$(DO_CC)

$(BUILDDIR)/client/mersennetwister.o: $(SOURCEDIR)/shared/mersennetwister.c
	$(DO_CC)

$(BUILDDIR)/client/shared.o: $(SOURCEDIR)/shared/shared.c
	$(DO_CC)

$(BUILDDIR)/client/string.o: $(SOURCEDIR)/shared/string.c
	$(DO_CC)


OBJS_CGAME=\
	$(BUILDDIR)/baseq2/cgame/cg_api.o \
	$(BUILDDIR)/baseq2/cgame/cg_decals.o \
	$(BUILDDIR)/baseq2/cgame/cg_draw.o \
	$(BUILDDIR)/baseq2/cgame/cg_entities.o \
	$(BUILDDIR)/baseq2/cgame/cg_hud.o \
	$(BUILDDIR)/baseq2/cgame/cg_inventory.o \
	$(BUILDDIR)/baseq2/cgame/cg_light.o \
	$(BUILDDIR)/baseq2/cgame/cg_localents.o \
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
	$(BUILDDIR)/baseq2/cgame/pmove.o \
	\
	$(BUILDDIR)/baseq2/cgame/infostrings.o \
	$(BUILDDIR)/baseq2/cgame/mathlib.o \
	$(BUILDDIR)/baseq2/cgame/mersennetwister.o \
	$(BUILDDIR)/baseq2/cgame/shared.o \
	$(BUILDDIR)/baseq2/cgame/string.o \


$(BUILDDIR)/baseq2/cgame_$(ARCH).$(SHLIBEXT): $(OBJS_CGAME)
	@echo Linking cgame dll;
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJS_CGAME) $(MODULE_LDFLAGS)

$(BUILDDIR)/baseq2/cgame/cg_api.o: $(SOURCEDIR)/cgame/cg_api.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_decals.o: $(SOURCEDIR)/cgame/cg_decals.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_draw.o: $(SOURCEDIR)/cgame/cg_draw.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_entities.o: $(SOURCEDIR)/cgame/cg_entities.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_hud.o: $(SOURCEDIR)/cgame/cg_hud.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_inventory.o: $(SOURCEDIR)/cgame/cg_inventory.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_light.o: $(SOURCEDIR)/cgame/cg_light.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_localents.o: $(SOURCEDIR)/cgame/cg_localents.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_main.o: $(SOURCEDIR)/cgame/cg_main.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_mapeffects.o: $(SOURCEDIR)/cgame/cg_mapeffects.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_media.o: $(SOURCEDIR)/cgame/cg_media.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_muzzleflash.o: $(SOURCEDIR)/cgame/cg_muzzleflash.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_parse.o: $(SOURCEDIR)/cgame/cg_parse.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_parteffects.o: $(SOURCEDIR)/cgame/cg_parteffects.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_partgloom.o: $(SOURCEDIR)/cgame/cg_partgloom.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_particles.o: $(SOURCEDIR)/cgame/cg_particles.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_partsustain.o: $(SOURCEDIR)/cgame/cg_partsustain.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_partthink.o: $(SOURCEDIR)/cgame/cg_partthink.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_parttrail.o: $(SOURCEDIR)/cgame/cg_parttrail.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_players.o: $(SOURCEDIR)/cgame/cg_players.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_predict.o: $(SOURCEDIR)/cgame/cg_predict.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_screen.o: $(SOURCEDIR)/cgame/cg_screen.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_tempents.o: $(SOURCEDIR)/cgame/cg_tempents.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_view.o: $(SOURCEDIR)/cgame/cg_view.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/cg_weapon.o: $(SOURCEDIR)/cgame/cg_weapon.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/pmove.o: $(SOURCEDIR)/cgame/pmove.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/infostrings.o: $(SOURCEDIR)/shared/infostrings.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/mathlib.o: $(SOURCEDIR)/shared/mathlib.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/mersennetwister.o: $(SOURCEDIR)/shared/mersennetwister.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/shared.o: $(SOURCEDIR)/shared/shared.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/cgame/string.o: $(SOURCEDIR)/shared/string.c
	$(DO_SHLIB_CC)


OBJS_UI=\
	$(BUILDDIR)/baseq2/ui/ui_addrbook.o \
	$(BUILDDIR)/baseq2/ui/ui_api.o \
	$(BUILDDIR)/baseq2/ui/ui_backend.o \
	$(BUILDDIR)/baseq2/ui/ui_common.o \
	$(BUILDDIR)/baseq2/ui/ui_controls.o \
	$(BUILDDIR)/baseq2/ui/ui_credits.o \
	$(BUILDDIR)/baseq2/ui/ui_cursor.o \
	$(BUILDDIR)/baseq2/ui/ui_dlopts.o \
	$(BUILDDIR)/baseq2/ui/ui_dmflags.o \
	$(BUILDDIR)/baseq2/ui/ui_draw.o \
	$(BUILDDIR)/baseq2/ui/ui_effects.o \
	$(BUILDDIR)/baseq2/ui/ui_game.o \
	$(BUILDDIR)/baseq2/ui/ui_glexts.o \
	$(BUILDDIR)/baseq2/ui/ui_gloom.o \
	$(BUILDDIR)/baseq2/ui/ui_hud.o \
	$(BUILDDIR)/baseq2/ui/ui_input.o \
	$(BUILDDIR)/baseq2/ui/ui_joinserver.o \
	$(BUILDDIR)/baseq2/ui/ui_keys.o \
	$(BUILDDIR)/baseq2/ui/ui_loadgame.o \
	$(BUILDDIR)/baseq2/ui/ui_loadscreen.o \
	$(BUILDDIR)/baseq2/ui/ui_mainmenu.o \
	$(BUILDDIR)/baseq2/ui/ui_misc.o \
	$(BUILDDIR)/baseq2/ui/ui_multiplayer.o \
	$(BUILDDIR)/baseq2/ui/ui_options.o \
	$(BUILDDIR)/baseq2/ui/ui_playercfg.o \
	$(BUILDDIR)/baseq2/ui/ui_quit.o \
	$(BUILDDIR)/baseq2/ui/ui_savegame.o \
	$(BUILDDIR)/baseq2/ui/ui_screen.o \
	$(BUILDDIR)/baseq2/ui/ui_sound.o \
	$(BUILDDIR)/baseq2/ui/ui_startserver.o \
	$(BUILDDIR)/baseq2/ui/ui_video.o \
	$(BUILDDIR)/baseq2/ui/ui_vidsettings.o \
	\
	$(BUILDDIR)/baseq2/ui/infostrings.o \
	$(BUILDDIR)/baseq2/ui/mathlib.o \
	$(BUILDDIR)/baseq2/ui/mersennetwister.o \
	$(BUILDDIR)/baseq2/ui/shared.o \
	$(BUILDDIR)/baseq2/ui/string.o \

$(BUILDDIR)/baseq2/ui_$(ARCH).$(SHLIBEXT): $(OBJS_UI)
	@echo Linking ui dll;
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJS_UI) $(MODULE_LDFLAGS)

$(BUILDDIR)/baseq2/ui/ui_addrbook.o: $(SOURCEDIR)/ui/ui_addrbook.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_api.o: $(SOURCEDIR)/ui/ui_api.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_backend.o: $(SOURCEDIR)/ui/ui_backend.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_common.o: $(SOURCEDIR)/ui/ui_common.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_controls.o: $(SOURCEDIR)/ui/ui_controls.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_credits.o: $(SOURCEDIR)/ui/ui_credits.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_cursor.o: $(SOURCEDIR)/ui/ui_cursor.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_dlopts.o: $(SOURCEDIR)/ui/ui_dlopts.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_dmflags.o: $(SOURCEDIR)/ui/ui_dmflags.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_draw.o: $(SOURCEDIR)/ui/ui_draw.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_effects.o: $(SOURCEDIR)/ui/ui_effects.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_game.o: $(SOURCEDIR)/ui/ui_game.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_glexts.o: $(SOURCEDIR)/ui/ui_glexts.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_gloom.o: $(SOURCEDIR)/ui/ui_gloom.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_hud.o: $(SOURCEDIR)/ui/ui_hud.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_input.o: $(SOURCEDIR)/ui/ui_input.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_joinserver.o: $(SOURCEDIR)/ui/ui_joinserver.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_keys.o: $(SOURCEDIR)/ui/ui_keys.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_loadgame.o: $(SOURCEDIR)/ui/ui_loadgame.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_loadscreen.o: $(SOURCEDIR)/ui/ui_loadscreen.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_mainmenu.o: $(SOURCEDIR)/ui/ui_mainmenu.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_misc.o: $(SOURCEDIR)/ui/ui_misc.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_multiplayer.o: $(SOURCEDIR)/ui/ui_multiplayer.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_options.o: $(SOURCEDIR)/ui/ui_options.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_playercfg.o: $(SOURCEDIR)/ui/ui_playercfg.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_quit.o: $(SOURCEDIR)/ui/ui_quit.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_savegame.o: $(SOURCEDIR)/ui/ui_savegame.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_screen.o: $(SOURCEDIR)/ui/ui_screen.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_sound.o: $(SOURCEDIR)/ui/ui_sound.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_startserver.o: $(SOURCEDIR)/ui/ui_startserver.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_video.o: $(SOURCEDIR)/ui/ui_video.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/ui_vidsettings.o: $(SOURCEDIR)/ui/ui_vidsettings.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/infostrings.o: $(SOURCEDIR)/shared/infostrings.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/mathlib.o: $(SOURCEDIR)/shared/mathlib.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/mersennetwister.o: $(SOURCEDIR)/shared/mersennetwister.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/shared.o: $(SOURCEDIR)/shared/shared.c
	$(DO_SHLIB_CC)

$(BUILDDIR)/baseq2/ui/string.o: $(SOURCEDIR)/shared/string.c
	$(DO_SHLIB_CC)


clean: clean-debug clean-release

clean-debug:
	$(MAKE) -f $(EGL_MAKEFILE) clean2 BUILDDIR=$(BUILD_DEBUG_DIR)

clean-release:
	$(MAKE) -f $(EGL_MAKEFILE) clean2 BUILDDIR=$(BUILD_RELEASE_DIR)

clean2:
	@-rm -f \
	$(OBJS_CLIENT) \
	$(OBJS_CGAME) \
	$(OBJS_UI)

distclean:
	@-rm -rf $(BUILD_DEBUG_DIR) $(BUILD_RELEASE_DIR)
