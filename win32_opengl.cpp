
global HGLRC g_opengl_rendering_ctx;

// Pointer to the FunctioN glUseProgram, which is a PROCedure
typedef void (APIENTRY *gl_use_program_ptr)(GLuint program);
gl_use_program_ptr glUseProgram = 0;

typedef const GLubyte *(APIENTRY *gl_get_stringi_ptr)(GLenum name, GLuint index);
gl_get_stringi_ptr glGetStringi = 0;

typedef const char *(WINAPI *wgl_get_extensions_string_ARB_ptr)(HDC hdc);
internal wgl_get_extensions_string_ARB_ptr wglGetExtensionsStringARB = 0;

typedef BOOL (WINAPI *wgl_choose_pixel_format_arb_ptr)(HDC hdc, 
                                                       const int *piAttribIList, 
                                                       const FLOAT *pfAttribFList, 
                                                       UINT nMaxFormats, 
                                                       int *piFormats, 
                                                       UINT *nNumFormats);
internal wgl_choose_pixel_format_arb_ptr wglChoosePixelFormatARB = 0;

typedef HGLRC (WINAPI *wgl_create_context_attribs_arb_ptr)(HDC hDC, HGLRC hShareContext, const int *attribList);
wgl_create_context_attribs_arb_ptr wglCreateContextAttribsARB = 0;

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB  0x2011
#define WGL_PIXEL_TYPE_ARB     0x2013
#define WGL_TYPE_RGBA_ARB      0x202B
#define WGL_COLOR_BITS_ARB     0x2014
#define WGL_DEPTH_BITS_ARB     0x2022
#define WGL_STENCIL_BITS_ARB   0x2023

// NOTE(al): Not sure whether this should be a global thing.
internal s32 g_attrib_list[]={
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    WGL_COLOR_BITS_ARB, 32,
    WGL_DEPTH_BITS_ARB, 24,
    WGL_STENCIL_BITS_ARB, 8,
    0,
};

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

internal s32 wgl_create_context_attrib_list[]={
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    //WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    0,
};

internal void *w32_load_opengl_proc(const char *proc_name){
    void *result = (void *)wglGetProcAddress(proc_name);
    if((result == 0) || (result == (void *)0x01) ||
       (result == (void *)0x02) || (result == (void *)0x03) || 
       (result == (void *)-1)){
        HMODULE opengl_dll = LoadLibraryA("opengl32.dll");
        result = (void *)GetProcAddress(opengl_dll, proc_name);
    }
    return(result);
}

internal b32 w32_init_opengl(HDC device_ctx){
    b32 result = 0;
    
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; // RGBA framebuffer.
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24; // 24-bit z-buffer.
    pfd.cStencilBits = 8; // 8-bit stencil-buffer.
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    s32 suggested_index = ChoosePixelFormat(device_ctx, &pfd);
    if(suggested_index){
        PIXELFORMATDESCRIPTOR suggested_pfd;
        DescribePixelFormat(device_ctx,
                            suggested_index, 
                            sizeof(PIXELFORMATDESCRIPTOR), 
                            &suggested_pfd);
        SetPixelFormat(device_ctx, suggested_index, &suggested_pfd);
        
        HGLRC dummy_opengl_rendering_ctx = wglCreateContext(device_ctx);
        wglMakeCurrent(device_ctx, dummy_opengl_rendering_ctx);
        
#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C
#define GL_NUM_EXTENSIONS 0x821D
        
        s32 gl_major_version;
        s32 gl_minor_version;
        s32 gl_extensions_count;
        
        glGetIntegerv(GL_MAJOR_VERSION, &gl_major_version);
        glGetIntegerv(GL_MINOR_VERSION, &gl_minor_version);
        glGetIntegerv(GL_NUM_EXTENSIONS, &gl_extensions_count);
        
        glGetStringi = (gl_get_stringi_ptr)w32_load_opengl_proc("glGetStringi");
        Assert(glGetStringi);
        
        for(s32 extension_index = 0; 
            extension_index < gl_extensions_count;
            ++extension_index){
            u8 *extension = (u8 *)glGetStringi(GL_EXTENSIONS, extension_index);
            {
                u8 DEBUG_buf[128] = {};
                sprintf_s((char *)DEBUG_buf, sizeof(DEBUG_buf), "Extension: %s\n", extension);
                OutputDebugStringA((LPCSTR)DEBUG_buf);
            }
        }
        
        wglGetExtensionsStringARB = (wgl_get_extensions_string_ARB_ptr)w32_load_opengl_proc("wglGetExtensionsStringARB");
        Assert(wglGetExtensionsStringARB);
        
        char *wgl_extensions = (char *)wglGetExtensionsStringARB(device_ctx);
        {
            char *start = wgl_extensions;
            char *end = wgl_extensions;
            while(*end++){
                if(*end == ' '){
                    s32 len = (end - start);
                    char *extension_buf = (char *)malloc((sizeof(char)*len) + 1); // 0
                    strncpy(extension_buf, start, len);
                    extension_buf[len] = 0;
                    
                    char PRINT_buf[128];
                    sprintf_s(PRINT_buf, sizeof(PRINT_buf), "WGL extension: %s\n", extension_buf);
                    OutputDebugStringA(PRINT_buf);
                    
                    start = ++end;
                }
            }
        }
        
        wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb_ptr)w32_load_opengl_proc("wglChoosePixelFormatARB");
        Assert(wglChoosePixelFormatARB);
        
        s32 pixel_format_number;
        u32 formats_count;
        wglChoosePixelFormatARB(device_ctx, g_attrib_list, 0, 1, &pixel_format_number, &formats_count);
        
        {
            char PRINT_buf[1024];
            sprintf_s(PRINT_buf, sizeof(PRINT_buf), 
                      "Pixel_format_number: %i\nformats_count: %u\n\n", 
                      pixel_format_number, formats_count);
            OutputDebugStringA(PRINT_buf);
        }
        
        wglCreateContextAttribsARB = (wgl_create_context_attribs_arb_ptr)w32_load_opengl_proc("wglCreateContextAttribsARB");
        Assert(wglCreateContextAttribsARB);
        
        g_opengl_rendering_ctx = wglCreateContextAttribsARB(device_ctx, 0, wgl_create_context_attrib_list);
        if(g_opengl_rendering_ctx){
            wglMakeCurrent(device_ctx, 0);
            wglDeleteContext(dummy_opengl_rendering_ctx);
            wglMakeCurrent(device_ctx, g_opengl_rendering_ctx);
            
            //wglSwapIntervalsEXT(1);
            
            glGetIntegerv(GL_MAJOR_VERSION, &gl_major_version);
            glGetIntegerv(GL_MINOR_VERSION, &gl_minor_version);
            {
                char buf[256];
                sprintf_s(buf, sizeof(buf), "OpenGL version: %i.%i\n", gl_major_version, gl_minor_version);
                OutputDebugStringA(buf);
            }
        }
        else{
            // TODO: Error handling.
            InvalidCodePath;
        }
    }
    else{
        // TODO: Error handling.
        InvalidCodePath;
    }
    
    return(result);
}

internal void w32_opengl_shutdown(HDC device_ctx, HGLRC rendering_ctx){
    // Make current calling thread's rendering context no longer current.
    wglMakeCurrent(device_ctx, 0);
    
    // delete rendering context.
    wglDeleteContext(rendering_ctx);
}