/**
    main.cpp
    Główny plik projektu sound++, zawierający logikę programu.

    @author Dove
    @version 1.0.0 8/13/17

    @todo ustalenie jednolitego stylu: OpenGL
Zmienne
	zmienna_lokalna
	int sample_variable = 1234;
	zmienna_globalna_g
	int sample_variable_g = 1234;
Funkcje
	camelCase
	void pgSampleFunction();
Metody
	jw.
Klasy
	DUŻEmałemałemałe
	PGexample;
	PGvoidptr;
Struktury
	jw.
Typy
    jw.
Makra
	WIELKIE_LITERY
	#define PG_VERSION_0_2_0
*/
#ifndef DEBUG
#define DEBUG 0
#endif // DEBUG

#include <time.h>

#include "dec.h"

int main(int argc, char **argv) {
    sound *TEST_SND, *TEST_SND2;
	if (argc > 1) TEST_SND = new sound(argv[1]);
    else TEST_SND = new sound("win.flac");
	if (argc > 2) TEST_SND2 = new sound(argv[2]);
    else TEST_SND2 = new sound("win2.flac");
    bool xd = false;
	TEST_SND2->play_until(&xd, true, 1000);
	TEST_SND->play_for(0, 0.0625);
	clock_t time = clock();
	while (((clock() - time)/CLOCKS_PER_SEC) < 140) {
        char c = getchar();
        switch (c) {
            case '-': {
                TEST_SND2->metadata.volume -= 0.05;
                printf("%s: volume %f\n", TEST_SND2->filename, TEST_SND2->metadata.volume);
                break;
            }
            case '+': {
                TEST_SND2->metadata.volume += 0.05;
                printf("%s: volume %f\n", TEST_SND2->filename, TEST_SND2->metadata.volume);
                break;
            }
            case '/': {
                TEST_SND2->metadata.volume -= 0.5;
                printf("%s: volume %f\n", TEST_SND2->filename, TEST_SND2->metadata.volume);
                break;
            }
            case '*': {
                TEST_SND2->metadata.volume += 0.5;
                printf("%s: volume %f\n", TEST_SND2->filename, TEST_SND2->metadata.volume);
                break;
            }
            default:
                break;
        }
	}
	//Pa_Sleep(140000);
	xd = true;
	Pa_Sleep(3000);
	delete TEST_SND;
	delete TEST_SND2;
	return 0;
}

#include "imp.h"
