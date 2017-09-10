#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <setjmp.h>
#include <jconfig.h>
#include <jpeglib.h>
#include <matstack.h>

#define noop //pusta operacja, tak na wszelki wypadek
#define loop for(;;)

int szerokosc[3]={1920, 640, 922}, wysokosc[3]={1080, 480, 558}, //budowa: 0 (wymiary bryły obcinania), 1 (wymiary okna), 2 (wymiary 16:9)
    win_posx=320, win_posy=180, viewport_x=0, viewport_y=0;

GLint texturing;
GLuint vbo, vao, ebo, tex;
GLenum err;

float vertices[] =
{   //X      Y        R     G     B     A     S     T
    -960.0f, 540.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    960.0f,  540.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    960.0f,  -540.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    -960.0f, -540.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f
};

GLuint elements[] =
{
    0, 1, 2,
    2, 3, 0
};

const char *vertexSource = R"glsl(
    #version 440 core

    in vec2 position;
    in vec4 inColor;
    in vec2 texcoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec4 color;
    out vec2 texCoord;

    void main()
    {
        color = inColor;
        texCoord = texcoord;
        gl_Position = projection * view * model * vec4(position, 0.0, 1.0);
    }
)glsl";

const char *fragmentSource = R"glsl(
    #version 440 core

    in vec4 color;
    in vec2 texCoord;

    uniform sampler2D tex;
    uniform bool doTex;

    out vec4 outColor;

    void main()
    {
        if (doTex) outColor = texture(tex, texCoord) * color;
        else outColor = color;
    }
)glsl";

const char *GLErrString[] =
{
    "GL_NO_ERROR",
    "GL_INVALID_ENUM",
    "GL_INVALID_VALUE",
    "GL_INVALID_OPERATION",
    "GL_INVALID_FRAMEBUFFER_OPERATION",
    "GL_OUT_OF_MEMORY"
};

const char *pgGetGLErrString(GLenum code)
{
    switch (code) {
        /**An unacceptable value is specified for an enumerated argument. The offending command is ignored
        and has no other side effect than to set the error flag.*/
        case GL_INVALID_ENUM: return GLErrString[1];

        /**A numeric argument is out of range. The offending command is ignored and has no other side effect \
        than to set the error flag.*/
        case GL_INVALID_VALUE: return GLErrString[2];

        /**The specified operation is not allowed in the current state. The offending command is ignored \
        and has no other side effect than to set the error flag.*/
        case GL_INVALID_OPERATION: return GLErrString[3];

        /**The framebuffer object is not complete. The offending command is ignored and has no other side effect \
        than to set the error flag.*/
        case GL_INVALID_FRAMEBUFFER_OPERATION: return GLErrString[4];

        /**There is not enough memory left to execute the command. The state of the GL is undefined, \
        except for the state of the error flags, after this error is recorded.*/
        case GL_OUT_OF_MEMORY: return GLErrString[5];

        ///No error has been recorded. The value of this symbolic constant is guaranteed to be 0.
        default: return GLErrString[0]; //nie przewiduję, ale na wszelki wypadek
    }
}

static void errorCallback(int error, const char* description) //póki żyję, jeszcze nie zadziałało xDDD
{
    printf("Blad (nr: %d) - %s\n", error, description);
    //system("pause");
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key == GLFW_KEY_F4 && action == GLFW_RELEASE) { //pełny ekran - okienko
        if (glfwGetWindowMonitor(window) == nullptr) {
            glfwGetWindowPos(window, &win_posx, &win_posy);
            glfwGetWindowSize(window, &szerokosc[2], &wysokosc[2]);
            const GLFWvidmode* vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, vidmode->width, vidmode->height, vidmode->refreshRate);
        }
        else {
            glfwSetWindowMonitor(window, nullptr, win_posx, win_posy, szerokosc[2], wysokosc[2], 0);
        }
    }
}

static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    (void) window;
    wysokosc[1]=height; //dostosowanie tych czarnych pasków
    szerokosc[1]=width;
    float a=width/16, b=height/9;
    a > b ? glViewport((width-b*16)/2, 0, b*16, height) : glViewport(0, (height-a*9)/2, width, a*9);
}

unsigned char *loadJpgImage(const char *filename, int &outWidth, int &outHeight, bool &outHasAlpha, int &outLength)
{
     /* Poniższa struktura zawiera parametry dekompresji JPEG i wskaźniki na
     * przestrzeń roboczą (alokowaną automatycznie przez bibliotekę JPEG).
     */
    unsigned char *outData;
    struct jpeg_decompress_struct cinfo;

     /* Używamy własnej obsługi błędów rozszerzenia JPEG.
     * Pamiętaj, że ta struktura musi istnieć do czasu pozbycia się struktury
     * parametrów, by uniknąć problemów z osieroconymi wskaźnikami.
     */
    //struct my_error_mgr jerr;

     /* Inne rzeczy */
    FILE *infile;		/* plik ze obrazkiem */
    unsigned char* buffer;		/* wyjściowy bufor wiersza */
    int row_stride;		/* szerokość fizycznego wiersza w buforze wyjściowym */

     /* W tym przykładzie chcemy otworzyć plik przed wykonaniem dalszych operacji
     * tak, by setjmp() mogło przyjąć, że plik jest otwarty.
     * BARDZO WAŻNE: użyj opcji "b" w fopen(), jeśli jesteś na maszynie,
     * która wymaga tego, by odczytać plik binarnie.
     */

    if ((infile = fopen(filename, "rb")) == nullptr) {
        char *err_out = new char;
        strcpy(err_out, "Nie mozna otworzyc pliku ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        return nullptr;
    }

     /* Krok 1: alokacja i inicjalizacja obiektu dekompresji JPEG */

     /* Ustawiamy normalną procedurę błędów JPEG, następnie przejmujemy error_exit. */
    jpeg_error_mgr error_mgr;
    cinfo.err = jpeg_std_error(&error_mgr);
    jmp_buf setjmp_buffer;
    //error_mgr.error_exit = my_error_exit;
     /* Ustanowienie czegoś tam setjmp, żeby my_error_exit se tego użyło (MUSZĘ wywalić obsługę błędów). */
    if (setjmp(setjmp_buffer)) {
         /* Wywołanie zwrotne błędu kodu JPEG.
         * Należy posprzątać (obiekt JPEG, plik) i zakończyć.
         */
        char *err_out = new char;
        strcpy(err_out, "BLAD\n\n\n\n\n ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return nullptr;
    }
     /* Teraz możemy stworzyć obiekt dekompresji JPEG. */
    jpeg_create_decompress(&cinfo);

     /* Krok 2: ustalenie źródła danych (np. plik) */

    jpeg_stdio_src(&cinfo, infile);

     /* Krok 3: wczytanie parametrów pliku przez jpeg_read_header() */

    if(!jpeg_read_header(&cinfo, TRUE)) {
        char *err_out = new char;
        strcpy(err_out, "Nie mozna wczytac naglowka pliku ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        return nullptr;
    }
     /* Możemy zignorować wartość zwrotną jpeg_read_header, ponieważ
     *   (a) zawieszenie sie nie jest możliwe przy źródle danych stdio,
     *   (b) drugi parametr (TRUE) odrzuca błędy związane z plikami JPEG opartych na tabelach.
     * Zobacz libjpeg.txt w celu dowiedzenia się, o co chodzi.
     */

     /* Krok 4: ustawienie parametrów dekompresji */

     /* W tym przykładzie nie potrzebujemy zmieniać parametrów domyślnych
     * (ustawionych przez jpeg_read_header()), więc nic nie robimy.
     */

     /* Krok 5: rozpoczęcie dekompresji */

    if (!jpeg_start_decompress(&cinfo)) {
        char *err_out = new char;
        strcpy(err_out, "Nie mozna wczytac pliku ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        return nullptr;
    }
     /* Możemy zignorować wartość zwrotną, bo (patrz (a) wyżej) */

     /* Możemy potrzebować paru zmian przed wczytaniem danych.
     * Po użyciu jpeg_start_decompress() mamy odpowiedniego rozmiaru
     * obraz wyjściowy oraz wyjściową mapę kolorów (jeśli chcieliśmy
     * dokonać kwantyzacji.
     * W tym przykładzie potrzebujemy wyjściowego buforu roboczego o odpowiednim rozmiarze.
     */
     /* JSAMPLE na wiersz w buforze wyjściowym */
    outWidth = cinfo.output_width;
    outHeight = cinfo.output_height;
    outHasAlpha = false;
    row_stride = cinfo.output_width * cinfo.output_components;
    outLength = row_stride * cinfo.output_height;
     /* Jednowierszowa tablica bufora do usunięcia po skończeniu pracy */
    //buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    buffer = new unsigned char[row_stride];

    outData = new unsigned char[outLength];

     /* Krok 6: while (pozostałe do przeczytania wiersze) */
     /*           jpeg_read_scanlines(...); */

     /* Używamy tu zmiennej stanu biblioteki (cinfo.output_scanline) jako
     * licznika pętli, byśmy nie musieli używać własnego.
     */
    while (cinfo.output_scanline < cinfo.output_height) {
         /* jpeg_read_scanlines oczekuje tablicy wskaźników na skanowane wiersze.
         * Tutaj tablica ma tylko jeden element, ale możesz obsługiwać
         * więcej linii na raz, jeśli tak wygodniej.
         */
        (void) jpeg_read_scanlines(&cinfo, &buffer, 1);

        //memcpy(outData+((cinfo.output_scanline-1)*row_stride), buffer, row_stride);
        memcpy(outData + (outLength - (cinfo.output_scanline * row_stride)), buffer, row_stride);
    }

    delete[] buffer;

     /* Krok 7: Finalizacja dekompresji */

    if(!jpeg_finish_decompress(&cinfo)) {
        char *err_out = new char;
        strcpy(err_out, "Nie mozna zakonczyc wczytywania pliku ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        return nullptr;
    }
     /* Znów to samo.
     */

     /* Krok 8: zwolnienie obiektu dekompresji JPEG. */

     /* Ważny krok, uwalniający niemałą porcję pamięci. */
    jpeg_destroy_decompress(&cinfo);


     /* Po użyciu jpeg_finish_decompress możemy zamknąć plik. */
    fclose(infile);

     /* At this point you may want to check to see whether any corrupt-data
     * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
     */

     /* And we're done! */
    return outData;
}

static void glfwDisplay(GLFWwindow* window)
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    uint8_t mat_err = matrix_stack::current->getLastError();
    if (mat_err) errorCallback(mat_err, matrix_stack::getErrorString(mat_err));

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-960, 960, -540, 540, 1, -1);

    mat_err = matrix_stack::current->getLastError();
    if (mat_err) errorCallback(mat_err, matrix_stack::getErrorString(mat_err));

    glUniform1i(texturing, 1);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glUniform1i(texturing, 0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glScale(0.5, 0.5, 0.5);
    glRotate(0, 0, 1, 10);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glPopMatrix();

    err = glGetError();
    if (err) errorCallback(err, pgGetGLErrString(err));

    glFlush();
    glfwSwapBuffers(window);

    glfwPollEvents();
}

int main(void)
{
    GLFWwindow* window;
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) exit(EXIT_FAILURE);

    //zapasowy komentarz do szybkiego przełączania początkowego stanu aplikacji: okienko - pełny ekran
    //window = glfwCreateWindow(szerokosc[0], wysokosc[0], "OpenGL", glfwGetPrimaryMonitor(), NULL);
    window = glfwCreateWindow(szerokosc[1], wysokosc[1], "OpenGL", nullptr, nullptr);
    //glfwSetWindowPos(window, init_posx, init_posy);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    //obsluga klawiatury, myszy, okna
    glfwSetKeyCallback(window, keyCallback);
    //glfwSetMouseButtonCallback(window, mouseButtonCallback);
    //glfwSetCursorPosCallback(window, cursorPosCallback);
    //glfwSetWindowSizeCallback(window, windowSizeCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwMakeContextCurrent(window);

    if (!gladLoadGL()) {
        errorCallback(-1, "GLAD: Something went wrong");
        exit(EXIT_FAILURE);
    }

    //vsync
    glfwSwapInterval(1);

    printf("%s\n", glGetString(GL_VERSION));
    printf("%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    ///

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    GLint status;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (!status) errorCallback(status, "Vertex shader");
    char buffer[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    if (buffer[0] != '\0') printf("Vertex shader log: %s\n", buffer);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (!status) errorCallback(status, "Fragment shader");
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    if (buffer[0] != '\0') printf("Fragment shader log: %s\n", buffer);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "inColor");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(2*sizeof(float)));

    GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(6*sizeof(float)));

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    Mmat_stack = new matrix_stack(glGetUniformLocation(shaderProgram, "model"));
    Vmat_stack = new matrix_stack(glGetUniformLocation(shaderProgram, "view"));
    Pmat_stack = new matrix_stack(glGetUniformLocation(shaderProgram, "projection"), 2);

    framebufferSizeCallback(window, szerokosc[1], wysokosc[1]);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ///
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    unsigned char *tData;
    int tWidth, tHeight, tLength;
    bool tAlpha;
    tData = loadJpgImage("forest.jpg", tWidth, tHeight, tAlpha, tLength);

    glTexImage2D(GL_TEXTURE_2D, 0, tAlpha ? GL_RGBA : GL_RGB, tWidth, tHeight,
                 0, tAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, tData);
    glGenerateMipmap(GL_TEXTURE_2D);
    //glBindTexture(GL_TEXTURE_2D, 0);

    texturing = glGetUniformLocation(shaderProgram, "doTex");
    glUniform1i(texturing, 1);

    ///

    //główna pętla, polecam
    while (!glfwWindowShouldClose(window)) glfwDisplay(window);

    glfwDestroyWindow(window);
    glfwTerminate();
    glDetachShader(shaderProgram, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(shaderProgram, fragmentShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(shaderProgram);

    exit(EXIT_SUCCESS);
}
