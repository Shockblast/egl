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
option(BUILD_EGL_CGAME "Build Client CGame DLL" ON)

# Set directory locations (allowing us to move directories easily)
set(EGL_SRCDIR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
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
    ${EGL_SRCDIR_CGAME}/cg_weapon.c 
    
    ${EGL_SRCDIR_CGAME}/pmove.c

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

###
# Set GAME Source Files.
###
set(EGL_SRC_GAME
    ${EGL_SRCDIR_GAME}/g_ai.c 
    ${EGL_SRCDIR_GAME}/g_chase.c 
    ${EGL_SRCDIR_GAME}/g_cmds.c 
    ${EGL_SRCDIR_GAME}/g_combat.c 
    ${EGL_SRCDIR_GAME}/g_func.c 
    ${EGL_SRCDIR_GAME}/g_items.c 
    ${EGL_SRCDIR_GAME}/g_main.c 
    ${EGL_SRCDIR_GAME}/g_menus.c 
    ${EGL_SRCDIR_GAME}/g_misc.c 
    ${EGL_SRCDIR_GAME}/g_monster.c 
    ${EGL_SRCDIR_GAME}/g_phys.c 
    ${EGL_SRCDIR_GAME}/g_save.c 
    ${EGL_SRCDIR_GAME}/g_spawn.c 
    ${EGL_SRCDIR_GAME}/g_svcmds.c 
    ${EGL_SRCDIR_GAME}/g_target.c 
    ${EGL_SRCDIR_GAME}/g_trigger.c 
    ${EGL_SRCDIR_GAME}/g_turret.c 
    ${EGL_SRCDIR_GAME}/g_utils.c 
    ${EGL_SRCDIR_GAME}/g_weapon.c 
    ${EGL_SRCDIR_GAME}/m_actor.c 
    ${EGL_SRCDIR_GAME}/m_berserk.c 
    ${EGL_SRCDIR_GAME}/m_boss2.c 
    ${EGL_SRCDIR_GAME}/m_boss3.c 
    ${EGL_SRCDIR_GAME}/m_boss31.c 
    ${EGL_SRCDIR_GAME}/m_boss32.c 
    ${EGL_SRCDIR_GAME}/m_brain.c 
    ${EGL_SRCDIR_GAME}/m_chick.c 
    ${EGL_SRCDIR_GAME}/m_flipper.c 
    ${EGL_SRCDIR_GAME}/m_float.c 
    ${EGL_SRCDIR_GAME}/m_flyer.c 
    ${EGL_SRCDIR_GAME}/m_gladiator.c 
    ${EGL_SRCDIR_GAME}/m_gunner.c 
    ${EGL_SRCDIR_GAME}/m_hover.c 
    ${EGL_SRCDIR_GAME}/m_infantry.c 
    ${EGL_SRCDIR_GAME}/m_insane.c 
    ${EGL_SRCDIR_GAME}/m_medic.c 
    ${EGL_SRCDIR_GAME}/m_move.c 
    ${EGL_SRCDIR_GAME}/m_mutant.c 
    ${EGL_SRCDIR_GAME}/m_parasite.c 
    ${EGL_SRCDIR_GAME}/m_soldier.c 
    ${EGL_SRCDIR_GAME}/m_supertank.c 
    ${EGL_SRCDIR_GAME}/m_tank.c 
    ${EGL_SRCDIR_GAME}/p_client.c 
    ${EGL_SRCDIR_GAME}/p_hud.c 
    ${EGL_SRCDIR_GAME}/p_trail.c 
    ${EGL_SRCDIR_GAME}/p_view.c 
    ${EGL_SRCDIR_GAME}/p_weapon.c

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
        ${EGL_SRCDIR_UNIX}/unix_console.c    
        ${EGL_SRCDIR_UNIX}/unix_glimp.c
        ${EGL_SRCDIR_UNIX}/unix_main.c 
        ${EGL_SRCDIR_UNIX}/unix_snd_alsa.c 
        ${EGL_SRCDIR_UNIX}/unix_snd_cd.c 
        ${EGL_SRCDIR_UNIX}/unix_snd_main.c 
        ${EGL_SRCDIR_UNIX}/unix_snd_oss.c
        ${EGL_SRCDIR_UNIX}/unix_snd_sdl.c
        ${EGL_SRCDIR_UNIX}/unix_udp.c
        ${EGL_SRCDIR_UNIX}/x11_main.c 
        ${EGL_SRCDIR_UNIX}/x11_utils.c
    )
elseif(WIN32)
    list(APPEND EGL_SRC_ENGINE 
        ${EGL_SRCDIR_WIN32}/win_console.c 
        ${EGL_SRCDIR_WIN32}/win_glimp.c
        ${EGL_SRCDIR_WIN32}/win_input.c 
        ${EGL_SRCDIR_WIN32}/win_main.c 
        ${EGL_SRCDIR_WIN32}/win_snd.c 
        ${EGL_SRCDIR_WIN32}/win_snd_cd.c 
        ${EGL_SRCDIR_WIN32}/win_sock.c 
        ${EGL_SRCDIR_WIN32}/win_vid.c
    )
endif()

# Build the cgame(client game) dynamic library
if (${BUILD_EGL_CGAME})
	add_library(cgame MODULE ${EGL_SRC_CGAME})
	set_target_properties(cgame PROPERTIES
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/baseq2
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/baseq2
			SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
			)
	target_link_libraries(cgame ${EGL_LINKER_FLAGS})
endif()

# Build the game dynamic library
if (${BUILD_EGL_GAME})
	add_library(game MODULE ${EGL_SRC_GAME})
	set_target_properties(game PROPERTIES
			PREFIX ""
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/baseq2
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/baseq2
			SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}
			)
	target_link_libraries(game ${EGL_LINKER_FLAGS})
endif()

# Build the EGL executable
if(${BUILD_EGL})
    add_executable(EGL ${EGL_SRC_ENGINE})
    set_target_properties(EGL PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/
            )
    target_link_libraries(EGL ${EGL_LINKER_FLAGS} ${SDL_LIBRARY} ${OPENAL_LIBRARY} zlib libjpeg libpng)# ws2_32 winmm)
endif()

