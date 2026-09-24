#ifndef NOTES_H_
#define NOTES_H_

#include <stdint.h>

typedef struct
{
  uint32_t hz;
  int ms;
} note_t;

#define NOTE_GAP_MS 20

#define REST 0
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_B5 988
#define NOTE_C6 1047

static const note_t k_melody_alarm[] = {
    {NOTE_B5, 180}, {NOTE_E5, 180},
    {NOTE_B5, 180}, {NOTE_E5, 180},
    {NOTE_B5, 180}, {NOTE_E5, 180},
    {NOTE_B5, 180}, {NOTE_E5, 180},
    {NOTE_F4, 600},
};

static const note_t k_melody_success[] = {
    {NOTE_C5, 120},
    {NOTE_E5, 120},
    {NOTE_G5, 120},
    {NOTE_C6, 300},
};

#endif // NOTES_H_
