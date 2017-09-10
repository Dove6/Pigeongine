//<<<Obecne linkowanie>>>
//Windows: libglew32.a, libopengl, libglfw3, winmm, gdi32
//Linux (Ubuntu 14.04): libX11-xcb.so, libX11.so, libXxf86vm.so, libXrandr.so, libpthread.so, libXi.so, libGL.so (/usr/lib/x86_64-linux-gnu)
//                      libGLEW.a, libglfw3.so (/usr/lib)

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#if !defined(linux) && defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)
    #define linux
#endif
#if defined(_WIN32)
 #define GLFW_EXPOSE_NATIVE_WIN32
 #define GLFW_EXPOSE_NATIVE_WGL
 #include <windows.h>
#elif defined(linux)
 #include <X11/Xlibint.h>
 #include <X11/Xutil.h>
 #include <X11/StringDefs.h>
 #include <unistd.h>
 #define GLFW_EXPOSE_NATIVE_X11
 #define GLFW_EXPOSE_NATIVE_GLX
#endif
#include <GLFW/glfw3native.h>
#include <zlib.h>
#include <png.h>
#include <jconfig.h>
#include <jpeglib.h>

#define PI 3.14159265

#ifdef GLAD_DEBUG
//logs every gl call to the console
void pre_gl_call(const char *name, void *funcptr, int len_args, ...);
#endif

/**
    Wywołanie zwrotne dotyczące błędów.

    @param error kod błędu
    @param description wskaźnik na opis błędu
*/
static void errorCallback(int error, const char* description);

/**
    Funkcja wczytania danych obrazu z pliku w formacie .png.

    @param name nazwa pliku
    @param outWidth wskaźnik dla zwrotu szerokości obrazu
    @param outHeight wskaźnik dla zwrotu wysokości obrazu
    @param outHasAlpha wskaźnik dla zwrotu alfa/nie
    @param outLength wskaźnik dla zwrotu rozmiaru tablicy
*/
unsigned char* loadPngImage(const char *name, int &outWidth, int &outHeight, bool &outHasAlpha, int &outLength);

/**
    Funkcja wczytania danych obrazu z pliku w formacie .jpg.

    @param filename nazwa pliku
    @param outWidth wskaźnik dla zwrotu szerokości obrazu
    @param outHeight wskaźnik dla zwrotu wysokości obrazu
    @param outHasAlpha wskaźnik dla zwrotu alfa/nie
    @param outLength wskaźnik dla zwrotu rozmiaru tablicy
*/
unsigned char *loadJpgImage(const char *filename, int &outWidth, int &outHeight, bool &outHasAlpha, int &outLength);

class texture{
public:
    int width;
    int height;
    bool alpha;
    GLubyte *data;
    int length;
    GLuint *tID;

    texture();

    texture(const char *name, int format);

    texture(const char *name, GLuint *ID, int format);

    texture(const texture &a);

    texture &operator =(const texture &a);

    ~texture();

    void chDATA(const char *name);

    void chID(GLuint *ID);

    bool bind();

    bool unbind();

    bool draw(float x, float y, float z);
};

/**
    Główna pętla programu. Zawiera m.in. dane nt. renderowania obrazu.

    @param window wskaźnik na zmienną okna GLFW
*/
static void glfwDisplay(GLFWwindow* window/*, texture *zubr, texture *forest*/);

/**
    Wywołanie zwrotne obsługujące klawiaturę.

    @param window wskaźnik na zmienną okna GLFW
    @param key kod klawisza
    @param scancode kod klawisza zale¿ny od środowiska
    @param action akcja
    @param mods bity modyfikacji
*/
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

/**
    Wywołanie zwrotne pozycji kursora.

    @param window wskaźnik na zmienną okna GLFW
    @param xpos pozycja na osi X
    @param ypos pozycja na osi Y
*/
static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

/**
    Wywołanie zwrotne przycisku myszy.

    @param window wskaźnik na zmienną okna GLFW
    @param button kod przycisku
    @param action akcja
    @param mods bity modyfikacji
*/
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

/**
    Wywołanie zwrotne rozmiaru okna.

    @param window wskaźnik na zmienną okna GLFW
    @param width szerokośæ okna
    @param height wysokośæ okna
*/
void windowSizeCallback(GLFWwindow* window, int width, int height);

/**
    Wywołanie zwrotne rozmiaru bufora ramki.

    @param window wskaźnik na zmienną okna GLFW
    @param width szerokośæ bufora ramki
    @param height wysokośæ bufora ramki
*/
void framebufferSizeCallback(GLFWwindow* window, int width, int height);

/**
    Menu kontekstowe (WinAPI/X11).

    @param xpos pozycja kursora (oś x)
    @param ypos pozycja kursora (oś y)
    @param xpos2 pozycja okna (oś x)
    @param xpos2 pozycja okna (oś y)
*/
void pgContextMenu(double xpos, double ypos, int xpos2, int ypos2);

void pgDrawText(const char *tekst, int ras_x, int ras_y);

#if defined(linux)
/*
    Menu kontekstowe dla Linuksa.

    @param window wskaźnik na zmienną okna GLFW
*/
void XPopupMenu(GLFWwindow *window);
#endif
