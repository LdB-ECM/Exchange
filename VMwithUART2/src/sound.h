#ifndef _SOUND_H_
#define _SOUND_H_

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif


extern void init_audio_jack(void);

extern void play_audio (void);


#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif