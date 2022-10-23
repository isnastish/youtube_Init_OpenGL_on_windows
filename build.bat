
@echo off

set CommonCompilerOptions=-nologo -Od -Zi -FC -TP -EHsc -Fe: opengl.exe /I..\initializing_opengl_on_win32\code\ /I..\initializing_opengl_on_win32\code\gl\
set CommonLinkerOptions=user32.lib kernel32.lib gdi32.lib opengl32.lib

if not exist ..\..\build ( mkdir ..\..\build )

pushd ..\..\build

cl %CommonCompilerOptions% ..\initializing_opengl_on_win32\code\win32_main.cpp /link %CommonLinkerOptions% %*

popd