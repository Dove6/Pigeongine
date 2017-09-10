#include <linmath.h>

static const char *matStackErrString[3] =
{
    "Matrix_stack: GL_NO_ERROR",
    "Matrix_stack: GL_UNDERFLOW",
    "Matrix_stack: GL_OVERFLOW"
};

struct matrix_stack {
private:
    uint8_t pointer;
    uint8_t max_size;
    mat4x4 *fifo;
    uint8_t error; //no_error = 0, underflow = 1, overflow = 2
    GLint uniMat;
public:
    static matrix_stack *current;
    matrix_stack(uint8_t i = 32)
    : pointer(0), max_size(i), error(0), fifo(0), uniMat(0)
    {
        fifo = (mat4x4 *)malloc(max_size*sizeof(mat4x4));
        mat4x4_identity(fifo[pointer]);
    }
    matrix_stack(GLuint shaderProgram, const char *uniLocation, uint8_t i = 32)
    : pointer(0), max_size(i), error(0), fifo(0), uniMat(0)
    {
        fifo = (mat4x4 *)malloc(max_size*sizeof(mat4x4));
        mat4x4_identity(fifo[pointer]);
        uniMat = glGetUniformLocation(shaderProgram, uniLocation);
        send(this);
    }
    matrix_stack(GLint uniMatrix, uint8_t i = 32)
    : pointer(0), max_size(i), error(0), fifo(0), uniMat(0)
    {
        fifo = (mat4x4 *)malloc(max_size*sizeof(mat4x4));
        mat4x4_identity(fifo[pointer]);
        uniMat = uniMatrix;
        send(this);
    }
    ~matrix_stack()
    {
        free(fifo);
    }
    static void bind_pre(matrix_stack *m, GLint shaderProgram, const char *uniLocation)
    {
        m->uniMat = glGetUniformLocation(shaderProgram, uniLocation);
        send(m);
    }
    static void bind_post(matrix_stack *m, GLint uniID)
    {
        m->uniMat = uniID;
        send(m);
    }
    static void send(matrix_stack *m)
    {
        glUniformMatrix4fv(m->uniMat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
    }
    static void push(matrix_stack *m)
    {
        if (m->pointer < m->max_size-1) {
            memcpy(&m->fifo[m->pointer+1], &m->fifo[m->pointer], sizeof(mat4x4));
            m->pointer++;
        }
        else m->error = 2;
    }
    static void pop(matrix_stack *m)
    {
        if (m->pointer > 0) m->pointer--;
        else m->error = 1;
    }
    static void tr(matrix_stack *m, float x, float y, float z)
    {
        mat4x4_translate_in_place(m->fifo[m->pointer], x, y, z);
        send(m);
    }
    static void rot(matrix_stack *m, float x, float y, float z, float angle)
    {
        mat4x4_rotate(m->fifo[m->pointer], m->fifo[m->pointer], x, y, z, angle);
        send(m);
    }
    static void sc(matrix_stack *m, float x, float y, float z)
    {
        mat4x4_scale_aniso(m->fifo[m->pointer], m->fifo[m->pointer], x, y, z);
        send(m);
    }
    static void ort(matrix_stack *m, float l, float r, float b, float t, float n, float f)
    {
        mat4x4_ortho(m->fifo[m->pointer], l, r, b, t, n, f);
        send(m);
    }
    static void reset(matrix_stack *m)
    {
        mat4x4_identity(m->fifo[m->pointer]);
        send(m);
    }
    uint8_t getLastError()
    {
        uint8_t temp = error;
        error = 0;
        return temp;
    }
    static const char *getErrorString(uint8_t code)
    {
        return matStackErrString[code];
    }
} *Mmat_stack, *Vmat_stack, *Pmat_stack;

matrix_stack *matrix_stack::current = 0;

#define GL_MODEL Mmat_stack
#define GL_VIEW Vmat_stack
#define GL_MODELVIEW Vmat_stack
#define GL_PROJECTION Pmat_stack
#define glMatrixMode(x); matrix_stack::current = x;
#define glPushMatrix(); matrix_stack::push(matrix_stack::current);
#define glPopMatrix(); matrix_stack::pop(matrix_stack::current);
#define glTranslate(x, y, z); matrix_stack::tr(matrix_stack::current, x, y, z);
#define glRotate(x, y, z, angle); matrix_stack::rot(matrix_stack::current, x, y, z, angle);
#define glScale(x, y, z); matrix_stack::sc(matrix_stack::current, x, y, z);
#define glLoadIdentity(); matrix_stack::reset(matrix_stack::current);
#define glOrtho(l, r, b, t, n, f); matrix_stack::ort(matrix_stack::current, l, r, b, t, n, f);

/*
struct matrix_stack {
private:
    uint8_t pointer;
    uint8_t max_size;
    mat4x4 *fifo;
    uint8_t error; //no_error = 0, underflow = 1, overflow = 2, no_uniform_location = 3
    GLint uniMat;
public:
    static matrix_stack current; //MODEL = 0, VIEW = 1, PROJECTION = 2
    matrix_stack(uint8_t i = 32)
    : pointer(0), max_size(i), error(0), fifo(0), uniMat(0)
    {
        fifo = (mat4x4 *)malloc(max_size*sizeof(mat4x4));
        mat4x4_identity(fifo[pointer]);
    }
    matrix_stack(const char *uniLocation, uint8_t i = 32)
    : pointer(0), max_size(i), error(0), fifo(0), uniMat(0)
    {
        fifo = (mat4x4 *)malloc(max_size*sizeof(mat4x4));
        mat4x4_identity(fifo[pointer]);
        uniMat = glGetUniformLocation(shaderProgram, uniLocation);
    }
    ~matrix_stack()
    {
        free(fifo);
    }
    static void bind(matrix_stack *m, const char *uniLocation)
    {
        m->uniMat = glGetUniformLocation(shaderProgram, uniLocation);
    }
    static void send(matrix_stack *m)
    {
        if (m->uniMat) glUniformMatrix4fv(m->uniMat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
        else m->error = 3;
    }
    static void push(matrix_stack *m)
    {
        if (m->pointer < m->max_size-1) {
            memcpy(&m->fifo[m->pointer+1], &m->fifo[m->pointer], sizeof(mat4x4));
            m->pointer++;
        }
        else m->error = 2;
    }
    static void pop(matrix_stack *m)
    {
        if (m->pointer > 0) m->pointer--;
        else m->error = 1;
    }
    static void tr(matrix_stack *m, float x, float y, float z)
    {
        mat4x4_translate(m->fifo[m->pointer], x, y, z);
        switch (matrix_stack::current) {
            case 0: glUniformMatrix4fv(uniMmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            case 1: glUniformMatrix4fv(uniVmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            case 2: glUniformMatrix4fv(uniPmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            default:
                break;
        }
    }
    static void rot(matrix_stack *m, float x, float y, float z, float angle)
    {
        mat4x4_rotate(m->fifo[m->pointer], m->fifo[m->pointer], x, y, z, angle);
        switch (matrix_stack::current) {
            case 0: glUniformMatrix4fv(uniMmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            case 1: glUniformMatrix4fv(uniVmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            case 2: glUniformMatrix4fv(uniPmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            default:
                break;
        }
    }
    static void sc(matrix_stack *m, float x, float y, float z)
    {
        mat4x4_scale_aniso(m->fifo[m->pointer], m->fifo[m->pointer], x, y, z);
        switch (matrix_stack::current) {
            case 0: glUniformMatrix4fv(uniMmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            case 1: glUniformMatrix4fv(uniVmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            case 2: glUniformMatrix4fv(uniPmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            default:
                break;
        }
    }
    static void ort(matrix_stack *m, float l, float r, float b, float t, float n, float f)
    {
        mat4x4_ortho(m->fifo[m->pointer], l, r, b, t, n, f);
        switch (matrix_stack::current) {
            case 0: glUniformMatrix4fv(uniMmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            case 1: glUniformMatrix4fv(uniVmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            case 2: glUniformMatrix4fv(uniPmat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
                break;
            default:
                break;
        }
    }
    static void reset(matrix_stack *m)
    {
        mat4x4_identity(m->fifo[m->pointer]);
        send(m);
    }
    uint8_t getLastError(matrix_stack *m)
    {
        uint8_t temp = m->error;
        m->error = 0;
        return temp;
    }
} Mmat_stack("model"), Vmat_stack("view"), Pmat_stack("projection", 2);

uint8_t matrix_stack::current = 0;

#define GL_MODEL 0
#define GL_VIEW 1
#define GL_MODELVIEW 1
#define GL_PROJECTION 2
#define glMatrixMode(x); matrix_stack::current = x;
#define glPushMatrix(); matrix_stack::push(matrix_stack::current);
#define glPopMatrix(); matrix_stack::pop(matrix_stack::current);
#define glTranslate(x, y, z); matrix_stack::tr(matrix_stack::current, x, y, z);
#define glRotate(x, y, z, angle); matrix_stack::rot(matrix_stack::current, x, y, z, angle);
#define glScale(x, y, z); matrix_stack::sc(matrix_stack::current, x, y, z);
#define glLoadIdentity(); matrix_stack::reset(matrix_stack::current);
#define glOrtho(l, r, b, t, n, f); matrix_stack::ort(matrix_stack::current, l, r, b, t, n, f);


struct matrix_stack {
private:
    uint8_t pointer;
    uint8_t max_size;
    mat4x4 *fifo;
    uint8_t error; //no_error = 0, underflow = 1, overflow = 2, no_uniform_location = 3
    GLint uniMat;
public:
    static matrix_stack *current; //MODEL = 0, VIEW = 1, PROJECTION = 2
    matrix_stack(uint8_t i = 32)
    : pointer(0), max_size(i), error(0), fifo(0), uniMat(0)
    {
        fifo = (mat4x4 *)malloc(max_size*sizeof(mat4x4));
        mat4x4_identity(fifo[pointer]);
    }
    matrix_stack(GLuint shaderProgram, const char *uniLocation, uint8_t i = 32)
    : pointer(0), max_size(i), error(0), fifo(0), uniMat(0)
    {
        fifo = (mat4x4 *)malloc(max_size*sizeof(mat4x4));
        mat4x4_identity(fifo[pointer]);
        uniMat = glGetUniformLocation(shaderProgram, uniLocation);
    }
    matrix_stack(GLint uniMatrix, uint8_t i = 32)
    : pointer(0), max_size(i), error(0), fifo(0), uniMat(0)
    {
        fifo = (mat4x4 *)malloc(max_size*sizeof(mat4x4));
        mat4x4_identity(fifo[pointer]);
        uniMat = uniMatrix;
    }
    ~matrix_stack()
    {
        free(fifo);
    }
    static void bind(matrix_stack *m, GLint shaderProgram, const char *uniLocation)
    {
        m->uniMat = glGetUniformLocation(shaderProgram, uniLocation);
    }
    static void send(matrix_stack *m)
    {
        if (m->uniMat) glUniformMatrix4fv(m->uniMat, 1, GL_FALSE, &m->fifo[m->pointer][0][0]);
        else m->error = 3;
    }
    static void push(matrix_stack *m)
    {
        if (m->pointer < m->max_size-1) {
            memcpy(&m->fifo[m->pointer+1], &m->fifo[m->pointer], sizeof(mat4x4));
            m->pointer++;
        }
        else m->error = 2;
    }
    static void pop(matrix_stack *m)
    {
        if (m->pointer > 0) m->pointer--;
        else m->error = 1;
    }
    static void tr(matrix_stack *m, float x, float y, float z)
    {
        mat4x4_translate(m->fifo[m->pointer], x, y, z);
        send(m);
    }
    static void rot(matrix_stack *m, float x, float y, float z, float angle)
    {
        mat4x4_rotate(m->fifo[m->pointer], m->fifo[m->pointer], x, y, z, angle);
        send(m);
    }
    static void sc(matrix_stack *m, float x, float y, float z)
    {
        mat4x4_scale_aniso(m->fifo[m->pointer], m->fifo[m->pointer], x, y, z);
        send(m);
    }
    static void ort(matrix_stack *m, float l, float r, float b, float t, float n, float f)
    {
        mat4x4_ortho(m->fifo[m->pointer], l, r, b, t, n, f);
        send(m);
    }
    static void reset(matrix_stack *m)
    {
        mat4x4_identity(m->fifo[m->pointer]);
        send(m);
    }
    uint8_t getLastError(matrix_stack *m)
    {
        uint8_t temp = m->error;
        m->error = 0;
        return temp;
    }
} *Mmat_stack, *Vmat_stack, *Pmat_stack;

matrix_stack *matrix_stack::current = 0;

#define GL_MODEL Mmat_stack
#define GL_VIEW Vmat_stack
#define GL_MODELVIEW Vmat_stack
#define GL_PROJECTION Pmat_stack
#define glMatrixMode(x); matrix_stack::current = x;
#define glPushMatrix(); matrix_stack::push(matrix_stack::current);
#define glPopMatrix(); matrix_stack::pop(matrix_stack::current);
#define glTranslate(x, y, z); matrix_stack::tr(matrix_stack::current, x, y, z);
#define glRotate(x, y, z, angle); matrix_stack::rot(matrix_stack::current, x, y, z, angle);
#define glScale(x, y, z); matrix_stack::sc(matrix_stack::current, x, y, z);
#define glLoadIdentity(); matrix_stack::reset(matrix_stack::current);
#define glOrtho(l, r, b, t, n, f); matrix_stack::ort(matrix_stack::current, l, r, b, t, n, f);
*/
