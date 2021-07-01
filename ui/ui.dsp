# Microsoft Developer Studio Project File - Name="UI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=UI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ui.mak" CFG="UI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UI - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "UI - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "UI - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "c:\quake2\baseq2\"
# PROP Intermediate_Dir "release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UI_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"c:\quake2\baseq2\egluix86.dll"

!ELSEIF  "$(CFG)" == "UI - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "c:\quake2\baseq2\"
# PROP Intermediate_Dir "debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gi /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UI_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"debug/eglui.pdb" /map /debug /machine:I386 /def:".\exports.def" /out:"c:\quake2\baseq2\egluix86.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "UI - Win32 Release"
# Name "UI - Win32 Debug"
# Begin Group "handling"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_api.c
# End Source File
# Begin Source File

SOURCE=.\ui_api.h
# End Source File
# Begin Source File

SOURCE=.\ui_backend.c
# End Source File
# Begin Source File

SOURCE=.\ui_common.c
# End Source File
# Begin Source File

SOURCE=.\ui_common.h
# End Source File
# Begin Source File

SOURCE=.\ui_cursor.c
# End Source File
# Begin Source File

SOURCE=.\ui_draw.c
# End Source File
# Begin Source File

SOURCE=.\ui_keys.c
# End Source File
# Begin Source File

SOURCE=.\ui_local.h
# End Source File
# End Group
# Begin Group "menus"

# PROP Default_Filter ""
# Begin Group "multiplayer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_addrbook.c
# End Source File
# Begin Source File

SOURCE=.\ui_dlopts.c
# End Source File
# Begin Source File

SOURCE=.\ui_dmflags.c
# End Source File
# Begin Source File

SOURCE=.\ui_joinserver.c
# End Source File
# Begin Source File

SOURCE=.\ui_multiplayer.c
# End Source File
# Begin Source File

SOURCE=.\ui_playercfg.c
# End Source File
# Begin Source File

SOURCE=.\ui_startserver.c
# End Source File
# End Group
# Begin Group "options"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_controls.c
# End Source File
# Begin Source File

SOURCE=.\ui_effects.c
# End Source File
# Begin Source File

SOURCE=.\ui_fog.c
# End Source File
# Begin Source File

SOURCE=.\ui_gloom.c
# End Source File
# Begin Source File

SOURCE=.\ui_hud.c
# End Source File
# Begin Source File

SOURCE=.\ui_input.c
# End Source File
# Begin Source File

SOURCE=.\ui_misc.c
# End Source File
# Begin Source File

SOURCE=.\ui_options.c
# End Source File
# Begin Source File

SOURCE=.\ui_screen.c
# End Source File
# Begin Source File

SOURCE=.\ui_sound.c
# End Source File
# End Group
# Begin Group "single player"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_game.c
# End Source File
# Begin Source File

SOURCE=.\ui_loadgame.c
# End Source File
# Begin Source File

SOURCE=.\ui_savegame.c
# End Source File
# End Group
# Begin Group "video"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui_glexts.c
# End Source File
# Begin Source File

SOURCE=.\ui_video.c
# End Source File
# Begin Source File

SOURCE=.\ui_vidsettings.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\ui_credits.c
# End Source File
# Begin Source File

SOURCE=.\ui_mainmenu.c
# End Source File
# Begin Source File

SOURCE=.\ui_quit.c
# End Source File
# End Group
# Begin Group "resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\exports.def

!IF  "$(CFG)" == "UI - Win32 Release"

!ELSEIF  "$(CFG)" == "UI - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "shared"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\shared\endian.c
# End Source File
# Begin Source File

SOURCE=..\shared\infostrings.c
# End Source File
# Begin Source File

SOURCE=..\shared\mathlib.c
# End Source File
# Begin Source File

SOURCE=..\shared\shared.c
# End Source File
# Begin Source File

SOURCE=..\shared\shared.h
# End Source File
# Begin Source File

SOURCE=..\shared\string.c
# End Source File
# End Group
# End Target
# End Project
