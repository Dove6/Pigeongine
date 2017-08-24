/**
    dec.h
    Zbiór deklaracji projektu sound++.

    @author Dove
    @version 1.0.0 8/13/17

    @todo ustalenie jednolitego stylu.
    @todo uporządkowanie zmiennych w klasie (podział na struktury?).
    @todo konstruktor kopiujący (i tak nikomu się nie przyda).
*/
#include <portaudio.h>
#include <FLAC/all.h>

#include <stdio.h>
#include <string.h>
#include <pthread.h>

/**< Klasa odpowiedzialna za obsługę efektów dźwiękowych: ich wczytanie, odtworzenie czy modyfikację. */
class sound {
private:
    /**< Wskaźnik pozwalający na zaalokowanie pamięci pod dynamiczną tablicę przechowującą próbki dźwiękowe. */
    void **data;

    /**< Numer bieżącej próbki (zapisywanej lub odtwarzanej). */
    unsigned long int pos;

public:
    /**< Ścieżka do pliku zawierającego próbki dźwiękowe. */
	char filename[512];

	/**< Struktura przechowująca podstawowe informacje nt. ścieżki dźwiękowej. */
    struct sound_info {
        /**< Liczba bitów, które zajmuje jedna próbka. */
        uint8_t bps;

        /**< Liczba kanałów zapisanych w pliku dźwiękowym. */
        uint8_t channels;

        /**< Częstotliwość próbkowania ścieżki dźwiękowej. */
        unsigned int sampleRate;

        /**< Całkowita liczba próbek ścieżki dźwiękowej. */
        FLAC__uint64 totalSamples;

        /**< Mnożnik odpowiadający za głośność odtwarzanej ścieżki dźwiękowej. */
        double volume;

        /**< Zmienna informująca o tym, czy ścieżka jest obecnie odtwarzana choć w jednej instancji. */
        bool playing;
    } metadata;

	/**< Struktura przechowująca zmienne biblioteki PortAudio unikalne dla obiektu. */
    struct PA_info {
        /**< Struktura zawierajaca informacje nt. parametrów wyjściowych ścieżki. */
        PaStreamParameters outputParameters;

        /**< Wskaźnik na obiekt strumienia biblioteki PortAudio. */
        PaStream *stream;

        /**< Indykator błędów biblioteki PortAudio. */
        PaError err;

        /**< Liczba klatek przetwarzanych za jednym wywołaniem funkcji umieszczającej dane dźwiękowe w buforze urządzenia. */
        unsigned long framesPerBuffer;
    } PA_data;

    /**< Struktura zawierajaca dane niezbędne do odtwarzania warunkowego. */
    struct condit {
        /**< Wskaźnik na sprawdzaną zmienną. */
        bool *condition;

        /**< Stan, który musi osiągnąć zmienna *condition, by zakończyło się odtwarzanie ścieżki. */
        bool state;

        /**
            Czas, co jaki sprawdzany jest stan zmiennej *condition.
            W przypadku dwukrotnej zmiany wartości *condition w czasie interval, nie następuje przerwanie odtwarzania.
        */
        long interval;

        /**< Wskaźnik na obiekt klasy @link sound sound. @endlink */
        sound *snd;
    };

    /**< Struktura zawierajaca dane niezbędne do odtwarzania czasowego. */
    struct chrono {
        /**< Czas, przez jaki będzie odtwarzany dźwięk (w przybliżeniu, dokładność zależna od liczby bieżących wątków). */
        long msec;

        /**< Wskaźnik na obiekt klasy @link sound sound. @endlink */
        sound *snd;
    };
    static const PaDeviceInfo *deviceInfo;
    static PaDeviceIndex deviceIndex;

    /**
        Podfunkcja wywołania @link sound::FLAC_decode FLAC_decode. @endlink
        Odpowiada za zarządzanie zdekompresowanymi danymi o próbkach 8- lub 4-bitowych.

        @param frame wskaźnik na obecnie przetwarzaną ramkę.
        @param buffer wskaźnik na bufor przechowujący zdekodowane próbki dźwiękowe.
        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void FLAC_write_int8(const FLAC__Frame *frame,
                                const FLAC__int32 * const buffer[],
                                sound *snd);

    /**
        Podfunkcja wywołania @link sound::FLAC_decode FLAC_decode. @endlink
        Odpowiada za zarządzanie zdekompresowanymi danymi o próbkach 16-bitowych.

        @param frame wskaźnik na obecnie przetwarzaną ramkę.
        @param buffer wskaźnik na bufor przechowujący zdekodowane próbki dźwiękowe.
        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void FLAC_write_int16(const FLAC__Frame *frame,
                                 const FLAC__int32 * const buffer[],
                                 sound *snd);

    /**
        Podfunkcja wywołania @link sound::FLAC_decode FLAC_decode. @endlink
        Odpowiada za zarządzanie zdekompresowanymi danymi o próbkach 24- lub 32-bitowych.

        @param frame wskaźnik na obecnie przetwarzaną ramkę.
        @param buffer wskaźnik na bufor przechowujący zdekodowane próbki dźwiękowe.
        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void FLAC_write_int32(const FLAC__Frame *frame,
                                 const FLAC__int32 * const buffer[],
                                 sound *snd);

    /**
        Wywołanie zwrotne dla metody @link sound::FLAC_decode FLAC_decode. @endlink
        Odpowiada za zarządzanie zdekompresowanymi danymi.

        @param decoder wskaźnik na obiekt dekodera.
        @param frame wskaźnik na obecnie przetwarzaną ramkę.
        @param buffer wskaźnik na bufor przechowujący zdekodowane próbki dźwiękowe.
        @param client_data wskaźnik na obiekt klasy @link sound sound. @endlink

        @return kod błędu.
    */
    static FLAC__StreamDecoderWriteStatus FLAC_write(const FLAC__StreamDecoder *decoder,
                                                     const FLAC__Frame *frame,
                                                     const FLAC__int32 * const buffer[],
                                                     void *client_data);

    /**
        Wywołanie zwrotne dla metody @link sound::FLAC_decode FLAC_decode. @endlink
        Odpowiada za zarządzanie informacjami na temat pliku.

        @param decoder wskaźnik na obiekt dekodera.
        @param metadata wskaźnik na obiekt metadanych.
        @param client_data wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void FLAC_meta(const FLAC__StreamDecoder *decoder,
                          const FLAC__StreamMetadata *metadata,
                          void *client_data);

    /**
        Wywołanie zwrotne dla metody @link sound::FLAC_decode FLAC_decode. @endlink
        Odpowiada za obsługę błędów dekompresji i odczytu.

        @param decoder wskaźnik na obiekt dekodera.
        @param status kod błędu.
        @param client_data wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void FLAC_error(const FLAC__StreamDecoder *decoder,
                           FLAC__StreamDecoderErrorStatus status,
                           void *client_data);

    /**
        Funkcja odpowiadająca za wczytanie i dekompresję danych dźwiękowych formatu FLAC.
        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink

        @return kod błędu.
    */
    static int FLAC_decode(sound *snd);

    /**
        Wywołanie zwrotne biblioteki PortAudio.
        Odpowiada za obsługę błędów strumienia.

        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
        @return kod błędu.
    */
    static int PA_error(sound *snd);

    /**
        Podfunkcja wywołania @link sound::PA_write PA_write. @endlink
        Odpowiada za buforowanie 4- i 8-bitowych całkowitych danych dźwiękowych.

        @param outputBuffer wskaźnik na bufor wyjścia.
        @param framesPerBuffer liczba ramek przetwarzanych w czasie jednego wywołania zwrotnego.
        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void PA_write_int8(void *outputBuffer,
                              unsigned long framesPerBuffer,
                              sound *snd);

     /**
        Podfunkcja wywołania @link sound::PA_write PA_write. @endlink
        Odpowiada za buforowanie 16-bitowych całkowitych danych dźwiękowych.

        @param outputBuffer wskaźnik na bufor wyjścia.
        @param framesPerBuffer liczba ramek przetwarzanych w czasie jednego wywołania zwrotnego.
        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void PA_write_int16(void *outputBuffer,
                               unsigned long framesPerBuffer,
                               sound *snd);

     /**
        Podfunkcja wywołania @link sound::PA_write PA_write. @endlink
        Odpowiada za buforowanie 24- i 32-bitowych całkowitych danych dźwiękowych.

        @param outputBuffer wskaźnik na bufor wyjścia.
        @param framesPerBuffer liczba ramek przetwarzanych w czasie jednego wywołania zwrotnego.
        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void PA_write_int32(void *outputBuffer,
                               unsigned long framesPerBuffer,
                               sound *snd);

    /**
        Wywołanie zwrotne biblioteki PortAudio.
        Odpowiada za buforowanie danych dźwiękowych.

        @param inputBuffer wskaźnik na bufor wejścia (dla strumienia wejściowego).
        @param outputBuffer wskaźnik na bufor wyjścia (dla strumienia wyjściowego).
        @param framesPerBuffer liczba ramek przetwarzanych w czasie jednego wywołania zwrotnego.
        @param timeInfo wskaźnik na strukturę PaStreamCallbackTimeInfo. @see PaStreamCallbackTimeInfo
        @param statusFlags kod statusu strumienia. @see PaStreamCallbackFlags
        @param userData wskaźnik na obiekt klasy @link sound sound. @endlink
        @return kod błędu.
    */
    static int PA_write(const void *inputBuffer,
                        void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData);

    /**
        Wywołanie zwrotne biblioteki PortAudio.
        Określa działanie wykonywane po zamknięciu strumienia.

        @param userData wskaźnik na obiekt klasy @link sound sound. @endlink
    */
    static void StreamFinished(void* userData);

    /**
        Funkcja inicjalizująca bibliotekę PortAudio i domyślne urządzenie wyjściowe.
        Wymagane jedno wywołanie na czas działania programu.

        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
        @return kod błędu.
    */
    static int PA_init_hw(sound *snd);

    /**
        Funkcja ustawiająca parametry urządzenia indywidualne dla każdego pliku dźwiękowego.
        Wymagane wywołanie dla każdego tworzonego obiektu.

        @param snd wskaźnik na obiekt klasy @link sound sound. @endlink
        @return kod błędu.
    */
    static int PA_init_snd(sound *snd);

    /**
        Domyślny konstruktor.
        Nie robi nic poza alokacja przestrzeni dla obiektu i inicjalizacją zmiennych.
    */
    sound(void);

    /**
        Konstruktor bezzwłocznie dekompresujący plik dźwiękowy.

        @param fname wskaźnik na nazwę pliku dźwiękowego.
    */
    sound(char *fname);

    /**
        Domyślny dekonstruktor.
    */
    ~sound();

    /**
        Metoda rozpoczynająca odtwarzanie dźwięku.

        @param volume mnożnik głośności (zaleca się mnożnika z zakresu <0, 1>,
                    większe wartości powodują nieprzyjemne efekty dźwiękowe)

        @return kod błędu.
    */
    int play_loop(double volume = 1);

    /**
        Metoda kończąca odtwarzanie dźwięku.

        @return kod błędu.
    */
    int stop();

    /**
        Statyczna metoda wykonywana w oddzielnym wątku w ramach metody @link sound::play_for play_for. @endlink
        Kończy odtwarzanie dźwięku po określonym czasie.

        @param userData wskaźnik na strukturę chrono. @see chrono
    */
    static void *t_stop(void *userData);

    /**
        Metoda odtwarzająca dźwięk przez określony czas.
        Podany czas nie wstrzymuje głównego wątku.

        @param msec czas (w milisekundach) odtwarzania pliku dźwiękowego.
                    Brak wartości lub wartość 0 skutkuje jednorazowym odtworzeniem.
        @param volume mnożnik głośności (zaleca się mnożnika z zakresu <0, 1>,
                    większe wartości powodują nieprzyjemne efekty dźwiękowe)

        @return kod błędu.
    */
    int play_for(long msec = 0,
                 double volume = 1);

    /**
        Statyczna metoda wykonywana w oddzielnym wątku w ramach metody @link sound::play_until play_until. @endlink
        Kończy odtwarzanie dźwięku, jeśli spełniony jest warunek.

        @todo zmiana nazwy.

        @param userData wskaźnik na strukturę condit. @see condit
    */
    static void* t_stop_after(void* userData);

    /**
        Metoda odtwarzająca dźwięk, dopóki zmienna typu bool nie przyjmie określonego stanu.

        @param condition wskaźnik na zmienną typu bool.
        @param state określony stan, kończący odtwarzanie dźwięku.
        @param interval okres (w milisekundach) co jaki sprawdzana jest wartość zmiennej typu bool.
        @param volume mnożnik głośności (zaleca się mnożnika z zakresu <0, 1>,
                    większe wartości powodują nieprzyjemne efekty dźwiękowe)

        @return kod błędu.
    */
    int play_until(bool *condition,
                   bool state,
                   long interval,
                   double volume = 1);
};

const PaDeviceInfo *sound::deviceInfo = 0;
PaDeviceIndex sound::deviceIndex = 0;
