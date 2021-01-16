//=================================================================================================
// Non-portable system services
//=================================================================================================

#pragma once

void	Sys_Init( void );

void	Sys_AppActivate( void );

void	Sys_UnloadGame( void );
void	*Sys_GetGameAPI( void *parms );
// loads the game dll and calls the api init function

char	*Sys_ConsoleInput( void );
void	Sys_ConsoleOutput( const char *string );
void	Sys_SendKeyEvents( void );

[[noreturn]]
void	Sys_Error( const char *error, ... );
[[noreturn]]
void	Sys_Quit( void );

char	*Sys_GetClipboardData( void );
void	Sys_CopyProtect( void );