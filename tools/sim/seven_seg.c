#include "seven_seg.h"

#include <SDL_image.h>
#include <stdio.h>
#include <string.h>

static SDL_Texture *load_layer(SDL_Renderer *renderer, const char *name)
{
    char path[1024];
    char *base = SDL_GetBasePath();
    SDL_Surface *surface;
    SDL_Texture *tex;

    if (base != NULL) {
        snprintf(path, sizeof path, "%simg/7_segment_display_layers/%s", base, name);
        SDL_free(base);
        surface = IMG_Load(path);
    } else {
        surface = NULL;
    }
    if (surface == NULL) {
        snprintf(path, sizeof path, "tools/sim/img/7_segment_display_layers/%s", name);
        surface = IMG_Load(path);
    }
    if (surface == NULL) {
        SDL_Log("seven_seg IMG_Load %s failed: %s", name, IMG_GetError());
        return NULL;
    }
    tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (tex != NULL)
        SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    return tex;
}

int seven_seg_init(SevenSegDisplay *disp, SDL_Renderer *renderer)
{
    static const char *seg_files[7] = {
        "a.png", "b.png", "c.png", "d.png", "e.png", "f.png", "g.png",
    };
    int i;

    memset(disp, 0, sizeof *disp);
    disp->base = load_layer(renderer, "all_off.png");
    if (disp->base == NULL)
        return -1;
    for (i = 0; i < 7; i++) {
        disp->seg[i] = load_layer(renderer, seg_files[i]);
        if (disp->seg[i] == NULL)
            return -1;
    }
    disp->dp = load_layer(renderer, "dp.png");
    if (disp->dp == NULL)
        return -1;
    seven_seg_clear(disp);
    return 0;
}

void seven_seg_quit(SevenSegDisplay *disp)
{
    int i;

    if (disp->base != NULL) {
        SDL_DestroyTexture(disp->base);
        disp->base = NULL;
    }
    for (i = 0; i < 7; i++) {
        if (disp->seg[i] != NULL) {
            SDL_DestroyTexture(disp->seg[i]);
            disp->seg[i] = NULL;
        }
    }
    if (disp->dp != NULL) {
        SDL_DestroyTexture(disp->dp);
        disp->dp = NULL;
    }
}

void seven_seg_clear(SevenSegDisplay *disp)
{
    memset(disp->mask, 0, sizeof disp->mask);
}

void seven_seg_set_mask(SevenSegDisplay *disp, int digit, uint8_t mask)
{
    if (digit < 0 || digit >= SEVEN_SEG_DIGITS)
        return;
    disp->mask[digit] = mask;
}

uint8_t seven_seg_get_mask(const SevenSegDisplay *disp, int digit)
{
    if (digit < 0 || digit >= SEVEN_SEG_DIGITS)
        return 0;
    return disp->mask[digit];
}

uint8_t seven_seg_encode_char(char c)
{
    switch (c) {
    case '0':
        return SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_F;
    case '1':
        return SEVEN_SEG_B | SEVEN_SEG_C;
    case '2':
        return SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_G;
    case '3':
        return SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_G;
    case '4':
        return SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_F | SEVEN_SEG_G;
    case '5':
        return SEVEN_SEG_A | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_F | SEVEN_SEG_G;
    case '6':
        return SEVEN_SEG_A | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_F | SEVEN_SEG_G;
    case '7':
        return SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C;
    case '8':
        return SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_F |
               SEVEN_SEG_G;
    case '9':
        return SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_F | SEVEN_SEG_G;
    case '-':
    case '_':
        return SEVEN_SEG_G;
    case ' ':
        return 0;
    default:
        if (c >= 'a' && c <= 'f') {
            static const uint8_t hex[6] = {
                SEVEN_SEG_A | SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_E | SEVEN_SEG_F | SEVEN_SEG_G,
                SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_F | SEVEN_SEG_G,
                SEVEN_SEG_A | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_F | SEVEN_SEG_G,
                SEVEN_SEG_B | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_E | SEVEN_SEG_G,
                SEVEN_SEG_A | SEVEN_SEG_C | SEVEN_SEG_D | SEVEN_SEG_F | SEVEN_SEG_G,
                SEVEN_SEG_A | SEVEN_SEG_C | SEVEN_SEG_E | SEVEN_SEG_F | SEVEN_SEG_G,
            };
            return hex[c - 'a'];
        }
        if (c >= 'A' && c <= 'F')
            return seven_seg_encode_char((char)(c - 'A' + 'a'));
        return 0;
    }
}

void seven_seg_put_char(SevenSegDisplay *disp, int digit, char c)
{
    seven_seg_set_mask(disp, digit, seven_seg_encode_char(c));
}

int seven_seg_print(SevenSegDisplay *disp, const char *text)
{
    int slot = 0;
    int last = -1;
    int n = 0;

    seven_seg_clear(disp);
    if (text == NULL)
        return 0;

    for (; *text != '\0' && slot < SEVEN_SEG_DIGITS; text++) {
        char c = *text;

        if (c == '.') {
            if (last >= 0)
                disp->mask[last] |= SEVEN_SEG_DP;
            continue;
        }
        disp->mask[slot] = seven_seg_encode_char(c);
        last = slot;
        slot++;
        n++;
    }
    return n;
}

static void blit_layer(SDL_Renderer *renderer, SDL_Texture *tex, int x, int y, int w, int h,
                       const SDL_Rect *clip)
{
    SDL_Rect src = {0, 0, w, h};
    SDL_Rect dst = {x, y, w, h};

    if (clip != NULL) {
        if (dst.x < clip->x) {
            int cut = clip->x - dst.x;
            src.x += cut;
            src.w -= cut;
            dst.x = clip->x;
            dst.w -= cut;
        }
        if (dst.y < clip->y) {
            int cut = clip->y - dst.y;
            src.y += cut;
            src.h -= cut;
            dst.y = clip->y;
            dst.h -= cut;
        }
        if (dst.x + dst.w > clip->x + clip->w) {
            dst.w = clip->x + clip->w - dst.x;
            src.w = dst.w;
        }
        if (dst.y + dst.h > clip->y + clip->h) {
            dst.h = clip->y + clip->h - dst.y;
            src.h = dst.h;
        }
        if (dst.w <= 0 || dst.h <= 0)
            return;
    }
    SDL_RenderCopy(renderer, tex, &src, &dst);
}

static void draw_digit(const SevenSegDisplay *disp, SDL_Renderer *renderer, int x, int y,
                       uint8_t mask, const SDL_Rect *clip)
{
    int bit;
    int w = SEVEN_SEG_DIGIT_W;
    int h = SEVEN_SEG_DIGIT_H;

    blit_layer(renderer, disp->base, x, y, w, h, clip);
    for (bit = 0; bit < 7; bit++) {
        if ((mask & (1u << bit)) != 0)
            blit_layer(renderer, disp->seg[bit], x, y, w, h, clip);
    }
    if ((mask & SEVEN_SEG_DP) != 0)
        blit_layer(renderer, disp->dp, x, y, w, h, clip);
}

void seven_seg_draw(const SevenSegDisplay *disp, SDL_Renderer *renderer, int x, int y,
                    int canvas_w, int canvas_h)
{
    int i;
    SDL_Rect clip = {0, 0, canvas_w, canvas_h};

    for (i = 0; i < SEVEN_SEG_DIGITS; i++) {
        draw_digit(disp, renderer, x + i * SEVEN_SEG_DIGIT_PITCH, y, disp->mask[i], &clip);
    }
}
