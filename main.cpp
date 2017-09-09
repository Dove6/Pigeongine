/**
    main.cpp
    Przeznaczenie: główny plik programu, wyświetla okno i zarządza jego zawartością.

    @author Dove
    @version 0.1.1 8/16/16
*/

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <process.h>

#define noop //pusta operacja, tak na wszelki wypadek
#define loop for(;;)

#include "declarations.h"

#include "matstack.h"

GLenum err;

int szerokosc[3]={1920, 992, 922},//
    wysokosc[3]={1080, 558, 558}, //budowa: 0 (wymiary bryły obcinania), 1 (wymiary okna), 2 (wymiary 16:9)
    win_posx=320, win_posy=180, viewport_x=0, viewport_y=0;
double dest_posx=0, dest_posy=0, curr_posx=0, curr_posy=0, tryg_c=0, angle_x=0, angle_y=0, x_step=0, y_step=0,
    czas=0, past_time=0, animacja=0, animacja_back;
int new_width=szerokosc[1], new_height=wysokosc[1], pop_return=0;
bool pauza=false;
GLuint texID[4];

#include "alt_font.h"

texture *g_zubr, *g_forest, *g_pigeon, *g_font;

#if defined(_WIN32)
 HWND win32_handle;
 HINSTANCE win32_instance;
 HMENU popup_menu;
#elif defined(linux)
 Display *x11_display;
 Window x11_handle, popup;
 XWindowChanges pop_changes={0, 0, 0, 0, 0, 0, 0};
 int screen, font_direction, font_ascent, font_descent;
 XFontStruct *fontinfo;
 XGCValues gr_values;
 GC graphical_context[2];
 char *pop_option[4] = {"Stop", "Skok", "Kontynuacja", "Koniec"};
 XCharStruct text_structure;
 XEvent x11_event;
 bool pop_linux=false;
#endif

//wzór przydatnych później flag do poszczególnych klawiszy, dodałem, żeby nie szukać
/* klawisze
GLboolean <klawisz>_REPEAT=0; */

/********************************************************************************************************************************************************************************************************/

// Funkcja główna
int main(void)
{
    GLFWwindow* window; //nowy obiekt okna
    glfwSetErrorCallback(errorCallback); //ustawienie specyficznego wywołania zwrotnego
    if (!glfwInit()) exit(EXIT_FAILURE); //inicjalizacja GLFW

    const GLFWvidmode* kreacja = glfwGetVideoMode(glfwGetPrimaryMonitor());
    wysokosc[1]=(kreacja->height)*3/4;
    szerokosc[1]=wysokosc[1]/9*16;
    int init_posx = ((kreacja->width)-szerokosc[1])/2, init_posy = ((kreacja->height)-wysokosc[1])/2;
    if (init_posx<0) init_posx=0;
    if (init_posy<0) init_posy=0;
    //zapasowy komentarz do szybkiego przełączania początkowego stanu aplikacji: okienko - pełny ekran
    //window = glfwCreateWindow(szerokosc[0], wysokosc[0], "Weather Pigeon", glfwGetPrimaryMonitor(), NULL);
    window = glfwCreateWindow(szerokosc[1], wysokosc[1], "Weather Pigeon", nullptr, nullptr);
    glfwSetWindowPos(window, init_posx, init_posy);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    //obsluga klawiatury, myszy, okna
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwMakeContextCurrent(window);

    if (!gladLoadGL()) {
        errorCallback(-1, "GLAD: Something went wrong");
        exit(EXIT_FAILURE);
    }

    #ifdef GLAD_DEBUG
    // before every opengl call call pre_gl_call
    glad_set_pre_callback(pre_gl_call);

    // post callback checks for glGetError by default

    // don't use the callback for glClear
    // (glClear could be replaced with your own function)
    glad_debug_glClear = glad_glClear;
    #endif

    //vsync
    glfwSwapInterval(1);

    //jakieś funkcje
    glEnable(GL_LINE_SMOOTH);
    //glEnable(GL_POINT_SMOOTH);
    #pragma message("TODO: point fade (I think pixel shader)")
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    //glEnableClientState(GL_VERTEX_ARRAY);
    //glVertexPointer(2, GL_FLOAT, 0, Rex_vertex);

    glEnable(GL_TEXTURE_2D);
    texture zubr("zubr.png", &texID[0], 0), forest("forest.jpg", &texID[1], 1), pigeon("pigeon.png", &texID[2], 0), font("font_alt_27x42.png", &texID[3], 0);
    g_zubr = &zubr;
    g_forest = &forest;
    g_pigeon = &pigeon;
    g_font = &font;

    #if defined(_WIN32)
     win32_handle = glfwGetWin32Window(window);
     win32_instance = (HINSTANCE)GetWindowLongPtr(win32_handle, GWLP_HINSTANCE);
     popup_menu = CreatePopupMenu();
     InsertMenu(popup_menu, 0, MF_BYPOSITION | MF_STRING, 3, "Koniec");
     InsertMenu(popup_menu, 0, MF_BYPOSITION | MF_STRING, 2, "Kontynuacja");
     InsertMenu(popup_menu, 0, MF_BYPOSITION | MF_STRING, 1, "Skok");
     InsertMenu(popup_menu, 0, MF_BYPOSITION | MF_STRING, 0, "Stop");
     SetForegroundWindow(win32_handle);
    #elif defined(linux)
     x11_display=glfwGetX11Display();
     x11_handle=glfwGetX11Window(window);
     screen = DefaultScreen(x11_display);
     fontinfo = XLoadQueryFont(x11_display, "10x20");
     gr_values.font = fontinfo->fid;
     gr_values.foreground = XBlackPixel(x11_display, 0);
     popup=XCreateSimpleWindow(x11_display, x11_handle, 0, 0, 150, 90, 1, BlackPixel(x11_display, screen), WhitePixel(x11_display, screen)/2);
     graphical_context[0] = XCreateGC(x11_display, popup, GCFont+GCForeground, &gr_values);
     gr_values.foreground = XWhitePixel(x11_display, 0);
     graphical_context[1] = XCreateGC(x11_display, popup, GCFont+GCForeground, &gr_values);
     //XSelectInput (x11_display, RootWindow(x11_display, DefaultScreen(x11_display)), SubstructureNotifyMask);
    #endif

    //główna pętla, polecam
    while (!glfwWindowShouldClose(window)) glfwDisplay(window/*, &zubr, &forest*/);

    #if defined(linux)
    XDestroyWindow(x11_display, popup);
    #endif
    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}

#include "implementations.h"
