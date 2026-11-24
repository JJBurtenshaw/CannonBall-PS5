// CANNONBALL_PS5_BETA1C2: two-item auto-saving Settings menu.
#ifdef CANNONBALL_PS5_BETA1B
// CannonBall-PS5 Beta 1B pause overlay and settings (no SDK dependencies).
#include "ps5_pause_ui.hpp"
#include <cstring>

Ps5PauseUI ps5_pause_ui;

Ps5PauseUI::Ps5PauseUI()
    : active_(false), selected_(0), screen_(MAIN), confirm_yes_(false),
      fps_visible_(false), attract_music_(true), freeplay_(false), save_result_(0) {}

void Ps5PauseUI::sync_settings(bool fps, bool attract, bool freeplay)
{
    fps_visible_ = fps;
    attract_music_ = attract;
    freeplay_ = freeplay;
}

void Ps5PauseUI::set_save_result(bool ok)
{
    save_result_ = ok ? 1 : -1;
}


void Ps5PauseUI::reset()
{
    active_ = false;
    selected_ = 0;
    screen_ = MAIN;
    confirm_yes_ = false;
    save_result_ = 0;
}

Ps5PauseUI::Action Ps5PauseUI::press(Button button)
{
    if (button == TOUCHPAD) {
        if (active_) reset();
        else {
            active_ = true;
            selected_ = 0;
            screen_ = MAIN;
            confirm_yes_ = false;
            save_result_ = 0;
        }
        return NO_ACTION;
    }
    if (!active_) return NO_ACTION;

    if (button == BACK) {
        if (screen_ == MAIN) reset();
        else { screen_ = MAIN; selected_ = 1; confirm_yes_ = false; save_result_ = 0; }
        return NO_ACTION;
    }

    if (screen_ == MAIN) {
        if (button == UP) selected_ = (selected_ + 3) % 4;
        else if (button == DOWN) selected_ = (selected_ + 1) % 4;
        else if (button == ACCEPT) {
            if (selected_ == 0) reset();
            else if (selected_ == 1) { screen_ = SETTINGS; selected_ = 0; save_result_ = 0; }
            else {
                screen_ = selected_ == 2 ? CONFIRM_RESTART : CONFIRM_MENU;
                confirm_yes_ = false; // Default to NO to protect ongoing games.
            }
        }
        return NO_ACTION;
    }
    if (screen_ == SETTINGS) {
        // FPS counter, Save Settings, Back. Free Play is always on.
        if (button == UP) selected_ = (selected_ + 1) % 2;
        else if (button == DOWN) selected_ = (selected_ + 1) % 2;
        else if (button == LEFT || button == RIGHT || button == ACCEPT) {
            switch (selected_) {
                case 0: fps_visible_ = !fps_visible_; return TOGGLE_FPS;
                case 1:
                    if (button == ACCEPT) { screen_ = MAIN; selected_ = 1; }
                    break;
            }
        }
        return NO_ACTION;
    }
    // Confirmation screen: NO selected by default. Left/right/up/down toggle.
    if (button == LEFT || button == RIGHT || button == UP || button == DOWN) {
        confirm_yes_ = !confirm_yes_;
    } else if (button == ACCEPT) {
        if (confirm_yes_) {
            const Action result = screen_ == CONFIRM_RESTART ? RESTART_GAME : RETURN_TO_MENU;
            reset();
            return result;
        }
        screen_ = MAIN;
    }
    return NO_ACTION;
}

namespace {
struct Glyph { char c; uint8_t bits[7]; };
// 5x7 uppercase bitmap font. Each row uses the low five bits.
static const Glyph FONT[] = {
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{15,16,16,16,16,16,15}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{15,16,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{31,4,4,4,4,4,31}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
 {':',{0,4,4,0,4,4,0}}, {'?',{14,17,1,2,4,0,4}},
 {'-',{0,0,0,31,0,0,0}}, {'/',{1,1,2,4,8,16,16}},
 {'(',{2,4,8,8,8,4,2}}, {')',{8,4,2,2,2,4,8}},
 {'[',{14,8,8,8,8,8,14}}, {']',{14,2,2,2,2,2,14}},
 {'.',{0,0,0,0,0,12,12}}, {'!',{4,4,4,4,4,0,4}},
 {'>',{16,8,4,2,4,8,16}}
};

static const uint8_t* glyph_for(char c)
{
    const unsigned count = sizeof(FONT) / sizeof(FONT[0]);
    for (unsigned i = 0; i < count; ++i)
        if (FONT[i].c == c) return FONT[i].bits;
    return 0;
}

static void rect(uint32_t* p, int w, int h, int stride,
                 int x, int y, int rw, int rh, uint32_t colour)
{
    if (!p || w <= 0 || h <= 0 || stride < w || rw <= 0 || rh <= 0) return;
    if (x < 0) { rw += x; x = 0; }
    if (y < 0) { rh += y; y = 0; }
    if (x >= w || y >= h) return;
    if (rw > w - x) rw = w - x;
    if (rh > h - y) rh = h - y;
    if (rw <= 0 || rh <= 0) return;
    for (int yy = y; yy < y + rh; ++yy) {
        uint32_t* row = p + yy * stride + x;
        for (int xx = 0; xx < rw; ++xx) row[xx] = colour;
    }
}

static void text(uint32_t* p, int w, int h, int stride,
                 int x, int y, const char* msg, uint32_t colour, int scale)
{
    if (!msg || scale < 1) return;
    for (const char* s = msg; *s; ++s, x += 6 * scale) {
        const uint8_t* glyph = glyph_for(*s);
        if (!glyph) continue;
        for (int gy = 0; gy < 7; ++gy) {
            for (int gx = 0; gx < 5; ++gx) {
                if (glyph[gy] & (1u << (4 - gx)))
                    rect(p, w, h, stride, x + gx * scale, y + gy * scale,
                         scale, scale, colour);
            }
        }
    }
}

static void centered(uint32_t* p, int w, int h, int stride, int cy,
                     const char* msg, uint32_t colour, int scale)
{
    const int tw = static_cast<int>(std::strlen(msg)) * 6 * scale;
    text(p, w, h, stride, (w - tw) / 2, cy, msg, colour, scale);
}
}

void Ps5PauseUI::draw(uint32_t* dst, int width, int height, int stride,
                      uint32_t panel, uint32_t heading, uint32_t highlight,
                      uint32_t white, uint32_t muted, uint32_t black) const
{
    if (!active_ || !dst || width < 200 || height < 150 || stride < width) return;

    // Dim the existing OutRun frame without changing simulation or ROM assets.
    // Works for both ARGB8888 and ABGR8888 (preserves alpha in high byte).
    for (int y = 0; y < height; ++y) {
        uint32_t* row = dst + y * stride;
        for (int x = 0; x < width; ++x) {
            const uint32_t c = row[x];
            row[x] = (c & 0xff000000u) | ((c & 0x00fefefeu) >> 1);
        }
    }

    const int scale = width >= 640 ? 2 : 1;
    const int pw = width > 370 ? 310 : width - 28;
    const int ph = height > 260 ? 190 : 166;
    const int x = (width - pw) / 2;
    const int y = (height - ph) / 2;
    rect(dst, width, height, stride, x - 2, y - 2, pw + 4, ph + 4, black);
    rect(dst, width, height, stride, x, y, pw, ph, panel);
    rect(dst, width, height, stride, x, y, pw, 30, heading);
    centered(dst, width, height, stride, y + 10, "PAUSED", white, scale);

    if (screen_ == MAIN) {
        const char* items[] = {
            "RESUME GAME", "SETTINGS", "RESTART GAME", "RETURN TO MAIN MENU"
        };
        const int base = y + 47;
        for (int i = 0; i < 4; ++i) {
            const int row = base + i * 23;
            if (i == selected_) rect(dst, width, height, stride,
                                     x + 11, row - 5, pw - 22, 19, highlight);
            centered(dst, width, height, stride, row, items[i],
                     white, scale);
        }
        centered(dst, width, height, stride, y + ph - 17,
                 "TOUCHPAD: RESUME   X: SELECT", muted, 1);
    } else if (screen_ == SETTINGS) {
        const char* names[2] = {
            fps_visible_ ? "FPS COUNTER: ON" : "FPS COUNTER: OFF",
            "BACK"
        };
        for (int i = 0; i < 2; ++i) {
            const int row = y + 44 + i * 23;
            if (i == selected_)
                rect(dst, width, height, stride, x + 11, row - 4, pw - 22, 19, highlight);
            centered(dst, width, height, stride, row, names[i], white, scale);
        }
        centered(dst, width, height, stride, y + ph - 18,
                 save_result_ == -1 ? "SAVE FAILED - CHECK PATH" :
                 "LEFT/RIGHT: CHANGE  X: SELECT", muted, 1);
    } else {
        centered(dst, width, height, stride, y + 56,
                 screen_ == CONFIRM_RESTART ? "RESTART GAME?" : "RETURN TO MENU?",
                 white, scale);
        centered(dst, width, height, stride, y + 79,
                 "UNSAVED PROGRESS WILL BE LOST", muted, 1);
        const int bw = 65;
        const int by = y + 105;
        rect(dst, width, height, stride, width / 2 - 76, by, bw, 22,
             confirm_yes_ ? highlight : heading);
        rect(dst, width, height, stride, width / 2 + 11, by, bw, 22,
             confirm_yes_ ? heading : highlight);
        text(dst, width, height, stride, width / 2 - 52, by + 8,
             "YES", white, 1);
        text(dst, width, height, stride, width / 2 + 36, by + 8,
             "NO", white, 1);
        centered(dst, width, height, stride, y + ph - 17,
                 "X: CONFIRM   O: BACK", muted, 1);
    }
}

#else
// CannonBall-PS5 Beta 1A pause overlay and input state (no SDK dependencies).
#include "ps5_pause_ui.hpp"
#include <cstring>

Ps5PauseUI ps5_pause_ui;

Ps5PauseUI::Ps5PauseUI() : active_(false), selected_(0), screen_(MAIN), confirm_yes_(false) {}

void Ps5PauseUI::reset()
{
    active_ = false;
    selected_ = 0;
    screen_ = MAIN;
    confirm_yes_ = false;
}

Ps5PauseUI::Action Ps5PauseUI::press(Button button)
{
    if (button == TOUCHPAD) {
        if (active_) reset();
        else {
            active_ = true;
            selected_ = 0;
            screen_ = MAIN;
            confirm_yes_ = false;
        }
        return NO_ACTION;
    }
    if (!active_) return NO_ACTION;

    if (button == BACK) {
        if (screen_ == MAIN) reset();
        else { screen_ = MAIN; confirm_yes_ = false; }
        return NO_ACTION;
    }

    if (screen_ == MAIN) {
        if (button == UP) selected_ = (selected_ + 3) % 4;
        else if (button == DOWN) selected_ = (selected_ + 1) % 4;
        else if (button == ACCEPT) {
            if (selected_ == 0) reset();
            else if (selected_ == 1) screen_ = SETTINGS_NOTICE;
            else {
                screen_ = selected_ == 2 ? CONFIRM_RESTART : CONFIRM_MENU;
                confirm_yes_ = false; // Default to NO to protect ongoing games.
            }
        }
        return NO_ACTION;
    }
    if (screen_ == SETTINGS_NOTICE) {
        if (button == ACCEPT) screen_ = MAIN;
        return NO_ACTION;
    }
    // Confirmation screen: NO selected by default. Left/right/up/down toggle.
    if (button == LEFT || button == RIGHT || button == UP || button == DOWN) {
        confirm_yes_ = !confirm_yes_;
    } else if (button == ACCEPT) {
        if (confirm_yes_) {
            const Action result = screen_ == CONFIRM_RESTART ? RESTART_GAME : RETURN_TO_MENU;
            reset();
            return result;
        }
        screen_ = MAIN;
    }
    return NO_ACTION;
}

namespace {
struct Glyph { char c; uint8_t bits[7]; };
// 5x7 uppercase bitmap font. Each row uses the low five bits.
static const Glyph FONT[] = {
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{15,16,16,16,16,16,15}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{15,16,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{31,4,4,4,4,4,31}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
 {':',{0,4,4,0,4,4,0}}, {'?',{14,17,1,2,4,0,4}},
 {'-',{0,0,0,31,0,0,0}}, {'/',{1,1,2,4,8,16,16}},
 {'(',{2,4,8,8,8,4,2}}, {')',{8,4,2,2,2,4,8}},
 {'[',{14,8,8,8,8,8,14}}, {']',{14,2,2,2,2,2,14}},
 {'.',{0,0,0,0,0,12,12}}, {'!',{4,4,4,4,4,0,4}},
 {'>',{16,8,4,2,4,8,16}}
};

static const uint8_t* glyph_for(char c)
{
    const unsigned count = sizeof(FONT) / sizeof(FONT[0]);
    for (unsigned i = 0; i < count; ++i)
        if (FONT[i].c == c) return FONT[i].bits;
    return 0;
}

static void rect(uint32_t* p, int w, int h, int stride,
                 int x, int y, int rw, int rh, uint32_t colour)
{
    if (!p || w <= 0 || h <= 0 || stride < w || rw <= 0 || rh <= 0) return;
    if (x < 0) { rw += x; x = 0; }
    if (y < 0) { rh += y; y = 0; }
    if (x >= w || y >= h) return;
    if (rw > w - x) rw = w - x;
    if (rh > h - y) rh = h - y;
    if (rw <= 0 || rh <= 0) return;
    for (int yy = y; yy < y + rh; ++yy) {
        uint32_t* row = p + yy * stride + x;
        for (int xx = 0; xx < rw; ++xx) row[xx] = colour;
    }
}

static void text(uint32_t* p, int w, int h, int stride,
                 int x, int y, const char* msg, uint32_t colour, int scale)
{
    if (!msg || scale < 1) return;
    for (const char* s = msg; *s; ++s, x += 6 * scale) {
        const uint8_t* glyph = glyph_for(*s);
        if (!glyph) continue;
        for (int gy = 0; gy < 7; ++gy) {
            for (int gx = 0; gx < 5; ++gx) {
                if (glyph[gy] & (1u << (4 - gx)))
                    rect(p, w, h, stride, x + gx * scale, y + gy * scale,
                         scale, scale, colour);
            }
        }
    }
}

static void centered(uint32_t* p, int w, int h, int stride, int cy,
                     const char* msg, uint32_t colour, int scale)
{
    const int tw = static_cast<int>(std::strlen(msg)) * 6 * scale;
    text(p, w, h, stride, (w - tw) / 2, cy, msg, colour, scale);
}
}

void Ps5PauseUI::draw(uint32_t* dst, int width, int height, int stride,
                      uint32_t panel, uint32_t heading, uint32_t highlight,
                      uint32_t white, uint32_t muted, uint32_t black) const
{
    if (!active_ || !dst || width < 200 || height < 150 || stride < width) return;

    // Dim the existing OutRun frame without changing simulation or ROM assets.
    // Works for both ARGB8888 and ABGR8888 (preserves alpha in high byte).
    for (int y = 0; y < height; ++y) {
        uint32_t* row = dst + y * stride;
        for (int x = 0; x < width; ++x) {
            const uint32_t c = row[x];
            row[x] = (c & 0xff000000u) | ((c & 0x00fefefeu) >> 1);
        }
    }

    const int scale = width >= 640 ? 2 : 1;
    const int pw = width > 370 ? 310 : width - 28;
    const int ph = height > 260 ? 190 : 166;
    const int x = (width - pw) / 2;
    const int y = (height - ph) / 2;
    rect(dst, width, height, stride, x - 2, y - 2, pw + 4, ph + 4, black);
    rect(dst, width, height, stride, x, y, pw, ph, panel);
    rect(dst, width, height, stride, x, y, pw, 30, heading);
    centered(dst, width, height, stride, y + 10, "PAUSED", white, scale);

    if (screen_ == MAIN) {
        const char* items[] = {
            "RESUME GAME", "SETTINGS [BETA 1B]", "RESTART GAME", "RETURN TO MAIN MENU"
        };
        const int base = y + 47;
        for (int i = 0; i < 4; ++i) {
            const int row = base + i * 23;
            if (i == selected_) rect(dst, width, height, stride,
                                     x + 11, row - 5, pw - 22, 19, highlight);
            centered(dst, width, height, stride, row, items[i],
                     (i == 1 ? muted : white), scale);
        }
        centered(dst, width, height, stride, y + ph - 17,
                 "TOUCHPAD: RESUME   X: SELECT", muted, 1);
    } else if (screen_ == SETTINGS_NOTICE) {
        centered(dst, width, height, stride, y + 57,
                 "SETTINGS", white, scale);
        centered(dst, width, height, stride, y + 82,
                 "COMING IN BETA 1B", muted, 1);
        centered(dst, width, height, stride, y + 116,
                 "X / O: BACK", white, 1);
    } else {
        centered(dst, width, height, stride, y + 56,
                 screen_ == CONFIRM_RESTART ? "RESTART GAME?" : "RETURN TO MENU?",
                 white, scale);
        centered(dst, width, height, stride, y + 79,
                 "UNSAVED PROGRESS WILL BE LOST", muted, 1);
        const int bw = 65;
        const int by = y + 105;
        rect(dst, width, height, stride, width / 2 - 76, by, bw, 22,
             confirm_yes_ ? highlight : heading);
        rect(dst, width, height, stride, width / 2 + 11, by, bw, 22,
             confirm_yes_ ? heading : highlight);
        text(dst, width, height, stride, width / 2 - 52, by + 8,
             "YES", white, 1);
        text(dst, width, height, stride, width / 2 + 36, by + 8,
             "NO", white, 1);
        centered(dst, width, height, stride, y + ph - 17,
                 "X: CONFIRM   O: BACK", muted, 1);
    }
}

#endif // CANNONBALL_PS5_BETA1B
