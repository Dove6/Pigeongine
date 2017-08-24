/**
    imp.h
    Zbiór implementacji projektu sound++.

    @author Dove
    @version 1.0.0 8/13/17

    @todo ustalenie jednolitego stylu.
*/

//Podfunkcja wywołania FLAC_decode.
//Odpowiada za zarządzanie zdekompresowanymi danymi o próbkach 8- lub 4-bitowych.
void sound::FLAC_write_int8(const FLAC__Frame *frame,
                            const FLAC__int32 * const buffer[],
                            sound *snd)
{
    int8_t *data;
    for(unsigned i = 0; i < frame->header.blocksize; i++) {
        for (int channel = 0; channel < snd->metadata.channels; channel++) {
            data = (int8_t *)snd->data[channel];
            data[snd->pos] = (int8_t)buffer[channel][i];
        }
        #ifdef FLAC_DEBUG
        if(!((int)snd->pos%(int)1000000))
            printf("wrote_flac_data: %lu\n", snd->pos);
        #endif // FLAC_DEBUG
        snd->pos++;
    }
}

//Podfunkcja wywołania FLAC_decode.
//Odpowiada za zarządzanie zdekompresowanymi danymi o próbkach 16-bitowych.
void sound::FLAC_write_int16(const FLAC__Frame *frame,
                             const FLAC__int32 * const buffer[],
                             sound *snd)
{
    int16_t *data;
    for(unsigned i = 0; i < frame->header.blocksize; i++) {
        for (int channel = 0; channel < snd->metadata.channels; channel++) {
            data = (int16_t *)snd->data[channel];
            data[snd->pos] = (int16_t)buffer[channel][i];
        }
        #ifdef FLAC_DEBUG
        if(!((int)snd->pos%(int)1000000))
            printf("wrote_flac_data: %lu\n", snd->pos);
        #endif // FLAC_DEBUG
        snd->pos++;
    }
}

//Podfunkcja wywołania @link FLAC_decode. @endlink
//Odpowiada za zarządzanie zdekompresowanymi danymi o próbkach 24- lub 32-bitowych.
void sound::FLAC_write_int32(const FLAC__Frame *frame,
                             const FLAC__int32 * const buffer[],
                             sound *snd)
{
    int32_t *data;
    for(unsigned i = 0; i < frame->header.blocksize; i++) {
        for (int channel = 0; channel < snd->metadata.channels; channel++) {
            data = (int32_t *)snd->data[channel];
            data[snd->pos] = (int32_t)buffer[channel][i];
        }
        #ifdef FLAC_DEBUG
        if(!((int)snd->pos%(int)1000000))
            printf("wrote_flac_data: %lu\n", snd->pos);
        #endif // FLAC_DEBUG
        snd->pos++;
    }
}

//Wywołanie zwrotne dla metody FLAC_decode.
//Odpowiada za zarządzanie zdekompresowanymi danymi.
FLAC__StreamDecoderWriteStatus sound::FLAC_write(const FLAC__StreamDecoder *decoder,
                                                 const FLAC__Frame *frame,
                                                 const FLAC__int32 * const buffer[],
                                                 void *client_data)
{
    sound *snd = (sound *)client_data;
    (void)decoder;

    if(snd->metadata.totalSamples == 0) {
        fprintf(stderr, "FLAC ERROR: no total_samples count in STREAMINFO\n");
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    for (int channel = 0; channel < snd->metadata.channels; channel++) {
        if(buffer[channel] == NULL) {
            fprintf(stderr, "FLAC ERROR: buffer [%d] of %d is NULL\n", channel, snd->metadata.channels );
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
    }

    switch (snd->metadata.bps) {
        case 4:
        case 8: {
            FLAC_write_int8(frame, buffer, snd);
            break;
            } /* 8 bit unsigned integer output */
        case 16: {
            FLAC_write_int16(frame, buffer, snd);
            break;
        } /* 16 bit signed integer output */
        case 24: {
            FLAC_write_int32(frame, buffer, snd);
            break;
        } /* 24 bit signed integer output */
        default: {
            FLAC_write_int32(frame, buffer, snd);
        }
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

//Wywołanie zwrotne dla metody FLAC_decode.
//Odpowiada za zarządzanie informacjami na temat pliku.
void sound::FLAC_meta(const FLAC__StreamDecoder *decoder,
                      const FLAC__StreamMetadata *metadata,
                      void *client_data)
{
    sound *snd = (sound *)client_data;
    (void)decoder, (void)client_data;

    /* print some stats */
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        /* save for later */
        snd->metadata.totalSamples = metadata->data.stream_info.total_samples;
        snd->metadata.sampleRate = metadata->data.stream_info.sample_rate;
        snd->metadata.channels = metadata->data.stream_info.channels;
        snd->data = (void **)calloc(snd->metadata.channels, sizeof(void *));

        snd->metadata.bps = metadata->data.stream_info.bits_per_sample;
        switch (snd->metadata.bps) {
            case 4:
            case 8: {
                for (int channel = 0; channel < snd->metadata.channels; channel++)
                    snd->data[channel] = calloc(snd->metadata.totalSamples, sizeof(int8_t));
                break;
            } /* 8 bit unsigned integer output */
            case 16: {
                for (int channel = 0; channel < snd->metadata.channels; channel++)
                    snd->data[channel] = calloc(snd->metadata.totalSamples, sizeof(int16_t));
                break;
            } /* 16 bit signed integer output */
            case 24: {
                for (int channel = 0; channel < snd->metadata.channels; channel++)
                    snd->data[channel] = calloc(snd->metadata.totalSamples, sizeof(int32_t));
                break;
            } /* 24 bit signed integer output */
            default: {
                fprintf(stderr, "Error: Unsupported sample format: %d.\n", snd->metadata.bps);
                return;
            }
        }

        #ifdef DEBUG
        fprintf(stderr, "sample rate    : %u Hz\n", snd->metadata.sampleRate);
        fprintf(stderr, "channels       : %u\n", snd->metadata.channels);
        fprintf(stderr, "bits per sample: %u\n", snd->metadata.bps);
        fprintf(stderr, "total samples  : %lu\n", (unsigned long int)snd->metadata.totalSamples);
        #endif // DEBUG
    }
}

//Wywołanie zwrotne dla metody FLAC_decode.
//Odpowiada za obsługę błędów dekompresji i odczytu.
void sound::FLAC_error(const FLAC__StreamDecoder *decoder,
                       FLAC__StreamDecoderErrorStatus status,
                       void *client_data)
{
    (void)decoder, (void)client_data;

    fprintf(stderr, "FLAC Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}

//Wywołanie zwrotne biblioteki PortAudio.
//Odpowiada za obsługę błędów strumienia.
int sound::PA_error(sound *snd)
{
    Pa_Terminate();
    fprintf(stderr, "An error occured while using the portaudio stream\n");
    fprintf(stderr, "Error number: %d\n", snd->PA_data.err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(snd->PA_data.err));
    return snd->PA_data.err;
}

//Podfunkcja wywołania PA_write.
//Odpowiada za buforowanie 4- i 8-bitowych całkowitych danych dźwiękowych.
void sound::PA_write_int8(void *outputBuffer,
                          unsigned long framesPerBuffer,
                          sound *snd)
{
    int8_t *out = (int8_t *)outputBuffer;
    int8_t *data;
    for(unsigned long i = 0; i < framesPerBuffer; i++) {
        if (snd->pos < snd->metadata.totalSamples) {
            if (snd->metadata.channels < snd->deviceInfo->maxOutputChannels) {
                for(int channel = 0, hw_channel = 0; hw_channel < snd->deviceInfo->maxOutputChannels; hw_channel++) {
                    data = (int8_t *)snd->data[channel];
                    *out++ = data[snd->pos] * snd->metadata.volume;
                    if (channel >= snd->metadata.channels) channel = 0;
                }
                snd->pos++;
            }
            else {
                for(int channel = 0; channel < snd->metadata.channels; channel++) {
                    data = (int8_t *)snd->data[channel];
                    *out++ = data[snd->pos] * snd->metadata.volume;
                }
                snd->pos++;
            }
        }
        else {
            if (snd->metadata.channels < snd->deviceInfo->maxOutputChannels) {
                for(int channel = 0; channel < snd->deviceInfo->maxOutputChannels; channel++)
                    *out++ = 0;
                snd->pos = 0;
            }
            else {
                for(int channel = 0; channel < snd->metadata.channels; channel++)
                    *out++ = 0;
                snd->pos = 0;
            }
            #ifdef DEBUG
            printf("Played %s once\n", snd->filename);
            #endif // DEBUG
        }
    }
}

//Podfunkcja wywołania PA_write.
//Odpowiada za buforowanie 16-bitowych całkowitych danych dźwiękowych.
void sound::PA_write_int16(void *outputBuffer,
                           unsigned long framesPerBuffer,
                           sound *snd)
{
    int16_t *out = (int16_t *)outputBuffer;
    int16_t *data;
    for(unsigned long i = 0; i < framesPerBuffer; i++) {
        if (snd->pos < snd->metadata.totalSamples) {
            if (snd->metadata.channels < snd->deviceInfo->maxOutputChannels) {
                for(int channel = 0, hw_channel = 0; hw_channel < snd->deviceInfo->maxOutputChannels; hw_channel++) {
                    data = (int16_t *)snd->data[channel];
                    *out++ = data[snd->pos] * snd->metadata.volume;
                    if (channel >= snd->metadata.channels) channel = 0;
                }
                snd->pos++;
            }
            else {
                for(int channel = 0; channel < snd->metadata.channels; channel++) {
                    data = (int16_t *)snd->data[channel];
                    *out++ = data[snd->pos] * snd->metadata.volume;
                }
                snd->pos++;
            }
        }
        else {
            if (snd->metadata.channels < snd->deviceInfo->maxOutputChannels) {
                for(int channel = 0; channel < snd->deviceInfo->maxOutputChannels; channel++)
                    *out++ = 0;
                snd->pos = 0;
            }
            else {
                for(int channel = 0; channel < snd->metadata.channels; channel++)
                    *out++ = 0;
                snd->pos = 0;
            }
            #ifdef DEBUG
            printf("Played %s once\n", snd->filename);
            #endif // DEBUG
        }
    }
}

//Podfunkcja wywołania PA_write.
//Odpowiada za buforowanie 24- i 32-bitowych całkowitych danych dźwiękowych.
void sound::PA_write_int32(void *outputBuffer,
                           unsigned long framesPerBuffer,
                           sound *snd)
{
    int32_t *out = (int32_t *)outputBuffer;
    int32_t *data;
    for(unsigned long i = 0; i < framesPerBuffer; i++) {
        if (snd->pos < snd->metadata.totalSamples) {
            if (snd->metadata.channels < snd->deviceInfo->maxOutputChannels) {
                for(int channel = 0, hw_channel = 0; hw_channel < snd->deviceInfo->maxOutputChannels; hw_channel++) {
                    data = (int32_t *)snd->data[channel];
                    *out++ = data[snd->pos] * snd->metadata.volume;
                    if (channel >= snd->metadata.channels) channel = 0;
                }
                snd->pos++;
            }
            else {
                for(int channel = 0; channel < snd->metadata.channels; channel++) {
                    data = (int32_t *)snd->data[channel];
                    *out++ = data[snd->pos] * snd->metadata.volume;
                }
                snd->pos++;
            }
        }
        else {
            if (snd->metadata.channels < snd->deviceInfo->maxOutputChannels) {
                for(int channel = 0; channel < snd->deviceInfo->maxOutputChannels; channel++)
                    *out++ = 0;
                snd->pos = 0;
            }
            else {
                for(int channel = 0; channel < snd->metadata.channels; channel++)
                    *out++ = 0;
                snd->pos = 0;
            }
            #ifdef DEBUG
            printf("Played %s once\n", snd->filename);
            #endif // DEBUG
        }
    }
}

//Wywołanie zwrotne biblioteki PortAudio.
//Odpowiada za buforowanie danych dźwiękowych.
int sound::PA_write(const void *inputBuffer,
                    void *outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *userData)
{
    sound *snd = (sound *)userData;

    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    switch (snd->metadata.bps) {
        case 4:
        case 8: PA_write_int8(outputBuffer, framesPerBuffer, snd); /* 8 bit unsigned integer output */
            break;
        case 16: PA_write_int16(outputBuffer, framesPerBuffer, snd); /* 16 bit signed integer output */
            break;
        case 24: PA_write_int32(outputBuffer, framesPerBuffer, snd); /* 24 bit signed integer output */
            break;
        default: {
            PA_write_int32(outputBuffer, framesPerBuffer, snd);
        }
    }

    return paContinue;
}

//Wywołanie zwrotne biblioteki PortAudio.
//Określa działanie wykonywane po zamknięciu strumienia.
void sound::StreamFinished(void* userData)
{
    #ifdef DEBUG
    printf( "Stream Completed\n" );
    #endif // DEBUG
}

//Funkcja odpowiadająca za wczytanie i dekompresję danych dźwiękowych formatu FLAC.
int sound::FLAC_decode(sound *snd)
{
    if (snd->filename[0] != '\0') {
        FLAC__bool ok = true;
        FLAC__StreamDecoder *decoder = 0;
        FLAC__StreamDecoderInitStatus initStatus;

        if ((decoder = FLAC__stream_decoder_new()) == NULL) {
            fprintf(stderr, "ERROR: allocating decoder\n");
            return -1;
        }

        (void)FLAC__stream_decoder_set_md5_checking(decoder, true);

        initStatus = FLAC__stream_decoder_init_file(decoder, snd->filename, FLAC_write, FLAC_meta, FLAC_error, snd /*user*/);
        if(initStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
            fprintf(stderr, "ERROR: initializing decoder: %s\n", FLAC__StreamDecoderInitStatusString[initStatus]);
            ok = false;
        }

        if(ok) {
            ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
            fprintf(stderr, "decoding: %s\n", ok? "succeeded" : "FAILED");
            fprintf(stderr, "   state: %s\n", FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
        }

        FLAC__stream_decoder_delete(decoder);

        #ifdef DEBUG
        printf("End of decoding file\n\n");
        #endif // DEBUG

        return ok? 0 : -1;
    }
    else return -1;
}

//Funkcja inicjalizująca bibliotekę PortAudio i domyślne urządzenie wyjściowe.
//Wymagane jedno wywołanie na czas działania programu.
int sound::PA_init_hw(sound *snd)
{
    snd->PA_data.err = Pa_Initialize();
    if (snd->PA_data.err) return PA_error(snd);

    if (!deviceIndex || !deviceInfo) {
        deviceIndex = Pa_GetDefaultOutputDevice();
        deviceInfo = Pa_GetDeviceInfo(sound::deviceIndex);
    }

    snd->PA_data.outputParameters.device = deviceIndex;
    if (snd->PA_data.outputParameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default output device.\n");
        return PA_error(snd);
    }
    snd->PA_data.outputParameters.channelCount = snd->deviceInfo->maxOutputChannels;       /* output channels */
    snd->PA_data.outputParameters.suggestedLatency = snd->deviceInfo->defaultLowOutputLatency;
    snd->PA_data.outputParameters.hostApiSpecificStreamInfo = NULL;
    return 0;
}

//Funkcja ustawiająca parametry urządzenia indywidualne dla każdego pliku dźwiękowego.
//Wymagane wywołanie dla każdego tworzonego obiektu.
int sound::PA_init_snd(sound *snd)
{
    switch (snd->metadata.bps) {
        case 4:
        case 8: snd->PA_data.outputParameters.sampleFormat = paInt8; /* 8 bit unsigned integer output */
            break;
        case 16: snd->PA_data.outputParameters.sampleFormat = paInt16; /* 16 bit signed integer output */
            break;
        case 24: snd->PA_data.outputParameters.sampleFormat = paInt24; /* 24 bit signed integer output */
            break;
        default: {
            fprintf(stderr, "Error: Unsupported format.\n");
            PA_error(snd);
            return -1;
        }
    }
    return 0;
}

//Domyślny konstruktor.
//Nie robi nic poza alokacja przestrzeni dla obiektu i inicjalizacją zmiennych.
sound::sound(void)
    :data(nullptr), pos(0), filename(""), metadata()
{
}

//Konstruktor bezzwłocznie dekompresujący plik dźwiękowy.
sound::sound(char *fname)
    :data(nullptr), pos(0), filename(""), metadata()
{
    strcpy(filename, fname);
    FLAC_decode(this);
    PA_init_hw(this);
    PA_init_snd(this);
}

//Domyślny dekonstruktor.
sound::~sound()
{
    if (metadata.playing) stop();

    for (int channel = 0; channel < metadata.channels; channel++)
        free(data[channel]);
    free(data);
    Pa_Terminate();
}

//Metoda rozpoczynająca odtwarzanie dźwięku.
int sound::play_loop(double volume)
{
    if (volume < 0) metadata.volume = -volume;
    else metadata.volume = volume;
    PA_data.err = Pa_OpenStream(
              &PA_data.stream,
              NULL, /* no input */
              &PA_data.outputParameters,
              metadata.sampleRate,
              PA_data.framesPerBuffer,
              paNoFlag,//paClipOff, /* we won't output out of range samples so don't bother clipping them */
              PA_write,
              this); //userdata
    if (PA_data.err) return PA_error(this);

    PA_data.err = Pa_SetStreamFinishedCallback(PA_data.stream, &StreamFinished);
    if (PA_data.err) return PA_error(this);

    PA_data.err = Pa_StartStream(PA_data.stream);
    if (PA_data.err) return PA_error(this);

    metadata.playing = true;

    #ifdef DEBUG
    if (deviceInfo->maxOutputChannels > metadata.channels)
        printf("Playing file %s on %d channel(s) extended to %d channel(s). Volume x%f\n",
            filename, metadata.channels, deviceInfo->maxOutputChannels, metadata.volume);
    else
        printf("Playing file %s on %d channel(s). Volume x%f\n",
            filename, metadata.channels, metadata.volume);
    #endif // DEBUG

    return PA_data.err;
}

//Metoda kończąca odtwarzanie dźwięku.
int sound::stop()
{
    PA_data.err = Pa_StopStream(PA_data.stream);
    if (PA_data.err) return PA_error(this);

    metadata.playing = false;

    PA_data.err = Pa_CloseStream(PA_data.stream);
    if (PA_data.err) return PA_error(this);

    Pa_Terminate();
    #ifdef DEBUG
    printf("Playback finished.\n");
    #endif // DEBUG

    return PA_data.err;
}

//Statyczna metoda wykonywana w oddzielnym wątku w ramach metody play_for.
//Kończy odtwarzanie dźwięku po określonym czasie.
void *sound::t_stop(void *userData)
{
    struct chrono *t_data = (struct chrono *)userData;
    Pa_Sleep(t_data->msec);
    t_data->snd->stop();
    free(t_data);
    return nullptr;
}

//Metoda odtwarzająca dźwięk przez określony czas.
//Podany czas nie wstrzymuje głównego wątku.
int sound::play_for(long msec,
                    double volume)
{
    if (!msec) msec = (metadata.totalSamples)/(metadata.sampleRate)*1000;
    if (play_loop(volume)) return PA_error(this);
    struct chrono *t_data = (struct chrono *)malloc(sizeof(struct chrono));
    t_data->msec = msec;
    t_data->snd = this;
    pthread_t id;
    if (pthread_create(&id, NULL, sound::t_stop, t_data))
        printf("Nie udalo sie stworzyc watku dla pliku %s", filename);
    return (int)id;
}

//Statyczna metoda wykonywana w oddzielnym wątku w ramach metody play_until.
//Kończy odtwarzanie dźwięku, jeśli spełniony jest warunek.
void *sound::t_stop_after(void* userData)
{
    struct condit *t_data = (struct condit *)userData;
    while (*t_data->condition != t_data->state) Pa_Sleep(t_data->interval);
    t_data->snd->stop();
    free(t_data);
    return nullptr;
}

//Metoda odtwarzająca dźwięk, dopóki zmienna typu bool nie przyjmie określonego stanu.
int sound::play_until(bool *condition,
                      bool state,
                      long interval,
                      double volume)
{
    if (play_loop(volume)) return PA_error(this);

    struct condit *t_data = (struct condit *)malloc(sizeof(struct condit));
    t_data->condition = condition;
    t_data->state = state;
    t_data->interval = interval;
    t_data->snd = this;

    pthread_t id;
    if (pthread_create(&id, NULL, sound::t_stop_after, t_data))
        printf("Nie udalo sie stworzyc watku dla pliku %s", filename);

    return (int)id;
}
