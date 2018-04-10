#include "yuvdisplay.h"
#include <stdio.h>
#include <string.h>
/// OpenGL ES 2.0 includes
#include <GLES2/gl2.h>
#include <EGL/egl.h>
/// X11 includes
#include  <X11/Xlib.h>
#include  <X11/Xatom.h>
#include  <X11/Xutil.h>

#if DEBUG
#define gles_log printf
#define gles_err printf
#else
#define gles_log
#define gles_err
#endif

#define SHADERSTR(x) #x

static GLchar vShaderStr[] = SHADERSTR(
            attribute vec4 a_position;
        attribute vec2 a_texCoord;
varying vec2 v_texCoord;
void main()
{
    gl_Position = a_position;
    v_texCoord = a_texCoord;
}
);

//float nx = v_texCoord.x, ny = v_texCoord.y;
//vec2(nx/2.0, ny/2.0)
static GLchar fShaderStr_NV12[] = SHADERSTR(
            precision mediump float;
        varying vec2 v_texCoord;
uniform sampler2D tex_y;
uniform sampler2D tex_uv;
void main()
{
    vec3 yuv;
    vec3 rgb;
    yuv.x = texture2D(tex_y, v_texCoord).r - 0.0625;
    yuv.y = texture2D(tex_uv, v_texCoord).r - 0.5;
    yuv.z = texture2D(tex_uv, v_texCoord).a - 0.5;
    rgb = mat3( 1,       1,         1,
                0,       -0.39465,  2.03211,
                1.13983, -0.58060,  0) * yuv;
    gl_FragColor = vec4(rgb, 1.0);
}
);


static GLchar fShaderStr_NV21[] = SHADERSTR(
            varying lowp vec2 v_texCoord;
        uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
void main(void)
{
    mediump vec3 yuv;
    lowp vec3 rgb;
    yuv.x = texture2D(tex_y, v_texCoord).r;
    yuv.y = texture2D(tex_u, v_texCoord).r - 0.5;
    yuv.z = texture2D(tex_v, v_texCoord).r - 0.5;
    rgb = mat3( 1,       1,         1,
                0,       -0.39465,  2.03211,
                1.13983, -0.58060,  0) * yuv;
    gl_FragColor = vec4(rgb, 1.0);
}
);


// Size
GLint frame_width, frame_height;
// Context
/// Window width
GLint       wnd_width;
/// Window height
GLint       wnd_height;
/// Window handle
EGLNativeWindowType  hWnd;
/// EGL display
EGLDisplay  eglDisplay;
/// EGL context
EGLContext  eglContext;
/// EGL surface
EGLSurface  eglSurface;

// Handle to a program object
GLuint programObject;
// Attribute locations
GLint  positionLoc;
GLint  texCoordLoc;
// Sampler locations
GLint YMapLoc;
GLint UVMapLoc;
// Texture handle
GLuint YTexId;
GLuint UVTexId;

uint8_t* yuv_y = NULL;
uint8_t* yuv_uv = NULL;

//设置opengl 渲染的坐标系统
static float squareVertices[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f,  1.0f,
    1.0f,  1.0f,
};
//设置纹理基本坐标
static float coordVertices[] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f,  0.0f,
    1.0f,  0.0f,
};

static void checkGlError(const char *tag);
static GLuint loadShader(GLenum type, const char *shaderSrc);

void ShutDown()
{
    // Delete texture object
    glDeleteTextures(1, &YTexId);
    glDeleteTextures(1, &UVTexId);

    // Delete program object
    glDeleteProgram(programObject);
}

static void checkGlError(const char *tag)
{
    int error;
    error = glGetError();
    if (error != GL_NO_ERROR) {
        //
        printf("%s check error.\n", tag);
        exit(-1);
    }
}

// 创建纹理
void createTexture()
{
    /// Build Y texture
    glGenTextures(1, &YTexId);
    // Bind texture
    glBindTexture (GL_TEXTURE_2D, YTexId);
    // Texture attributes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width, frame_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

    /// Build UV texture
    glGenTextures(1, &UVTexId);
    // Bind texture
    glBindTexture (GL_TEXTURE_2D, UVTexId);
    // Texture attributes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, frame_width >> 1, frame_height >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);
}

#define WINDOWS_TITLE NULL//"mpp_rtsp_player"
int initWindow()
{
    EGLint attribList[] =
    {
        EGL_RED_SIZE,       8,
        EGL_GREEN_SIZE,     8,
        EGL_BLUE_SIZE,      8,
        EGL_ALPHA_SIZE,     8 /*EGL_DONT_CARE*/,
        EGL_DEPTH_SIZE,     EGL_DONT_CARE,
        EGL_STENCIL_SIZE,   EGL_DONT_CARE,
        EGL_SAMPLE_BUFFERS, 0,
        EGL_NONE
    };
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
    Window root;
    XSetWindowAttributes swa;
    XSetWindowAttributes  xattr;
    Atom wm_state;
    XWMHints hints;
    XEvent xev;
    EGLint numConfigs;
    EGLint majorVersion;
    EGLint minorVersion;
    EGLConfig config;
    Display *x_display = NULL;

    // Image size
    wnd_width = frame_width = 1920;
    wnd_height = frame_height = 1080;

    // X11 windows initialise
    x_display = XOpenDisplay(NULL);
    if(x_display == NULL) {
        return -1;
    }
    root = DefaultRootWindow(x_display);

    swa.event_mask  =  ExposureMask | PointerMotionMask | KeyPressMask;
    hWnd = XCreateWindow(
               x_display, root,
               0, 0, wnd_width, wnd_height, 0,
               CopyFromParent, InputOutput,
               CopyFromParent, CWEventMask,
               &swa);

    xattr.override_redirect = 0;
    XChangeWindowAttributes(x_display, hWnd, CWOverrideRedirect, &xattr);

    hints.input = 1;
    hints.flags = InputHint;
    XSetWMHints(x_display, hWnd, &hints);

    // make the window visible on the screen
    XMapWindow(x_display, hWnd);
    XStoreName(x_display, hWnd, WINDOWS_TITLE);

    // get identifiers for the provided atom name strings
    wm_state = XInternAtom (x_display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(x_display, "_NET_WM_STATE_FULLSCREEN", False);
    Atom fullmons = XInternAtom(x_display, "_NET_WM_FULLSCREEN_MONITORS", False);
    memset(&xev, 0, sizeof(xev));
    xev.type                 = ClientMessage;
    xev.xclient.window       = hWnd;
    xev.xclient.message_type = fullmons;
    xev.xclient.format       = 32;
    xev.xclient.data.l[0] = 0; /* your topmost monitor number */
    xev.xclient.data.l[1] = 0; /* bottommost */
    xev.xclient.data.l[2] = 0; /* leftmost */
    xev.xclient.data.l[3] = 1; /* rightmost */
    xev.xclient.data.l[4] = 0; /* source indication */
    XSendEvent (
       x_display,
       DefaultRootWindow(x_display),
       0,
       SubstructureNotifyMask,
       &xev);

    // Get Display
    eglDisplay = eglGetDisplay((EGLNativeDisplayType)x_display);
    if (eglDisplay == EGL_NO_DISPLAY) {
        return -1;
    }
    // Initialize EGL
    if (!eglInitialize(eglDisplay, &majorVersion, &minorVersion)) {
        return -1;
    }
    // Get configs
    if (!eglGetConfigs(eglDisplay, NULL, 0, &numConfigs)) {
        return -1;
    }
    // Choose config
    if (!eglChooseConfig(eglDisplay, attribList, &config, 1, &numConfigs)) {
        return -1;
    }
    // Create a surface
    eglSurface = eglCreateWindowSurface(eglDisplay, config, (EGLNativeWindowType)hWnd, NULL);
    if (eglSurface == EGL_NO_SURFACE) {
        return -1;
    }
    // Create a GL context
    eglContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (eglContext == EGL_NO_CONTEXT) {
        return -1;
    }
    // Make the context current
    if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
        return -1;
    }

    return 0;
}

int initShader()
{
    GLuint vertexShader;
    GLuint fragmentShader;
    GLint linked;

    // Data buffer
    yuv_y = (uint8_t*) malloc(frame_width * frame_height);
    yuv_uv = (uint8_t*) malloc(frame_width * frame_height / 2);

    // Load the vertex/fragment shaders
    vertexShader = loadShader(GL_VERTEX_SHADER, vShaderStr);
    if(vertexShader == 0)
       return -1;
    fragmentShader = loadShader(GL_FRAGMENT_SHADER, fShaderStr_NV12);
    if(fragmentShader == 0) {
       glDeleteShader(vertexShader);
       return -1;
    }

    // Create the program object
    programObject = glCreateProgram();
    if(programObject == 0)
       return -1;

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);

    // Link the program
    glLinkProgram(programObject);

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
    if (!linked) {
       /*GLint infoLen = 0;
       glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
       if (infoLen > 1) {
          char* infoLog = malloc (sizeof(char) * infoLen);

          glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
          esLogMessage("Error linking program:\n%s\n", infoLog);

          free(infoLog);
       }*/
       glDeleteProgram(programObject);
       return -1;
    }
    // Free up no longer needed shader resources
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //启动纹理
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glActiveTexture(GL_TEXTURE1);
    // 获取VertexShader变量
    positionLoc = glGetAttribLocation(programObject, "a_position");
    texCoordLoc = glGetAttribLocation(programObject, "a_texCoord");
    // 获取FrameShader变量
    YMapLoc  = glGetUniformLocation(programObject, "tex_y");
    UVMapLoc = glGetUniformLocation(programObject, "tex_uv");
    // 使用滤镜着色器程序
    glUseProgram(programObject);
    //给变量赋值
    glUniform1i(YMapLoc, 0);
    glUniform1i(UVMapLoc, 1);
    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(texCoordLoc);
    // 设置Vertex Shader数据
    glVertexAttribPointer(positionLoc, 2, GL_FLOAT, GL_FALSE, 0, squareVertices);
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, coordVertices);

    //创建yuv纹理
    createTexture();

    return 0;
}

void copy_data(uint8_t const* data)
{
    memcpy(yuv_y, data, frame_width * frame_height);
    memcpy(yuv_uv, data + frame_width * frame_height, frame_width * frame_height / 2);
}

void onDrawFrame()
{
    // 重绘背景色
    glClear(GL_COLOR_BUFFER_BIT);

    // y
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, YTexId);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    frame_width,
                    frame_height,
                    GL_LUMINANCE,
                    GL_UNSIGNED_BYTE,
                    yuv_y);
    // uv
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, UVTexId);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    frame_width >> 1,
                    frame_height >> 1,
                    GL_LUMINANCE_ALPHA,
                    GL_UNSIGNED_BYTE,
                    yuv_uv);

    //绘制
    glViewport(0, 0, frame_width, frame_height);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //
    eglSwapBuffers(eglDisplay, eglSurface);
}

static GLuint loadShader(GLenum type, const char *shaderSrc)
{
   GLuint shader;
   GLint compiled;
   
   // Create the shader object
   shader = glCreateShader(type);
   if(shader == 0)
   	return 0;

   // Load the shader source
   glShaderSource(shader, 1, &shaderSrc, NULL);
   // Compile the shader
   glCompileShader(shader);

   // Check the compile status
   glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
   if (!compiled) {
      /*GLint infoLen = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
      if(infoLen > 1) {
         char* infoLog = malloc (sizeof(char) * infoLen);
         glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
         esLogMessage("Error compiling shader:\n%s\n", infoLog);
         free(infoLog);
      }*/
      glDeleteShader(shader);
      return 0;
   }

   return shader;

}

