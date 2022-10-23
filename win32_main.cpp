
#include <windows.h>

#include <gl/gl.h>

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define Assert(expr) assert((expr))
#define out(s) OutputDebugStringA((s))
#define InvalidCodePath Assert(0);

#define global static 
#define internal static
#define local_persist static 

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef s32 b32;

struct W32_WindowMetrics{
    s32 width;
    s32 height;
};

// NOTE(al): Global variables:
global b32 g_running;


#include "win32_opengl.cpp"

internal W32_WindowMetrics w32_get_window_metrics(HWND window){
    W32_WindowMetrics result = {};
    
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;
    
    return(result);
}

LRESULT CALLBACK w32_main_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam){
    LRESULT result = 0;
    
    switch(message){
        case WM_CLOSE:
        case WM_DESTROY:{
            g_running = false;
        }break;
        
        case WM_SIZE:{
            out("Resizing the window.\n");
        }break;
        case WM_ACTIVATEAPP:{
            out("App was activated.\n");
        }break;
        
        case WM_PAINT:{
            PAINTSTRUCT paint = {};
            HDC device_ctx = BeginPaint(window, &paint);
            
            SetBkColor(device_ctx, RGB(150, 136, 142));
            ExtTextOutA(device_ctx, 0, 0, ETO_OPAQUE, &paint.rcPaint, "", 0, 0);
            
            EndPaint(window, &paint);
        }break;
        
        default:{
            result = DefWindowProc(window, message, wParam, lParam);
        }break;
    }
    return(result);
}


INT WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR cmd_string, int show_code){
    WNDCLASSA window_class = {};
    window_class.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = w32_main_window_proc;
    window_class.hInstance = GetModuleHandleA(0);
    window_class.hbrBackground = (HBRUSH)(GetStockObject(DKGRAY_BRUSH));
    window_class.lpszClassName = "OpenglWidowClass";
    
    RegisterClass(&window_class);
    
    HWND window = CreateWindowExA(0, window_class.lpszClassName, 
                                  "opengl_initialization", WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 
                                  window_class.hInstance, 0);
    if(!window){
        goto quit_app;
    }
    
    HDC device_ctx = GetDC(window);
    if(w32_init_opengl(device_ctx)){
        goto quit_app;
    }
    
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    
    g_running = true;
    while(g_running){
        MSG msg;
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)){
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        
        W32_WindowMetrics window_metrics = w32_get_window_metrics(window);
        glViewport(0, 0, window_metrics.width, window_metrics.height);
        
        f32 c = 64.0f / 255.0f;
        glClearColor(c, c, c, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        
        // NOTE(al): glBegin, glEnd don't work for now because we are using OpenGL 3.3 CORE Profile, 
        // not COMPATIBLE profile. 
        // Have to figure out how to use compatible profile with 3.3 version if it's possible.
        // Probably have to play around with wgl_create_context_attrib_list's attributes...
        glBegin(GL_LINES);
        {
            glColor3f(81.0f / 255.0f, 1.0f, 0.0f);
            
            // First line color.
            glVertex2f(-0.5f, 0.0f);
            glVertex2f(0.5f, 0.5f);
            
            // Second line color.
            glVertex2f(-0.5f, -0.3f);
            glVertex2f(0.5f, 0.2f);
        }
        glEnd();
        
        SwapBuffers(device_ctx);
    }
    
    // shut down OpenGL.
    w32_opengl_shutdown(device_ctx, g_opengl_rendering_ctx);
    
    quit_app:;
    return(0);
}
