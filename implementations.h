#ifdef GLAD_DEBUG
//logs every gl call to the console
void pre_gl_call(const char *name, void *funcptr, int len_args, ...) {
    printf("Calling: %s (%d arguments)\n", name, len_args);
}
#endif

static void errorCallback(int error, const char* description) //póki ¿yjê, jeszcze nie zadzia³a³o xDDD
{
    printf("Blad (nr: %d) - %s\n", error, description);
    system("pause");
}

unsigned char *loadPngImage(const char *name, int &outWidth, int &outHeight, bool &outHasAlpha, int &outLength)
{
    png_structp png_ptr;
    png_infop info_ptr;
    unsigned int sig_read = 0;
    int color_type, interlace_type;
    FILE *fp;
    unsigned char* outData;

    if ((fp = fopen(name, "rb")) == nullptr)
        return nullptr;

    /* Tworzenie i inicjalizacja png_struct
     * z obs³ug¹ b³êdów. Dla stderr i longjump
     * (domyœlne) u¿yj NULL w trzech ostatnich
     * parametrach. Wersja dla sprawdzenia
     * kompatybilnoœci.   WYMAGANE. */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                     nullptr, nullptr, nullptr);

    if (png_ptr == nullptr)
        {fclose(fp);
        return nullptr;
        }

    /* Alokacja i inicjalizacja
     * pamiêci pod obraz.  WYMAGANE. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
        {fclose(fp);
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return nullptr;
        }

    /* Obs³uga b³êdów dla setjmp/longjmp
     * WYMAGANE, jeœli wczeœniej nie ustawi³eœ
     * innego sposobu obs³ugi b³êdów. */
    if (setjmp(png_jmpbuf(png_ptr)))
        {/* Czyszczenie pamiêci z png_ptr i info_ptr */
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        fclose(fp);
        /* Problem z wczytaniem pliku */
        return nullptr;
        }

    /* Kontrola wyjœcia dla strumienia C */
    png_init_io(png_ptr, fp);

    /* Jeœli wczytano trochê sygnatury */
    png_set_sig_bytes(png_ptr, sig_read);

    /*
     * Jeœli masz tyle pamiêci, by wczytaæ
     * ca³y obrazek na raz i potrzebujesz
     * jedynie ustaliæ przekszta³cenia
     * za pomoc¹ bitów PNG_TRANSFORM_*
     * (nie zawiera mieszania kolorów,
     * wype³niania, ustalania t³a
     * i regulacji gammy), mo¿esz wczytaæ
     * ca³y obraz (wraz z pikselami)
     * do struktury info za pomoc¹
     * odwo³ania
     *
     * PNG_TRANSFORM_STRIP_16 |
     * PNG_TRANSFORM_PACKING  (wymusza 8-bit)
     * PNG_TRANSFORM_EXPAND (wymusza
     *  rozszerzenie palety do RGB)
     */
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, nullptr);

    png_uint_32 png_width, png_height;
    int bit_depth;
    png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bit_depth, &color_type,
                 &interlace_type, nullptr, nullptr);
    outWidth = png_width;
    outHeight = png_height;
    outHasAlpha = (color_type == 6);

    unsigned int row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    outLength = row_bytes * outHeight;
    outData = new unsigned char[outLength];

    png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);

    for (int i = 0; i < outHeight; i++) {
        /* (png jest u³o¿one od góry
           do do³u, a OpenGL oczekuje odwrotnej
           kolejnoœci -> zmiana u³o¿enia) */
        memcpy(outData+(row_bytes * (outHeight-1-i)), row_pointers[i], row_bytes);
    }

    /* Porz¹dki i zwolnienie pamiêci
     * po wczytaniu */
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    /* Zamkniêcie pliku */
    fclose(fp);

    /* Zakoñczenie funkcji */
    return outData;
}



/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

//struct my_error_mgr {
//    struct jpeg_error_mgr pub;	/* "public" fields */

//    std::jmp_buf setjmp_buffer;	/* for return to caller */
//};

//typedef struct my_error_mgr *my_error_ptr;

//void my_error_exit(j_common_ptr cinfo)
//{
     /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
//    my_error_ptr myerr = (my_error_ptr) cinfo->err;

     /* Always display the message. */
     /* We could postpone this until after returning, if we chose. */
//    (*cinfo->err->output_message) (cinfo);

     /* Return control to the setjmp point */
//    longjmp(myerr->setjmp_buffer, 1);
//}
unsigned char *loadJpgImage(const char *filename, int &outWidth, int &outHeight, bool &outHasAlpha, int &outLength)
{
     /* Poni¿sza struktura zawiera parametry dekompresji JPEG i wskaŸniki na
     * przestrzeñ robocz¹ (alokowan¹ automatycznie przez bibliotekê JPEG).
     */
    unsigned char *outData;
    struct jpeg_decompress_struct cinfo;

     /* U¿ywamy w³asnej obs³ugi b³êdów rozszerzenia JPEG.
     * Pamiêtaj, ¿e ta struktura musi istnieæ do czasu pozbycia siê struktury
     * parametrów, by unikn¹æ problemów z osieroconymi wskaŸnikami.
     */
    //struct my_error_mgr jerr;

     /* Inne rzeczy */
    FILE *infile;		/* plik ze obrazkiem */
    unsigned char* buffer;		/* wyjœciowy bufor wiersza */
    int row_stride;		/* szerokoœæ fizycznego wiersza w buforze wyjœciowym */

     /* W tym przyk³adzie chcemy otworzyæ plik przed wykonaniem dalszych operacji
     * tak, by setjmp() mog³o przyj¹æ, ¿e plik jest otwarty.
     * BARDZO WA¯NE: u¿yj opcji "b" w fopen(), jeœli jesteœ na maszynie,
     * która wymaga tego, by odczytaæ plik binarnie.
     */

    if ((infile = fopen(filename, "rb")) == nullptr) {
        char *err_out = new char;
        strcpy(err_out, "Nie mozna otworzyc pliku ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        return nullptr;
    }

     /* Krok 1: alokacja i inicjalizacja obiektu dekompresji JPEG */

     /* Ustawiamy normaln¹ procedurê b³êdów JPEG, nastêpnie przejmujemy error_exit. */
    jpeg_error_mgr error_mgr;
    cinfo.err = jpeg_std_error(&error_mgr);
    jmp_buf setjmp_buffer;
    //error_mgr.error_exit = my_error_exit;
     /* Ustanowienie czegoœ tam setjmp, ¿eby my_error_exit se tego u¿y³o (MUSZÊ wywaliæ obs³ugê b³êdów). */
    if (setjmp(setjmp_buffer)) {
         /* Wywo³anie zwrotne b³êdu kodu JPEG.
         * Nale¿y posprz¹taæ (obiekt JPEG, plik) i zakoñczyæ.
         */
        char *err_out = new char;
        strcpy(err_out, "BLAD\n\n\n\n\n ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return nullptr;
    }
     /* Teraz mo¿emy stworzyæ obiekt dekompresji JPEG. */
    jpeg_create_decompress(&cinfo);

     /* Krok 2: ustalenie Ÿród³a danych (np. plik) */

    jpeg_stdio_src(&cinfo, infile);

     /* Krok 3: wczytanie parametrów pliku przez jpeg_read_header() */

    if(!jpeg_read_header(&cinfo, TRUE)) {
        char *err_out = new char;
        strcpy(err_out, "Nie mozna wczytac naglowka pliku ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        return nullptr;
    }
     /* Mo¿emy zignorowaæ wartoœæ zwrotn¹ jpeg_read_header, poniewa¿
     *   (a) zawieszenie sie nie jest mo¿liwe przy Ÿródle danych stdio,
     *   (b) drugi parametr (TRUE) odrzuca b³êdy zwi¹zane z plikami JPEG opartych na tabelach.
     * Zobacz libjpeg.txt w celu dowiedzenia siê, o co chodzi.
     */

     /* Krok 4: ustawienie parametrów dekompresji */

     /* W tym przyk³adzie nie potrzebujemy zmieniaæ parametrów domyœlnych
     * (ustawionych przez jpeg_read_header()), wiêc nic nie robimy.
     */

     /* Krok 5: rozpoczêcie dekompresji */

    if (!jpeg_start_decompress(&cinfo)) {
        char *err_out = new char;
        strcpy(err_out, "Nie mozna wczytac pliku ");
        strcpy(err_out, filename);
        errorCallback(0, err_out);
        return nullptr;
    }
     /* Mo¿emy zignorowaæ wartoœæ zwrotn¹, bo (patrz (a) wy¿ej) */

     /* Mo¿emy potrzebowaæ paru zmian przed wczytaniem danych.
     * Po u¿yciu jpeg_start_decompress() mamy odpowiedniego rozmiaru
     * obraz wyjœciowy oraz wyjœciow¹ mapê kolorów (jeœli chcieliœmy
     * dokonaæ kwantyzacji.
     * W tym przyk³adzie potrzebujemy wyjœciowego buforu roboczego o odpowiednim rozmiarze.
     */
     /* JSAMPLE na wiersz w buforze wyjœciowym */
    outWidth = cinfo.output_width;
    outHeight = cinfo.output_height;
    outHasAlpha = false;
    row_stride = cinfo.output_width * cinfo.output_components;
    outLength = row_stride * cinfo.output_height;
     /* Jednowierszowa tablica bufora do usuniêcia po skoñczeniu pracy */
    //buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    buffer = new unsigned char[row_stride];

    outData = new unsigned char[outLength];

     /* Krok 6: while (pozosta³e do przeczytania wiersze) */
     /*           jpeg_read_scanlines(...); */

     /* U¿ywamy tu zmiennej stanu biblioteki (cinfo.output_scanline) jako
     * licznika pêtli, byœmy nie musieli u¿ywaæ w³asnego.
     */
    while (cinfo.output_scanline < cinfo.output_height) {
         /* jpeg_read_scanlines oczekuje tablicy wskaŸników na skanowane wiersze.
         * Tutaj tablica ma tylko jeden element, ale mo¿esz obs³ugiwaæ
         * wiêcej linii na raz, jeœli tak wygodniej.
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

     /* Wa¿ny krok, uwalniaj¹cy niema³¹ porcjê pamiêci. */
    jpeg_destroy_decompress(&cinfo);


     /* Po u¿yciu jpeg_finish_decompress mo¿emy zamkn¹æ plik. */
    fclose(infile);

     /* At this point you may want to check to see whether any corrupt-data
     * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
     */

     /* And we're done! */
    return outData;
}

texture::texture()
    :width(0), height(0), alpha(false), data(nullptr), length(0), tID(nullptr)
{
}

texture::texture(const char *name, int format)
    :width(0), height(0), alpha(false), data(nullptr), length(0), tID(nullptr)
{
    switch (format) {
        case 0: this->data = loadPngImage(name, this->width, this->height, this->alpha, this->length);
            break;
        case 1: this->data = loadJpgImage(name, this->width, this->height, this->alpha, this->length);
            break;
        default: this->data = loadPngImage(name, this->width, this->height, this->alpha, this->length);
            break;
    }
    printf("Plik: %s\n", name);
    printf("Adres: %p\n", (void *)this->data);
    printf("Rozmiar: %d kB\n\n", this->length/1000);
}

texture::texture(const char *name, GLuint *ID, int format)
    :width(0), height(0), alpha(false), data(nullptr), length(0), tID(ID)
{
    glGenTextures(1, tID);
    glBindTexture(GL_TEXTURE_2D, *tID);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    switch (format) {
        case 0: this->data = loadPngImage(name, this->width, this->height, this->alpha, this->length);
            break;
        case 1: this->data = loadJpgImage(name, this->width, this->height, this->alpha, this->length);
            break;
        default: this->data = loadPngImage(name, this->width, this->height, this->alpha, this->length);
            break;
    }
    printf("Plik: %s\n", name);
    printf("Adres: %p\n", (void *)this->data);
    printf("Rozmiar: %d kB\n\n", this->length/1000);

    glTexImage2D(GL_TEXTURE_2D, 0, this->alpha ? GL_RGBA : GL_RGB, this->width, this->height,
                 0, this->alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, this->data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

texture::texture(const texture &a)
    :width(a.width), height(a.height), alpha(a.alpha), data(nullptr), length(a.length), tID(nullptr)
{
    this->data = new GLubyte[this->length];
    memcpy(this->data, a.data, this->length);
}

texture &texture::operator =(const texture &a)
{
    if(&a != this) {
        this->width = a.width;
        this->height = a.height;
        this->alpha = a.alpha;
        this->length = a.length;
        this->data = new GLubyte[this->length];
        memcpy(this->data, a.data, this->length);
        this->tID = a.tID;
    }
    return *this;
}

texture::~texture()
{
    if (glIsTexture(*this->tID)) glDeleteTextures(1, tID);
    delete this->tID;
    delete &this->length;
    delete[] this->data;
    delete &this->alpha;
    delete &this->height;
    delete &this->width;
}

void texture::chDATA(const char *name)
{
    this->data = loadPngImage(name, this->width, this->height, this->alpha, this->length);
}

void texture::chID(GLuint *ID)
{
    this->tID = ID;
}

bool texture::bind()
{
    if(this->tID == nullptr) return false;
    if(!glIsTexture(*this->tID)) return false;
    if(this->data == nullptr) return false;
    glGenTextures(1, tID);
    glBindTexture(GL_TEXTURE_2D, *tID);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, this->alpha ? GL_RGBA : GL_RGB, this->width, this->height,
                 0, this->alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, this->data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool texture::unbind()
{
    if(this->tID == nullptr) return false;
    if(!glIsTexture(*this->tID)) return false;
    glDeleteTextures(1, tID);
    return true;
}

bool texture::draw(float x, float y, float z)
{
    if(!glIsTexture(*this->tID)) return false;
    glBindTexture(GL_TEXTURE_2D, *this->tID);
    glBegin(GL_QUADS);
    glVertex3f((-this->width/2)+x, (-this->height/2)+y, z);
    glTexCoord2f(0, 1);
    glVertex3f((-this->width/2)+x, (this->height/2)+y, z);
    glTexCoord2f(1, 1);
    glVertex3f((this->width/2)+x, (this->height/2)+y, z);
    glTexCoord2f(1, 0);
    glVertex3f((this->width/2)+x, (-this->height/2)+y, z);
    glTexCoord2f(0, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

static void glfwDisplay(GLFWwindow* window/*, texture *zubr, texture *forest*/)
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glPushMatrix(); //macierz bia³ego prostok¹ta xD
    glColor3f(1, 1, 1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    g_forest->draw(0, 0, 0);
    glPopMatrix();

    glPushMatrix(); //macierz z kwadratem

    tryg_c=sqrt(pow(dest_posx-curr_posx, 2)+pow(dest_posy-curr_posy, 2)); //oblicza za ka¿dym razem, co pozwala np. na powrót po miniêciu celu
    angle_x=asin((dest_posx-curr_posx)/tryg_c)*180/PI;
    angle_y=asin((dest_posy-curr_posy)/tryg_c)*180/PI;
    x_step = sin(angle_x*PI/180);
    y_step = sin(angle_y*PI/180);

    czas = glfwGetTime()-past_time;
    past_time = glfwGetTime();

    //poruszanie siê obiektu
    if (numeric_limits<float>::epsilon() > abs(curr_posx - dest_posx) && std::numeric_limits<float>::epsilon() > abs(curr_posy - dest_posy)) {
        switch (pop_return) {
            case 0: animacja=animacja_back;
                break;
            case 1: animacja=46;
                break;
            case 2: if (animacja>46) animacja = 46;
                break;
            default: animacja-=czas*60;
                break;
        }
    }
    else if (((dest_posx-curr_posx)<2.1 && (dest_posx-curr_posx)>-2.1) && ((dest_posy-curr_posy)<2.1 && (dest_posy-curr_posy)>-2.1)) {
        curr_posx=dest_posx;
        curr_posy=dest_posy;
    }
    else {
        curr_posx+=x_step*czas*200;
        curr_posy+=y_step*czas*200;
    }
    animacja_back = animacja;

    //czasem pojawia siê pewien b³¹d - jedna z pozycji przyjmuje wartoœæ NaN i kwadrat znika
    if (std::isnan(curr_posx)) system("pause");
    if (std::isnan(curr_posy)) system("pause");

    //to, co program wypluwa na konsolê - pozycja obecna, pozycja docelowa, stan animacji
    printf("%f x %f -> %f x %f\n%f\n", curr_posx, curr_posy, dest_posx, dest_posy, animacja);

    glTranslate(curr_posx, curr_posy, 0);
    //glRotatef(180, 0, 1, 0);
    //glScalef(4, 4, 0);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(5);
    glPointSize(5);

    glColor4f(1, 1, 1, 1);

    g_zubr->draw(0, 0, 0);
    g_font->draw(0, 0, 0);

    pgDrawText("xD placek", -400, 400);

    glPopMatrix();

    glPushMatrix();

    glTranslated((animacja-30)*45, 0, 0);

    glColor4f(1, 1, 1, 1);

    g_pigeon->draw(0, 0, 0);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-szerokosc[0]/2, szerokosc[0]/2, -wysokosc[0]/2, wysokosc[0]/2, -100, 100); //"2D"

    animacja+=czas*60; //"animacja"
    if (animacja>60) animacja=0;
    if (animacja<0) animacja=0;

    err = glGetError();
    if (err) errorCallback(err, "Blad");

    glFlush();
    glfwSwapBuffers(window);

    glfwPollEvents();
    #if defined(linux)
    if (pop_linux) XPopupMenu(window);
    #endif
    if (pop_return==3) glfwSetWindowShouldClose(window, GL_TRUE);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    #if (GLFW_VERSION_MAJOR == 3) && (GLFW_VERSION_MINOR > 1)
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
    #endif

    //druga czêœæ przypominajki o flagach dla klawiszy
    /* wiele klawiszy
    if (glfwGetKey(window, GLFW_KEY_<klawisz>)==GLFW_PRESS) {
        <klawisz>_REPEAT=true;
    }
    else if (glfwGetKey(window, GLFW_KEY_<klawisz>)==GLFW_RELEASE) {
        <klawisz>_REPEAT=false;
    } */
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    (void) window;
    (void) xpos;
    (void) ypos;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void) mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) { //wyznaczenie celu
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (xpos >= viewport_x && xpos <= szerokosc[1]-viewport_x && ypos >= viewport_y && ypos <= wysokosc[1]-viewport_y) {
            dest_posx = (xpos-szerokosc[1]/2)/(szerokosc[1]-2*viewport_x)*szerokosc[0];
            dest_posy = -(ypos-wysokosc[1]/2)/(wysokosc[1]-2*viewport_y)*wysokosc[0];
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) { //obs³uga menu kontekstowego
        double xpos, ypos;
        int xpos2, ypos2;
        glfwGetCursorPos(window, &xpos, &ypos);
        glfwGetWindowPos(window, &xpos2, &ypos2);
        pgContextMenu(xpos, ypos, xpos2, ypos2);
    }
}

void windowSizeCallback(GLFWwindow* window, int width, int height)
{
    (void) window;
    (void) width;
    (void) height;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    (void) window;
    wysokosc[1]=height; //dostosowanie tych czarnych pasków
    szerokosc[1]=width;
    float a=width/16, b=height/9;
    viewport_x=0;
    viewport_y=0;
    new_width=width;
    new_height=height;
    if (a>b) {
        new_width=b*16;
        viewport_x=(width-new_width)/2;
    }
    else if (a<b) {
        new_height=a*9;
        viewport_y=(height-new_height)/2;
    }
    glViewport(viewport_x, viewport_y, new_width, new_height);
    #if defined(_WIN32)
    //glfwDisplay(window);
    #endif
}

void pgContextMenu(double xpos, double ypos, int xpos2, int ypos2)
{
    #if defined(_WIN32)
     pop_return = TrackPopupMenu(popup_menu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTBUTTON, xpos+xpos2, ypos+ypos2, 0, win32_handle, nullptr);
    #elif defined(linux)
     pop_changes.x=xpos;
     pop_changes.y=ypos;
     XConfigureWindow(x11_display, popup, CWX, &pop_changes);
     XConfigureWindow(x11_display, popup, CWY, &pop_changes);
     XMapWindow(x11_display, popup);
     XDrawString(x11_display, popup, graphical_context[0], 5, 20, pop_option[0].c_str(), strlen(pop_option[0].c_str()));
     XDrawString(x11_display, popup, graphical_context[0], 5, 40, pop_option[1].c_str(), strlen(pop_option[1].c_str()));
     XDrawString(x11_display, popup, graphical_context[0], 5, 60, pop_option[2].c_str(), strlen(pop_option[2].c_str()));
     XDrawString(x11_display, popup, graphical_context[0], 5, 80, pop_option[3].c_str(), strlen(pop_option[3].c_str()));
     pop_linux=true;
    #endif
    //_endthread();
}

void pgDrawText(const char *tekst, int ras_x, int ras_y)
{
    int a = strlen(tekst);
    glRasterPos2f(ras_x, ras_y);
    for(int i=0; i<a; i++) {
        glBitmap(32, 48, 0, 0, 0, 0, alt_font[(unsigned char)tekst[i]]);
        ras_x+=44;
        glRasterPos2f(ras_x, ras_y);
    }
}

#if defined(linux)
void XPopupMenu(GLFWwindow *window)
{
    bool left_click=false;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS) left_click=true;
    glfwGetCursorPos(window, &xpos, &ypos);
    if ((xpos-pop_changes.x)>0 && (xpos-pop_changes.x)<150) {
        if ((ypos-pop_changes.y)>0 && (ypos-pop_changes.y)<30) {
            XDrawString(x11_display, popup, graphical_context[1], 5, 20, pop_option[0].c_str(), strlen(pop_option[0].c_str()));
            if (left_click) {
                XUnmapWindow(x11_display, popup);
                pop_linux=false;
                pop_return = 0;
            }
        }
        else XDrawString(x11_display, popup, graphical_context[0], 5, 20, pop_option[0].c_str(), strlen(pop_option[0].c_str()));

        if ((ypos-pop_changes.y)>30 && (ypos-pop_changes.y)<50) {
            XDrawString(x11_display, popup, graphical_context[1], 5, 40, pop_option[1].c_str(), strlen(pop_option[1].c_str()));
            if (left_click) {
                XUnmapWindow(x11_display, popup);
                pop_linux=false;
                pop_return = 1;
            }
        }
        else XDrawString(x11_display, popup, graphical_context[0], 5, 40, pop_option[1].c_str(), strlen(pop_option[1].c_str()));

        if ((ypos-pop_changes.y)>50 && (ypos-pop_changes.y)<70) {
            XDrawString(x11_display, popup, graphical_context[1], 5, 60, pop_option[2].c_str(), strlen(pop_option[2].c_str()));
            if (left_click) {
                XUnmapWindow(x11_display, popup);
                pop_linux=false;
                pop_return = 2;
            }
        }
        else XDrawString(x11_display, popup, graphical_context[0], 5, 60, pop_option[2].c_str(), strlen(pop_option[2].c_str()));

        if ((ypos-pop_changes.y)>70 && (ypos-pop_changes.y)<90) {
            XDrawString(x11_display, popup, graphical_context[1], 5, 80, pop_option[3].c_str(), strlen(pop_option[3].c_str()));
            if (left_click) {
                XUnmapWindow(x11_display, popup);
                pop_linux=false;
                pop_return = 3;
            }
        }
        else XDrawString(x11_display, popup, graphical_context[0], 5, 80, pop_option[3].c_str(), strlen(pop_option[3].c_str()));
    }
    else {
        XDrawString(x11_display, popup, graphical_context[0], 5, 20, pop_option[0].c_str(), strlen(pop_option[0].c_str()));
        XDrawString(x11_display, popup, graphical_context[0], 5, 40, pop_option[1].c_str(), strlen(pop_option[1].c_str()));
        XDrawString(x11_display, popup, graphical_context[0], 5, 60, pop_option[2].c_str(), strlen(pop_option[2].c_str()));
        XDrawString(x11_display, popup, graphical_context[0], 5, 80, pop_option[3].c_str(), strlen(pop_option[3].c_str()));
        if (left_click) {
            XUnmapWindow(x11_display, popup);
            pop_linux=false;
        }
    }
}
#endif


