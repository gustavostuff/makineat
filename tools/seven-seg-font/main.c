#include "seven_seg.h"
#include "seven_seg_hitmap.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROWS 128
/* Picture is scaled up by an integer nearest-neighbor factor. */
#define CANVAS_W 320
#define CANVAS_H 180
#define VIEW_SCALE 2
#define DIGIT_SCALE 2
#define MARGIN 4
#define GAP 2
#define COL_GAP 20
#define PAD 4
#define BAR_W 4
#define FOOT_H 22
#define LIST_Y 2
#define SCROLL_STEP 16
#define FACE 232

typedef struct Row {
    char symbol;
    uint8_t mask;
} Row;

static Row rows[MAX_ROWS];
static Row saved[MAX_ROWS];
static int row_count;
static int saved_count;
static int focus_row = -1;
static int scroll_y;
static int text_w;
static int text_h;
static int field_w;
static int field_h;
static int new_w;
static int new_h;
static SDL_Texture *trash_tex;
static int trash_src_w;
static int trash_src_h;
static int bar_drag;
static int bar_grab;
static int hover_x;
static int hover_y;
static int hover_valid;
static int warn_taken;
static Uint32 warn_at;
static char font_file[1024];

static void layout_from_font(TTF_Font *font)
{
    int h = 0;

    TTF_SizeUTF8(font, "0", &text_w, &text_h);
    TTF_SizeUTF8(font, "New", &new_w, &h);
    field_w = text_w + PAD * 2;
    field_h = text_h + PAD * 2;
    new_w += 10;
    new_h = h + 6;
}

static int list_top(void)
{
    return LIST_Y;
}

static int footer_y(void)
{
    return CANVAS_H - FOOT_H;
}

static int list_view(void)
{
    int h = footer_y() - list_top();

    return h > 0 ? h : 0;
}

static void note_taken(void)
{
    warn_taken = 1;
    warn_at = SDL_GetTicks();
}

static void age_warning(void)
{
    if (warn_taken && SDL_GetTicks() - warn_at >= 2000)
        warn_taken = 0;
}

static int row_pitch(const SevenSegDisplay *disp)
{
    int cell = disp->digit_h * DIGIT_SCALE;

    if (field_h > cell)
        cell = field_h;
    return cell + GAP;
}

static int rows_px(const SevenSegDisplay *disp)
{
    return row_count * row_pitch(disp);
}

static int max_scroll_of(const SevenSegDisplay *disp)
{
    int extra = rows_px(disp) - list_view();

    return extra > 0 ? extra : 0;
}

static void clamp_scroll(const SevenSegDisplay *disp)
{
    int max_scroll = max_scroll_of(disp);

    if (max_scroll < 0)
        max_scroll = 0;
    if (scroll_y > max_scroll)
        scroll_y = max_scroll;
    if (scroll_y < 0)
        scroll_y = 0;
}

static int unsaved(void)
{
    int i;

    if (row_count != saved_count)
        return 1;
    for (i = 0; i < row_count; i++) {
        if (rows[i].symbol != saved[i].symbol || rows[i].mask != saved[i].mask)
            return 1;
    }
    return 0;
}

static void remember_saved(void)
{
    memcpy(saved, rows, (size_t)row_count * sizeof rows[0]);
    saved_count = row_count;
}

static int symbol_taken(char symbol, int except)
{
    int i;

    if (symbol == '\0')
        return 0;
    for (i = 0; i < row_count; i++) {
        if (i != except && rows[i].symbol == symbol)
            return 1;
    }
    return 0;
}

static int insert_row(int at)
{
    if (row_count >= MAX_ROWS || at < 0 || at > row_count)
        return 0;
    memmove(&rows[at + 1], &rows[at], (size_t)(row_count - at) * sizeof rows[0]);
    rows[at].symbol = '\0';
    rows[at].mask = 0;
    row_count++;
    focus_row = at;
    return 1;
}

static void remove_row(int at)
{
    if (at < 0 || at >= row_count)
        return;
    memmove(&rows[at], &rows[at + 1], (size_t)(row_count - at - 1) * sizeof rows[0]);
    row_count--;
    if (focus_row == at)
        focus_row = -1;
    else if (focus_row > at)
        focus_row--;
}

static int parse_glyph_line(const char *line, char *symbol, unsigned *mask)
{
    const char *p = strstr(line, "{ '");
    char *end = NULL;

    if (p == NULL)
        return 0;
    p += 3;
    if (*p == '\\') {
        p++;
        if (*p != '\\' && *p != '\'')
            return 0;
        *symbol = *p++;
    } else if (*p != '\0' && *p != '\'') {
        *symbol = *p++;
    } else {
        return 0;
    }
    if (*p != '\'')
        return 0;
    p = strstr(p, "0x");
    if (p == NULL)
        return 0;
    *mask = (unsigned)strtoul(p, &end, 16);
    if (end == p)
        return 0;
    return 1;
}

static void load_compiled(void)
{
    int i;

    row_count = 0;
    for (i = 0; i < seven_seg_font_count && row_count < MAX_ROWS; i++) {
        if (seven_seg_font[i].symbol == '\0')
            continue;
        rows[row_count].symbol = seven_seg_font[i].symbol;
        rows[row_count].mask = seven_seg_font[i].mask;
        row_count++;
    }
}

static int find_font_file(void)
{
    const char *rels[] = {
        "tools/fw/seven_seg_font.c",
        "../fw/seven_seg_font.c",
        NULL,
    };
    char *base = SDL_GetBasePath();
    int i;
    FILE *fp;

    font_file[0] = '\0';
    if (base != NULL) {
        snprintf(font_file, sizeof font_file, "%s../fw/seven_seg_font.c", base);
        SDL_free(base);
        fp = fopen(font_file, "r");
        if (fp != NULL) {
            fclose(fp);
            return 0;
        }
    }
    for (i = 0; rels[i] != NULL; i++) {
        fp = fopen(rels[i], "r");
        if (fp != NULL) {
            fclose(fp);
            snprintf(font_file, sizeof font_file, "%s", rels[i]);
            return 0;
        }
    }
    font_file[0] = '\0';
    return -1;
}

static int load_font_file(void)
{
    FILE *fp;
    char line[256];
    int n = 0;
    Row parsed[MAX_ROWS];

    if (font_file[0] == '\0')
        return -1;
    fp = fopen(font_file, "r");
    if (fp == NULL)
        return -1;
    while (fgets(line, sizeof line, fp) != NULL && n < MAX_ROWS) {
        char symbol = '\0';
        unsigned mask = 0;

        if (!parse_glyph_line(line, &symbol, &mask))
            continue;
        if (symbol == '\0')
            continue;
        parsed[n].symbol = symbol;
        parsed[n].mask = (uint8_t)mask;
        n++;
    }
    fclose(fp);
    memcpy(rows, parsed, (size_t)n * sizeof rows[0]);
    row_count = n;
    return 0;
}

static void reveal_row(const SevenSegDisplay *disp, int index)
{
    int top;
    int bottom;

    if (index < 0 || disp->digit_h <= 0)
        return;
    top = list_top() + index * row_pitch(disp);
    bottom = top + row_pitch(disp) - GAP;
    if (index == row_count - 1)
        bottom = list_top() + row_count * row_pitch(disp);
    if (top - scroll_y < list_top())
        scroll_y = top - list_top();
    if (bottom - scroll_y > footer_y())
        scroll_y = bottom - footer_y();
}

static int save_font_file(void)
{
    FILE *fp;
    char tmp[1040];
    int i;
    int kept = 0;
    int bad;
    Row next[MAX_ROWS];

    if (font_file[0] == '\0')
        return -1;
    if (snprintf(tmp, sizeof tmp, "%s.tmp", font_file) >= (int)sizeof tmp)
        return -1;
    for (i = 0; i < row_count; i++) {
        if (rows[i].symbol == '\0')
            continue;
        next[kept++] = rows[i];
    }
    fp = fopen(tmp, "w");
    if (fp == NULL)
        return -1;
    fputs("/* seven-seg font. Saved by tools/seven-seg-font. */\n", fp);
    fputs("#include \"seven_seg_font.h\"\n\n", fp);
    if (kept == 0) {
        fputs("const SevenSegGlyph seven_seg_font[1] = {\n", fp);
        fputs("    { 0, 0x00 },\n", fp);
        fputs("};\n\n", fp);
        fputs("const int seven_seg_font_count = 0;\n\n", fp);
    } else {
        fputs("const SevenSegGlyph seven_seg_font[] = {\n", fp);
        for (i = 0; i < kept; i++) {
            char symbol = next[i].symbol;

            if (symbol == '\\' || symbol == '\'')
                fprintf(fp, "    { '\\%c', 0x%02X },\n", symbol, (unsigned)next[i].mask);
            else
                fprintf(fp, "    { '%c', 0x%02X },\n", symbol, (unsigned)next[i].mask);
        }
        fputs("};\n\n", fp);
        fprintf(fp, "const int seven_seg_font_count = %d;\n\n", kept);
    }
    fputs("static int seven_seg_font_find(char c, uint8_t *mask)\n", fp);
    fputs("{\n", fp);
    fputs("    int i;\n\n", fp);
    fputs("    if (c == '\\0')\n", fp);
    fputs("        return 0;\n", fp);
    fputs("    for (i = 0; i < seven_seg_font_count; i++) {\n", fp);
    fputs("        if (seven_seg_font[i].symbol == c) {\n", fp);
    fputs("            *mask = seven_seg_font[i].mask;\n", fp);
    fputs("            return 1;\n", fp);
    fputs("        }\n", fp);
    fputs("    }\n", fp);
    fputs("    return 0;\n", fp);
    fputs("}\n\n", fp);
    fputs("uint8_t seven_seg_font_mask(char c)\n", fp);
    fputs("{\n", fp);
    fputs("    uint8_t mask = 0;\n", fp);
    fputs("    char other = 0;\n\n", fp);
    fputs("    if (seven_seg_font_find(c, &mask))\n", fp);
    fputs("        return mask;\n", fp);
    fputs("    if (c >= 'a' && c <= 'z')\n", fp);
    fputs("        other = (char)(c - 'a' + 'A');\n", fp);
    fputs("    else if (c >= 'A' && c <= 'Z')\n", fp);
    fputs("        other = (char)(c - 'A' + 'a');\n", fp);
    fputs("    if (other != 0 && seven_seg_font_find(other, &mask))\n", fp);
    fputs("        return mask;\n", fp);
    fputs("    return 0;\n", fp);
    fputs("}\n\n", fp);
    fputs("uint8_t seven_seg_font_max7219(char c)\n", fp);
    fputs("{\n", fp);
    fputs("    uint8_t mask = seven_seg_font_mask(c);\n", fp);
    fputs("    uint8_t out = 0;\n\n", fp);
    fputs("    if ((mask & SEVEN_SEG_A) != 0)\n", fp);
    fputs("        out |= 0x40;\n", fp);
    fputs("    if ((mask & SEVEN_SEG_B) != 0)\n", fp);
    fputs("        out |= 0x20;\n", fp);
    fputs("    if ((mask & SEVEN_SEG_C) != 0)\n", fp);
    fputs("        out |= 0x10;\n", fp);
    fputs("    if ((mask & SEVEN_SEG_D) != 0)\n", fp);
    fputs("        out |= 0x08;\n", fp);
    fputs("    if ((mask & SEVEN_SEG_E) != 0)\n", fp);
    fputs("        out |= 0x04;\n", fp);
    fputs("    if ((mask & SEVEN_SEG_F) != 0)\n", fp);
    fputs("        out |= 0x02;\n", fp);
    fputs("    if ((mask & SEVEN_SEG_G) != 0)\n", fp);
    fputs("        out |= 0x01;\n", fp);
    fputs("    if ((mask & SEVEN_SEG_DP) != 0)\n", fp);
    fputs("        out |= 0x80;\n", fp);
    fputs("    return out;\n", fp);
    fputs("}\n", fp);
    bad = ferror(fp);
    if (fclose(fp) != 0 || bad) {
        remove(tmp);
        return -1;
    }
    if (rename(tmp, font_file) != 0) {
        remove(tmp);
        return -1;
    }
    memcpy(rows, next, (size_t)kept * sizeof rows[0]);
    row_count = kept;
    focus_row = -1;
    remember_saved();
    return 0;
}

static TTF_Font *open_ui_font(void)
{
    const char *rels[] = {
        "tools/assets/fonts/proggy-tiny.ttf",
        "../assets/fonts/proggy-tiny.ttf",
        NULL,
    };
    char *base = SDL_GetBasePath();
    char path[1024];
    TTF_Font *font = NULL;
    int i;

    if (base != NULL) {
        snprintf(path, sizeof path, "%s../assets/fonts/proggy-tiny.ttf", base);
        SDL_free(base);
        font = TTF_OpenFont(path, 16);
    }
    for (i = 0; font == NULL && rels[i] != NULL; i++)
        font = TTF_OpenFont(rels[i], 16);
    if (font == NULL) {
        SDL_Log("TTF_OpenFont failed: %s", TTF_GetError());
        return NULL;
    }
    TTF_SetFontHinting(font, TTF_HINTING_MONO);
    TTF_SetFontKerning(font, 0);
    return font;
}

static void draw_label(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y,
                       SDL_Color color)
{
    SDL_Surface *surface;
    SDL_Texture *tex;
    SDL_Rect dst;

    if (text == NULL || text[0] == '\0')
        return;
    surface = TTF_RenderUTF8_Solid(font, text, color);
    if (surface == NULL)
        return;
    tex = SDL_CreateTextureFromSurface(renderer, surface);
    dst.x = x;
    dst.y = y;
    dst.w = surface->w;
    dst.h = surface->h;
    SDL_FreeSurface(surface);
    if (tex == NULL)
        return;
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

static int point_in(const SDL_Rect *box, int x, int y)
{
    return x >= box->x && y >= box->y && x < box->x + box->w && y < box->y + box->h;
}

static int box_hot(const SDL_Rect *box)
{
    return hover_valid && point_in(box, hover_x, hover_y);
}

static void fill_face(SDL_Renderer *renderer, const SDL_Rect *box)
{
    int v = box_hot(box) ? 255 : FACE;

    SDL_SetRenderDrawColor(renderer, v, v, v, 255);
    SDL_RenderFillRect(renderer, box);
}

static void draw_button(SDL_Renderer *renderer, TTF_Font *font, const SDL_Rect *box, const char *label)
{
    SDL_Color black = {0, 0, 0, 255};
    int tw = 0;
    int th = 0;

    fill_face(renderer, box);
    TTF_SizeUTF8(font, label, &tw, &th);
    draw_label(renderer, font, label, box->x + (box->w - tw) / 2, box->y + (box->h - th) / 2, black);
}

static void row_boxes(const SevenSegDisplay *disp, int index, SDL_Rect *field, SDL_Rect *digit,
                      SDL_Rect *remove)
{
    int digit_h = disp->digit_h * DIGIT_SCALE;
    int top = list_top() + index * row_pitch(disp) - scroll_y;
    int mid = top + (digit_h - field_h) / 2;
    int x = MARGIN + text_w * 3 + 10;

    field->x = x;
    field->y = mid;
    field->w = field_w;
    field->h = field_h;
    x += field_w + COL_GAP;
    digit->x = x;
    digit->y = top;
    digit->w = disp->digit_w * DIGIT_SCALE;
    digit->h = digit_h;
    x += digit->w + COL_GAP;
    remove->w = trash_src_w > 0 ? trash_src_w : 11;
    remove->h = trash_src_h > 0 ? trash_src_h : 12;
    remove->x = x;
    remove->y = top + (digit_h - remove->h) / 2;
}

static void new_box(SDL_Rect *add)
{
    add->w = new_w;
    add->h = new_h;
    add->x = CANVAS_W - MARGIN - add->w;
    add->y = footer_y() + (FOOT_H - add->h) / 2;
}

static int segment_at(const SDL_Rect *digit, int mx, int my)
{
    int px;
    int py;
    int id;

    if (!point_in(digit, mx, my))
        return 0;
    px = (mx - digit->x) / DIGIT_SCALE;
    py = (my - digit->y) / DIGIT_SCALE;
    if (px < 0 || py < 0 || px >= SEVEN_SEG_HIT_W || py >= SEVEN_SEG_HIT_H)
        return 0;
    id = seven_seg_hitmap[py * SEVEN_SEG_HIT_W + px];
    if (id < 1 || id > 8)
        return 0;
    return id;
}

static void illuminate_hover(const SevenSegDisplay *disp, SDL_Renderer *renderer, const SDL_Rect *digit,
                            int id)
{
    SDL_Texture *tex;
    SDL_BlendMode blend = SDL_BLENDMODE_BLEND;
    SDL_Rect src = {0, 0, disp->digit_w, disp->digit_h};

    if (id < 1 || id > 8)
        return;
    tex = id == 8 ? disp->dp : disp->seg[id - 1];
    if (tex == NULL)
        return;
    SDL_GetTextureBlendMode(tex, &blend);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(tex, 110, 110, 110);
    SDL_RenderCopy(renderer, tex, &src, digit);
    SDL_SetTextureColorMod(tex, 255, 255, 255);
    SDL_SetTextureBlendMode(tex, blend);
}

static void toggle_at(Row *row, const SDL_Rect *digit, int mx, int my)
{
    int id = segment_at(digit, mx, my);

    if (id == 0)
        return;
    row->mask ^= (uint8_t)(1u << (id - 1));
}

static int over_segment(const SevenSegDisplay *disp, int mx, int my)
{
    int i;
    SDL_Rect field;
    SDL_Rect digit;
    SDL_Rect remove;

    if (my < list_top() || my >= footer_y())
        return 0;
    for (i = 0; i < row_count; i++) {
        row_boxes(disp, i, &field, &digit, &remove);
        if (digit.y + digit.h <= list_top() || digit.y >= footer_y())
            continue;
        if (segment_at(&digit, mx, my) != 0)
            return 1;
    }
    return 0;
}

static int over_button(const SevenSegDisplay *disp, int mx, int my)
{
    int i;
    SDL_Rect field;
    SDL_Rect digit;
    SDL_Rect remove;
    SDL_Rect add;

    new_box(&add);
    if (point_in(&add, mx, my))
        return 1;
    if (my < list_top() || my >= footer_y())
        return 0;
    for (i = 0; i < row_count; i++) {
        row_boxes(disp, i, &field, &digit, &remove);
        if (remove.y + remove.h <= list_top() || remove.y >= footer_y())
            continue;
        if (point_in(&remove, mx, my))
            return 1;
    }
    return 0;
}

static int over_field(const SevenSegDisplay *disp, int mx, int my)
{
    int i;
    SDL_Rect field;
    SDL_Rect digit;
    SDL_Rect remove;

    if (my < list_top() || my >= footer_y())
        return 0;
    for (i = 0; i < row_count; i++) {
        row_boxes(disp, i, &field, &digit, &remove);
        if (field.y + field.h <= list_top() || field.y >= footer_y())
            continue;
        if (point_in(&field, mx, my))
            return 1;
    }
    return 0;
}

static void bar_geom(const SevenSegDisplay *disp, SDL_Rect *track, SDL_Rect *thumb)
{
    int view = list_view();
    int content = rows_px(disp);
    int max_scroll = max_scroll_of(disp);
    int thumb_h;

    if (view < 1)
        view = 1;
    if (content < view)
        content = view;
    track->x = CANVAS_W - BAR_W;
    track->y = 0;
    track->w = BAR_W;
    track->h = footer_y();
    if (track->h < 1)
        track->h = 1;
    thumb_h = (int)((long)track->h * view / content);
    if (thumb_h < 8)
        thumb_h = 8;
    if (thumb_h > track->h)
        thumb_h = track->h;
    thumb->x = track->x;
    thumb->w = BAR_W;
    thumb->h = thumb_h;
    if (max_scroll <= 0 || thumb_h >= track->h)
        thumb->y = track->y;
    else
        thumb->y = track->y + (int)((long)(track->h - thumb_h) * scroll_y / max_scroll);
}

static void scroll_from_thumb(const SevenSegDisplay *disp, int thumb_y)
{
    int max_scroll = max_scroll_of(disp);
    int span;
    SDL_Rect track;
    SDL_Rect thumb;

    bar_geom(disp, &track, &thumb);
    span = track.h - thumb.h;
    if (thumb_y < 0)
        thumb_y = 0;
    if (thumb_y > span)
        thumb_y = span;
    if (span <= 0 || max_scroll <= 0)
        scroll_y = 0;
    else
        scroll_y = (int)((long)thumb_y * max_scroll / span);
    clamp_scroll(disp);
}

static int bar_press(const SevenSegDisplay *disp, int mx, int my)
{
    SDL_Rect track;
    SDL_Rect thumb;

    bar_geom(disp, &track, &thumb);
    if (!point_in(&track, mx, my))
        return 0;
    if (point_in(&thumb, mx, my)) {
        bar_drag = 1;
        bar_grab = my - thumb.y;
    } else {
        scroll_from_thumb(disp, my - thumb.h / 2);
    }
    return 1;
}

static void on_click(const SevenSegDisplay *disp, int mx, int my)
{
    int i;
    SDL_Rect field;
    SDL_Rect digit;
    SDL_Rect remove;
    SDL_Rect add;

    new_box(&add);
    if (point_in(&add, mx, my)) {
        if (insert_row(row_count))
            reveal_row(disp, row_count - 1);
        return;
    }
    if (my < list_top() || my >= footer_y())
        return;
    for (i = 0; i < row_count; i++) {
        row_boxes(disp, i, &field, &digit, &remove);
        if (digit.y + digit.h <= list_top() || digit.y >= footer_y())
            continue;
        if (point_in(&field, mx, my)) {
            if (focus_row != i)
                warn_taken = 0;
            focus_row = i;
            return;
        }
        if (point_in(&remove, mx, my)) {
            remove_row(i);
            return;
        }
        if (point_in(&digit, mx, my)) {
            toggle_at(&rows[i], &digit, mx, my);
            return;
        }
    }
    focus_row = -1;
}

static void draw_trash(SDL_Renderer *renderer, const SDL_Rect *box)
{
    SDL_Rect src = {0, 0, trash_src_w, trash_src_h};

    if (trash_tex == NULL) {
        SDL_SetRenderDrawColor(renderer, 226, 226, 226, 255);
        SDL_RenderFillRect(renderer, box);
        return;
    }
    SDL_RenderCopy(renderer, trash_tex, &src, box);
}

static void draw_rows(SDL_Renderer *renderer, TTF_Font *font, const SevenSegDisplay *disp)
{
    SDL_Color black = {0, 0, 0, 255};
    SDL_Rect clip;
    SDL_Rect foot;
    int i;

    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
    SDL_RenderClear(renderer);

    clip.x = 0;
    clip.y = list_top();
    clip.w = CANVAS_W - BAR_W;
    clip.h = list_view();
    SDL_RenderSetClipRect(renderer, &clip);
    for (i = 0; i < row_count; i++) {
        SDL_Rect field;
        SDL_Rect digit;
        SDL_Rect remove;
        char text[2];

        row_boxes(disp, i, &field, &digit, &remove);
        if (digit.y + digit.h <= list_top() || digit.y >= footer_y())
            continue;
        {
            char idx[16];
            int tw = 0;
            int th = 0;
            SDL_Color num = {220, 220, 220, 255};

            snprintf(idx, sizeof idx, "%d", i);
            TTF_SizeUTF8(font, idx, &tw, &th);
            draw_label(renderer, font, idx, MARGIN, digit.y + (digit.h - th) / 2, num);
        }
        fill_face(renderer, &field);
        if (rows[i].symbol == ' ') {
            SDL_Rect mark;

            mark.w = text_w - 2;
            if (mark.w < 1)
                mark.w = 1;
            mark.h = 1;
            mark.x = field.x + (field.w - mark.w) / 2;
            mark.y = field.y + field.h / 2;
            SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
            SDL_RenderFillRect(renderer, &mark);
        } else if (rows[i].symbol != '\0') {
            int tw = 0;
            int th = 0;

            text[0] = rows[i].symbol;
            text[1] = '\0';
            TTF_SizeUTF8(font, text, &tw, &th);
            draw_label(renderer, font, text, field.x + (field.w - tw) / 2,
                       field.y + (field.h - th) / 2, black);
        }
        seven_seg_draw_digit(disp, renderer, digit.x, digit.y, rows[i].mask, DIGIT_SCALE);
        if (hover_valid) {
            int id = segment_at(&digit, hover_x, hover_y);

            if (id != 0 && (rows[i].mask & (uint8_t)(1u << (id - 1))) == 0)
                illuminate_hover(disp, renderer, &digit, id);
        }
        draw_trash(renderer, &remove);
    }
    SDL_RenderSetClipRect(renderer, NULL);

    foot.x = 0;
    foot.y = footer_y();
    foot.w = CANVAS_W;
    foot.h = FOOT_H;
    SDL_SetRenderDrawColor(renderer, 54, 54, 54, 255);
    SDL_RenderFillRect(renderer, &foot);
    if (warn_taken) {
        SDL_Color msg = {255, 214, 0, 255};
        int tw = 0;
        int th = 0;

        TTF_SizeUTF8(font, "already assigned", &tw, &th);
        draw_label(renderer, font, "already assigned", MARGIN, foot.y + (foot.h - th) / 2, msg);
    }
    {
        SDL_Rect add;

        new_box(&add);
        draw_button(renderer, font, &add, "New");
    }
    {
        SDL_Rect track;
        SDL_Rect thumb;

        bar_geom(disp, &track, &thumb);
        SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
        SDL_RenderFillRect(renderer, &track);
        SDL_SetRenderDrawColor(renderer, 170, 170, 170, 255);
        SDL_RenderFillRect(renderer, &thumb);
    }
}

static void view_rect(SDL_Renderer *renderer, int *scale, SDL_Rect *dst)
{
    int out_w;
    int out_h;
    int fit;

    SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
    *scale = out_w / CANVAS_W;
    fit = out_h / CANVAS_H;
    if (fit < *scale)
        *scale = fit;
    if (*scale < 1)
        *scale = 1;
    dst->w = CANVAS_W * *scale;
    dst->h = CANVAS_H * *scale;
    dst->x = (out_w - dst->w) / 2;
    dst->y = (out_h - dst->h) / 2;
}

static void present_canvas(SDL_Renderer *renderer, SDL_Texture *canvas)
{
    int scale;
    SDL_Rect dst;

    SDL_SetRenderTarget(renderer, NULL);
    view_rect(renderer, &scale, &dst);
    SDL_SetRenderDrawColor(renderer, 28, 28, 28, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, canvas, NULL, &dst);
    SDL_RenderPresent(renderer);
}

static int window_to_canvas(SDL_Window *window, SDL_Renderer *renderer, int x, int y, int *cx, int *cy)
{
    int scale;
    int win_w;
    int win_h;
    int out_w;
    int out_h;
    SDL_Rect dst;

    SDL_GetWindowSize(window, &win_w, &win_h);
    SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
    if (win_w < 1 || win_h < 1)
        return 0;
    x = (int)((long)x * out_w / win_w);
    y = (int)((long)y * out_h / win_h);
    view_rect(renderer, &scale, &dst);
    if (x < dst.x || y < dst.y || x >= dst.x + dst.w || y >= dst.y + dst.h)
        return 0;
    *cx = (x - dst.x) / scale;
    *cy = (y - dst.y) / scale;
    return 1;
}

static int canvas_y(SDL_Window *window, SDL_Renderer *renderer, int y, int *cy)
{
    int scale;
    int win_w;
    int win_h;
    int out_w;
    int out_h;
    SDL_Rect dst;

    SDL_GetWindowSize(window, &win_w, &win_h);
    SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
    if (win_h < 1)
        return 0;
    y = (int)((long)y * out_h / win_h);
    view_rect(renderer, &scale, &dst);
    if (scale < 1)
        return 0;
    *cy = (y - dst.y) / scale;
    if (*cy < 0)
        *cy = 0;
    if (*cy >= CANVAS_H)
        *cy = CANVAS_H - 1;
    (void)win_w;
    return 1;
}

static void update_cursor(SDL_Window *window, SDL_Renderer *renderer, SDL_Cursor *hand,
                          const SevenSegDisplay *disp)
{
    int x;
    int y;
    int cx;
    int cy;
    int over = 0;
    static int shown = -1;

    SDL_GetMouseState(&x, &y);
    hover_valid = window_to_canvas(window, renderer, x, y, &cx, &cy);
    if (hover_valid) {
        hover_x = cx;
        hover_y = cy;
    }
    if (hand != NULL && hover_valid &&
        (over_segment(disp, cx, cy) || over_button(disp, cx, cy) || over_field(disp, cx, cy)))
        over = 1;
    if (over == shown)
        return;
    shown = over;
    SDL_SetCursor(over ? hand : SDL_GetDefaultCursor());
}

static void apply_symbol(char symbol)
{
    if (focus_row < 0 || focus_row >= row_count)
        return;
    if (symbol < 0x20 || symbol > 0x7e)
        return;
    if (symbol_taken(symbol, focus_row)) {
        note_taken();
        return;
    }
    warn_taken = 0;
    rows[focus_row].symbol = symbol;
}

static SDL_Texture *load_trash(SDL_Renderer *renderer)
{
    char path[1024];
    char *base = SDL_GetBasePath();
    SDL_Surface *surface = NULL;
    SDL_Texture *tex;

    if (base != NULL) {
        snprintf(path, sizeof path, "%simg/trash_can.png", base);
        surface = IMG_Load(path);
        if (surface == NULL) {
            snprintf(path, sizeof path, "%s../sim/img/trash_can.png", base);
            surface = IMG_Load(path);
        }
        SDL_free(base);
    }
    if (surface == NULL)
        surface = IMG_Load("tools/sim/img/trash_can.png");
    if (surface == NULL) {
        SDL_Log("trash IMG_Load failed: %s", IMG_GetError());
        return NULL;
    }
    tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (tex != NULL)
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    return tex;
}

int main(void)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *canvas;
    SDL_Cursor *hand;
    TTF_Font *font;
    SevenSegDisplay disp;
    int running = 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    font = open_ui_font();
    if (font == NULL) {
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    layout_from_font(font);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    window = SDL_CreateWindow("7-segment font", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              CANVAS_W * VIEW_SCALE, CANVAS_H * VIEW_SCALE,
                              SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (renderer == NULL)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    if (renderer == NULL) {
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    if (seven_seg_init(&disp, renderer) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, CANVAS_W,
                               CANVAS_H);
    if (canvas == NULL) {
        seven_seg_quit(&disp);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureScaleMode(canvas, SDL_ScaleModeNearest);
    trash_tex = load_trash(renderer);
    if (trash_tex != NULL)
        SDL_QueryTexture(trash_tex, NULL, NULL, &trash_src_w, &trash_src_h);
    hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    load_compiled();
    if (find_font_file() == 0)
        load_font_file();
    remember_saved();
    SDL_StartTextInput();
    SDL_SetRenderTarget(renderer, canvas);
    draw_rows(renderer, font, &disp);
    present_canvas(renderer, canvas);
    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);
    present_canvas(renderer, canvas);

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_MOUSEWHEEL) {
                scroll_y -= event.wheel.y * SCROLL_STEP;
            } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                bar_drag = 0;
            } else if (event.type == SDL_MOUSEMOTION && bar_drag) {
                int cy;

                if (canvas_y(window, renderer, event.motion.y, &cy))
                    scroll_from_thumb(&disp, cy - bar_grab);
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int cx;
                int cy;

                if (window_to_canvas(window, renderer, event.button.x, event.button.y, &cx, &cy) &&
                    !bar_press(&disp, cx, cy))
                    on_click(&disp, cx, cy);
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    if (focus_row >= 0 && focus_row < row_count) {
                        rows[focus_row].symbol = '\0';
                        warn_taken = 0;
                    }
                }
            } else if (event.type == SDL_TEXTINPUT) {
                if ((SDL_GetModState() & (KMOD_CTRL | KMOD_ALT)) == 0 && event.text.text[0] != '\0' &&
                    event.text.text[1] == '\0')
                    apply_symbol(event.text.text[0]);
            }
        }
        age_warning();
        clamp_scroll(&disp);
        update_cursor(window, renderer, hand, &disp);
        SDL_SetRenderTarget(renderer, canvas);
        draw_rows(renderer, font, &disp);
        present_canvas(renderer, canvas);
        SDL_Delay(16);
    }

    if (unsaved() && save_font_file() != 0)
        SDL_Log("could not save %s", font_file);
    SDL_StopTextInput();
    if (hand != NULL)
        SDL_FreeCursor(hand);
    SDL_DestroyTexture(canvas);
    if (trash_tex != NULL)
        SDL_DestroyTexture(trash_tex);
    seven_seg_quit(&disp);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
