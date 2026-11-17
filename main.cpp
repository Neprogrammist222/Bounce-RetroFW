#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

const int SCREEN_W = 320;
const int SCREEN_H = 240;

const Uint32 COL_SKY   = 0xB0E0F0;
const Uint32 COL_WATER = 0x1060B0;
const Uint32 COL_HUD   = 0x0853AA;
const Uint32 COL_WHITE = 0xFFFFFE;
const Uint32 COL_ORANGE= 0xFF9813;

// --- ЗВУКОВОЙ СИНТЕЗАТОР ---
struct SoundTone {
    float freq;
    int samplesLeft;
};

static std::vector<SoundTone> soundQueue;
static float soundPhase = 0.0f;

void AudioCallback(void* userdata, Uint8* stream, int len) {
    Sint16* out = (Sint16*)stream;
    int count = len / 2;
    for (int i = 0; i < count; i++) {
        if (soundQueue.empty()) {
            out[i] = 0;
        } else {
            SoundTone& t = soundQueue.front();
            if (t.freq > 0.0f) {
                soundPhase += t.freq / 22050.0f;
                if (soundPhase >= 1.0f) soundPhase -= 1.0f;
                out[i] = (soundPhase < 0.5f) ? 6000 : -6000;
            } else {
                out[i] = 0;
            }
            t.samplesLeft--;
            if (t.samplesLeft <= 0) soundQueue.erase(soundQueue.begin());
        }
    }
}

void PlayBeep(float freq, int ms) {
    SDL_LockAudio();
    soundQueue.push_back({freq, (22050 * ms) / 1000});
    SDL_UnlockAudio();
}

void PlaySoundHoop() {
    SDL_LockAudio();
    soundQueue.clear();
    soundQueue.push_back({880.0f, (22050 * 35) / 1000});
    soundQueue.push_back({1108.7f, (22050 * 35) / 1000});
    soundQueue.push_back({1318.5f, (22050 * 65) / 1000});
    SDL_UnlockAudio();
}

void PlaySoundCrystal() {
    SDL_LockAudio();
    soundQueue.clear();
    soundQueue.push_back({1046.5f, (22050 * 40) / 1000});
    soundQueue.push_back({1568.0f, (22050 * 80) / 1000});
    SDL_UnlockAudio();
}

void PlaySoundPop() {
    SDL_LockAudio();
    soundQueue.clear();
    soundQueue.push_back({392.0f, (22050 * 45) / 1000});
    soundQueue.push_back({293.7f, (22050 * 55) / 1000});
    soundQueue.push_back({220.0f, (22050 * 85) / 1000});
    SDL_UnlockAudio();
}

void PlaySoundWin() {
    SDL_LockAudio();
    soundQueue.clear();
    soundQueue.push_back({523.3f, (22050 * 70) / 1000});
    soundQueue.push_back({659.3f, (22050 * 70) / 1000});
    soundQueue.push_back({784.0f, (22050 * 70) / 1000});
    soundQueue.push_back({1046.5f, (22050 * 140) / 1000});
    SDL_UnlockAudio();
}

// Матрицы столкновений через строки
static const std::vector<std::string> SLOPE_MASK = {
    "000000000001", "000000000011", "000000000111", "000000001111",
    "000000011111", "000000111111", "000001111111", "000011111111",
    "000111111111", "001111111111", "011111111111", "111111111111"
};

static const std::vector<std::string> BALL_MASK = {
    "000011110000", "001111111100", "011111111110", "011111111110",
    "111111111111", "111111111111", "111111111111", "111111111111",
    "011111111110", "011111111110", "001111111100", "000011110000"
};

static const std::vector<std::string> BIG_BALL_MASK = {
    "0000011111100000", "0001111111111000", "0011111111111100", "0111111111111110",
    "0111111111111110", "1111111111111111", "1111111111111111", "1111111111111111",
    "1111111111111111", "1111111111111111", "1111111111111111", "0111111111111110",
    "0111111111111110", "0011111111111100", "0001111111111000", "0000011111100000"
};

static const std::vector<int> HOOP_LAYER_BACK  = {35,36,17,19,43,44,25,27,31,32,13,15,39,40,21,23};
static const std::vector<int> HOOP_LAYER_FRONT = {33,34,18,20,41,42,26,28,29,30,14,16,37,38,22,24};

static const std::vector<uint8_t> FONT_DATA = {
    0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00,
    0x14,0x7F,0x14,0x7F,0x14, 0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62,
    0x36,0x49,0x55,0x22,0x50, 0x00,0x05,0x03,0x00,0x00, 0x00,0x1C,0x22,0x41,0x00,
    0x00,0x41,0x22,0x1C,0x00, 0x14,0x08,0x3E,0x08,0x14, 0x08,0x08,0x3E,0x08,0x08,
    0x00,0x50,0x30,0x00,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x60,0x60,0x00,0x00,
    0x20,0x10,0x08,0x04,0x02, 0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00,
    0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4B,0x31, 0x18,0x14,0x12,0x7F,0x10,
    0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03,
    0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E, 0x00,0x36,0x36,0x00,0x00,
    0x00,0x56,0x36,0x00,0x00, 0x08,0x14,0x22,0x41,0x00, 0x14,0x14,0x14,0x14,0x14,
    0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x51,0x09,0x06, 0x32,0x49,0x79,0x41,0x3E,
    0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
    0x7F,0x41,0x41,0x22,0x1C, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01,
    0x3E,0x41,0x49,0x49,0x7A, 0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00,
    0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41, 0x7F,0x40,0x40,0x40,0x40,
    0x7F,0x02,0x0C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
    0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46,
    0x46,0x49,0x49,0x49,0x31, 0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F,
    0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F, 0x63,0x14,0x08,0x14,0x63,
    0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43
};

void DrawChar(SDL_Surface* dst, int x, int y, char c, Uint32 color, int scale = 1) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 32 || c > 'Z') return;
    int base = (c - 32) * 5;
    for (int col = 0; col < 5; col++) {
        uint8_t bits = FONT_DATA.at(base + col);
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                SDL_Rect r = {(Sint16)(x + col * scale), (Sint16)(y + row * scale), (Uint16)scale, (Uint16)scale};
                SDL_FillRect(dst, &r, color);
            }
        }
    }
}

void DrawText(SDL_Surface* dst, int x, int y, const std::string& str, Uint32 color, int scale = 1) {
    for (size_t i = 0; i < str.size(); i++) {
        DrawChar(dst, x + (int)i * 6 * scale, y, str.at(i), color, scale);
    }
}

void DrawTextCentered(SDL_Surface* dst, int y, const std::string& str, Uint32 color, int scale = 1) {
    int w = (int)str.size() * 6 * scale;
    DrawText(dst, (dst->w - w) / 2, y, str, color, scale);
}

inline Uint32 GetPixelFast(SDL_Surface* s, int x, int y) {
    return *(Uint32*)((Uint8*)s->pixels + y * s->pitch + x * 4);
}

inline void PutPixelFast(SDL_Surface* s, int x, int y, Uint32 pixel) {
    *(Uint32*)((Uint8*)s->pixels + y * s->pitch + x * 4) = pixel;
}

Uint32 GetPixel(SDL_Surface* s, int x, int y) {
    int bpp = s->format->BytesPerPixel;
    Uint8* p = (Uint8*)s->pixels + y * s->pitch + x * bpp;
    switch (bpp) {
        case 1: return *p;
        case 2: return *(Uint16*)p;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) return (*(p) << 16) | (*(p + 1) << 8) | *(p + 2);
            else return *(p) | (*(p + 1) << 8) | (*(p + 2) << 16);
        case 4: return *(Uint32*)p;
        default: return 0;
    }
}

void PutPixel(SDL_Surface* s, int x, int y, Uint32 pixel) {
    int bpp = s->format->BytesPerPixel;
    Uint8* p = (Uint8*)s->pixels + y * s->pitch + x * bpp;
    switch (bpp) {
        case 1: *p = pixel; break;
        case 2: *(Uint16*)p = pixel; break;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                *(p) = (pixel >> 16) & 0xff; *(p + 1) = (pixel >> 8) & 0xff; *(p + 2) = pixel & 0xff;
            } else {
                *(p) = pixel & 0xff; *(p + 1) = (pixel >> 8) & 0xff; *(p + 2) = (pixel >> 16) & 0xff;
            }
            break;
        case 4: *(Uint32*)p = pixel; break;
    }
}

inline Uint32 Blend50(Uint32 c1, Uint32 c2) {
    return (((c1 ^ c2) & 0xFEFEFE) >> 1) + (c1 & c2);
}

inline Uint32 Blend25(Uint32 c75, Uint32 c25) {
    return Blend50(c75, Blend50(c75, c25));
}

// Фильтр Scale2x (EPX) для чёткого сглаживания пиксельной графики
void BlitScale2x(SDL_Surface* src, SDL_Surface* dst, int offX, int offY) {
    int w = src->w, h = src->h;
    SDL_LockSurface(src);
    SDL_LockSurface(dst);
    for (int y = 0; y < h; y++) {
        int ym = (y > 0) ? y - 1 : 0;
        int yp = (y + 1 < h) ? y + 1 : h - 1;
        for (int x = 0; x < w; x++) {
            int xm = (x > 0) ? x - 1 : 0;
            int xp = (x + 1 < w) ? x + 1 : w - 1;
            Uint32 B = GetPixelFast(src, x, ym);
            Uint32 D = GetPixelFast(src, xm, y);
            Uint32 E = GetPixelFast(src, x, y);
            Uint32 F = GetPixelFast(src, xp, y);
            Uint32 H = GetPixelFast(src, x, yp);
            Uint32 E0 = E, E1 = E, E2 = E, E3 = E;
            if (B != H && D != F) {
                if (D == B) E0 = D;
                if (B == F) E1 = F;
                if (D == H) E2 = D;
                if (H == F) E3 = F;
            }
            PutPixelFast(dst, offX + x * 2,     offY + y * 2,     E0);
            PutPixelFast(dst, offX + x * 2 + 1, offY + y * 2,     E1);
            PutPixelFast(dst, offX + x * 2,     offY + y * 2 + 1, E2);
            PutPixelFast(dst, offX + x * 2 + 1, offY + y * 2 + 1, E3);
        }
    }
    SDL_UnlockSurface(dst);
    SDL_UnlockSurface(src);
}

// Быстрый билинейный фильтр 2x
void BlitBilinear2x(SDL_Surface* src, SDL_Surface* dst, int offX, int offY) {
    int w = src->w, h = src->h;
    SDL_LockSurface(src);
    SDL_LockSurface(dst);
    for (int y = 0; y < h; y++) {
        int yp = (y + 1 < h) ? y + 1 : h - 1;
        for (int x = 0; x < w; x++) {
            int xp = (x + 1 < w) ? x + 1 : w - 1;
            Uint32 c00 = GetPixelFast(src, x, y);
            Uint32 c10 = GetPixelFast(src, xp, y);
            Uint32 c01 = GetPixelFast(src, x, yp);
            Uint32 c11 = GetPixelFast(src, xp, yp);
            Uint32 right = Blend50(c00, c10);
            Uint32 down  = Blend50(c00, c01);
            Uint32 diag  = Blend50(right, Blend50(c01, c11));
PutPixelFast(dst, offX + x * 2,     offY + y * 2,     c00);
PutPixelFast(dst, offX + x * 2 + 1, offY + y * 2,     Blend25(c00, right));
PutPixelFast(dst, offX + x * 2,     offY + y * 2 + 1, Blend25(c00, down));
PutPixelFast(dst, offX + x * 2 + 1, offY + y * 2 + 1, Blend25(c00, diag));
}
}
SDL_UnlockSurface(dst);
SDL_UnlockSurface(src);
}
// Лёгкое сглаживание контуров для полноэкранного режима 320x240
void ApplySoftFilter320(SDL_Surface* buf, int maxH) {
SDL_LockSurface(buf);
for (int y = 1; y < maxH - 1; y++) {
for (int x = 1; x < SCREEN_W - 1; x++) {
Uint32 c = GetPixelFast(buf, x, y);
Uint32 r = GetPixelFast(buf, x + 1, y);
Uint32 d = GetPixelFast(buf, x, y + 1);
if (c != r || c != d) {
Uint32 avg = Blend50(r, d);
PutPixelFast(buf, x, y, Blend25(c, avg));
}
}
}
SDL_UnlockSurface(buf);
}
SDL_Surface* CreateRGBA(int w, int h) {
SDL_Surface* s = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
Uint32 key = SDL_MapRGB(s->format, 255, 0, 255);
SDL_FillRect(s, NULL, key);
SDL_SetColorKey(s, SDL_SRCCOLORKEY, key);
return s;
}
enum Manip {
FLIP_H = 0, FLIP_V = 1, FLIP_BOTH = 2,
ROT_90 = 3, ROT_180 = 4, ROT_270 = 5
};
SDL_Surface* ModifyImage(SDL_Surface* src, int manip) {
int w = src->w, h = src->h;
SDL_Surface* dst = CreateRGBA(w, h);
SDL_LockSurface(src);
SDL_LockSurface(dst);
for (int y = 0; y < h; y++) {
for (int x = 0; x < w; x++) {
Uint32 c = GetPixel(src, x, y);
int dx = x, dy = y;
switch (manip) {
case FLIP_H:    dx = w - 1 - x; dy = y; break;
case FLIP_V:    dx = x; dy = h - 1 - y; break;
case FLIP_BOTH: dx = w - 1 - x; dy = h - 1 - y; break;
case ROT_90:    dx = y; dy = w - 1 - x; break;
case ROT_180:   dx = w - 1 - x; dy = h - 1 - y; break;
case ROT_270:   dx = h - 1 - y; dy = x; break;
}
PutPixel(dst, dx, dy, c);
}
}
SDL_UnlockSurface(dst);
SDL_UnlockSurface(src);
return dst;
}
SDL_Surface* CutTile(SDL_Surface* sheet, int tx, int ty, Uint32 bgColor = 0, bool hasBg = false) {
SDL_Surface* dst = CreateRGBA(12, 12);
Uint32 key = SDL_MapRGB(dst->format, 255, 0, 255);
if (hasBg) {
SDL_FillRect(dst, NULL, SDL_MapRGB(dst->format, (bgColor >> 16) & 0xFF, (bgColor >> 8) & 0xFF, bgColor & 0xFF));
} else {
SDL_FillRect(dst, NULL, key);
}
SDL_LockSurface(sheet);
SDL_LockSurface(dst);
for (int y = 0; y < 12; y++) {
for (int x = 0; x < 12; x++) {
Uint32 c = GetPixel(sheet, tx * 12 + x, ty * 12 + y);
Uint8 r, g, b, a;
SDL_GetRGBA(c, sheet->format, &r, &g, &b, &a);
bool isWhiteBg = (r == 255 && g == 255 && b == 255);
if (a >= 128 && !isWhiteBg) {
if (r == 255 && g == 0 && b == 255) r = 254;
PutPixel(dst, x, y, SDL_MapRGB(dst->format, r, g, b));
}
}
}
SDL_UnlockSurface(dst);
SDL_UnlockSurface(sheet);
return dst;
}
SDL_Surface* Mirror16x16Tile(SDL_Surface* src) {
SDL_Surface* dst = CreateRGBA(16, 16);
SDL_Surface* fh = ModifyImage(src, FLIP_H);
SDL_Surface* fv = ModifyImage(src, FLIP_V);
SDL_Surface* r180 = ModifyImage(src, ROT_180);
auto blitRaw = [](SDL_Surface* s, SDL_Surface* d, int ox, int oy) {
SDL_LockSurface(s); SDL_LockSurface(d);
for (int y = 0; y < s->h; y++) {
for (int x = 0; x < s->w; x++) {
int dx = ox + x, dy = oy + y;
if (dx >= 0 && dx < d->w && dy >= 0 && dy < d->h) {
Uint32 c = GetPixel(s, x, y);
Uint8 r, g, b, a;
SDL_GetRGBA(c, s->format, &r, &g, &b, &a);
if (a >= 128 && !(r == 255 && g == 255 && b == 255) && !(r == 255 && g == 0 && b == 255)) {
PutPixel(d, dx, dy, SDL_MapRGB(d->format, r, g, b));
}
}
}
}
SDL_UnlockSurface(d); SDL_UnlockSurface(s);
};
blitRaw(src, dst, -4, -4);
blitRaw(fh,  dst,  8, -4);
blitRaw(fv,  dst, -4,  8);
blitRaw(r180,dst,  8,  8);
SDL_FreeSurface(fh); SDL_FreeSurface(fv); SDL_FreeSurface(r180);
return dst;
}
SDL_Surface* MirrorExitTile(SDL_Surface* src) {
SDL_Surface* dst = CreateRGBA(24, 48);
auto fill = [&](int x, int y, int w, int h, Uint32 col) {
SDL_Rect r = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
SDL_FillRect(dst, &r, SDL_MapRGB(dst->format, (col>>16)&0xFF, (col>>8)&0xFF, col&0xFF));
};
fill(0, 0, 24, 48, COL_SKY);
fill(4, 0, 16, 48, 0xFC9D9E);
fill(6, 0, 10, 48, 0xE33A3F);
fill(10, 0, 4, 48, 0xC2848E);
SDL_Surface* fh = ModifyImage(src, FLIP_H);
SDL_Surface* fv = ModifyImage(src, FLIP_V);
SDL_Surface* fb = ModifyImage(src, FLIP_BOTH);
SDL_Rect r0 = {0, 0, 12, 12}, r1 = {12, 0, 12, 12}, r2 = {0, 12, 12, 12}, r3 = {12, 12, 12, 12};
SDL_BlitSurface(src, NULL, dst, &r0);
SDL_BlitSurface(fh,  NULL, dst, &r1);
SDL_BlitSurface(fv,  NULL, dst, &r2);
SDL_BlitSurface(fb,  NULL, dst, &r3);
SDL_FreeSurface(fh); SDL_FreeSurface(fv); SDL_FreeSurface(fb);
return dst;
}
struct DynThorn {
int16_t bx, by;
int16_t tx, ty;
int16_t vx, vy;
int16_t px, py;
int16_t prevPx, prevPy;
};
enum ScreenMode {
MODE_FULL_320x240 = 0,
MODE_SCALE_2X     = 1,
MODE_ORIG_1X      = 2
};
enum FpsMode {
FPS_ORIG_20   = 0,
FPS_SMOOTH_50 = 1,
FPS_SMOOTH_60 = 2
};
enum FilterMode {
FILTER_NONE     = 0,
FILTER_SCALE2X  = 1,
FILTER_BILINEAR = 2
};
enum GameState {
STATE_SPLASH,
STATE_MENU,
STATE_SETTINGS,
STATE_LEVEL_SELECT,
STATE_PLAYING,
STATE_LEVEL_COMPLETE,
STATE_GAME_OVER,
STATE_HIGH_SCORE
};
class BouncePort {
public:
    SDL_Surface* screen = nullptr;
    SDL_Surface* backBuffer = nullptr;
    SDL_Surface* splashImg = nullptr;
    std::vector<SDL_Surface*> sprites;
    SDL_Surface* dynThornSprite = nullptr;
    SDL_Surface* hudBall = nullptr;
    SDL_Surface* hudHoop = nullptr;
    SDL_Surface* exitDoorClip = nullptr;

    GameState state = STATE_SPLASH;
    ScreenMode screenMode = MODE_FULL_320x240;
    FpsMode fpsMode = FPS_SMOOTH_50;
    FilterMode filterMode = FILTER_SCALE2X;

    int splashTimer = 0;
    int maxLevels = 1;
    int highScore = 0;
    int menuCursor = 0;
    int settingsCursor = 0;
    int levelCursor = 0;
    bool hasActiveGame = false;
    bool newHighScore = false;
    bool wonGame = false;

    int level = 1;
    int score = 0;
    int lives = 3;
    int hoopsScored = 0;
    int hoopsTotal = 0;
    int levelW = 0, levelH = 0;
    std::vector<std::vector<uint16_t>> tiles;
    std::vector<DynThorn> dynThorns;

    int initBallX = 0, initBallY = 0, initBallSize = 12;
    int exitX = 0, exitY = 0;
    int exitAnimY = 0;
    bool exitUnlocked = false;
    bool exitOpened = false;
    bool levelFinished = false;
    int levelBannerTimer = 0;

    int ballX = 0, ballY = 0;
    int prevBallX = 0, prevBallY = 0;
    int velX = 0, velY = 0;
    int ballSize = 12, ballRadius = 6;
    int checkpointX = 0, checkpointY = 0, checkpointSize = 12;
    int inputMask = 0;
    int bounceBoost = 0;
    int speedTimer = 0;
    int gravTimer = 0;
    int jumpTimer = 0;
    bool onGround = false;
    bool onRubber = false;
    bool hitSlopeOrWall = false;
    int slideCounter = 0;
    int ballState = 0;
    int popTimer = 0;

    int camX = 0, camY = 0;
    bool godMode = false;

    BouncePort() {
        sprites.assign(70, nullptr);
        LoadSave();
    }

    bool InitSDL() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
        screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, SDL_SWSURFACE);
        if (!screen) return false;
        backBuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_W, SCREEN_H, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
        if (!backBuffer) return false;
        SDL_ShowCursor(SDL_DISABLE);
        SDL_WM_SetCaption("Bounce RS-97", nullptr);

        SDL_AudioSpec desired;
        memset(&desired, 0, sizeof(desired));
        desired.freq = 22050;
        desired.format = AUDIO_S16SYS;
        desired.channels = 1;
        desired.samples = 512;
        desired.callback = AudioCallback;
        if (SDL_OpenAudio(&desired, NULL) == 0) {
            SDL_PauseAudio(0);
        }

        splashImg = IMG_Load("icons/bouncesplash.png");
        LoadSprites();
        return true;
    }

    void LoadSave() {
        FILE* f = fopen("bounce.sav", "rb");
        if (f) {
            fread(&maxLevels, sizeof(int), 1, f);
            fread(&highScore, sizeof(int), 1, f);
            int sMode = 0, fMode = 1, fltMode = 1;
            if (fread(&sMode, sizeof(int), 1, f) == 1) screenMode = (ScreenMode)std::clamp(sMode, 0, 2);
            if (fread(&fMode, sizeof(int), 1, f) == 1) fpsMode = (FpsMode)std::clamp(fMode, 0, 2);
            if (fread(&fltMode, sizeof(int), 1, f) == 1) filterMode = (FilterMode)std::clamp(fltMode, 0, 2);
            fclose(f);
        }
        if (maxLevels < 1) maxLevels = 1;
        if (maxLevels > 11) maxLevels = 11;
    }

    void WriteSave() {
        if (level > maxLevels) maxLevels = std::min(level, 11);
        if (score > highScore) {
            highScore = score;
            newHighScore = true;
        }
        FILE* f = fopen("bounce.sav", "wb");
        if (f) {
            fwrite(&maxLevels, sizeof(int), 1, f);
            fwrite(&highScore, sizeof(int), 1, f);
            int sMode = (int)screenMode, fMode = (int)fpsMode, fltMode = (int)filterMode;
            fwrite(&sMode, sizeof(int), 1, f);
            fwrite(&fMode, sizeof(int), 1, f);
            fwrite(&fltMode, sizeof(int), 1, f);
            fclose(f);
        }
    }

    void LoadSprites() {
        SDL_Surface* ss = IMG_Load("icons/objects_nm.png");
        if (!ss) {
            fprintf(stderr, "Failed to load icons/objects_nm.png\n");
            exit(1);
        }

        sprites.at(0)  = CutTile(ss, 1, 0);
        sprites.at(1)  = CutTile(ss, 1, 2);
        sprites.at(2)  = CutTile(ss, 0, 3, COL_SKY, true);
        sprites.at(3)  = ModifyImage(sprites.at(2), FLIP_V);
        sprites.at(4)  = ModifyImage(sprites.at(2), ROT_90);
        sprites.at(5)  = ModifyImage(sprites.at(2), ROT_270);
        sprites.at(6)  = CutTile(ss, 0, 3, COL_WATER, true);
        sprites.at(7)  = ModifyImage(sprites.at(6), FLIP_V);
        sprites.at(8)  = ModifyImage(sprites.at(6), ROT_90);
        sprites.at(9)  = ModifyImage(sprites.at(6), ROT_270);
        sprites.at(10) = CutTile(ss, 0, 4);
        sprites.at(11) = CutTile(ss, 3, 4);
        SDL_Surface* exitRaw = CutTile(ss, 2, 3);
        sprites.at(12) = MirrorExitTile(exitRaw);
        SDL_FreeSurface(exitRaw);

        sprites.at(13) = CutTile(ss, 0, 5);
        sprites.at(14) = ModifyImage(sprites.at(13), FLIP_V);
        sprites.at(15) = ModifyImage(sprites.at(13), FLIP_H);
        sprites.at(16) = ModifyImage(sprites.at(14), FLIP_H);

        sprites.at(17) = CutTile(ss, 1, 5);
        sprites.at(18) = ModifyImage(sprites.at(17), FLIP_V);
        sprites.at(19) = ModifyImage(sprites.at(17), FLIP_H);
        sprites.at(20) = ModifyImage(sprites.at(18), FLIP_H);

        sprites.at(21) = CutTile(ss, 2, 5);
        sprites.at(22) = ModifyImage(sprites.at(21), FLIP_V);
        sprites.at(23) = ModifyImage(sprites.at(21), FLIP_H);
        sprites.at(24) = ModifyImage(sprites.at(22), FLIP_H);

        sprites.at(25) = CutTile(ss, 3, 5);
        sprites.at(26) = ModifyImage(sprites.at(25), FLIP_V);
        sprites.at(27) = ModifyImage(sprites.at(25), FLIP_H);
        sprites.at(28) = ModifyImage(sprites.at(26), FLIP_H);

        sprites.at(29) = ModifyImage(sprites.at(13), ROT_270);
        sprites.at(30) = ModifyImage(sprites.at(29), FLIP_V);
        sprites.at(31) = ModifyImage(sprites.at(29), FLIP_H);
        sprites.at(32) = ModifyImage(sprites.at(30), FLIP_H);

        sprites.at(33) = ModifyImage(sprites.at(17), ROT_270);
        sprites.at(34) = ModifyImage(sprites.at(33), FLIP_V);
        sprites.at(35) = ModifyImage(sprites.at(33), FLIP_H);
        sprites.at(36) = ModifyImage(sprites.at(34), FLIP_H);

        sprites.at(37) = ModifyImage(sprites.at(21), ROT_270);
        sprites.at(38) = ModifyImage(sprites.at(37), FLIP_V);
        sprites.at(39) = ModifyImage(sprites.at(37), FLIP_H);
        sprites.at(40) = ModifyImage(sprites.at(38), FLIP_H);

        sprites.at(41) = ModifyImage(sprites.at(25), ROT_270);
        sprites.at(42) = ModifyImage(sprites.at(41), FLIP_V);
        sprites.at(43) = ModifyImage(sprites.at(41), FLIP_H);
        sprites.at(44) = ModifyImage(sprites.at(42), FLIP_H);

        sprites.at(45) = CutTile(ss, 3, 3);
        sprites.at(46) = CutTile(ss, 1, 3);
        sprites.at(47) = CutTile(ss, 2, 0);
        sprites.at(48) = CutTile(ss, 0, 1);
        SDL_Surface* bigRaw = CutTile(ss, 3, 0);
        sprites.at(49) = Mirror16x16Tile(bigRaw);
        SDL_FreeSurface(bigRaw);

        sprites.at(50) = CutTile(ss, 3, 1);
        sprites.at(51) = CutTile(ss, 2, 4);
        sprites.at(52) = CutTile(ss, 3, 2);
        sprites.at(53) = CutTile(ss, 1, 1);
        sprites.at(54) = CutTile(ss, 2, 2);

        sprites.at(55) = CutTile(ss, 0, 0, COL_SKY, true);
        sprites.at(56) = ModifyImage(sprites.at(55), ROT_90);
        sprites.at(57) = ModifyImage(sprites.at(55), ROT_180);
        sprites.at(58) = ModifyImage(sprites.at(55), ROT_270);

        sprites.at(59) = CutTile(ss, 0, 0, COL_WATER, true);
        sprites.at(60) = ModifyImage(sprites.at(59), ROT_90);
        sprites.at(61) = ModifyImage(sprites.at(59), ROT_180);
        sprites.at(62) = ModifyImage(sprites.at(59), ROT_270);

        sprites.at(63) = CutTile(ss, 0, 2);
        sprites.at(64) = ModifyImage(sprites.at(63), ROT_90);
        sprites.at(65) = ModifyImage(sprites.at(63), ROT_180);
        sprites.at(66) = ModifyImage(sprites.at(63), ROT_270);

        hudBall = CutTile(ss, 2, 1);
        hudHoop = CutTile(ss, 1, 4);

        auto CopyWithAlpha = [](SDL_Surface* src, int sx, int sy, SDL_Surface* dst, int dx, int dy, int w, int h) {
            SDL_LockSurface(src);
            SDL_LockSurface(dst);
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    if (sx + x < src->w && sy + y < src->h && dx + x < dst->w && dy + y < dst->h) {
                        PutPixel(dst, dx + x, dy + y, GetPixel(src, sx + x, sy + y));
                    }
                }
            }
            SDL_UnlockSurface(dst);
            SDL_UnlockSurface(src);
        };

        dynThornSprite = CreateRGBA(24, 24);
        SDL_Surface* q0 = sprites.at(46);
        SDL_Surface* q1 = ModifyImage(q0, FLIP_H);
        SDL_Surface* q2 = ModifyImage(q0, ROT_180);
        SDL_Surface* q3 = ModifyImage(q0, FLIP_V);
        CopyWithAlpha(q0, 0, 0, dynThornSprite,  0,  0, 12, 12);
        CopyWithAlpha(q1, 0, 0, dynThornSprite, 12,  0, 12, 12);
        CopyWithAlpha(q2, 0, 0, dynThornSprite, 12, 12, 12, 12);
        CopyWithAlpha(q3, 0, 0, dynThornSprite,  0, 12, 12, 12);
        SDL_FreeSurface(q1); SDL_FreeSurface(q2); SDL_FreeSurface(q3);

        exitDoorClip = CreateRGBA(24, 24);
        SDL_FreeSurface(ss);
    }

    void UpdateExitSprite() {
        SDL_Rect src = {0, (Sint16)exitAnimY, 24, 24};
        SDL_Rect dst = {0, 0, 24, 24};
        SDL_BlitSurface(sprites.at(12), &src, exitDoorClip, &dst);
    }

    bool LoadLevel(int lvlId) {
        std::string numStr = std::to_string(lvlId);
        while (numStr.size() < 3) numStr = "0" + numStr;
        std::string path = "levels/J2MElvl." + numStr;
        FILE* f = fopen(path.c_str(), "rb");
if (!f) return false;
auto readU8 = [&]() -> int { int c = fgetc(f); return (c == EOF) ? 0 : c; };
auto readS8 = [&]() -> int16_t { return (int16_t)(int8_t)readU8(); };
initBallX = readU8();
initBallY = readU8();
int isBig = readU8();
initBallSize = (isBig == 0) ? 12 : 16;
exitX = readU8();
exitY = readU8();
hoopsTotal = readU8();
levelW = readU8();
levelH = readU8();
tiles.assign(levelH, std::vector<uint16_t>(levelW, 0));
for (int y = 0; y < levelH; y++) {
for (int x = 0; x < levelW; x++) {
tiles.at(y).at(x) = (uint16_t)readU8();
}
}
int dCount = readU8();
dynThorns.clear();
for (int i = 0; i < dCount; i++) {
DynThorn dt;
dt.bx = (int16_t)readU8();
dt.by = (int16_t)readU8();
dt.tx = (int16_t)readU8();
dt.ty = (int16_t)readU8();
dt.vx = readS8();
dt.vy = readS8();
dt.px = (int16_t)readU8();
dt.py = (int16_t)readU8();
dt.prevPx = dt.px;
dt.prevPy = dt.py;
dynThorns.push_back(dt);
}
fclose(f);
exitAnimY = 0;
exitUnlocked = false;
exitOpened = false;
levelFinished = false;
hoopsScored = 0;
levelBannerTimer = 100;
UpdateExitSprite();
SpawnBall(initBallX * 12 + 6, initBallY * 12 + 6, initBallSize);
checkpointX = initBallX;
checkpointY = initBallY;
checkpointSize = initBallSize;
return true;
}
void SpawnBall(int x, int y, int sz) {
ballX = x;
ballY = y;
prevBallX = x;
prevBallY = y;
velX = 0;
velY = 0;
ballSize = sz;
ballRadius = sz / 2;
onGround = false;
onRubber = false;
hitSlopeOrWall = false;
ballState = 0;
popTimer = 0;
speedTimer = 0;
gravTimer = 0;
jumpTimer = 0;
bounceBoost = 0;
slideCounter = 0;
inputMask = 0;
if (sz == 16) UseBigBall();
prevBallX = ballX;
prevBallY = ballY;
}
static bool RectOverlap(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
return (ax1 <= bx2 && ay1 <= by2 && bx1 <= ax2 && by1 <= ay2);
}
bool CheckWallCollision(int px, int py, int ty, int tx) {
int left = tx * 12, top = ty * 12;
int dx = px - ballRadius - left;
int dy = py - ballRadius - top;
int x0 = (dx >= 0) ? dx : 0;
int x1 = (dx >= 0) ? 12 : ballSize + dx;
int y0 = (dy >= 0) ? dy : 0;
int y1 = (dy >= 0) ? 12 : ballSize + dy;
if (x1 > 12) x1 = 12;
if (y1 > 12) y1 = 12;
for (int ix = x0; ix < x1; ix++) {
for (int iy = y0; iy < y1; iy++) {
char hit = (ballSize == 16) ? BIG_BALL_MASK.at(iy - dy).at(ix - dx) : BALL_MASK.at(iy - dy).at(ix - dx);
if (hit == '1') return true;
}
}
return false;
}
void BounceOffSlope(int tileId) {
int oldVx = velX;
switch (tileId) {
case 35: velX = (velX > -velY) ? velX : velY; velY = oldVx; break;
case 37: velX = (-velX > velY) ? velX : velY; velY = oldVx; break;
case 34: velX = (velX < velY) ? velX : -velY; velY = -oldVx; break;
case 36: velX = (velX > velY) ? velX : -velY; velY = -oldVx; break;
case 31: velX = (velX > -velY) ? velX : (velY >> 1); velY = oldVx; break;
case 33: velX = (-velX > velY) ? velX : (velY >> 1); velY = oldVx; break;
case 30: velX = (velX < velY) ? velX : -(velY >> 1); velY = -oldVx; break;
case 32: velX = (velX > velY) ? velX : -(velY >> 1); velY = -oldVx; break;
}
}
bool CheckSlopeCollision(int px, int py, int ty, int tx, int tileId) {
int left = tx * 12, top = ty * 12;
int dx = px - ballRadius - left;
int dy = py - ballRadius - top;
int flipX = 0, flipY = 0;
if (tileId == 30 || tileId == 34) { flipX = 11; flipY = 11; }
else if (tileId == 31 || tileId == 35) { flipY = 11; }
else if (tileId == 33 || tileId == 37) { flipX = 11; }
int x0 = (dx >= 0) ? dx : 0;
int x1 = (dx >= 0) ? 12 : ballSize + dx;
int y0 = (dy >= 0) ? dy : 0;
int y1 = (dy >= 0) ? 12 : ballSize + dy;
if (x1 > 12) x1 = 12;
if (y1 > 12) y1 = 12;
for (int ix = x0; ix < x1; ix++) {
for (int iy = y0; iy < y1; iy++) {
char bHit = (ballSize == 16) ? BIG_BALL_MASK.at(iy - dy).at(ix - dx) : BALL_MASK.at(iy - dy).at(ix - dx);
char sHit = SLOPE_MASK.at(std::abs(iy - flipY)).at(std::abs(ix - flipX));
if (bHit == '1' && sHit == '1') {
if (!onGround) BounceOffSlope(tileId);
return true;
}
}
}
return false;
}
bool CheckObjectBox(int px, int py, int ty, int tx, int tileId) {
int x1 = tx * 12, y1 = ty * 12;
int x2 = x1 + 12, y2 = y1 + 12;
switch (tileId) {
case 3: case 5: case 9: case 13: case 14: case 17: case 18: case 21: case 22: case 43: case 45:
x1 += 4; x2 -= 4; break;
case 4: case 6: case 15: case 16: case 19: case 20: case 23: case 24: case 44: case 46:
y1 += 4; y2 -= 4; break;
}
return RectOverlap(px - ballRadius, py - ballRadius, px + ballRadius - 1, py + ballRadius - 1, x1, y1, x2 - 1, y2 - 1);
}
bool CheckHoopRim(int px, int py, int ty, int tx, int tileId) {
int x1 = tx * 12, y1 = ty * 12;
int x2 = x1 + 12, y2 = y1 + 12;
switch (tileId) {
case 15: case 19: case 23: case 27: y1 += 6; y2 -= 6; x2 -= 11; break;
case 16: case 20: case 24: case 28: y1 += 6; y2 -= 6; x1 += 11; break;
case 13: case 17: x1 += 6; x2 -= 6; y2 -= 11; break;
case 21: case 25: y2 = y1; y1--; x1 += 6; x2 -= 6; break;
case 14: case 18: case 22: case 26: x1 += 6; x2 -= 6; y1 += 11; break;
}
return RectOverlap(px - ballRadius, py - ballRadius, px + ballRadius, py + ballRadius, x1, y1, x2, y2);
}
void KillBall() {
if (godMode) return;
PlaySoundPop();
popTimer = 7;
ballState = 2;
lives--;
speedTimer = 0;
gravTimer = 0;
jumpTimer = 0;
}
void UseBigBall() {
ballSize = 16;
ballRadius = 8;
bool solved = false;
for (int offset = 1; !solved && offset < 12; offset++) {
solved = true;
if (CanMoveTo(ballX, ballY - offset)) { ballY -= offset; continue; }
if (CanMoveTo(ballX - offset, ballY - offset)) { ballX -= offset; ballY -= offset; continue; }
if (CanMoveTo(ballX + offset, ballY - offset)) { ballX += offset; ballY -= offset; continue; }
if (CanMoveTo(ballX, ballY + offset)) { ballY += offset; continue; }
if (CanMoveTo(ballX - offset, ballY + offset)) { ballX -= offset; ballY += offset; continue; }
if (CanMoveTo(ballX + offset, ballY + offset)) { ballX += offset; ballY += offset; continue; }
solved = false;
}
}
void UseRegularBall() {
ballSize = 12;
ballRadius = 6;
}
bool TestTileCollision(int px, int py, int ty, int tx) {
if (ty >= levelH || ty < 0 || tx >= levelW || tx < 0) return false;
if (ballState == 2) return false;
bool canPass = true;
int waterBit = tiles.at(ty).at(tx) & 0x40;
int tileId = tiles.at(ty).at(tx) & (~0x40) & (~0x80);
switch (tileId) {
case 1:
if (CheckWallCollision(px, py, ty, tx)) canPass = false;
hitSlopeOrWall = true;
break;
case 2:
if (CheckWallCollision(px, py, ty, tx)) { onRubber = true; canPass = false; }
else hitSlopeOrWall = true;
break;
case 34: case 35: case 36: case 37:
if (CheckSlopeCollision(px, py, ty, tx, tileId)) {
onRubber = true; canPass = false; hitSlopeOrWall = true;
}
break;
case 30: case 31: case 32: case 33:
if (CheckSlopeCollision(px, py, ty, tx, tileId)) {
canPass = false; hitSlopeOrWall = true;
}
break;
case 10: {
for (size_t i = 0; i < dynThorns.size(); i++) {
DynThorn& dt = dynThorns.at(i);
if (tx >= dt.bx && tx < dt.tx && ty >= dt.by && ty < dt.ty) {
int m = dt.bx * 12 + dt.px;
int n = dt.by * 12 + dt.py;
if (RectOverlap(px - ballRadius + 1, py - ballRadius + 1, px + ballRadius - 1, py + ballRadius - 1, m + 1, n + 1, m + 23, n + 23)) {
canPass = false;
KillBall();
}
}
}
break;
}
case 3: case 4: case 5: case 6:
if (CheckObjectBox(px, py, ty, tx, tileId)) { canPass = false; KillBall(); }
break;
case 7:
PlaySoundCrystal();
score += 200;
tiles.at(checkpointY).at(checkpointX) = 0;
checkpointX = tx; checkpointY = ty; checkpointSize = ballSize;
tiles.at(ty).at(tx) = 8;
break;
case 23:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (CheckHoopRim(px, py, ty, tx, tileId)) { canPass = false; break; }
PlaySoundHoop();
score += 500; hoopsScored++;
tiles.at(ty).at(tx) = (uint16_t)(27 | waterBit);
tiles.at(ty).at(tx + 1) = (uint16_t)(28 | waterBit);
}
break;
case 15:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (ballSize == 16) { canPass = false; break; }
if (CheckHoopRim(px, py, ty, tx, tileId)) canPass = false;
PlaySoundHoop();
score += 500; hoopsScored++;
tiles.at(ty).at(tx) = (uint16_t)(19 | waterBit);
tiles.at(ty).at(tx + 1) = (uint16_t)(20 | waterBit);
}
break;
case 24:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (CheckHoopRim(px, py, ty, tx, tileId)) canPass = false;
PlaySoundHoop();
score += 500; hoopsScored++;
tiles.at(ty).at(tx) = (uint16_t)(28 | waterBit);
tiles.at(ty).at(tx - 1) = (uint16_t)(27 | waterBit);
}
break;
case 16:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (ballSize == 16) { canPass = false; break; }
if (CheckHoopRim(px, py, ty, tx, tileId)) canPass = false;
PlaySoundHoop();
score += 500; hoopsScored++;
tiles.at(ty).at(tx) = (uint16_t)(20 | waterBit);
tiles.at(ty).at(tx - 1) = (uint16_t)(19 | waterBit);
}
break;
case 21:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (CheckHoopRim(px, py, ty, tx, tileId)) canPass = false;
PlaySoundHoop();
score += 500; hoopsScored++;
tiles.at(ty).at(tx) = (uint16_t)(25 | waterBit);
tiles.at(ty + 1).at(tx) = (uint16_t)(26 | waterBit);
}
break;
case 13:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (ballSize == 16) { canPass = false; break; }
if (CheckHoopRim(px, py, ty, tx, tileId)) canPass = false;
PlaySoundHoop();
score += 500; hoopsScored++;
tiles.at(ty).at(tx) = (uint16_t)(17 | waterBit);
tiles.at(ty + 1).at(tx) = (uint16_t)(18 | waterBit);
}
break;
case 22:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
PlaySoundHoop();
score += 500; hoopsScored++;
tiles.at(ty).at(tx) = (uint16_t)(26 | waterBit);
tiles.at(ty - 1).at(tx) = (uint16_t)(25 | waterBit);
}
break;
case 14:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (ballSize == 16) { canPass = false; break; }
PlaySoundHoop();
score += 500; hoopsScored++;
tiles.at(ty).at(tx) = (uint16_t)(18 | waterBit);
tiles.at(ty - 1).at(tx) = (uint16_t)(17 | waterBit);
}
break;
case 17: case 19: case 20:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (ballSize == 16) { canPass = false; break; }
if (CheckHoopRim(px, py, ty, tx, tileId)) canPass = false;
}
break;
case 25: case 27: case 28:
if (CheckHoopRim(px, py, ty, tx, tileId)) canPass = false;
break;
case 18:
if (CheckObjectBox(px, py, ty, tx, tileId) && ballSize == 16) canPass = false;
break;
case 9:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
if (exitOpened) levelFinished = true;
else canPass = false;
}
break;
case 29:
PlaySoundCrystal();
score += 1000;
if (lives < 5) lives++;
tiles.at(ty).at(tx) = 0;
break;
case 39: case 40: case 41: case 42:
canPass = false;
if (ballSize == 16) { PlayBeep(400.0f, 40); UseRegularBall(); }
break;
case 43: case 44: case 45: case 46:
if (CheckObjectBox(px, py, ty, tx, tileId)) {
canPass = false;
if (ballSize == 12) { PlayBeep(600.0f, 40); UseBigBall(); }
}
break;
case 47: case 48: case 49: case 50:
gravTimer = 300; onGround = false; canPass = false; PlayBeep(750.0f, 35);
break;
case 51: case 52: case 53: case 54:
jumpTimer = 300; canPass = false; PlayBeep(850.0f, 35);
break;
case 38:
speedTimer = 300; canPass = false; PlayBeep(950.0f, 35);
break;
}
return canPass;
}
bool CanMoveTo(int px, int py) {
int x0 = (px - ballRadius) / 12;
int y0 = (py - ballRadius) / 12;
int x1 = (px - 1 + ballRadius) / 12 + 1;
int y1 = (py - 1 + ballRadius) / 12 + 1;
for (int tx = x0; tx < x1; tx++) {
for (int ty = y0; ty < y1; ty++) {
if (!TestTileCollision(px, py, ty, tx)) return false;
}
}
return true;
}
void UpdatePhysics() {
prevBallX = ballX;
prevBallY = ballY;
if (ballState == 2) {
popTimer--;
if (popTimer <= 0) {
if (lives < 0) {
WriteSave();
wonGame = false;
hasActiveGame = false;
state = STATE_GAME_OVER;
return;
}
SpawnBall(checkpointX * 12 + 6, checkpointY * 12 + 6, checkpointSize);
}
return;
}
int tx = std::clamp(ballX / 12, 0, levelW - 1);
int ty = std::clamp(ballY / 12, 0, levelH - 1);
bool inWater = (tiles.at(ty).at(tx) & 0x40) != 0;
int maxVy = 80, grav = 4;
if (inWater) {
if (ballSize == 16) { maxVy = -30; grav = -2; if (onGround) velY = -10; }
else { maxVy = 42; grav = 6; }
} else if (ballSize == 16) {
maxVy = 38; grav = 3;
}
bool revGrav = false;
if (gravTimer > 0) {
revGrav = true;
maxVy *= -1; grav *= -1;
gravTimer--;
if (gravTimer == 0) {
revGrav = false; onGround = false; maxVy *= -1; grav *= -1;
}
}
if (jumpTimer > 0) {
if (-std::abs(bounceBoost) > -80) bounceBoost = revGrav ? 80 : -80;
jumpTimer--;
}
slideCounter = (slideCounter + 1) % 3;
velY = std::clamp(velY, -150, 150);
velX = std::clamp(velX, -150, 150);
if (velY < 10 && velY > 0 && !inWater && !revGrav) velY = 10;
int stepsY = std::abs(velY) / 10;
for (int s = 0; s < stepsY; s++) {
int dirY = (velY == 0) ? 0 : ((velY < 0) ? -1 : 1);
if (CanMoveTo(ballX, ballY + dirY)) {
ballY += dirY;
onGround = false;
if (maxVy == -30) {
int nty = std::clamp(ballY / 12, 0, levelH - 1);
if ((tiles.at(nty).at(tx) & 0x40) == 0) {
velY >>= 1;
if (velY <= 10 && velY >= -10) velY = 0;
}
}
} else {
if (hitSlopeOrWall && velX < 10 && slideCounter == 0) {
if (CanMoveTo(ballX + 1, ballY + dirY)) { ballX += 1; ballY += dirY; hitSlopeOrWall = false; }
else if (CanMoveTo(ballX - 1, ballY + dirY)) { ballX -= 1; ballY += dirY; hitSlopeOrWall = false; }
}
if (dirY > 0 || (revGrav && dirY < 0)) {
velY = -velY / 2;
onGround = true;
if (onRubber && (inputMask & 8) != 0) {
onRubber = false;
bounceBoost += revGrav ? 10 : -10;
} else if (jumpTimer == 0) {
bounceBoost = 0;
}
if (velY < 10 && velY > -10) {
velY = revGrav ? -10 : 10;
break;
}
break;
}
if (dirY < 0 || (revGrav && dirY > 0)) {
velY = revGrav ? -20 : (-velY >> 1);
}
}
}
if (revGrav) {
if (grav == -2 && velY < maxVy) { velY += grav; if (velY > maxVy) velY = maxVy; }
else if (!onGround && velY > maxVy) { velY += grav; if (velY < maxVy) velY = maxVy; }
} else {
if (grav == -2 && velY > maxVy) { velY += grav; if (velY < maxVy) velY = maxVy; }
else if (!onGround && velY < maxVy) { velY += grav; if (velY > maxVy) velY = maxVy; }
}
int maxVx = 50;
if (speedTimer > 0) { maxVx = 100; speedTimer--; }
if ((inputMask & 2) && velX < maxVx) velX += 6;
else if ((inputMask & 1) && velX > -maxVx) velX -= 6;
else if (velX > 0) velX -= 4;
else if (velX < 0) velX += 4;
if (ballSize == 16 && jumpTimer == 0) bounceBoost += revGrav ? 5 : -5;
if (onGround && (inputMask & 8)) {
velY = (revGrav ? 67 : -67) + bounceBoost;
onGround = false;
}
int stepsX = std::abs(velX) / 10;
for (int s = 0; s < stepsX; s++) {
int dirX = (velX == 0) ? 0 : ((velX < 0) ? -1 : 1);
if (CanMoveTo(ballX + dirX, ballY)) {
ballX += dirX;
} else if (hitSlopeOrWall) {
hitSlopeOrWall = false;
int dy = revGrav ? 1 : -1;
if (CanMoveTo(ballX + dirX, ballY + dy)) { ballX += dirX; ballY += dy; }
else if (CanMoveTo(ballX + dirX, ballY - dy)) { ballX += dirX; ballY -= dy; }
else velX = -(velX >> 1);
}
}
}
void UpdateDynThorns() {
for (auto& dt : dynThorns) {
dt.prevPx = dt.px;
dt.prevPy = dt.py;
dt.px += dt.vx;
int maxX = (dt.tx - dt.bx - 2) * 12;
if (dt.px <= 0) { dt.px = 0; dt.vx = -dt.vx; }
else if (dt.px >= maxX) { dt.px = maxX; dt.vx = -dt.vx; }
dt.py += dt.vy;
int maxY = (dt.ty - dt.by - 2) * 12;
if (dt.py <= 0) { dt.py = 0; dt.vy = -dt.vy; }
else if (dt.py >= maxY) { dt.py = maxY; dt.vy = -dt.vy; }
}
}
void Tick() {
if (state == STATE_SPLASH) {
splashTimer++;
if (splashTimer > 30) {
state = STATE_MENU;
}
return;
}
if (state != STATE_PLAYING) return;
if (levelBannerTimer > 0) levelBannerTimer--;
UpdatePhysics();
if (state != STATE_PLAYING) return;
if (!dynThorns.empty()) UpdateDynThorns();
if (hoopsScored >= hoopsTotal) exitUnlocked = true;
if (exitUnlocked && !exitOpened) {
exitAnimY += 4;
if (exitAnimY >= 24) {
exitAnimY = 24;
exitOpened = true;
}
UpdateExitSprite();
}
if (levelFinished) {
PlaySoundWin();
levelFinished = false;
level++;
score += 5000;
WriteSave();
if (level > 11) {
wonGame = true;
hasActiveGame = false;
state = STATE_GAME_OVER;
} else {
state = STATE_LEVEL_COMPLETE;
}
}
}
void DrawTileAt(SDL_Surface* target, int tx, int ty, int sx, int sy) {
if (tx < 0 || ty < 0 || tx >= levelW || ty >= levelH) {
SDL_Rect r = {(Sint16)sx, (Sint16)sy, 12, 12};
SDL_BlitSurface(sprites.at(0), NULL, target, &r);
return;
}
uint16_t raw = tiles.at(ty).at(tx);
bool inWater = (raw & 0x40) != 0;
int id = raw & (~0x40) & (~0x80);
Uint32 bg = inWater ? COL_WATER : COL_SKY;
SDL_Rect cell = {(Sint16)sx, (Sint16)sy, 12, 12};
auto fillBg = [&]() {
SDL_FillRect(target, &cell, SDL_MapRGB(target->format, (bg>>16)&0xFF, (bg>>8)&0xFF, bg&0xFF));
};
auto blit = [&](SDL_Surface* sp) {
if (sp) SDL_BlitSurface(sp, NULL, target, &cell);
};
switch (id) {
case 0: fillBg(); break;
case 1: blit(sprites.at(0)); break;
case 2: blit(sprites.at(1)); break;
case 3: blit(sprites.at(inWater ? 6 : 2)); break;
case 4: blit(sprites.at(inWater ? 9 : 5)); break;
case 5: blit(sprites.at(inWater ? 7 : 3)); break;
case 6: blit(sprites.at(inWater ? 8 : 4)); break;
case 7: fillBg(); blit(sprites.at(10)); break;
case 8: fillBg(); blit(sprites.at(11)); break;
case 9: {
fillBg();
int ox = (tx - exitX) * 12;
int oy = (ty - exitY) * 12;
SDL_Rect src = {(Sint16)ox, (Sint16)oy, 12, 12};
SDL_BlitSurface(exitDoorClip, &src, target, &cell);
break;
}
case 10: fillBg(); break;
case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20:
case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28:
fillBg();
blit(sprites.at(HOOP_LAYER_BACK.at(id - 13)));
blit(sprites.at(HOOP_LAYER_FRONT.at(id - 13)));
break;
case 29: fillBg(); blit(sprites.at(45)); break;
case 30: blit(sprites.at(inWater ? 61 : 57)); break;
case 31: blit(sprites.at(inWater ? 60 : 56)); break;
case 32: blit(sprites.at(inWater ? 59 : 55)); break;
case 33: blit(sprites.at(inWater ? 62 : 58)); break;
case 34: fillBg(); blit(sprites.at(65)); break;
case 35: fillBg(); blit(sprites.at(64)); break;
case 36: fillBg(); blit(sprites.at(63)); break;
case 37: fillBg(); blit(sprites.at(66)); break;
case 38: fillBg(); blit(sprites.at(53)); break;
case 39: fillBg(); blit(sprites.at(50)); break;
case 40: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(50), ROT_270); blit(s); SDL_FreeSurface(s); } break;
case 41: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(50), ROT_180); blit(s); SDL_FreeSurface(s); } break;
case 42: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(50), ROT_90);  blit(s); SDL_FreeSurface(s); } break;
case 43: fillBg(); blit(sprites.at(51)); break;
case 44: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(51), ROT_270); blit(s); SDL_FreeSurface(s); } break;
case 45: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(51), ROT_180); blit(s); SDL_FreeSurface(s); } break;
case 46: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(51), ROT_90);  blit(s); SDL_FreeSurface(s); } break;
case 47: fillBg(); blit(sprites.at(52)); break;
case 48: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(52), ROT_270); blit(s); SDL_FreeSurface(s); } break;
case 49: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(52), ROT_180); blit(s); SDL_FreeSurface(s); } break;
case 50: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(52), ROT_90);  blit(s); SDL_FreeSurface(s); } break;
case 51: fillBg(); blit(sprites.at(54)); break;
case 52: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(54), ROT_270); blit(s); SDL_FreeSurface(s); } break;
case 53: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(54), ROT_180); blit(s); SDL_FreeSurface(s); } break;
case 54: fillBg(); { SDL_Surface* s=ModifyImage(sprites.at(54), ROT_90);  blit(s); SDL_FreeSurface(s); } break;
default: fillBg(); break;
}
}
void DrawHoopForeground(SDL_Surface* target, int offX, int offY, int rBallX, int rBallY) {
int x0 = std::max(0, (rBallX - ballRadius) / 12);
int y0 = std::max(0, (rBallY - ballRadius) / 12);
int x1 = std::min(levelW, (rBallX + ballRadius - 1) / 12 + 1);
int y1 = std::min(levelH, (rBallY + ballRadius - 1) / 12 + 1);
for (int tx = x0; tx < x1; tx++) {
for (int ty = y0; ty < y1; ty++) {
int id = tiles.at(ty).at(tx) & (~0x40) & (~0x80);
if (id >= 13 && id <= 28) {
SDL_Rect r = {(Sint16)(tx * 12 - camX + offX), (Sint16)(ty * 12 - camY + offY), 12, 12};
SDL_BlitSurface(sprites.at(HOOP_LAYER_FRONT.at(id - 13)), NULL, target, &r);
}
}
}
}
void RenderGameplay(float alpha) {
if (fpsMode == FPS_ORIG_20) alpha = 1.0f;
int rBallX = (int)std::round(prevBallX + (ballX - prevBallX) * alpha);
int rBallY = (int)std::round(prevBallY + (ballY - prevBallY) * alpha);
int viewW = 128, viewH = 96, hudH = 32, scale = 1, offX = 0, offY = 0;
if (screenMode == MODE_FULL_320x240) {
viewW = 320; viewH = 216; hudH = 24; scale = 1; offX = 0; offY = 0;
} else if (screenMode == MODE_SCALE_2X) {
viewW = 128; viewH = 88; hudH = 32; scale = 2; offX = 32; offY = 0;
} else {
viewW = 128; viewH = 96; hudH = 32; scale = 1; offX = 96; offY = 56;
}
camX = rBallX - viewW / 2;
if (camX < 0) camX = 0;
if (camX > levelW * 12 - viewW) camX = std::max(0, levelW * 12 - viewW);
if (screenMode == MODE_FULL_320x240) {
camY = rBallY - viewH / 2;
if (camY < 0) camY = 0;
if (camY > levelH * 12 - viewH) camY = std::max(0, levelH * 12 - viewH);
} else {
while (ballY - 6 < camY) camY -= 7 * 12;
while (ballY + 6 > camY + viewH) camY += 7 * 12;
if (camY < 0) camY = 0;
if (camY > levelH * 12 - viewH) camY = std::max(0, levelH * 12 - viewH);
}
SDL_FillRect(backBuffer, NULL, 0);
SDL_Surface* canvas = (scale == 1) ? backBuffer : CreateRGBA(128, 120);
int drawOffX = (scale == 1) ? offX : 0;
int drawOffY = (scale == 1) ? offY : 0;
SDL_Rect viewClip = {(Sint16)drawOffX, (Sint16)drawOffY, (Uint16)viewW, (Uint16)viewH};
SDL_SetClipRect(canvas, &viewClip);
int startTx = camX / 12;
int startTy = camY / 12;
int endTx = startTx + viewW / 12 + 2;
int endTy = startTy + viewH / 12 + 2;
for (int ty = startTy; ty < endTy; ty++) {
for (int tx = startTx; tx < endTx; tx++) {
DrawTileAt(canvas, tx, ty, tx * 12 - camX + drawOffX, ty * 12 - camY + drawOffY);
}
}
for (size_t i = 0; i < dynThorns.size(); i++) {
DynThorn& dt = dynThorns.at(i);
int rPx = (int)std::round(dt.prevPx + (dt.px - dt.prevPx) * alpha);
int rPy = (int)std::round(dt.prevPy + (dt.py - dt.prevPy) * alpha);
int sx = dt.bx * 12 + rPx - camX + drawOffX;
int sy = dt.by * 12 + rPy - camY + drawOffY;
SDL_Rect dst = {(Sint16)sx, (Sint16)sy, 24, 24};
SDL_BlitSurface(dynThornSprite, NULL, canvas, &dst);
}
if (ballState == 2) {
SDL_Rect br = {(Sint16)(rBallX - 6 - camX + drawOffX), (Sint16)(rBallY - 6 - camY + drawOffY), 12, 12};
SDL_BlitSurface(sprites.at(48), NULL, canvas, &br);
} else {
SDL_Surface* bsp = (ballSize == 16) ? sprites.at(49) : sprites.at(47);
SDL_Rect br = {(Sint16)(rBallX - ballRadius - camX + drawOffX), (Sint16)(rBallY - ballRadius - camY + drawOffY), (Uint16)ballSize, (Uint16)ballSize};
SDL_BlitSurface(bsp, NULL, canvas, &br);
}
DrawHoopForeground(canvas, drawOffX, drawOffY, rBallX, rBallY);
if (levelBannerTimer > 0) {
DrawText(canvas, drawOffX + viewW / 2 - 20, drawOffY + viewH - 14, "LEVEL " + std::to_string(level), COL_WHITE, 1);
}
SDL_SetClipRect(canvas, NULL);
if (screenMode == MODE_FULL_320x240 && filterMode != FILTER_NONE) {
ApplySoftFilter320(canvas, viewH);
}
SDL_Rect hudRect = {(Sint16)drawOffX, (Sint16)(drawOffY + viewH), (Uint16)viewW, (Uint16)hudH};
SDL_FillRect(canvas, &hudRect, SDL_MapRGB(canvas->format, 0x08, 0x53, 0xAA));
std::string scoreStr = std::to_string(score);
while (scoreStr.size() < 8) scoreStr = "0" + scoreStr;
if (screenMode == MODE_FULL_320x240) {
for (int i = 0; i < lives; i++) {
SDL_Rect lr = {(Sint16)(8 + i * 13), (Sint16)(viewH + 6), 12, 12};
SDL_BlitSurface(hudBall, NULL, canvas, &lr);
}
for (int i = 0; i < hoopsTotal - hoopsScored; i++) {
SDL_Rect hr = {(Sint16)(90 + i * 10), (Sint16)(viewH + 6), 12, 12};
SDL_BlitSurface(hudHoop, NULL, canvas, &hr);
}
DrawText(canvas, 250, viewH + 8, scoreStr, COL_WHITE, 1);
} else {
for (int i = 0; i < lives; i++) {
SDL_Rect lr = {(Sint16)(drawOffX + 5 + i * 11), (Sint16)(drawOffY + viewH + 2), 12, 12};
SDL_BlitSurface(hudBall, NULL, canvas, &lr);
}
for (int i = 0; i < hoopsTotal - hoopsScored; i++) {
SDL_Rect hr = {(Sint16)(drawOffX + 5 + i * 8), (Sint16)(drawOffY + viewH + 15), 12, 12};
SDL_BlitSurface(hudHoop, NULL, canvas, &hr);
}
DrawText(canvas, drawOffX + 64, drawOffY + viewH + 4, scoreStr, COL_WHITE, 1);
}
int boost = std::max({speedTimer, gravTimer, jumpTimer});
if (boost > 0) {
int bh = (hudH * boost) / 300;
SDL_Rect br = {(Sint16)(drawOffX + 1), (Sint16)(drawOffY + viewH + hudH - bh), 4, (Uint16)bh};
SDL_FillRect(canvas, &br, SDL_MapRGB(canvas->format, 0xFF, 0x98, 0x13));
}
if (scale == 2) {
if (filterMode == FILTER_SCALE2X) {
BlitScale2x(canvas, backBuffer, offX, 0);
} else if (filterMode == FILTER_BILINEAR) {
BlitBilinear2x(canvas, backBuffer, offX, 0);
} else {
SDL_LockSurface(canvas);
SDL_LockSurface(backBuffer);
for (int y = 0; y < 120; y++) {
for (int x = 0; x < 128; x++) {
Uint32 c = GetPixelFast(canvas, x, y);
PutPixelFast(backBuffer, offX + x * 2,     y * 2,     c);
PutPixelFast(backBuffer, offX + x * 2 + 1, y * 2,     c);
PutPixelFast(backBuffer, offX + x * 2,     y * 2 + 1, c);
PutPixelFast(backBuffer, offX + x * 2 + 1, y * 2 + 1, c);
}
}
SDL_UnlockSurface(backBuffer);
SDL_UnlockSurface(canvas);
}
SDL_FreeSurface(canvas);
}
}
void Render(float alpha) {
if (state == STATE_SPLASH) {
SDL_FillRect(backBuffer, NULL, 0);
if (splashImg) {
SDL_Rect dst = {(Sint16)((SCREEN_W - splashImg->w)/2), (Sint16)((SCREEN_H - splashImg->h)/2), 0, 0};
SDL_BlitSurface(splashImg, NULL, backBuffer, &dst);
} else {
DrawTextCentered(backBuffer, 110, "NOKIA BOUNCE", COL_WHITE, 2);
}
} else if (state == STATE_MENU) {
SDL_FillRect(backBuffer, NULL, SDL_MapRGB(backBuffer->format, 0x08, 0x53, 0xAA));
DrawTextCentered(backBuffer, 22, "BOUNCE", COL_WHITE, 3);
std::vector<std::string> items;
if (hasActiveGame) items.push_back("CONTINUE");
items.push_back("NEW GAME");
items.push_back("SETTINGS");
items.push_back("HIGH SCORE");
items.push_back("EXIT");
if (menuCursor >= (int)items.size()) menuCursor = 0;
for (size_t i = 0; i < items.size(); i++) {
Uint32 col = ((int)i == menuCursor) ? COL_ORANGE : COL_WHITE;
std::string label = ((int)i == menuCursor) ? ("> " + items.at(i) + " <") : items.at(i);
DrawTextCentered(backBuffer, 80 + (int)i * 22, label, col, 1);
}
DrawTextCentered(backBuffer, 218, "SELECT: SCREEN MODE   START: MENU", 0xB0E0F0, 1);
} else if (state == STATE_SETTINGS) {
SDL_FillRect(backBuffer, NULL, SDL_MapRGB(backBuffer->format, 0x08, 0x53, 0xAA));
DrawTextCentered(backBuffer, 22, "SETTINGS", COL_WHITE, 2);
std::vector<std::string> opts;
opts.push_back(std::string("SCREEN: ") + (screenMode == 0 ? "320X240 FULL" : (screenMode == 1 ? "2X CLASSIC" : "1X 128X128")));
opts.push_back(std::string("FPS: ") + (fpsMode == 0 ? "20 ORIG" : (fpsMode == 1 ? "50 SMOOTH" : "60 SMOOTH")));
opts.push_back(std::string("FILTER: ") + (filterMode == 0 ? "NONE" : (filterMode == 1 ? "SCALE2X" : "BILINEAR")));
opts.push_back("BACK");
for (size_t i = 0; i < opts.size(); i++) {
Uint32 col = ((int)i == settingsCursor) ? COL_ORANGE : COL_WHITE;
std::string label = ((int)i == settingsCursor) ? ("> " + opts.at(i) + " <") : opts.at(i);
DrawTextCentered(backBuffer, 75 + (int)i * 26, label, col, 1);
}
DrawTextCentered(backBuffer, 210, "A/LEFT/RIGHT: CHANGE   B: BACK", 0xB0E0F0, 1);
} else if (state == STATE_LEVEL_SELECT) {
SDL_FillRect(backBuffer, NULL, SDL_MapRGB(backBuffer->format, 0x08, 0x53, 0xAA));
DrawTextCentered(backBuffer, 20, "SELECT LEVEL", COL_WHITE, 2);
for (int i = 0; i < maxLevels; i++) {
Uint32 col = (i == levelCursor) ? COL_ORANGE : COL_WHITE;
DrawTextCentered(backBuffer, 55 + i * 15, "LEVEL " + std::to_string(i + 1), col, 1);
}
} else if (state == STATE_PLAYING) {
RenderGameplay(alpha);
} else if (state == STATE_LEVEL_COMPLETE) {
SDL_FillRect(backBuffer, NULL, SDL_MapRGB(backBuffer->format, 0x08, 0x53, 0xAA));
DrawTextCentered(backBuffer, 70, "LEVEL " + std::to_string(level - 1) + " COMPLETED!", COL_WHITE, 2);
DrawTextCentered(backBuffer, 115, "SCORE: " + std::to_string(score), COL_ORANGE, 2);
DrawTextCentered(backBuffer, 175, "PRESS A OR START TO CONTINUE", COL_WHITE, 1);
} else if (state == STATE_GAME_OVER) {
SDL_FillRect(backBuffer, NULL, SDL_MapRGB(backBuffer->format, 0x08, 0x53, 0xAA));
DrawTextCentered(backBuffer, 55, wonGame ? "CONGRATULATIONS!" : "GAME OVER", COL_WHITE, 2);
if (newHighScore) DrawTextCentered(backBuffer, 95, "NEW HIGH SCORE!", COL_ORANGE, 1);
DrawTextCentered(backBuffer, 125, "FINAL SCORE: " + std::to_string(score), COL_WHITE, 2);
DrawTextCentered(backBuffer, 180, "PRESS A OR START", COL_WHITE, 1);
} else if (state == STATE_HIGH_SCORE) {
SDL_FillRect(backBuffer, NULL, SDL_MapRGB(backBuffer->format, 0x08, 0x53, 0xAA));
DrawTextCentered(backBuffer, 65, "HIGH SCORE", COL_WHITE, 2);
std::string hs = std::to_string(highScore);
while (hs.size() < 8) hs = "0" + hs;
DrawTextCentered(backBuffer, 115, hs, COL_ORANGE, 2);
DrawTextCentered(backBuffer, 180, "PRESS B TO BACK", COL_WHITE, 1);
}
SDL_BlitSurface(backBuffer, NULL, screen, NULL);
SDL_Flip(screen);
}
void ChangeSetting(int dir) {
if (settingsCursor == 0) {
screenMode = (ScreenMode)((screenMode + dir + 3) % 3);
} else if (settingsCursor == 1) {
fpsMode = (FpsMode)((fpsMode + dir + 3) % 3);
} else if (settingsCursor == 2) {
filterMode = (FilterMode)((filterMode + dir + 3) % 3);
}
WriteSave();
}
void HandleInput(bool& running) {
SDL_Event e;
while (SDL_PollEvent(&e)) {
if (e.type == SDL_QUIT) {
running = false;
} else if (e.type == SDL_KEYDOWN) {
SDLKey k = e.key.keysym.sym;
if (k == SDLK_ESCAPE) {
screenMode = (ScreenMode)((screenMode + 1) % 3);
WriteSave();
continue;
}
if (state == STATE_SPLASH) {
state = STATE_MENU;
} else if (state == STATE_MENU) {
int totalItems = hasActiveGame ? 5 : 4;
if (k == SDLK_UP) menuCursor = (menuCursor - 1 + totalItems) % totalItems;
else if (k == SDLK_DOWN) menuCursor = (menuCursor + 1) % totalItems;
else if (k == SDLK_LCTRL || k == SDLK_RETURN) {
int idx = hasActiveGame ? menuCursor : menuCursor + 1;
if (idx == 0) {
state = STATE_PLAYING;
} else if (idx == 1) {
if (maxLevels > 1) {
levelCursor = 0;
state = STATE_LEVEL_SELECT;
} else {
level = 1; score = 0; lives = 3; newHighScore = false;
LoadLevel(1);
hasActiveGame = true;
state = STATE_PLAYING;
}
} else if (idx == 2) {
settingsCursor = 0;
state = STATE_SETTINGS;
} else if (idx == 3) {
state = STATE_HIGH_SCORE;
} else if (idx == 4) {
running = false;
}
}
} else if (state == STATE_SETTINGS) {
if (k == SDLK_UP) settingsCursor = (settingsCursor - 1 + 4) % 4;
else if (k == SDLK_DOWN) settingsCursor = (settingsCursor + 1) % 4;
else if (k == SDLK_LEFT) ChangeSetting(-1);
else if (k == SDLK_RIGHT) ChangeSetting(1);
else if (k == SDLK_LALT) state = STATE_MENU;
else if (k == SDLK_LCTRL || k == SDLK_RETURN) {
if (settingsCursor == 3) state = STATE_MENU;
else ChangeSetting(1);
}
} else if (state == STATE_LEVEL_SELECT) {
if (k == SDLK_UP) levelCursor = (levelCursor - 1 + maxLevels) % maxLevels;
else if (k == SDLK_DOWN) levelCursor = (levelCursor + 1) % maxLevels;
else if (k == SDLK_LALT) state = STATE_MENU;
else if (k == SDLK_LCTRL || k == SDLK_RETURN) {
level = levelCursor + 1;
score = 0; lives = 3; newHighScore = false;
LoadLevel(level);
hasActiveGame = true;
state = STATE_PLAYING;
}
} else if (state == STATE_PLAYING) {
if (k == SDLK_RETURN) {
inputMask = 0;
state = STATE_MENU;
} else if (k == SDLK_LEFT)  inputMask |= 1;
else if (k == SDLK_RIGHT) inputMask |= 2;
else if (k == SDLK_DOWN)  inputMask |= 4;
else if (k == SDLK_UP || k == SDLK_LCTRL || k == SDLK_SPACE) inputMask |= 8;
else if (k == SDLK_TAB) {
godMode = !godMode;
} else if (k == SDLK_BACKSPACE && godMode) {
levelFinished = true;
}
} else if (state == STATE_LEVEL_COMPLETE) {
if (k == SDLK_LCTRL || k == SDLK_RETURN) {
LoadLevel(level);
state = STATE_PLAYING;
}
} else if (state == STATE_GAME_OVER || state == STATE_HIGH_SCORE) {
if (k == SDLK_LCTRL || k == SDLK_LALT || k == SDLK_RETURN) {
state = STATE_MENU;
}
}
} else if (e.type == SDL_KEYUP) {
SDLKey k = e.key.keysym.sym;
if (k == SDLK_LEFT)  inputMask &= ~1;
else if (k == SDLK_RIGHT) inputMask &= ~2;
else if (k == SDLK_DOWN)  inputMask &= ~4;
else if (k == SDLK_UP || k == SDLK_LCTRL || k == SDLK_SPACE) inputMask &= ~8;
}
}
}
};
int main(int argc, char* argv[]) {
BouncePort game;
if (!game.InitSDL()) {
fprintf(stderr, "SDL Init failed!\n");
return 1;
}
bool running = true;
const Uint32 TICK_MS = 50; // Оригинальные 20 тиков физики в секунду
Uint32 lastTime = SDL_GetTicks();
Uint32 accumulator = 0;
while (running) {
Uint32 frameStart = SDL_GetTicks();
Uint32 dt = frameStart - lastTime;
lastTime = frameStart;
if (dt > 100) dt = 100;
accumulator += dt;
game.HandleInput(running);
while (accumulator >= TICK_MS) {
game.Tick();
accumulator -= TICK_MS;
}
float alpha = (float)accumulator / (float)TICK_MS;
game.Render(alpha);
Uint32 targetFrameMs = 50;
if (game.fpsMode == FPS_SMOOTH_50) targetFrameMs = 20;
else if (game.fpsMode == FPS_SMOOTH_60) targetFrameMs = 16;
Uint32 elapsed = SDL_GetTicks() - frameStart;
if (elapsed < targetFrameMs) {
SDL_Delay(targetFrameMs - elapsed);
}
}
SDL_CloseAudio();
SDL_Quit();
return 0;
}