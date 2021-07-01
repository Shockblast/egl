# Microsoft Developer Studio Project File - Name="EGL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=EGL - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "egl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "egl.mak" CFG="EGL - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EGL - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "EGL - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EGL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release"
# PROP Intermediate_Dir ".\release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"release\egl.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib dxguid.lib vc6zlibr.lib vc6libpngr.lib libjpeg.lib OpenAL32.lib ALut.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc" /out:"c:\quake2\egl.exe" /libpath:"./win32/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "EGL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "c:\quake2\"
# PROP Intermediate_Dir "debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gi /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib dxguid.lib vc6zlibd.lib vc6libpngd.lib libjpeg.lib OpenAL32d.lib ALutd.lib /nologo /subsystem:windows /pdb:"debug/egl.pdb" /map /debug /machine:I386 /nodefaultlib:"libc" /libpath:"./win32/lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "EGL - Win32 Release"
# Name "EGL - Win32 Debug"
# Begin Group "client"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cgame\cg_api.h
# End Source File
# Begin Source File

SOURCE=.\client\cl_cgapi.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_cin.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_demo.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_input.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_loc.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_local.h
# End Source File
# Begin Source File

SOURCE=.\client\cl_main.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_parse.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_screen.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_uiapi.c
# End Source File
# Begin Source File

SOURCE=.\client\console.c
# End Source File
# Begin Source File

SOURCE=.\client\graphpal.h
# End Source File
# Begin Source File

SOURCE=.\client\keys.c
# End Source File
# Begin Source File

SOURCE=.\client\keys.h
# End Source File
# Begin Source File

SOURCE=.\ui\ui_api.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\alias.c
# End Source File
# Begin Source File

SOURCE=.\common\cbuf.c
# End Source File
# Begin Source File

SOURCE=.\common\cmd.c
# End Source File
# Begin Source File

SOURCE=.\common\cmodel.c
# End Source File
# Begin Source File

SOURCE=.\common\common.c
# End Source File
# Begin Source File

SOURCE=.\common\common.h
# End Source File
# Begin Source File

SOURCE=.\common\crc.c
# End Source File
# Begin Source File

SOURCE=.\common\cvar.c
# End Source File
# Begin Source File

SOURCE=.\common\files.c
# End Source File
# Begin Source File

SOURCE=.\common\files.h
# End Source File
# Begin Source File

SOURCE=.\common\font.h
# End Source File
# Begin Source File

SOURCE=.\common\md4.c
# End Source File
# Begin Source File

SOURCE=.\common\memory.c
# End Source File
# Begin Source File

SOURCE=.\common\net_chan.c
# End Source File
# Begin Source File

SOURCE=.\common\net_msg.c
# End Source File
# End Group
# Begin Group "game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\game\game.h
# End Source File
# End Group
# Begin Group "renderer"

# PROP Default_Filter ""
# Begin Group "jpg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\renderer\jpg\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\renderer\jpg\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\renderer\jpg\jpeglib.h
# End Source File
# End Group
# Begin Group "png"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\renderer\png\png.h
# End Source File
# Begin Source File

SOURCE=.\renderer\png\pngconf.h
# End Source File
# Begin Source File

SOURCE=.\renderer\png\zconf.h
# End Source File
# Begin Source File

SOURCE=.\renderer\png\zlib.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\renderer\defpal.h
# End Source File
# Begin Source File

SOURCE=.\renderer\glext.h
# End Source File
# Begin Source File

SOURCE=.\renderer\qgl.h
# End Source File
# Begin Source File

SOURCE=.\renderer\r_alias.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_backend.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_cin.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_cull.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_draw.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_entity.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_fx.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_glstate.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_image.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_light.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_local.h
# End Source File
# Begin Source File

SOURCE=.\renderer\r_main.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_model.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_model.h
# End Source File
# Begin Source File

SOURCE=.\renderer\r_shader.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_skeletal.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_sprite.c
# End Source File
# Begin Source File

SOURCE=.\renderer\r_world.c
# End Source File
# Begin Source File

SOURCE=.\renderer\refresh.h
# End Source File
# Begin Source File

SOURCE=.\renderer\warpsin.h
# End Source File
# End Group
# Begin Group "resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\egl.ico
# End Source File
# Begin Source File

SOURCE=.\win32\egl.rc
# End Source File
# End Group
# Begin Group "server"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\server\sv_ccmds.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_ents.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_gameapi.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_init.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_local.h
# End Source File
# Begin Source File

SOURCE=.\server\sv_main.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_pmove.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_send.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_user.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_world.c
# End Source File
# End Group
# Begin Group "shared"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\shared\infostrings.c
# End Source File
# Begin Source File

SOURCE=.\shared\mathlib.c
# End Source File
# Begin Source File

SOURCE=.\shared\shared.c
# End Source File
# Begin Source File

SOURCE=.\shared\shared.h
# End Source File
# Begin Source File

SOURCE=.\shared\string.c
# End Source File
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sound\cdaudio.h
# End Source File
# Begin Source File

SOURCE=.\sound\eax.h
# End Source File
# Begin Source File

SOURCE=.\sound\qal.h
# End Source File
# Begin Source File

SOURCE=.\sound\snd_dma.c
# End Source File
# Begin Source File

SOURCE=.\sound\snd_loc.h
# End Source File
# Begin Source File

SOURCE=.\sound\snd_mix.c
# End Source File
# Begin Source File

SOURCE=.\sound\snd_openal.c
# End Source File
# Begin Source File

SOURCE=.\sound\snd_pub.h
# End Source File
# End Group
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\alimp_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\alimp_win.h
# End Source File
# Begin Source File

SOURCE=.\win32\cd_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\con_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\conproc.c
# End Source File
# Begin Source File

SOURCE=.\win32\glimp_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\glimp_win.h
# End Source File
# Begin Source File

SOURCE=.\win32\in_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\net_wins.c
# End Source File
# Begin Source File

SOURCE=.\win32\qal_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\qgl_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\resource.h
# End Source File
# Begin Source File

SOURCE=.\win32\snd_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\sys_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\vid_win.c
# End Source File
# Begin Source File

SOURCE=.\win32\wglext.h
# End Source File
# Begin Source File

SOURCE=.\win32\winquake.h
# End Source File
# End Group
# End Target
# End Project
