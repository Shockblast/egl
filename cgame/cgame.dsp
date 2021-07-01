# Microsoft Developer Studio Project File - Name="CGAME" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=CGAME - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cgame.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cgame.mak" CFG="CGAME - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CGAME - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CGAME - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CGAME - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CGAME_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CGAME_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"c:\quake2\baseq2\eglcgamex86.dll"

!ELSEIF  "$(CFG)" == "CGAME - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CGAME_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gi /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CGAME_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"debug\eglcgamex86.pdb" /map /debug /machine:I386 /def:".\exports.def" /out:"c:\quake2\baseq2\eglcgamex86.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "CGAME - Win32 Release"
# Name "CGAME - Win32 Debug"
# Begin Group "cgame"

# PROP Default_Filter ""
# Begin Group "effects"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cg_decals.c
# End Source File
# Begin Source File

SOURCE=.\cg_light.c
# End Source File
# Begin Source File

SOURCE=.\cg_mapeffects.c
# End Source File
# Begin Source File

SOURCE=.\cg_parteffects.c
# End Source File
# Begin Source File

SOURCE=.\cg_partgloom.c
# End Source File
# Begin Source File

SOURCE=.\cg_particles.c
# End Source File
# Begin Source File

SOURCE=.\cg_partsustain.c
# End Source File
# Begin Source File

SOURCE=.\cg_partthink.c
# End Source File
# Begin Source File

SOURCE=.\cg_parttrail.c
# End Source File
# End Group
# Begin Group "entity"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cg_entities.c
# End Source File
# Begin Source File

SOURCE=.\cg_localents.c
# End Source File
# Begin Source File

SOURCE=.\cg_tempents.c
# End Source File
# Begin Source File

SOURCE=.\cg_weapon.c
# End Source File
# End Group
# Begin Group "other"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cg_api.c
# End Source File
# Begin Source File

SOURCE=.\cg_api.h
# End Source File
# Begin Source File

SOURCE=.\cg_local.h
# End Source File
# Begin Source File

SOURCE=.\cg_main.c
# End Source File
# Begin Source File

SOURCE=.\cg_media.c
# End Source File
# Begin Source File

SOURCE=.\cg_predict.c
# End Source File
# Begin Source File

SOURCE=.\cgref.h
# End Source File
# Begin Source File

SOURCE=.\colorpal.h
# End Source File
# Begin Source File

SOURCE=.\pmove.c
# End Source File
# End Group
# Begin Group "parse"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cg_muzzleflash.c
# End Source File
# Begin Source File

SOURCE=.\cg_parse.c
# End Source File
# Begin Source File

SOURCE=.\cg_players.c
# End Source File
# End Group
# Begin Group "view"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cg_draw.c
# End Source File
# Begin Source File

SOURCE=.\cg_hud.c
# End Source File
# Begin Source File

SOURCE=.\cg_inventory.c
# End Source File
# Begin Source File

SOURCE=.\cg_screen.c
# End Source File
# Begin Source File

SOURCE=.\cg_view.c
# End Source File
# End Group
# End Group
# Begin Group "game"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\game\game.h
# End Source File
# Begin Source File

SOURCE=..\game\m_flash.c
# End Source File
# End Group
# Begin Group "resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\exports.def

!IF  "$(CFG)" == "CGAME - Win32 Release"

!ELSEIF  "$(CFG)" == "CGAME - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "shared"

# PROP Default_Filter ""
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
