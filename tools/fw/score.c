#include "score.h"

#include "persist.h"

#define SCORE_MAX 9999

static int score;
static int saved;

void score_reset(void)
{
    score = 0;
}

void score_load(void)
{
    unsigned v = persist_read();

    if (v > SCORE_MAX)
        v = 0;
    saved = (int)v;
}

void score_clear_saved(void)
{
    saved = 0;
}

int score_get(void)
{
    return score;
}

void score_add(int points)
{
    if (points <= 0 || score >= SCORE_MAX)
        return;
    score += points;
    if (score > SCORE_MAX)
        score = SCORE_MAX;
}

int score_judge(void)
{
    int cmp;

    if (score == SCORE_MAX && saved == SCORE_MAX)
        cmp = 1;
    else if (score > saved)
        cmp = 1;
    else if (score < saved)
        cmp = -1;
    else
        cmp = 0;
    if (score > saved) {
        saved = score;
        persist_write((uint16_t)saved);
    }
    return cmp;
}
