#ifndef SNG_H
#define SNG_H

#include "typedefs.h"
#include <vector>

namespace Sng {
  struct Info {
    struct Bpm
    {
      f32 time;
      i16 measure;
      i16 beat;
      i32 phraseIteration;
      i32 mask;
    };
    std::vector<Bpm> bpm;

    struct Phrase
    {
      u8 solo;
      u8 disparity;
      u8 ignore;
      u8 paddin;
      i32 maxDifficulty;
      i32 phraseIterationLinks;
      u8 name[32];
    };
    std::vector<Phrase> phrase;

    struct Chord
    {
      u32 mask;
      u8 frets[6];
      u8 fingers[6];
      i32 notes[6];
      u8 name[32];
    };
    std::vector<Chord> chord;

    struct BendData
    {
      struct BendData32
      {
        f32 time;
        f32 step;
        i16 unk3_0;
        u8 unk4_0;
        u8 unk5;
      };

      BendData32 bendData[32];
      i32 UsedCount;
    };
    struct ChordNotes
    {
      u32 noteMask[6];
      BendData bendData[6];
      u8 slideTo[6];
      u8 slideUnpitchTo[6];
      i16 vibrato[6];

    };
    std::vector<ChordNotes> chordNotes;

    struct Vocal
    {
      f32 time;
      i32 note;
      f32 length;
      u8 lyric[48];
    };
    std::vector<Vocal> vocal;
    struct SymbolsHeader
    {
      i32 unk1;
      i32 unk2;
      i32 unk3;
      i32 unk4;
      i32 unk5;
      i32 unk6;
      i32 unk7;
      i32 unk8;
    };
    std::vector<SymbolsHeader> symbolsHeader;

    struct SymbolsTexture
    {
      u8 font[128];
      i32 fontpathLength;
      i32 unk10;
      i32 width;
      i32 height;
    };
    std::vector<SymbolsTexture> symbolsTexture;

    struct SymbolDefinition
    {
      struct Rect
      {
        f32 yMin;
        f32 xMin;
        f32 yMax;
        f32 xMax;
      };

      u8 text[12];
      Rect rectOutter;
      Rect rectInner;
    };
    std::vector<SymbolDefinition> symbolDefinition;

    struct PhraseIteration
    {
      i32 phraseId;
      f32 startTime;
      f32 nextPhraseTime;
      i32 difficulty[3];
    };
    std::vector<PhraseIteration> phraseIteration;

    struct PhraseExtraInfoByLevel
    {
      i32 phraseId;
      i32 difficulty;
      i32 empty;
      u8 levelJump;
      i16 redundant;
      u8 padding;
    };
    std::vector<PhraseExtraInfoByLevel> phraseExtraInfoByLevel;

    struct NLinkedDifficulty
    {
      i32 levelBreak;
      i32 phraseCount;
      std::vector<i32> nLDPhrase;
    };
    std::vector<NLinkedDifficulty> nLinkedDifficulty;

    struct Action
    {
      f32 time;
      u8 actionName[256];
    };
    std::vector<Action> action;

    struct Event
    {
      f32 time;
      u8 eventName[256];
    };
    std::vector<Event> event;

    struct Tone
    {
      f32 time;
      i32 toneId;
    };
    std::vector<Tone> tone;

    struct Dna
    {
      f32 time;
      i32 dnaId;
    };
    std::vector<Dna> dna;

    struct Section
    {
      u8 name[32];
      i32 number;
      f32 startTime;
      f32 endTime;
      i32 startPhraseIterationId;
      i32 endPhraseIterationId;
      u8 stringMask[36];
    };
    std::vector<Section> section;

    struct Arrangement
    {
      struct Anchor
      {
        f32 startBeatTime;
        f32 endBeatTime;
        f32 unk3_FirstNoteTime;
        f32 unk4_LastNoteTime;
        u8 fretId;
        u8 padding[3];
        i32 width;
        i32 phraseIterationId;
      };

      struct AnchorExtension
      {
        f32 beatTime;
        u8 fretId;
        i32 unk2_0;
        i16 unk3_0;
        u8 unk4_0;
      };

      struct Fingerprint
      {
        i32 chordId;
        f32 startTime;
        f32 endTime;
        f32 unk3_FirstNoteTime;
        f32 unk4_LastNoteTime;
      };

      struct Note
      {
        u32 noteMask;
        u32 noteFlags;
        u32 hash;
        f32 time;
        u8 stringIndex;
        u8 fretId;
        u8 anchorFretId;
        u8 anchorWidth;
        i32 chordId;
        i32 chordNotesId;
        i32 phraseId;
        i32 phraseIterationId;
        i16 fingerPrintId[2];
        i16 nextIterNote;
        i16 prevIterNote;
        i16 parentPrevNote;
        u8 slideTo;
        u8 slideUnpitchTo;
        u8 leftHand;
        u8 tap;
        u8 pickDirection;
        u8 slap;
        u8 pluck;
        i16 vibrato;
        f32 sustain;
        f32 maxBend;
        std::vector<BendData> bendData;
      };

      i32 difficulty;
      std::vector<Anchor> anchors;
      std::vector<AnchorExtension> anchorExtensions;
      std::vector<Fingerprint> fingerprints1;
      std::vector<Fingerprint> fingerprints2;
      std::vector<Note> notes;
      i32 phraseCount;
      std::vector<f32> averageNotesPerIteration;
      i32 phraseIterationCount1;
      std::vector<i32> notesInIteration1;
      i32 phraseIterationCount2;
      std::vector<i32> notesInIteration2;
    };
    std::vector<Arrangement> arrangement;

    struct Metadata
    {
      f64 maxScore;
      f64 maxNotesAndChords;
      f64 maxNotesAndChordsReal;
      f64 pointsPerNote;
      f32 firstBeatLength;
      f32 startTime;
      u8 capoFretId;
      u8 lastConversionDateTime[32];
      i16 part;
      f32 songLength;
      i32 stringCount;
      std::vector<i16> tuning;
      f32 unk11FirstNoteTime;
      f32 unk12FirstNoteTime;
      i32 maxDifficulty;
    };
    Metadata metadata;
  };

  Sng::Info parse(const std::vector<u8>& sngData);
}

#endif // SNG_H
