#ifndef NOTES_H_
#define NOTES_H_

typedef struct
{
  uint32_t hz;
  int ms;
} note_t;

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

// 0 - We Will Rock You; 1 - SW: Empire March
#define MELODY 0

#if MELODY == 0
static const note_t k_melody[] = {
    // First verse, opening phrase.
    {NOTE_D5, 188}, {NOTE_E5, 188},
    {NOTE_E5, 375}, {NOTE_E5, 375}, {NOTE_E5, 375},
    {NOTE_E5, 375}, {NOTE_E5, 375}, {NOTE_E5, 375}, {NOTE_E5, 750},

    {NOTE_E5, 188}, {NOTE_E5, 188},
    {NOTE_E5, 375}, {NOTE_E5, 375}, {NOTE_E5, 750},

    {NOTE_E5, 188}, {NOTE_D5, 188},
    {NOTE_E5, 375}, {NOTE_D5, 375}, {NOTE_E5, 375},
    {NOTE_A5, 375}, {NOTE_G5, 375}, {NOTE_G5, 750},
    {REST, 375},

    // First verse, second phrase.
    {NOTE_D5, 375}, {NOTE_E5, 375}, {NOTE_E5, 375},
    {NOTE_E5, 375}, {NOTE_D5, 375}, {NOTE_E5, 750},

    {NOTE_D5, 375}, {NOTE_E5, 375},
    {NOTE_E5, 188}, {NOTE_E5, 562},

    {NOTE_D5, 188}, {NOTE_D5, 188},
    {NOTE_D5, 375}, {NOTE_D5, 375}, {NOTE_B4, 375},
    {NOTE_A4, 188}, {NOTE_G4, 188}, {NOTE_E4, 375}, {NOTE_E4, 750},
    {NOTE_B4, 375}, {NOTE_D5, 750},
    {REST, 750},

    // Chorus at approximately 80 BPM.
    {NOTE_G5, 375}, {NOTE_FS5, 375},
    {NOTE_E5, 375}, {NOTE_D5, 375},
    {NOTE_E5, 375}, {NOTE_E5, 750},
    {REST, 750},

    {NOTE_G5, 375}, {NOTE_FS5, 375},
    {NOTE_E5, 375}, {NOTE_D5, 375},
    {NOTE_E5, 375}, {NOTE_E5, 750},
};
#elif MELODY == 1
static const note_t k_melody[] = {
    {NOTE_A4, 500}, {NOTE_A4, 500}, {NOTE_A4, 500},
    {NOTE_F4, 350}, {NOTE_C5, 150},
    {NOTE_A4, 500}, {NOTE_F4, 350}, {NOTE_C5, 150},
    {NOTE_A4, 650},
    {REST, 500},
    {NOTE_E5, 500}, {NOTE_E5, 500}, {NOTE_E5, 500},
    {NOTE_F5, 350}, {NOTE_C5, 150},
    {NOTE_GS4, 500}, {NOTE_F4, 350}, {NOTE_C5, 150},
    {NOTE_A4, 650},
    {REST, 500},
    {NOTE_A5, 500}, {NOTE_A4, 300}, {NOTE_A4, 150},
    {NOTE_A5, 500}, {NOTE_GS5, 325}, {NOTE_G5, 175},
    {NOTE_FS5, 125}, {NOTE_F5, 125}, {NOTE_FS5, 250},
    {REST, 325},
    {NOTE_AS4, 250}, {NOTE_DS5, 500}, {NOTE_D5, 325},
    {NOTE_CS5, 175}, {NOTE_C5, 125}, {NOTE_B4, 125},
    {NOTE_C5, 250},
    {REST, 350},
    {NOTE_F4, 250}, {NOTE_GS4, 500}, {NOTE_F4, 350},
    {NOTE_A4, 125}, {NOTE_C5, 500}, {NOTE_A4, 375},
    {NOTE_C5, 125}, {NOTE_E5, 650},
    {REST, 500},
    {NOTE_A5, 500}, {NOTE_A4, 300}, {NOTE_A4, 150},
    {NOTE_A5, 500}, {NOTE_GS5, 325}, {NOTE_G5, 175},
    {NOTE_FS5, 125}, {NOTE_F5, 125}, {NOTE_FS5, 250},
    {REST, 325},
    {NOTE_AS4, 250}, {NOTE_DS5, 500}, {NOTE_D5, 325},
    {NOTE_CS5, 175}, {NOTE_C5, 125}, {NOTE_B4, 125}, 
    {NOTE_C5, 250},
    {REST, 350},
    {NOTE_F4, 250}, {NOTE_GS4, 500}, {NOTE_F4, 350},
    {NOTE_C5, 125}, {NOTE_A4, 500}, {NOTE_F4, 375},
    {NOTE_C5, 125}, {NOTE_A4, 650},
};
#endif

#endif // NOTES_H_
