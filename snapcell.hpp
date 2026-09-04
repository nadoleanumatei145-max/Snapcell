//     ▗▄▄▖▗▖  ▗▖ ▗▄▖ ▗▄▄▖  ▗▄▄▖▗▄▄▄▖▗▖   ▗▖
//    ▐▌   ▐▛▚▖▐▌▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌   ▐▌   ▐▌
//     ▝▀▚▖▐▌ ▝▜▌▐▛▀▜▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌   ▐▌
//    ▗▄▄▞▘▐▌  ▐▌▐▌ ▐▌▐▌   ▝▚▄▄▖▐▙▄▄▖▐▙▄▄▖▐▙▄▄▖

#ifndef SNAPCELL_LIBRARY_HPP
#define SNAPCELL_LIBRARY_HPP

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <csignal>
#include <atomic>
#include <memory>
#include <cstdint>
#include <cctype>
#include <chrono>
#include <optional>
#include <fstream>
#include <cstdlib>
#include <webp/decode.h>
// Include stb_image direct aici
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace tui {
    // A global flag for the Linux kernel resize signal
    inline std::atomic<bool> global_screen_resized(false);
    inline void handle_resize_signal(int) { global_screen_resized.store(true); }

    //      ▗       ▗   ▗
    //    ▛▘▜▘▛▘▌▌▛▘▜▘  ▚▘  █▌▛▌▌▌▛▛▌▛▘
    //    ▄▌▐▖▌ ▙▌▙▖▐▖  ▚▌  ▙▖▌▌▙▌▌▌▌▄▌
    //

    enum class MouseButton {
        None,
        Left,
        Middle,
        Right,
        WheelUp,
        WheelDown
    };

    enum class MouseEventType {
        Press,
        Release,
        Move
    };

    struct MouseEvent {
        MouseEventType type = MouseEventType::Press;
        MouseButton button = MouseButton::None;
        int x = 0; // Coordonate 0-indexed pe grila TTY
        int y = 0;
        bool ctrl = false;
        bool alt = false;
        bool shift = false;

        explicit operator bool() const { return button != MouseButton::None || type == MouseEventType::Move; }
    };

    namespace key {
        // Taste speciale neafișabile
        inline constexpr uint32_t esc       = 27;
        inline constexpr uint32_t enter     = 10;
        inline constexpr uint32_t tab       = 9;
        inline constexpr uint32_t backspace = 127;
        inline constexpr uint32_t space     = 32;

        inline constexpr uint32_t up        = 1000;
        inline constexpr uint32_t down      = 1001;
        inline constexpr uint32_t right     = 1002;
        inline constexpr uint32_t left      = 1003;
        inline constexpr uint32_t home      = 1004;
        inline constexpr uint32_t end       = 1005;
        inline constexpr uint32_t delete_k  = 1006;
        inline constexpr uint32_t page_up   = 1007;
        inline constexpr uint32_t page_down = 1008;

        // F1 - F12
        inline constexpr uint32_t f1        = 1101;
        inline constexpr uint32_t f2        = 1102;
        inline constexpr uint32_t f3        = 1103;
        inline constexpr uint32_t f4        = 1104;
        inline constexpr uint32_t f5        = 1105;
        inline constexpr uint32_t f6        = 1106;
        inline constexpr uint32_t f7        = 1107;
        inline constexpr uint32_t f8        = 1108;
        inline constexpr uint32_t f9        = 1109;
        inline constexpr uint32_t f10       = 1110;
        inline constexpr uint32_t f11       = 1111;
        inline constexpr uint32_t f12       = 1112;
    }

    struct Key {
        uint32_t code = 0; // ASCII ('a', 'X', '1') SAU constantele key::*
        bool ctrl  = false;
        bool alt   = false;
        bool shift = false;

        // Verificări directe
        explicit operator bool() const { return code != 0; }

        // Suport comparare curată: if (k == 'q') sau if (k == key::up)
        bool operator==(char ch) const {
            return code == static_cast<uint32_t>(ch);
        }
        bool operator==(uint32_t key_code) const {
            return code == key_code;
        }

        bool is_char() const {
            return code >= 32 && code <= 126 && !ctrl && !alt;
        }
        char to_char() const { return static_cast<char>(code); }
    };

    struct Event {
        enum Type { None, KeyEv, MouseEv } type = None;
        Key key;
        MouseEvent mouse;
    };

    struct WidthMod {
        int value;
    };

    // Helper function pentru o sintaxă curată (lowercase)
    inline WidthMod width(int width) {
        return WidthMod{width};
    }

    struct HeightMod {
        int value;
    };

    inline HeightMod height(int height) {
        return HeightMod{height};
    }

    struct XMod {
        int value;
    };

    inline XMod x(int x) {
        return XMod{x};
    }

    struct YMod {
        int value;
    };

    inline YMod y(int y) {
        return YMod{y};
    }

    enum class ColorMode {
        TrueColor,  // Randare nativă RGB pixel cu pixel
        Solid,      // Culoare unică fixă peste tot (fără variație de luminozitate)
        Tint        // Culoare unică nuanțată (intensitatea depinde de luminozitatea pixelului)
    };

    enum Fill {
        nofill = 0,
        hfill,
        vfill,
        fill
    };

    enum Anchor {
        noanchor = 0,
        left     = 1 << 0, // 1
        hcenter  = 1 << 1, // 2
        right    = 1 << 2, // 4
        top      = 1 << 3, // 8
        vcenter  = 1 << 4, // 16
        bottom   = 1 << 5, // 32
        center   = hcenter | vcenter // 18
    };

    inline Anchor operator|(Anchor a, Anchor b) {
        return static_cast<Anchor>(static_cast<int>(a) | static_cast<int>(b));
    }

    enum Colors {
        black = 0,
        dark_red,
        dark_green,
        dark_yellow,
        dark_blue,
        dark_magenta,
        dark_cyan,
        light_grey,
        dark_gray,
        red,
        green,
        yellow,
        blue,
        magenta,
        cyan,
        white
    };

    enum Style {
        normal = 0,
        bold = 1,
        italic    = 3,
        underline = 4
    };

    enum Mod {
        border
    };

    struct Cell {
        std::string ch = " ";
        int fg = 15;        // Default text color (White)
        int bg = -1;         // Default background color (Black)
        int style = normal; // Default text format style

        // Suport TrueColor (RGB)
        bool is_rgb = false;
        uint8_t r = 255, g = 255, b = 255;
    };

    struct bg_color { Colors c; };

    struct ColorConfig {
        ColorMode mode = ColorMode::TrueColor;
        Colors solid_color = Colors::white; // Culoare ANSI fixă (pentru Solid)
        struct RGB { uint8_t r, g, b; } tint_color{255, 255, 255}; // Culoare RGB (pentru Tint)
    };

    //                 ▘▗
    //    ▛▘▛▌▛▛▌▛▌▛▌▛▘▌▜▘▛▌▛▘
    //    ▙▖▙▌▌▌▌▙▌▙▌▄▌▌▐▖▙▌▌
    //           ▌

    inline bool is_native_tty() {
        const char* term = std::getenv("TERM");
        if (!term) return false;
        std::string t(term);
        return (t == "linux" || t.rfind("tty", 0) == 0);
    }

    class compositor {
    private:
        int width = 0;
        int height = 0;
        struct termios orig_termios;

        std::vector<Cell> front_buffer;
        std::vector<Cell> back_buffer;

        // Internal tool: Asks Linux how big the physical TTY screen is
        void updateDimensions() {
            struct winsize w;
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
                width = w.ws_col;
                height = w.ws_row;
            } else {
                width = 80; height = 24; // Fallback
            }
            front_buffer.assign(width * height, {" "});
            back_buffer.assign(width * height, {" "});
            std::cout << "\033[2J"; // Clear physical screen on resize
        }

        // Internal tool: Moves terminal cursor instantly using ANSI escape codes
        void moveCursor(int x, int y) {
            std::cout << "\033[" << (y + 1) << ";" << (x + 1) << "H";
        }

        std::vector<char> input_buffer;

        // Citește tot ce este în STDIN în bufferul intern
        void read_stdin_to_buffer() {
            char chunk[256];
            while (true) {
                ssize_t n = read(STDIN_FILENO, chunk, sizeof(chunk));
                if (n <= 0) break;
                input_buffer.insert(input_buffer.end(), chunk, chunk + n);
            }
        }

        char pop_byte() {
            if (input_buffer.empty()) return 0;
            char b = input_buffer.front();
            input_buffer.erase(input_buffer.begin());
            return b;
        }

        char peek_byte(size_t offset = 0) {
            if (offset >= input_buffer.size()) return 0;
            return input_buffer[offset];
        }

        std::vector<std::string> ascii_palette = {" ", ".", ":", "*", "#", "%", "@"};

        // Funcție privată ajutătoare pentru parsarea corectă a secvențelor UTF-8
        static std::vector<std::string> parse_utf8_string(const std::string& str) {
            std::vector<std::string> chars;
            for (size_t i = 0; i < str.size(); ) {
                unsigned char c = str[i];
                int len = 1;
                if ((c & 0x80) == 0x00) len = 1;      // Caracter ASCII (1 octet)
                else if ((c & 0xE0) == 0xC0) len = 2; // UTF-8 (2 octeți)
                else if ((c & 0xF0) == 0xE0) len = 3; // UTF-8 (3 octeți - ex: ░, ▒, ▓, █)
                else if ((c & 0xF8) == 0xF0) len = 4; // UTF-8 (4 octeți)

                chars.push_back(str.substr(i, len));
                i += len;
            }
            return chars;
        }

        // Funcție internă de asistență pentru maparea pixelilor
        void render_pixel_buffer(const uint8_t* pixels, int img_w, int img_h, int channels,
                        int x, int y, int w, int h,
                        const ColorConfig& config)
{
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            int target_x = x + col;
            int target_y = y + row;

            if (target_x < 0 || target_x >= width || target_y < 0 || target_y >= height)
                continue;

            int src_x = (col * img_w) / w;
            int src_y = (row * img_h) / h;
            int idx = (src_y * img_w + src_x) * channels;

            uint8_t r = pixels[idx];
            uint8_t g = pixels[idx + 1];
            uint8_t b = pixels[idx + 2];
            uint8_t a = (channels == 4) ? pixels[idx + 3] : 255;

            if (a < 128) continue; // Skip pixeli transparenți

            uint8_t lum = static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b);

            size_t pal_idx = (static_cast<size_t>(lum) * (ascii_palette.size() - 1)) / 255;
            if (pal_idx >= ascii_palette.size()) pal_idx = ascii_palette.size() - 1;

            const std::string& ch = ascii_palette[pal_idx];

            // RENDER BAZAT PE MODUL ALES:
            switch (config.mode) {
                case ColorMode::Solid:
                    // Aceeași culoare fixă peste tot
                    setChar(target_x, target_y, ch, config.solid_color);
                    break;

                case ColorMode::Tint: {
                    // Scalăm nuanța target în funcție de luminozitate (0.0f - 1.0f)
                    float factor = lum / 255.0f;
                    uint8_t tr = static_cast<uint8_t>(config.tint_color.r * factor);
                    uint8_t tg = static_cast<uint8_t>(config.tint_color.g * factor);
                    uint8_t tb = static_cast<uint8_t>(config.tint_color.b * factor);

                    setCharRGB(target_x, target_y, ch, tr, tg, tb);
                    break;
                }

                case ColorMode::TrueColor:
                default:
                    // Culorile originale ale imaginii
                    setCharRGB(target_x, target_y, ch, r, g, b);
                    break;
            }
        }
    }
}

        // Stochează o culoare opțională unică pentru tot procesul de desenare
        std::optional<Colors> active_override_color = std::nullopt;

        ColorConfig current_color_config;

    public:
        compositor() {
            // 1. Turn on Raw Mode (Instant keypresses, hide typing)
            tcgetattr(STDIN_FILENO, &orig_termios);
            struct termios raw = orig_termios;
            raw.c_lflag &= ~ICANON;
            raw.c_lflag &= ~ECHO;
            raw.c_cc[VMIN] = 0;  // Non-blocking input reading
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

            // 2. Listen for Window Changes
            std::signal(SIGWINCH, handle_resize_signal);

            // 3. Detect initial size & prepare buffers
            updateDimensions();
            std::cout << "\033[?25l"; // Hide the blinking cursor for clean rendering

            std::cout << "\033[?1000h\033[?1002h\033[?1006h";
            std::cout.flush();

            updateDimensions();
            std::cout << "\033[?25l";
        }

        ~compositor() {
            // Cleanup: Restore original terminal when program closes!
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            std::cout << "\033[?25h\033[2J\033[H"; // Show cursor & clear screen
        }

        // Getters so your widgets know the boundaries
        int getWidth() const { return width; }
        int getHeight() const { return height; }

        // Setter flexibil care acceptă atât std::string (" ░▒▓█"), cât și std::vector<std::string>
        compositor& set_ascii_palette(const std::string& palette) {
            if (!palette.empty()) {
                ascii_palette = parse_utf8_string(palette);
            }
            return *this;
        }

        compositor& set_ascii_palette(const std::vector<std::string>& palette) {
            if (!palette.empty()) {
                ascii_palette = palette;
            }
            return *this;
        }

        // --- YOUR CLEAN CUSTOM SYNTAX API ---

        void setChar(int x, int y, std::string ch, int fg = 15, int bg = 0, int style = normal) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                int idx = y * width + x;
                back_buffer[idx].ch = ch;
                back_buffer[idx].fg = fg;
                back_buffer[idx].bg = bg;
                back_buffer[idx].style = style;
            }
        }

        void setString(int x, int y, const std::string& text, int fg = 15, int bg = 0, int style = normal) {
            size_t i = 0;
            int screen_offset = 0;

            while (i < text.length()) {
                unsigned char c = text[i];
                size_t len = 1;

                // Determine how many bytes this UTF-8 character takes up
                if ((c & 0x80) == 0)         len = 1; // Standard ASCII
                else if ((c & 0xE0) == 0xC0) len = 2; // 2-byte UTF-8
                else if ((c & 0xF0) == 0xE0) len = 3; // 3-byte UTF-8 (Like box-drawing arrows/lines)
                else if ((c & 0xF8) == 0xF0) len = 4; // 4-byte UTF-8 (Like Emojis)

                // Safety check to ensure we don't read past the end of a malformed string
                if (i + len > text.length()) break;

                // Extract the exact multi-byte character sequence
                std::string utf8_char = text.substr(i, len);

                // Place it into one single terminal layout cell coordinate
                setChar(x + screen_offset, y, utf8_char, fg, bg, style);

                i += len;            // Skip ahead in memory by the byte length
                screen_offset++;     // Move exactly 1 visual space horizontally on screen
            }
        }

        void setCharRGB(int x, int y, std::string ch, uint8_t r, uint8_t g, uint8_t b) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                int idx = y * width + x;
                back_buffer[idx].ch = ch;
                back_buffer[idx].is_rgb = true;
                back_buffer[idx].r = r;
                back_buffer[idx].g = g;
                back_buffer[idx].b = b;
                back_buffer[idx].bg = -1;
                back_buffer[idx].style = normal;
            }
        }

        // 1. Modul True Color (implicit)
        compositor& use_true_color() {
            current_color_config.mode = ColorMode::TrueColor;
            return *this;
        }

        // 2. Modul Solid (aceeași culoare peste tot)
        compositor& set_solid_color(Colors color) {
            current_color_config.mode = ColorMode::Solid;
            current_color_config.solid_color = color;
            return *this;
        }

        // 3. Modul Tint (nuanțare în funcție de luminozitate)
        compositor& set_tint_color(uint8_t r, uint8_t g, uint8_t b) {
            current_color_config.mode = ColorMode::Tint;
            current_color_config.tint_color = {r, g, b};
            return *this;
        }

        // Curățarea culorii unice (revenire la True RGB)
        compositor& reset_color() {
            active_override_color = std::nullopt;
            return *this;
        }

        Key getKey() {
            read_stdin_to_buffer();
            if (input_buffer.empty()) return Key{};

            char b = pop_byte();
            Key k;

            // 1. Detecție Ctrl + A ... Ctrl + Z (Valori ASCII 1..26)
            if (b >= 1 && b <= 26 && b != 9 && b != 10 && b != 13) {
                k.code = 'a' + (b - 1);
                k.ctrl = true;
                return k;
            }

            // 2. Parsare secvențe lungi ANSI (ESC / Alt)
            if (b == 27) { // ESC
                if (input_buffer.empty()) {
                    // Tasta ESC apăsată singură
                    k.code = key::esc;
                    return k;
                }

                char next = peek_byte(0);

                // Alt + Key (Secvență de tip ESC + caracter)
                if (next != '[' && next != 'O') {
                    k.alt = true;
                    char alt_ch = pop_byte();
                    if (alt_ch >= 1 && alt_ch <= 26) {
                        k.ctrl = true;
                        k.code = 'a' + (alt_ch - 1);
                    } else {
                        k.code = static_cast<uint32_t>(alt_ch);
                    }
                    return k;
                }

                // Secvență ANSI CSI (ESC [ ...)
                if (next == '[') {
                    pop_byte(); // Consumăm '['

                    std::string seq;
                    while (!input_buffer.empty()) {
                        char c = pop_byte();
                        seq += c;
                        // Secvențele CSI se termină într-un caracter final (A-Z, a-z, ~)
                        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '~') {
                            break;
                        }
                    }

                    // Extragere modificatori dacă există (ex: "1;5A" -> Ctrl + Up)
                    int modifier = 1;
                    size_t semi = seq.find(';');
                    if (semi != std::string::npos && semi + 1 < seq.size()) {
                        modifier = seq[semi + 1] - '0';
                    }

                    if (modifier == 2 || modifier == 4 || modifier == 6 || modifier == 8) k.shift = true;
                    if (modifier == 3 || modifier == 4 || modifier == 7 || modifier == 8) k.alt   = true;
                    if (modifier == 5 || modifier == 6 || modifier == 7 || modifier == 8) k.ctrl  = true;

                    char final_char = seq.back();

                    // Săgeți
                    if (final_char == 'A') { k.code = key::up; return k; }
                    if (final_char == 'B') { k.code = key::down; return k; }
                    if (final_char == 'C') { k.code = key::right; return k; }
                    if (final_char == 'D') { k.code = key::left; return k; }
                    if (final_char == 'H') { k.code = key::home; return k; }
                    if (final_char == 'F') { k.code = key::end; return k; }

                    // Coduri numerice cu '~' (ex: Delete, PageUp, F-keys)
                    if (final_char == '~') {
                        int num = std::atoi(seq.c_str());
                        switch (num) {
                            case 3:  k.code = key::delete_k; break;
                            case 5:  k.code = key::page_up; break;
                            case 6:  k.code = key::page_down; break;
                            case 15: k.code = key::f5; break;
                            case 17: k.code = key::f6; break;
                            case 18: k.code = key::f7; break;
                            case 19: k.code = key::f8; break;
                            case 20: k.code = key::f9; break;
                            case 21: k.code = key::f10; break;
                            case 23: k.code = key::f11; break;
                            case 24: k.code = key::f12; break;
                        }
                        return k;
                    }
                }

                // Format VT100 SS3 (ESC O P / Q / R / S pentru F1-F4)
                if (next == 'O') {
                    pop_byte(); // Consumăm 'O'
                    char code_char = pop_byte();
                    switch (code_char) {
                        case 'P': k.code = key::f1; break;
                        case 'Q': k.code = key::f2; break;
                        case 'R': k.code = key::f3; break;
                        case 'S': k.code = key::f4; break;
                    }
                    return k;
                }
            }

            // 3. Caractere normale / Enter / Backspace / Tab
            if (b == 13 || b == 10) k.code = key::enter;
            else if (b == 127 || b == 8) k.code = key::backspace;
            else if (b == 9) k.code = key::tab;
            else k.code = static_cast<unsigned char>(b);

            return k;
        }

        Event getEvent() {
        read_stdin_to_buffer();
        if (input_buffer.empty()) return Event{};

        // 1. Verificăm dacă secvența din buffer este de Mouse SGR: ESC [ < ...
        if (peek_byte(0) == 27 && peek_byte(1) == '[' && peek_byte(2) == '<') {
            // Căutăm 'm' sau 'M' care marchează finalul pachetului SGR
            size_t end_pos = 0;
            for (size_t i = 3; i < input_buffer.size(); ++i) {
                if (input_buffer[i] == 'm' || input_buffer[i] == 'M') {
                    end_pos = i;
                    break;
                }
            }

            if (end_pos > 0) {
                // Extragere secvență completă
                std::string seq(input_buffer.begin() + 3, input_buffer.begin() + end_pos);
                char action = input_buffer[end_pos]; // 'M' = Press/Move, 'm' = Release

                // Consumăm octeții din buffer
                input_buffer.erase(input_buffer.begin(), input_buffer.begin() + end_pos + 1);

                int btn_code = 0, px = 0, py = 0;
                if (sscanf(seq.c_str(), "%d;%d;%d", &btn_code, &px, &py) == 3) {
                    Event ev;
                    ev.type = Event::MouseEv;
                    ev.mouse.x = px - 1; // Conversie de la 1-based (ANSI) la 0-based
                    ev.mouse.y = py - 1;

                    // Extragere modificatori
                    if (btn_code & 4)   ev.mouse.shift = true;
                    if (btn_code & 8)   ev.mouse.alt = true;
                    if (btn_code & 16)  ev.mouse.ctrl = true;

                    int base_btn = btn_code & 67; // Izolăm tipul de buton/acțiune

                    if (base_btn == 64) {
                        ev.mouse.button = MouseButton::WheelUp;
                        ev.mouse.type = MouseEventType::Press;
                    } else if (base_btn == 65) {
                        ev.mouse.button = MouseButton::WheelDown;
                        ev.mouse.type = MouseEventType::Press;
                    } else if (btn_code & 32) { // Motion / Move
                        ev.mouse.type = MouseEventType::Move;
                        int b = btn_code & 3;
                        if (b == 0) ev.mouse.button = MouseButton::Left;
                        else if (b == 1) ev.mouse.button = MouseButton::Middle;
                        else if (b == 2) ev.mouse.button = MouseButton::Right;
                    } else { // Click standard
                        int b = btn_code & 3;
                        if (b == 0) ev.mouse.button = MouseButton::Left;
                        else if (b == 1) ev.mouse.button = MouseButton::Middle;
                        else if (b == 2) ev.mouse.button = MouseButton::Right;

                        ev.mouse.type = (action == 'M') ? MouseEventType::Press : MouseEventType::Release;
                    }
                    return ev;
                }
            }
        }

        // 2. Fallback pe citirea standard de tastatură
        Key k = getKey();
        if (k) {
            Event ev;
            ev.type = Event::KeyEv;
            ev.key = k;
            return ev;
        }

        return Event{};
    }

        void clear() {
            // Wipe the hidden back buffer
            for (auto& cell : back_buffer) cell.ch = ' ';
        }

        void display() {
            if (global_screen_resized.load()) {
                global_screen_resized.store(false);
                updateDimensions();
            }

            int last_fg = -1;
            int last_bg = -1;
            int last_style = -1;

            // Tracking pentru starea RGB
            bool last_was_rgb = false;
            uint8_t last_r = 0, last_g = 0, last_b = 0;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = y * width + x;

                    const auto& back = back_buffer[idx];
                    auto& front = front_buffer[idx];

                    // 1. Smart Diff-Rendering: verificare dacă s-a schimbat caracterul, tipul de culoare sau valorile
                    bool changed = (back.ch != front.ch) ||
                                   (back.is_rgb != front.is_rgb) ||
                                   (back.style != front.style);

                    if (back.is_rgb) {
                        changed = changed || (back.r != front.r) || (back.g != front.g) || (back.b != front.b);
                    } else {
                        changed = changed || (back.fg != front.fg) || (back.bg != front.bg);
                    }

                    if (changed) {
                        moveCursor(x, y);

                        // 2. Evaluare dacă stilul/culoarea trebuie re-emise
                        bool style_changed = false;

                        if (back.is_rgb) {
                            // Dacă celula curentă e RGB, re-emitem dacă înainte nu era RGB, dacă s-a schimba stilul sau componentele RGB
                            if (!last_was_rgb || back.style != last_style ||
                                back.r != last_r || back.g != last_g || back.b != last_b) {

                                std::cout << "\033[" << back.style << ";38;2;"
                                          << static_cast<int>(back.r) << ";"
                                          << static_cast<int>(back.g) << ";"
                                          << static_cast<int>(back.b);

                                if (back.bg >= 0) {
                                    std::cout << ";48;5;" << back.bg;
                                } else {
                                    std::cout << ";49";
                                }
                                std::cout << "m";

                                last_was_rgb = true;
                                last_r = back.r;
                                last_g = back.g;
                                last_b = back.b;
                                last_style = back.style;
                                last_bg = back.bg;
                            }
                        } else {
                            // Cale standard ANSI 256 / culori de bază
                            if (last_was_rgb || back.fg != last_fg || back.bg != last_bg || back.style != last_style) {
                                std::cout << "\033[" << back.style << ";38;5;" << back.fg;

                                if (back.bg >= 0) {
                                    std::cout << ";48;5;" << back.bg;
                                } else {
                                    std::cout << ";49";
                                }
                                std::cout << "m";

                                last_was_rgb = false;
                                last_fg = back.fg;
                                last_bg = back.bg;
                                last_style = back.style;
                            }
                        }

                        std::cout << back.ch;
                        front = back; // Sincronizare buffer
                    }
                }
            }

            // Curățare atribute la finalul cadrului
            std::cout << "\033[0m";
            std::cout.flush();
        }

        void delay_fps(int fps) {
            if (fps <= 0) fps = 60; // Fallback safety
            // Math formula: 1,000,000 microseconds / Frames Per Second
            long microseconds = 1000000 / fps;
            usleep(microseconds);
        }

        // --- FUNCTIA PRINCIPALA SI SIMPLA ---
        // Poți transmite fie o imagine statică (PNG/JPG/WebP), fie un GIF animat.
        // Daca nu treci 'color', va folosi culorile naturale.
        bool draw_image(const std::string& filepath, int x, int y, int w, int h) {
            return draw_image(filepath, x, y, w, h, this->current_color_config);
        }

        bool draw_image(const std::string& filepath, int x, int y, int w, int h, const ColorConfig& config) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(file_size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), file_size)) {
            return false;
        }

        int img_w = 0, img_h = 0, frames = 0, channels = 0;
        int* delays = nullptr;

        // 1. GIF Animat
        uint8_t* gif_data = static_cast<uint8_t*>(
            stbi_load_gif_from_memory(buffer.data(), static_cast<int>(buffer.size()),
                                      &delays, &img_w, &img_h, &frames, &channels, 4)
        );

        if (gif_data) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();

            int current_frame = (ms / 100) % frames;
            int frame_stride = img_w * img_h * 4;

            render_pixel_buffer(gif_data + (current_frame * frame_stride), img_w, img_h, 4, x, y, w, h, config);
            stbi_image_free(gif_data);
            if (delays) STBI_FREE(delays);
            return true;
        }

        // 2. PNG / JPG (stb_image)
        uint8_t* img_data = static_cast<uint8_t*>(
            stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()), &img_w, &img_h, &channels, 4)
        );

        if (img_data) {
            render_pixel_buffer(img_data, img_w, img_h, 4, x, y, w, h, config);
            stbi_image_free(img_data);
            return true;
        }

        // 3. Fallback WebP (libwebp)
        uint8_t* webp_data = WebPDecodeRGBA(buffer.data(), buffer.size(), &img_w, &img_h);
        if (webp_data) {
            render_pixel_buffer(webp_data, img_w, img_h, 4, x, y, w, h, config);
            WebPFree(webp_data);
            return true;
        }

        return false;
    }
    };

    struct Box {
        int x, y;
    };

    //      ▜          ▗
    //    █▌▐ █▌▛▛▌█▌▛▌▜▘▛▘
    //    ▙▖▐▖▙▖▌▌▌▙▖▌▌▐▖▄▌
    //

    // Calcularea numărului real de coloane vizuale
    inline size_t utf8_cols(const std::string& text) {
        size_t i = 0, cols = 0;
        while (i < text.length()) {
            unsigned char c = text[i];
            size_t len = 1;
            if ((c & 0x80) == 0)          len = 1;
            else if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;

            if (i + len > text.length()) break;
            i += len;
            cols++;
        }
        return cols;
    }

    // Tăierea sigură la nivel de caractere/coloane vizuale
    inline std::string utf8_substr(const std::string& text, size_t max_cols) {
        size_t i = 0, cols = 0;
        while (i < text.length() && cols < max_cols) {
            unsigned char c = text[i];
            size_t len = 1;
            if ((c & 0x80) == 0)          len = 1;
            else if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;

            if (i + len > text.length()) break;
            i += len;
            cols++;
        }
        return text.substr(0, i);
    }

    // The base class for EVERYTHING in your UI
    class Element : public std::enable_shared_from_this<Element> {
    public:
        int pos_x = 0, pos_y = 0, width = 0, height = 0, allocated_width = 0, allocated_height = 0;

        Style style = normal;
        Colors bg = black, fg = white;

        Fill fill_mode = nofill;
        Anchor anchor_mode = noanchor;

        bool bordered = false;

        // Referință slabă către părinte (nu crește reference count-ul)
        std::weak_ptr<Element> parent;

        virtual ~Element() = default;

        // Helper util pentru copii: returnează părintele sub formă de shared_ptr dacă încă există
        std::shared_ptr<Element> getParent() const {
            return parent.lock();
        }

        virtual Box getGeometry() { return { width, height }; }
        virtual Box getPosition() { return { pos_x, pos_y }; }

        Box getAnchorOffset() const {
            int eff_w = (allocated_width > 0) ? allocated_width : width;
            int eff_h = (allocated_height > 0) ? allocated_height : height;

            int ox = 0, oy = 0;
            int mode = static_cast<int>(anchor_mode);

            if (mode & hcenter) ox = (eff_w - width) / 2;
            else if (mode & right) ox = eff_w - width;

            if (mode & vcenter) oy = (eff_h - height) / 2;
            else if (mode & bottom) oy = eff_h - height;

            return { std::max(0, ox), std::max(0, oy) };
        }

        void renderBorder(compositor& tui, int rx, int ry, int rw, int rh) {
            if (rw < 2 || rh < 2) return;

            tui.setString(rx, ry, "┌", fg, bg, style);
            tui.setString(rx + rw - 1, ry, "┐", fg, bg, style);
            tui.setString(rx, ry + rh - 1, "└", fg, bg, style);
            tui.setString(rx + rw - 1, ry + rh - 1, "┘", fg, bg, style);

            for (int i = 1; i < rw - 1; ++i) {
                tui.setString(rx + i, ry, "─", fg, bg, style);
                tui.setString(rx + i, ry + rh - 1, "─", fg, bg, style);
            }

            for (int i = 1; i < rh - 1; ++i) {
                tui.setString(rx, ry + i, "│", fg, bg, style);
                tui.setString(rx + rw - 1, ry + i, "│", fg, bg, style);
            }
        }

        virtual void render(compositor& tui) = 0;
    };

    // A smart pointer alias to make syntax cleaner
    using element = std::shared_ptr<Element>;

    class Border : public Element {
    private:
        element child;
    public:
        Border(element child_node) : child(child_node) {}

        // A border widget needs 2 extra columns and 2 extra rows than its child!
        Box getGeometry() override {
            Box b = child->getGeometry();
            return { b.x + 2, b.y + 2 };
        }

        void render(compositor& tui) override {
            if (width < 2 || height < 2) return; // Too small to show anything

            // 1. Draw the beautiful frame borders using the screen size assigned to us
            tui.setString(pos_x, pos_y, "┌");
            tui.setString(pos_x + width - 1, pos_y, "┐");
            tui.setString(pos_x, pos_y + height - 1, "└");
            tui.setString(pos_x + width - 1, pos_y + height - 1, "┘");

            // Top and Bottom horizontal bars
            for (int i = 1; i < width - 1; ++i) {
                tui.setString(pos_x + i, pos_y, "─");
                tui.setString(pos_x + i, pos_y + height - 1, "─");
            }

            // Left and Right vertical sidebars
            for (int i = 1; i < height - 1; ++i) {
                tui.setString(pos_x, pos_y + i, "│");
                tui.setString(pos_x + width - 1, pos_y + i, "│");
            }

            // 2. Transmite coordonate absolute corecte pentru child fără să le acumulezi la infinit
            child->pos_x = this->pos_x + 1;
            child->pos_y = this->pos_y + 1;
            child->width = this->width - 2;
            child->height = this->height - 2;
            child->render(tui);
        }
    };

    class Text : public Element {
    public:
        std::string content;

        Text(std::string text) : content(text) {
            width = (int)utf8_cols(content);
            height = 1;
        }

        void render(compositor& tui) override {
            int eff_w = (allocated_width > 0) ? allocated_width : width;
            int eff_h = (allocated_height > 0) ? allocated_height : height;

            int render_x = pos_x;
            int render_y = pos_y;
            int text_max_w = eff_w;

            if (bordered) {
                renderBorder(tui, pos_x, pos_y, eff_w, eff_h);
                render_x += 1;
                render_y += 1;
                text_max_w = std::max(0, eff_w - 2);
            }

            std::string visible = utf8_substr(content, text_max_w);
            Box offset = getAnchorOffset();
            tui.setString(render_x + offset.x, render_y + offset.y, visible, fg, bg, style);
        }
    };

    // Helper function to create the clean FTXUI syntax
    inline element text(std::string text) {
        return std::make_shared<Text>(text);
    }

    class VBox : public Element {
    private:
        std::vector<element> children;
    public:
        VBox(std::initializer_list<element> list) : children(list) {}

        Box getGeometry() override {
            int max_w = 0;
            int total_h = 0;
            for (auto& child : children) {
                if (!child) continue;
                Box b = child->getGeometry();
                if (b.x > max_w) max_w = b.x;
                total_h += b.y;
            }
            // Dacă VBox are border, crește spațiul minim cerut!
            if (bordered) {
                max_w += 2;
                total_h += 2;
            }
            return { max_w, total_h };
        }

        void render(compositor& tui) override {
            if (allocated_width > 0)  width = allocated_width;
            if (allocated_height > 0) height = allocated_height;

            int pad = bordered ? 1 : 0;

            if (bordered) {
                renderBorder(tui, pos_x, pos_y, width, height);
            }

            // Calculăm spațiul util din interiorul VBox-ului
            int inner_x = pos_x + pad;
            int inner_y = pos_y + pad;
            int inner_w = std::max(0, width - pad * 2);
            int inner_h = std::max(0, height - pad * 2);

            int vfill_count = 0;
            int fixed_height_sum = 0;

            for (auto& child : children) {
                if (!child) continue;
                child->parent = shared_from_this();

                bool has_vfill = (child->fill_mode == vfill || child->fill_mode == fill);
                if (has_vfill) {
                    vfill_count++;
                } else {
                    fixed_height_sum += child->getGeometry().y;
                }
            }

            int available_space = std::max(0, inner_h - fixed_height_sum);
            int height_per_fill = (vfill_count > 0) ? (available_space / vfill_count) : 0;
            int remainder = (vfill_count > 0) ? (available_space % vfill_count) : 0;

            int current_y = inner_y;

            for (auto& child : children) {
                if (!child) continue;

                bool has_vfill = (child->fill_mode == Fill::vfill || child->fill_mode == Fill::fill);
                int child_h = has_vfill ? (height_per_fill + (remainder-- > 0 ? 1 : 0))
                                        : child->getGeometry().y;

                int max_allowed_h = (inner_y + inner_h) - current_y;
                if (max_allowed_h <= 0) break;
                child_h = std::min(child_h, max_allowed_h);

                child->pos_x = inner_x;
                child->pos_y = current_y;
                child->allocated_height = child_h;

                if (child->fill_mode == fill || child->fill_mode == hfill) {
                    child->allocated_width = inner_w;
                } else {
                    child->allocated_width = child->width;
                }

                child->render(tui);
                current_y += child_h;
            }
        }
        };

    // Helper function for the syntax
    inline element vbox(std::initializer_list<element> children) {
        return std::make_shared<VBox>(children);
    }

    class HBox : public Element {
    private:
        std::vector<element> children;
    public:
        HBox(std::initializer_list<element> list) : children(list) {}

        Box getGeometry() override {
            int total_w = 0;
            int max_h = 0;
            for (auto& child : children) {
                if (!child) continue;
                Box b = child->getGeometry();
                total_w += b.x;
                if (b.y > max_h) max_h = b.y;
            }
            if (bordered) {
                total_w += 2;
                max_h += 2;
            }
            return { total_w, max_h };
        }

        void render(compositor& tui) override {
            if (allocated_width > 0)  width = allocated_width;
            if (allocated_height > 0) height = allocated_height;

            int pad = bordered ? 1 : 0;
            if (bordered) {
                renderBorder(tui, pos_x, pos_y, width, height);
            }

            int inner_x = pos_x + pad;
            int inner_y = pos_y + pad;
            int inner_w = std::max(0, width - pad * 2);
            int inner_h = std::max(0, height - pad * 2);

            int hfill_count = 0;
            int fixed_width_sum = 0;

            for (auto& child : children) {
                if (!child) continue;
                child->parent = shared_from_this();

                bool has_hfill = (child->fill_mode == hfill || child->fill_mode == fill);
                if (has_hfill) {
                    hfill_count++;
                } else {
                    fixed_width_sum += child->getGeometry().x;
                }
            }

            int available_space = std::max(0, inner_w - fixed_width_sum);
            int width_per_fill = (hfill_count > 0) ? (available_space / hfill_count) : 0;
            int remainder = (hfill_count > 0) ? (available_space % hfill_count) : 0;

            int current_x = inner_x;

            for (auto& child : children) {
                if (!child) continue;

                bool has_hfill = (child->fill_mode == Fill::hfill || child->fill_mode == Fill::fill);
                int child_w = has_hfill ? (width_per_fill + (remainder-- > 0 ? 1 : 0))
                                        : child->getGeometry().x;

                int max_allowed_w = (inner_x + inner_w) - current_x;
                if (max_allowed_w <= 0) break;
                child_w = std::min(child_w, max_allowed_w);

                child->pos_x = current_x;
                child->pos_y = inner_y;
                child->allocated_width = child_w;

                if (child->fill_mode == fill || child->fill_mode == vfill) {
                    child->allocated_height = inner_h;
                } else {
                    child->allocated_height = child->height;
                }

                child->render(tui);
                current_x += child_w;
            }
        }
    };

    inline element hbox(std::initializer_list<element> children) {
        return std::make_shared<HBox>(children);
    }

    //              ▗
    //    ▛▌▛▌█▌▛▘▀▌▜▘▛▌▛▘▛▘
    //    ▙▌▙▌▙▖▌ █▌▐▖▙▌▌ ▄▌
    //      ▌

    inline element operator/(element el, Style mod) {
        // Try to cast the generic element down to our Text class
        if (auto txt_obj = std::dynamic_pointer_cast<Text>(el)) {
            switch (mod) {
                case bold:      txt_obj->style = bold; break;
                case italic:    txt_obj->style = italic; break;
                case underline: txt_obj->style = underline; break;
            }
        }
        return el; // Return the modified element so it can be nested!
    }

    /*inline element operator/(element el, Mod mod) {
        if (mod == border) {
            return std::make_shared<Border>(el);
        }
        return el;
    }*/

    inline element operator/(element el, Mod mod) {
        if (el && mod == border) {
            el->bordered = true;
        }
        return el;
    }

    inline element operator/(element el, Colors s) {
        if (auto txt_obj = std::dynamic_pointer_cast<Text>(el)) {
            txt_obj->fg = s;
        }
        return el;
    }

    inline element operator/(element el, bg_color bg) {
        if (auto txt_obj = std::dynamic_pointer_cast<Text>(el)) {
            txt_obj->bg = bg.c;
        }
        return el;
    }

    inline element operator/(element el, WidthMod m) {
        if (el) {
            el->width = m.value;
        }
        return el;
    }

    inline element operator/(element el, HeightMod m) {
        if (el) {
            el->height = m.value;
        }
        return el;
    }

    inline element operator/(element el, XMod m) {
        if (el) {
            el->pos_x = m.value;
        }
        return el;
    }

    inline element operator/(element el, YMod m) {
        if (el) {
            el->pos_y = m.value;
        }
        return el;
    }

    inline element operator/(element el, Fill mode) {
        if (el) {
            el->fill_mode = mode;
        }
        return el;
    }

    inline element operator/(element el, Anchor mode) {
        if (el) {
            int current = static_cast<int>(el->anchor_mode);
            int incoming = static_cast<int>(mode);

            // Dacă vine aliniere orizontală, curățăm doar bit-ii orizontali (1 | 2 | 4 = 7)
            if (incoming & 7) {
                current &= ~7;
            }
            // Dacă vine aliniere verticală, curățăm doar bit-ii verticali (8 | 16 | 32 = 56)
            if (incoming & 56) {
                current &= ~56;
            }

            el->anchor_mode = static_cast<Anchor>(current | incoming);
        }
        return el;
    }

}

#endif // SNAPCELL_LIBRARY_HPP