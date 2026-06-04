#ifndef MaybeInt_H
#define MaybeInt_H

typedef enum { TAG_None, TAG_Just } MaybeIntTag;

typedef struct {
    MaybeIntTag tag;
} MaybeInt;

#define match_MaybeInt(m, case_None, case_Just) \
    switch ((m).tag) { \
        case TAG_None: case_None(); break; \
        case TAG_Just: case_Just(); break; \
    }

#endif
