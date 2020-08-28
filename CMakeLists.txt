cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# Enforce "Debug" as standard build type.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# CMake project configuration.
project(EGL)

# Cmake module search path.
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/stuff/cmake/modules ${CMAKE_MODULE_PATH})

# Add extended path for FreeBSD and Homebrew on OS X.
list(APPEND CMAKE_PREFIX_PATH /usr/local)

# Enforce C compiler flags:
#  -Wall                -> More warnings
#  -fno-strict-aliasing -> Quake 2 is far away from strict aliasing
#  -fwrapv              -> Make signed integer overflows defined
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -fno-strict-aliasing -fwrapv")

# Enforce CXX compiler flags:
#  -Wall                -> More warnings
#  -fno-strict-aliasing -> Quake 2 is far away from strict aliasing
#  -fwrapv              -> Make signed integer overflows defined
#  -fno-writable-strings	-> Prevent 'warning: ISO C++11 does not allow conversion from string literal to 'char *' [-Wwritable-strings]' warnings.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -fno-strict-aliasing -Wno-writable-strings -fwrapv")

# Switch of some annoying warnings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	if (CMAKE_CXX_COMPILER_VERSION GREATER 7.99)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation -Wno-format-overflow")
	endif()
endif()

# Use -O2 as maximum optimization level. -O3 has it's problems with GoldQuake2.
string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Build Targets.
option(BUILD_EGL "Build GoldQuake II executable" ON)
option(BUILD_EGL_DED "Build GoldQuake II dedicated server executable" OFF)
option(BUILD_EGL_GAME "Build Game DLL" ON)
option(BUILD_EGL_REFGL "Build OpenGL Renderer" ON)

# Set directory locations (allowing us to move directories easily)
set(EGL_SRCDIR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(EGL_SRCDIR_CGAME ${EGL_SRCDIR_ROOT_DIR}/cgame)
set(EGL_SRCDIR_CLIENT ${EGL_SRCDIR_ROOT_DIR}/client)
set(EGL_SRCDIR_COMMON ${EGL_SRCDIR_ROOT_DIR}/common)
set(EGL_SRCDIR_GAME ${EGL_SRCDIR_ROOT_DIR}/game)
set(EGL_SRCDIR_RENDERER ${EGL_SRCDIR_ROOT_DIR}/renderer)
set(EGL_SRCDIR_SERVER ${EGL_SRCDIR_ROOT_DIR}/server)
set(EGL_SRCDIR_SHARED ${EGL_SRCDIR_ROOT_DIR}/shared)
set(EGL_SRCDIR_UNIX ${EGL_SRCDIR_ROOT_DIR}/unix)
set(EGL_SRCDIR_WIN32_DIR ${EGL_SRCDIR_ROOT_DIR}/win32)

# Required libraries to build the different components of the binaries. Find
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)

# General linker flags.
set(EGL_LINKER_FLAGS "")
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND EGL_LINKER_FLAGS "-lm -static-libgcc")
else()
	if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Haiku")
		list(APPEND EGL_LINKER_FLAGS "-lm -rdynamic")
	else()
		list(APPEND EGL_LINKER_FLAGS "-lm -lnetwork")
	endif()
endif()

list(APPEND EGL_LINKER_FLAGS ${CMAKE_DL_LIBS})

###
# Set CGAME Source Files.
###
set(EGL_SRC_CGAME
    ${EGL_SRCDIR_CGAME}/gmenu/menu_id_1.c
    
    ${EGL_SRCDIR_CGAME}/menu/m_local.h
    ${EGL_SRCDIR_CGAME}/menu/m_main.c 
    ${EGL_SRCDIR_CGAME}/menu/m_mp.c 
    ${EGL_SRCDIR_CGAME}/menu/m_mp_downloading.c
    ${EGL_SRCDIR_CGAME}/menu/m_mp_join.c 
    ${EGL_SRCDIR_CGAME}/menu/m_mp_join_addrbook.c 
    ${EGL_SRCDIR_CGAME}/menu/m_mp_player.c 
    ${EGL_SRCDIR_CGAME}/menu/m_mp_start.c 
    ${EGL_SRCDIR_CGAME}/menu/m_mp_start_dmflags.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts_controls.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts_effects.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts_gloom.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts_hud.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts_input.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts_misc.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts_screen.c 
    ${EGL_SRCDIR_CGAME}/menu/m_opts_sound.c 
    ${EGL_SRCDIR_CGAME}/menu/m_quit.c
    ${EGL_SRCDIR_CGAME}/menu/m_sp.c
    ${EGL_SRCDIR_CGAME}/menu/m_sp_credits.c 
    ${EGL_SRCDIR_CGAME}/menu/m_sp_loadgame.c 
    ${EGL_SRCDIR_CGAME}/menu/m_sp_savegame.c 
    ${EGL_SRCDIR_CGAME}/menu/m_vid.c
    ${EGL_SRCDIR_CGAME}/menu/m_vid_exts.c
    ${EGL_SRCDIR_CGAME}/menu/m_vid_settings.c 
    ${EGL_SRCDIR_CGAME}/menu/menu.c
    ${EGL_SRCDIR_CGAME}/menu/menu.h
    
    ${EGL_SRCDIR_CGAME}/ui/ui_backend.c
    ${EGL_SRCDIR_CGAME}/ui/ui_cursor.c
    ${EGL_SRCDIR_CGAME}/ui/ui_draw.c
    ${EGL_SRCDIR_CGAME}/ui/ui_items.c
    ${EGL_SRCDIR_CGAME}/ui/ui_keys.c
    ${EGL_SRCDIR_CGAME}/ui/ui_local.h
    
    ${EGL_SRCDIR_CGAME}/cg_api.c 
    ${EGL_SRCDIR_CGAME}/cg_decals.c 
    ${EGL_SRCDIR_CGAME}/cg_draw.c 
    ${EGL_SRCDIR_CGAME}/cg_entities.c
    ${EGL_SRCDIR_CGAME}/cg_hud.c 
    ${EGL_SRCDIR_CGAME}/cg_inventory.c 
    ${EGL_SRCDIR_CGAME}/cg_keys.c 
    ${EGL_SRCDIR_CGAME}/cg_light.c 
    ${EGL_SRCDIR_CGAME}/cg_loadscreen.c 
    ${EGL_SRCDIR_CGAME}/cg_localents.c 
    ${EGL_SRCDIR_CGAME}/cg_location.c 
    ${EGL_SRCDIR_CGAME}/cg_main.c 
    ${EGL_SRCDIR_CGAME}/cg_mapeffects.c 
    ${EGL_SRCDIR_CGAME}/cg_media.c 
    ${EGL_SRCDIR_CGAME}/cg_muzzleflash.c 
    ${EGL_SRCDIR_CGAME}/cg_parse.c 
    ${EGL_SRCDIR_CGAME}/cg_parteffects.c 
    ${EGL_SRCDIR_CGAME}/cg_partgloom.c 
    ${EGL_SRCDIR_CGAME}/cg_particles.c 
    ${EGL_SRCDIR_CGAME}/cg_partsustain.c 
    ${EGL_SRCDIR_CGAME}/cg_partthink.c 
    ${EGL_SRCDIR_CGAME}/cg_parttrail.c 
    ${EGL_SRCDIR_CGAME}/cg_players.c 
    ${EGL_SRCDIR_CGAME}/cg_predict.c 
    ${EGL_SRCDIR_CGAME}/cg_screen.c 
    ${EGL_SRCDIR_CGAME}/cg_tempents.c
    ${EGL_SRCDIR_CGAME}/cg_view.c 
    ${EGL_SRCDIR_CGAME}/cg_weapons.c 
    
    ${EGL_SRCDIR_CGAME}/pmove.c
)

###
# Set GAME Source Files.
###
set(EGL_SRC_DLLGAME

)

###
# Set ENGINE EXECUTABLE Source Files.
###
set(EGL_SRC_ENGINE
    ${EGL_SRCDIR_CLIENT}/cl_acapi.c
    ${EGL_SRCDIR_CLIENT}/cl_cgapi.c 
    ${EGL_SRCDIR_CLIENT}/cl_cin.c 
    ${EGL_SRCDIR_CLIENT}/cl_console.c 
    ${EGL_SRCDIR_CLIENT}/cl_demo.c 
    ${EGL_SRCDIR_CLIENT}/cl_download.c 
    ${EGL_SRCDIR_CLIENT}/cl_input.c 
    ${EGL_SRCDIR_CLIENT}/cl_keys.c 
    ${EGL_SRCDIR_CLIENT}/cl_main.c 
    ${EGL_SRCDIR_CLIENT}/cl_parse.c 
    ${EGL_SRCDIR_CLIENT}/cl_screen.c 
    ${EGL_SRCDIR_CLIENT}/gui_cursor.c 
    ${EGL_SRCDIR_CLIENT}/gui_draw.c 
    ${EGL_SRCDIR_CLIENT}/gui_events.c 
    ${EGL_SRCDIR_CLIENT}/gui_items.c 
    ${EGL_SRCDIR_CLIENT}/gui_keys.c 
    ${EGL_SRCDIR_CLIENT}/gui_main.c 
    ${EGL_SRCDIR_CLIENT}/gui_vars.c 
    ${EGL_SRCDIR_CLIENT}/snd_dma.c 
    ${EGL_SRCDIR_CLIENT}/snd_main.c
    ${EGL_SRCDIR_CLIENT}/snd_openal.c

    ${EGL_SRCDIR_COMMON}/alias.c
    ${EGL_SRCDIR_COMMON}/cbuf.c 
    ${EGL_SRCDIR_COMMON}/cm_common.c 
    ${EGL_SRCDIR_COMMON}/cm_q2_main.c
    ${EGL_SRCDIR_COMMON}/cm_q2_trace.c
    ${EGL_SRCDIR_COMMON}/cm_q3_main.c
    ${EGL_SRCDIR_COMMON}/cm_q3_trace.c 
    ${EGL_SRCDIR_COMMON}/cmd.c 
    ${EGL_SRCDIR_COMMON}/common.c 
    ${EGL_SRCDIR_COMMON}/crc.c 
    ${EGL_SRCDIR_COMMON}/cvar.c 
    ${EGL_SRCDIR_COMMON}/files.c 
    ${EGL_SRCDIR_COMMON}/md4.c 
    ${EGL_SRCDIR_COMMON}/memory.c 
    ${EGL_SRCDIR_COMMON}/net_chan.c 
    ${EGL_SRCDIR_COMMON}/net_msg.c 
    ${EGL_SRCDIR_COMMON}/parse.c

    ${EGL_SRCDIR_RENDERER}/r_math.c
    ${EGL_SRCDIR_RENDERER}/rb_batch.c 
    ${EGL_SRCDIR_RENDERER}/rb_entity.c 
    ${EGL_SRCDIR_RENDERER}/rb_light.c 
    ${EGL_SRCDIR_RENDERER}/rb_math.c 
    ${EGL_SRCDIR_RENDERER}/rb_qgl.c 
    ${EGL_SRCDIR_RENDERER}/rb_render.c 
    ${EGL_SRCDIR_RENDERER}/rb_shadow.c 
    ${EGL_SRCDIR_RENDERER}/rb_state.c 

    ${EGL_SRCDIR_RENDERER}/rf_2d.c 
    ${EGL_SRCDIR_RENDERER}/rf_alias.c 
    ${EGL_SRCDIR_RENDERER}/rf_cull.c 
    ${EGL_SRCDIR_RENDERER}/rf_decal.c 
    ${EGL_SRCDIR_RENDERER}/rf_font.c 
    ${EGL_SRCDIR_RENDERER}/rf_image.c 
    ${EGL_SRCDIR_RENDERER}/rf_init.c 
    ${EGL_SRCDIR_RENDERER}/rf_light.c
    ${EGL_SRCDIR_RENDERER}/rf_main.c 
    ${EGL_SRCDIR_RENDERER}/rf_material.c 
    ${EGL_SRCDIR_RENDERER}/rf_meshbuffer.c 
    ${EGL_SRCDIR_RENDERER}/rf_model.c 
    ${EGL_SRCDIR_RENDERER}/rf_program.c 
    ${EGL_SRCDIR_RENDERER}/rf_sky.c 
    ${EGL_SRCDIR_RENDERER}/rf_sprite.c 
    ${EGL_SRCDIR_RENDERER}/rf_world.c

    ${EGL_SRCDIR_SERVER}/sv_ccmds.c 
    ${EGL_SRCDIR_SERVER}/sv_ents.c 
    ${EGL_SRCDIR_SERVER}/sv_gameapi.c 
    ${EGL_SRCDIR_SERVER}/sv_init.c 
    ${EGL_SRCDIR_SERVER}/sv_main.c 
    ${EGL_SRCDIR_SERVER}/sv_pmove.c 
    ${EGL_SRCDIR_SERVER}/sv_send.c 
    ${EGL_SRCDIR_SERVER}/sv_user.c 
    ${EGL_SRCDIR_SERVER}/sv_world.c 

    ${EGL_SRCDIR_SHARED}/byteswap.c
    ${EGL_SRCDIR_SHARED}/infostrings.c 
    ${EGL_SRCDIR_SHARED}/m_angles.c 
    ${EGL_SRCDIR_SHARED}/m_bounds.c 
    ${EGL_SRCDIR_SHARED}/m_flash.c 
    ${EGL_SRCDIR_SHARED}/m_mat3.c
    ${EGL_SRCDIR_SHARED}/m_mat4.c
    ${EGL_SRCDIR_SHARED}/m_plane.c 
    ${EGL_SRCDIR_SHARED}/m_quat.c 
    ${EGL_SRCDIR_SHARED}/mathlib.c 
    ${EGL_SRCDIR_SHARED}/mersennetwister.c 
    ${EGL_SRCDIR_SHARED}/shared.c 
    ${EGL_SRCDIR_SHARED}/string.c 
)

# Append the proper Operating System specific code to the executable file its sources.
if(UNIX)
    list(APPEND EGL_SRC_ENGINE
        {EGL_SRCDIR_UNIX}/unix_console.c    
        {EGL_SRCDIR_UNIX}/unix_glimp.c
        {EGL_SRCDIR_UNIX}/unix_main.c 
        {EGL_SRCDIR_UNIX}/unix_snd_alsa.c 
        {EGL_SRCDIR_UNIX}/unix_snd_cd.c 
        {EGL_SRCDIR_UNIX}/unix_snd_main.c 
        {EGL_SRCDIR_UNIX}/unix_snd_oss.c
        {EGL_SRCDIR_UNIX}/unix_snd_sdl.c
        {EGL_SRCDIR_UNIX}/unix_udp.c
        {EGL_SRCDIR_UNIX}/x11_main.c 
        {EGL_SRCDIR_UNIX}/x11_utils.c
    )
elseif(WIN32)

endif()

set(Backends-Unix-Source
	${BACKENDS_SRC_DIR}/unix/main.cpp
	${BACKENDS_SRC_DIR}/unix/network.cpp
	${BACKENDS_SRC_DIR}/unix/signalhandler.cpp
	${BACKENDS_SRC_DIR}/unix/system.cpp
	${BACKENDS_SRC_DIR}/unix/shared/hunk.cpp
	)

set(Backends-Windows-Source
	${BACKENDS_SRC_DIR}/windows/icon.rc
	${BACKENDS_SRC_DIR}/windows/main.cpp
	${BACKENDS_SRC_DIR}/windows/network.cpp
	${BACKENDS_SRC_DIR}/windows/system.cpp
	${BACKENDS_SRC_DIR}/windows/shared/hunk.cpp
	)

set(Backends-Windows-Header
	${BACKENDS_SRC_DIR}/windows/header/resource.h
	)

set(REF-Windows-Source
	${BACKENDS_SRC_DIR}/windows/shared/hunk.cpp
	)

set(REF-Unix-Source
	${BACKENDS_SRC_DIR}/unix/shared/hunk.cpp
	)

# Set the nessesary platform specific source
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(Platform-Specific-Source ${Backends-Windows-Source} ${Backends-Windows-Header})
	set(REF-Platform-Specific-Source ${REF-Windows-Source})
else()
	set(Platform-Specific-Source ${Backends-Unix-Source})
	set(REF-Platform-Specific-Source ${REF-Unix-Source})
endif()

set(Game-Source
	${COMMON_SRC_DIR}/shared/flash.cpp
	${COMMON_SRC_DIR}/shared/rand.cpp
	${COMMON_SRC_DIR}/shared/shared.cpp
	${GAME_SRC_DIR}/g_ai.cpp
	${GAME_SRC_DIR}/g_chase.cpp
	${GAME_SRC_DIR}/g_cmds.cpp
	${GAME_SRC_DIR}/g_combat.cpp
	${GAME_SRC_DIR}/g_func.cpp
	${GAME_SRC_DIR}/g_items.cpp
	${GAME_SRC_DIR}/g_main.cpp
	${GAME_SRC_DIR}/g_misc.cpp
	${GAME_SRC_DIR}/g_monster.cpp
	${GAME_SRC_DIR}/g_phys.cpp
	${GAME_SRC_DIR}/g_spawn.cpp
	${GAME_SRC_DIR}/g_svcmds.cpp
	${GAME_SRC_DIR}/g_target.cpp
	${GAME_SRC_DIR}/g_trigger.cpp
	${GAME_SRC_DIR}/g_turret.cpp
	${GAME_SRC_DIR}/g_utils.cpp
	${GAME_SRC_DIR}/g_weapon.cpp
	${GAME_SRC_DIR}/monster/berserker/berserker.cpp
	${GAME_SRC_DIR}/monster/boss2/boss2.cpp
	${GAME_SRC_DIR}/monster/boss3/boss3.cpp
	${GAME_SRC_DIR}/monster/boss3/boss31.cpp
	${GAME_SRC_DIR}/monster/boss3/boss32.cpp
	${GAME_SRC_DIR}/monster/brain/brain.cpp
#	${GAME_SRC_DIR}/monster/chick/chick.cpp
#	${GAME_SRC_DIR}/monster/flipper/flipper.cpp
#	${GAME_SRC_DIR}/monster/float/float.cpp
#	${GAME_SRC_DIR}/monster/flyer/flyer.cpp
#	${GAME_SRC_DIR}/monster/gladiator/gladiator.cpp
#	${GAME_SRC_DIR}/monster/gunner/gunner.cpp
#	${GAME_SRC_DIR}/monster/hover/hover.cpp
#	${GAME_SRC_DIR}/monster/infantry/infantry.cpp
#	${GAME_SRC_DIR}/monster/insane/insane.cpp
#	${GAME_SRC_DIR}/monster/medic/medic.cpp
	${GAME_SRC_DIR}/monster/misc/move.cpp
#	${GAME_SRC_DIR}/monster/mutant/mutant.cpp
#	${GAME_SRC_DIR}/monster/parasite/parasite.cpp
#	${GAME_SRC_DIR}/monster/soldier/soldier.cpp
#	${GAME_SRC_DIR}/monster/supertank/supertank.cpp
#	${GAME_SRC_DIR}/monster/tank/tank.c
	${GAME_SRC_DIR}/player/client.cpp
	${GAME_SRC_DIR}/player/hud.cpp
	${GAME_SRC_DIR}/player/trail.cpp
	${GAME_SRC_DIR}/player/view.cpp
	${GAME_SRC_DIR}/player/weapon.cpp
	${GAME_SRC_DIR}/savegame/savegame.c
	)

set(Game-Header
	${GAME_SRC_DIR}/header/game.h
	${GAME_SRC_DIR}/header/local.h
	${GAME_SRC_DIR}/monster/berserker/berserker.h
	${GAME_SRC_DIR}/monster/boss2/boss2.h
	${GAME_SRC_DIR}/monster/boss3/boss31.h
	${GAME_SRC_DIR}/monster/boss3/boss32.h
	${GAME_SRC_DIR}/monster/brain/brain.h
	${GAME_SRC_DIR}/monster/chick/chick.h
	${GAME_SRC_DIR}/monster/flipper/flipper.h
	${GAME_SRC_DIR}/monster/float/float.h
	${GAME_SRC_DIR}/monster/flyer/flyer.h
	${GAME_SRC_DIR}/monster/gladiator/gladiator.h
	${GAME_SRC_DIR}/monster/gunner/gunner.h
	${GAME_SRC_DIR}/monster/hover/hover.h
	${GAME_SRC_DIR}/monster/infantry/infantry.h
	${GAME_SRC_DIR}/monster/insane/insane.h
	${GAME_SRC_DIR}/monster/medic/medic.h
	${GAME_SRC_DIR}/monster/misc/player.h
	${GAME_SRC_DIR}/monster/mutant/mutant.h
	${GAME_SRC_DIR}/monster/parasite/parasite.h
	${GAME_SRC_DIR}/monster/soldier/soldier.h
	${GAME_SRC_DIR}/monster/supertank/supertank.h
	${GAME_SRC_DIR}/monster/tank/tank.h
	${GAME_SRC_DIR}/savegame/tables/clientfields.h
	${GAME_SRC_DIR}/savegame/tables/fields.h
	${GAME_SRC_DIR}/savegame/tables/gamefunc_decs.h
	${GAME_SRC_DIR}/savegame/tables/gamefunc_list.h
	${GAME_SRC_DIR}/savegame/tables/gamemmove_decs.h
	${GAME_SRC_DIR}/savegame/tables/gamemmove_list.h
	${GAME_SRC_DIR}/savegame/tables/levelfields.h
	)

set(Client-Source
	${CLIENT_SRC_DIR}/cl_cin.cpp
	${CLIENT_SRC_DIR}/cl_console.cpp
	${CLIENT_SRC_DIR}/cl_download.cpp
	${CLIENT_SRC_DIR}/cl_effects.cpp
	${CLIENT_SRC_DIR}/cl_entities.cpp
	${CLIENT_SRC_DIR}/cl_input.cpp
	${CLIENT_SRC_DIR}/cl_inventory.cpp
	${CLIENT_SRC_DIR}/cl_keyboard.cpp
	${CLIENT_SRC_DIR}/cl_lights.cpp
	${CLIENT_SRC_DIR}/cl_main.cpp
	${CLIENT_SRC_DIR}/cl_network.cpp
	${CLIENT_SRC_DIR}/cl_parse.cpp
	${CLIENT_SRC_DIR}/cl_particles.cpp
	${CLIENT_SRC_DIR}/cl_prediction.cpp
	${CLIENT_SRC_DIR}/cl_screen.cpp
	${CLIENT_SRC_DIR}/cl_tempentities.cpp
	${CLIENT_SRC_DIR}/cl_view.cpp
	${CLIENT_SRC_DIR}/curl/download.cpp
	${CLIENT_SRC_DIR}/curl/qcurl.cpp
	${CLIENT_SRC_DIR}/input/sdl.cpp
	${CLIENT_SRC_DIR}/menu/menu.cpp
	${CLIENT_SRC_DIR}/menu/qmenu.cpp
	${CLIENT_SRC_DIR}/menu/videomenu.cpp
	${CLIENT_SRC_DIR}/sound/ogg.cpp
	${CLIENT_SRC_DIR}/sound/openal.cpp
	${CLIENT_SRC_DIR}/sound/qal.cpp
	${CLIENT_SRC_DIR}/sound/sdl.cpp
	${CLIENT_SRC_DIR}/sound/sound.cpp
	${CLIENT_SRC_DIR}/sound/wave.cpp
	${CLIENT_SRC_DIR}/vid/glimp_sdl.cpp
	${CLIENT_SRC_DIR}/vid/vid.cpp
	${COMMON_SRC_DIR}/argproc.cpp
	${COMMON_SRC_DIR}/clientserver.cpp
	${COMMON_SRC_DIR}/collision.cpp
	${COMMON_SRC_DIR}/crc.cpp
	${COMMON_SRC_DIR}/cmdparser.cpp
	${COMMON_SRC_DIR}/cvar.cpp
	${COMMON_SRC_DIR}/filesystem.cpp
	${COMMON_SRC_DIR}/glob.cpp
	${COMMON_SRC_DIR}/md4.cpp
	${COMMON_SRC_DIR}/movemsg.cpp
	${COMMON_SRC_DIR}/frame.cpp
	${COMMON_SRC_DIR}/netchan.cpp
	${COMMON_SRC_DIR}/pmove.cpp
	${COMMON_SRC_DIR}/szone.cpp
	${COMMON_SRC_DIR}/zone.cpp
	${COMMON_SRC_DIR}/shared/flash.cpp
	${COMMON_SRC_DIR}/shared/rand.cpp
	${COMMON_SRC_DIR}/shared/shared.cpp
	${COMMON_SRC_DIR}/unzip/ioapi.c
	${COMMON_SRC_DIR}/unzip/miniz.c
	${COMMON_SRC_DIR}/unzip/unzip.c
	${SERVER_SRC_DIR}/sv_cmd.cpp
	${SERVER_SRC_DIR}/sv_conless.cpp
	${SERVER_SRC_DIR}/sv_entities.cpp
	${SERVER_SRC_DIR}/sv_game.cpp
	${SERVER_SRC_DIR}/sv_init.cpp
	${SERVER_SRC_DIR}/sv_main.cpp
	${SERVER_SRC_DIR}/sv_save.cpp
	${SERVER_SRC_DIR}/sv_send.cpp
	${SERVER_SRC_DIR}/sv_user.cpp
	${SERVER_SRC_DIR}/sv_world.cpp
	)

set(Client-Header
	${CLIENT_SRC_DIR}/header/client.h
	${CLIENT_SRC_DIR}/header/console.h
	${CLIENT_SRC_DIR}/header/keyboard.h
	${CLIENT_SRC_DIR}/header/screen.h
	${CLIENT_SRC_DIR}/curl/header/download.h
	${CLIENT_SRC_DIR}/curl/header/qcurl.h
	${CLIENT_SRC_DIR}/input/header/input.h
	${CLIENT_SRC_DIR}/menu/header/qmenu.h
	${CLIENT_SRC_DIR}/sound/header/local.h
	${CLIENT_SRC_DIR}/sound/header/qal.h
	${CLIENT_SRC_DIR}/sound/header/sound.h
	${CLIENT_SRC_DIR}/sound/header/stb_vorbis.h
	${CLIENT_SRC_DIR}/sound/header/vorbis.h
	${CLIENT_SRC_DIR}/vid/header/ref.h
	${CLIENT_SRC_DIR}/vid/header/stb_image_write.h
	${CLIENT_SRC_DIR}/vid/header/vid.h
	${COMMON_SRC_DIR}/header/common.h
	${COMMON_SRC_DIR}/header/crc.h
	${COMMON_SRC_DIR}/header/files.h
	${COMMON_SRC_DIR}/header/glob.h
	${COMMON_SRC_DIR}/header/shared.h
	${COMMON_SRC_DIR}/header/zone.h
	${COMMON_SRC_DIR}/unzip/ioapi.h
	${COMMON_SRC_DIR}/unzip/miniz.h
	${COMMON_SRC_DIR}/unzip/minizconf.h
	${COMMON_SRC_DIR}/unzip/unzip.h
	${SERVER_SRC_DIR}/header/server.h
	)

set(Server-Source
	${COMMON_SRC_DIR}/argproc.cpp
	${COMMON_SRC_DIR}/clientserver.cpp
	${COMMON_SRC_DIR}/collision.cpp
	${COMMON_SRC_DIR}/crc.cpp
	${COMMON_SRC_DIR}/cmdparser.cpp
	${COMMON_SRC_DIR}/cvar.cpp
	${COMMON_SRC_DIR}/filesystem.cpp
	${COMMON_SRC_DIR}/glob.cpp
	${COMMON_SRC_DIR}/md4.cpp
	${COMMON_SRC_DIR}/frame.cpp
	${COMMON_SRC_DIR}/movemsg.cpp
	${COMMON_SRC_DIR}/netchan.cpp
	${COMMON_SRC_DIR}/pmove.cpp
	${COMMON_SRC_DIR}/szone.cpp
	${COMMON_SRC_DIR}/zone.cpp
	${COMMON_SRC_DIR}/shared/rand.cpp
	${COMMON_SRC_DIR}/shared/shared.cpp
	${COMMON_SRC_DIR}/unzip/ioapi.cpp
	${COMMON_SRC_DIR}/unzip/miniz.cpp
	${COMMON_SRC_DIR}/unzip/unzip.cpp
	${SERVER_SRC_DIR}/sv_cmd.cpp
	${SERVER_SRC_DIR}/sv_conless.cpp
	${SERVER_SRC_DIR}/sv_entities.cpp
	${SERVER_SRC_DIR}/sv_game.cpp
	${SERVER_SRC_DIR}/sv_init.cpp
	${SERVER_SRC_DIR}/sv_main.cpp
	${SERVER_SRC_DIR}/sv_save.cpp
	${SERVER_SRC_DIR}/sv_send.cpp
	${SERVER_SRC_DIR}/sv_user.cpp
	${SERVER_SRC_DIR}/sv_world.cpp
	)

set(Server-Header
	${COMMON_SRC_DIR}/header/common.h
	${COMMON_SRC_DIR}/header/crc.h
	${COMMON_SRC_DIR}/header/files.h
	${COMMON_SRC_DIR}/header/glob.h
	${COMMON_SRC_DIR}/header/shared.h
	${COMMON_SRC_DIR}/header/zone.h
	${COMMON_SRC_DIR}/unzip/ioapi.h
	${COMMON_SRC_DIR}/unzip/miniz.h
	${COMMON_SRC_DIR}/unzip/minizconf.h
	${COMMON_SRC_DIR}/unzip/unzip.h
	${SERVER_SRC_DIR}/header/server.h
	)

set(GL1-Source
	${REF_SRC_DIR}/gl1/qgl.cpp
	${REF_SRC_DIR}/gl1/gl1_draw.cpp
	${REF_SRC_DIR}/gl1/gl1_image.cpp
	${REF_SRC_DIR}/gl1/gl1_light.cpp
	${REF_SRC_DIR}/gl1/gl1_lightmap.cpp
	${REF_SRC_DIR}/gl1/gl1_main.cpp
	${REF_SRC_DIR}/gl1/gl1_mesh.cpp
	${REF_SRC_DIR}/gl1/gl1_misc.cpp
	${REF_SRC_DIR}/gl1/gl1_model.cpp
	${REF_SRC_DIR}/gl1/gl1_scrap.cpp
	${REF_SRC_DIR}/gl1/gl1_surf.cpp
	${REF_SRC_DIR}/gl1/gl1_warp.cpp
	${REF_SRC_DIR}/gl1/gl1_sdl.cpp
	${REF_SRC_DIR}/gl1/gl1_md2.cpp
	${REF_SRC_DIR}/gl1/gl1_sp2.cpp
	${REF_SRC_DIR}/files/pcx.cpp
	${REF_SRC_DIR}/files/stb.cpp
	${REF_SRC_DIR}/files/wal.cpp
	${REF_SRC_DIR}/files/pvs.cpp
	${COMMON_SRC_DIR}/shared/shared.cpp
	${COMMON_SRC_DIR}/md4.cpp
	)

set(GL1-Header
	${REF_SRC_DIR}/ref_shared.h
	${REF_SRC_DIR}/constants/anorms.h
	${REF_SRC_DIR}/constants/anormtab.h
	${REF_SRC_DIR}/constants/warpsin.h
	${REF_SRC_DIR}/files/stb_image.h
	${REF_SRC_DIR}/gl1/header/local.h
	${REF_SRC_DIR}/gl1/header/model.h
	${REF_SRC_DIR}/gl1/header/qgl.h
	${COMMON_SRC_DIR}/header/shared.h
	)

set(GL3-Source
	${REF_SRC_DIR}/gl3/gl3_draw.cpp
	${REF_SRC_DIR}/gl3/gl3_image.cpp
	${REF_SRC_DIR}/gl3/gl3_light.cpp
	${REF_SRC_DIR}/gl3/gl3_lightmap.cpp
	${REF_SRC_DIR}/gl3/gl3_main.cpp
	${REF_SRC_DIR}/gl3/gl3_mesh.cpp
	${REF_SRC_DIR}/gl3/gl3_misc.cpp
	${REF_SRC_DIR}/gl3/gl3_model.cpp
	${REF_SRC_DIR}/gl3/gl3_sdl.cpp
	${REF_SRC_DIR}/gl3/gl3_surf.cpp
	${REF_SRC_DIR}/gl3/gl3_warp.cpp
	${REF_SRC_DIR}/gl3/gl3_shaders.cpp
	${REF_SRC_DIR}/gl3/gl3_md2.cpp
	${REF_SRC_DIR}/gl3/gl3_sp2.cpp
	${REF_SRC_DIR}/gl3/glad/src/glad.cpp
	${REF_SRC_DIR}/files/pcx.cpp
	${REF_SRC_DIR}/files/stb.cpp
	${REF_SRC_DIR}/files/wal.cpp
	${REF_SRC_DIR}/files/pvs.cpp
	${COMMON_SRC_DIR}/shared/shared.cpp
	${COMMON_SRC_DIR}/md4.cpp
	)

set(GL3-Header
	${REF_SRC_DIR}/ref_shared.h
	${REF_SRC_DIR}/constants/anorms.h
	${REF_SRC_DIR}/constants/anormtab.h
	${REF_SRC_DIR}/constants/warpsin.h
	${REF_SRC_DIR}/files/stb_image.h
	${REF_SRC_DIR}/gl3/glad/include/glad/glad.h
	${REF_SRC_DIR}/gl3/glad/include/KHR/khrplatform.h
	${REF_SRC_DIR}/gl3/header/DG_dynarr.h
	${REF_SRC_DIR}/gl3/header/HandmadeMath.h
	${REF_SRC_DIR}/gl3/header/local.h
	${REF_SRC_DIR}/gl3/header/model.h
	${COMMON_SRC_DIR}/header/shared.h
	)

set(SOFT-Source
	${REF_SRC_DIR}/soft/sw_aclip.cpp
	${REF_SRC_DIR}/soft/sw_alias.cpp
	${REF_SRC_DIR}/soft/sw_bsp.cpp
	${REF_SRC_DIR}/soft/sw_draw.cpp
	${REF_SRC_DIR}/soft/sw_edge.cpp
	${REF_SRC_DIR}/soft/sw_image.cpp
	${REF_SRC_DIR}/soft/sw_light.cpp
	${REF_SRC_DIR}/soft/sw_main.cpp
	${REF_SRC_DIR}/soft/sw_misc.cpp
	${REF_SRC_DIR}/soft/sw_model.cpp
	${REF_SRC_DIR}/soft/sw_part.cpp
	${REF_SRC_DIR}/soft/sw_poly.cpp
	${REF_SRC_DIR}/soft/sw_polyset.cpp
	${REF_SRC_DIR}/soft/sw_rast.cpp
	${REF_SRC_DIR}/soft/sw_scan.cpp
	${REF_SRC_DIR}/soft/sw_sprite.cpp
	${REF_SRC_DIR}/soft/sw_surf.cpp
	${REF_SRC_DIR}/files/pcx.cpp
	${REF_SRC_DIR}/files/stb.cpp
	${REF_SRC_DIR}/files/wal.cpp
	${REF_SRC_DIR}/files/pvs.cpp
	${COMMON_SRC_DIR}/shared/shared.cpp
	${COMMON_SRC_DIR}/md4.cpp
	)

set(SOFT-Header
	${REF_SRC_DIR}/ref_shared.h
	${REF_SRC_DIR}/files/stb_image.h
	${REF_SRC_DIR}/files/stb_image_resize.h
	${REF_SRC_DIR}/soft/header/local.h
	${REF_SRC_DIR}/soft/header/model.h
	${COMMON_SRC_DIR}/header/shared.h
	)

# Wrapper for the Windows binary
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(Wrapper-Source
	src/win-wrapper/wrapper.cpp
	${BACKENDS_SRC_DIR}/windows/icon.rc
	)
endif()

# Main Quake 2 executable
if(${BUILD_GOLDQUAKEII})
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		add_executable(GoldQuakeII ${Client-Source} ${Client-Header} ${Platform-Specific-Source}
				${Backends-Generic-Source})
		set_target_properties(GoldQuakeII PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/
				)
		target_link_libraries(GoldQuakeII ${GoldQuake2LinkerFlags} ${GoldQuake2ClientLinkerFlags}
				${GoldQuake2SDLLinkerFlags} ${GoldQuake2ZLibLinkerFlags} ws2_32 winmm)

		# Wrapper for the Windows binary
		add_executable(quake2 ${Wrapper-Source})
		set_target_properties(quake2 PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
				)
	else()
		add_executable(GoldQuakeII ${Client-Source} ${Client-Header} ${Platform-Specific-Source}
				${Backends-Generic-Source})
		set_target_properties(GoldQuakeII PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
				)
		target_link_libraries(GoldQuakeII ${GoldQuake2LinkerFlags} ${GoldQuake2ClientLinkerFlags}
				${GoldQuake2SDLLinkerFlags} ${GoldQuake2ZLibLinkerFlags})
	endif()
endif()

# Quake 2 Dedicated Server
if (${BUILD_GOLDQUAKEII_DED})
	add_executable(GoldQuakeII_ded ${Server-Source} ${Server-Header} ${Platform-Specific-Source}
			${Backends-Generic-Source})
	set_target_properties(GoldQuakeII_ded PROPERTIES
		COMPILE_DEFINITIONS "DEDICATED_ONLY"
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
		)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		target_link_libraries(GoldQuakeII_ded ${GoldQuake2LinkerFlags} ${GoldQuake2SDLLinkerFlags} ${GoldQuake2ZLibLinkerFlags} ws2_32 winmm)
	else()
		target_link_libraries(GoldQuakeII_ded ${GoldQuake2LinkerFlags} ${GoldQuake2ServerLinkerFlags} ${GoldQuake2ZLibLinkerFlags})
	endif()
endif()

# Build the game dynamic library
if (${BUILD_GOLDQUAKEII_GAME})
	add_library(game MODULE ${Game-Source} ${Game-Header})
	set_target_properties(game PROPERTIES
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/baseq2
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/baseq2
			SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
			)
	target_link_libraries(game ${GoldQuake2LinkerFlags})
endif()

# Build the GL1 dynamic library
if (${BUILD_REF_OPENGL14})
	add_library(ref_gl1 MODULE ${GL1-Source} ${GL1-Header} ${REF-Platform-Specific-Source})
	set_target_properties(ref_gl1 PROPERTIES
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
			SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
			)
	target_link_libraries(ref_gl1 ${GoldQuake2LinkerFlags} ${GoldQuake2OpenGLLinkerFlags} ${GoldQuake2SDLLinkerFlags})
endif()

# Build the GL3 dynamic library
if (${BUILD_REF_OPENGL32})
	add_library(ref_gl3 MODULE ${GL3-Source} ${GL3-Header} ${REF-Platform-Specific-Source})
	set_target_properties(ref_gl3 PROPERTIES
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
			SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
			)
	target_link_libraries(ref_gl3 ${GoldQuake2LinkerFlags} ${GoldQuake2SDLLinkerFlags})
endif()

# Build the soft renderer dynamic library
if (${BUILD_REF_SOFTWARE})
	add_library(ref_soft MODULE ${SOFT-Source} ${SOFT-Header} ${REF-Platform-Specific-Source})
	set_target_properties(ref_soft PROPERTIES
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
			SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
			)
	target_link_libraries(ref_soft ${GoldQuake2LinkerFlags} ${GoldQuake2SDLLinkerFlags})
endif()
