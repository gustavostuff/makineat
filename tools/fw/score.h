#ifndef FW_SCORE_H
#define FW_SCORE_H

/*
 * The round's score. It stops at 9999.
 * score_judge compares it with the saved score. The saved score
 * changes only when the round's score is higher. score_load reads
 * that word from the EEPROM image. An erased image reads as no score.
 * The result is 1 when the score is higher, 0 when the two match,
 * and -1 when the score is lower. A score of 999 against a saved
 * 999 counts as higher.
 */

void score_reset(void);
void score_load(void);
void score_clear_saved(void);
void score_add(int points);
int score_get(void);
int score_judge(void);

#endif
