//      ▗▄▄▖▗▖  ▗▖ ▗▄▖ ▗▄▄▖ ▗▄▄▖▗▄▄▄▖▗▖   ▗▖
//      ▐▌   ▐▛▚▖▐▌▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌   ▐▌   ▐▌
//       ▝▀▚▖▐▌ ▝▜▌▐▛▀▜▌▐▛▀▘ ▐▌   ▐▛▀▀▘▐▌   ▐▌
//      ▗▄▄▞▘▐▌  ▐▌▐▌ ▐▌▐▌   ▝▚▄▄▖▐▙▄▄▖▐▙▄▄▖▐▙▄▄▖

#ifndef SNAPCELL_LIBRARY_HPP
#define SNAPCELL_LIBRARY_HPP
#define STB_IMAGE_IMPLEMENTATION


#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <csignal>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <chrono>
#include <stb/stb_image.h>

// Optional dependency forward-declarations for standard image decoding backends:
// To compile: define STB_IMAGE_IMPLEMENTATION before including this or link against libwebp
extern "C" {
    unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
    unsigned char *stbi_load_gif_from_memory(unsigned char const *buffer, int len, int **delays, int *x, int *y, int *z, int *comp, int req_comp);
    void stbi_image_free(void *retval_from_stbi_load);
    unsigned char *WebPDecodeRGBA(const unsigned char* data, size_t data_size, int* width, int* height);
}

namespace tui {
    // A global flag for the Linux kernel resize signal
    inline std::atomic<bool> global_screen_resized(false);
    inline void handle_resize_signal(int) { global_screen_resized.store(true); }

    //      ▗        ▗    ▗
    //    ▛▘▜▘▛▘▌▌▛▘▜▘ ▚  █▌▛▌▌▌▛▛▌▛▘
    //    ▄▌▐▖▌ ▙▌▙▖▐▖ ▚▌  ▙▖▌▌▙▌▌▌▌▄▌
    //

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
        border,
        fill,
        hfill,
        vfill,
        center,
        hcenter,
        vcenter,
        color
    };

    enum class LayoutDir { Horizontal, Vertical, None };

    struct Cell {
        std::string ch = " ";
        int fg = 15;        // Default text color (White)
        int bg = -1;         // Default background color (Black)
        int style = normal; // Default text format style
    };

    struct bg_color { Colors c; };
    struct custom_color { Colors c; custom_color(Colors color_val) : c(color_val) {} };

    //                  ▘▗
    //    ▛▘▛▌▛▛▌▛▌▛▌▛▘▌▜▘▛▌▛▘
    //    ▙▖▙▌▌▌▌▙▌▙▌▄▌▌▐▖▙▌▌
    //            ▌

    class compositor {
    private:
        int width = 0;
        int height = 0;
        struct termios orig_termios;

        std::vector<Cell> front_buffer;
        std::vector<Cell> back_buffer;

        // Hidden layout tracking maps
        std::unordered_map<int, std::vector<int>> horiz_lines; // y -> list of x coords
        std::unordered_map<int, std::vector<int>> vert_lines;  // x -> list of y coords

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
        }

        ~compositor() {
            // Cleanup: Restore original terminal when program closes!
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            std::cout << "\033[?25h\033[2J\033[H"; // Show cursor & clear screen
        }

        // Getters so your widgets know the boundaries
        int getWidth() const { return width; }
        int getHeight() const { return height; }

        // --- YOUR CLEAN CUSTOM SYNTAX API ---

        void setChar(int x, int y, std::string ch, int fg = 15, int bg = -1, int style = normal) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                // If the incoming char is a line or junction, check for structural intersections
                if (ch == "─" || ch == "│" || ch == "┌" || ch == "┐" ||
                    ch == "└" || ch == "┘" || ch == "├" || ch == "┤" ||
                    ch == "┬" || ch == "┴" || ch == "┼") {

                    bool has_h = hasLineH(x, y);
                    bool has_v = hasLineV(x, y);

                    if (has_h && has_v) {
                        // Determine layout constraints to pick the right intersection type
                        bool left  = hasLineH(x - 1, y);
                        bool right = hasLineH(x + 1, y);
                        bool up    = hasLineV(x, y - 1);
                        bool down  = hasLineV(x, y + 1);

                        if (left && right && up && down) ch = "┼";
                        else if (left && right && down)  ch = "┬";
                        else if (left && right && up)    ch = "┴";
                        else if (up && down && right)    ch = "├";
                        else if (up && down && left)     ch = "┤";
                    }
                    }

                int idx = y * width + x;
                back_buffer[idx].ch = ch;
                back_buffer[idx].fg = fg;
                back_buffer[idx].bg = bg;
                back_buffer[idx].style = style;
            }
        }

        void setString(int x, int y, const std::string& text, int fg = 15, int bg = -1, int style = normal) {
            size_t i = 0;
            int screen_offset = 0;

            while (i < text.length()) {
                unsigned char c = text[i];
                size_t len = 1;

                // Determine how many bytes this UTF-8 character takes up
                if ((c & 0x80) == 0)           len = 1; // Standard ASCII
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

        char getKey() {
            char c = 0;
            read(STDIN_FILENO, &c, 1);
            return c; // Returns 0 if no key was pressed
        }

        void clear() {
            // Wipe the hidden back buffer
            for (auto& cell : back_buffer) cell.ch = ' ';
            horiz_lines.clear();
            vert_lines.clear();
        }

        void display() {
            if (global_screen_resized.load()) {
                global_screen_resized.store(false);
                updateDimensions();
            }

            int last_fg = -1;
            int last_bg = -2;
            int last_style = -1;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = y * width + x;

                    // Smart Diff-Rendering: check if char, Colors, OR style changed
                    if (back_buffer[idx].ch    != front_buffer[idx].ch ||
                        back_buffer[idx].fg    != front_buffer[idx].fg ||
                        back_buffer[idx].bg    != front_buffer[idx].bg ||
                        back_buffer[idx].style != front_buffer[idx].style) {

                        moveCursor(x, y);

                        // Update formatting escape codes only when something shifts
                        if (back_buffer[idx].fg != last_fg ||
                            back_buffer[idx].bg != last_bg ||
                            back_buffer[idx].style != last_style) {

                            std::cout << "\033[" << back_buffer[idx].style;
                            std::cout << ";38;5;" << back_buffer[idx].fg;

                            // Only apply explicit background if it isn't set to native terminal fallback (-1)
                            if (back_buffer[idx].bg >= 0) {
                                std::cout << ";48;5;" << back_buffer[idx].bg;
                            } else {
                                // Explicitly clear background overrides to fall back to the terminal native skin
                                std::cout << ";49";
                            }
                            std::cout << "m";

                            last_fg = back_buffer[idx].fg;
                            last_bg = back_buffer[idx].bg;
                            last_style = back_buffer[idx].style;
                            }

                        std::cout << back_buffer[idx].ch;
                        front_buffer[idx] = back_buffer[idx]; // Sync
                        }
                }
            }
            // Clean up all attributes at the end of the frame
            std::cout << "\033[0m";
            std::cout.flush();
        }

        void delay_fps(int fps) {
            if (fps <= 0) fps = 60; // Fallback safety
            // Math formula: 1,000,000 microseconds / Frames Per Second
            long microseconds = 1000000 / fps;
            usleep(microseconds);
        }

        // High-level structural line registers
        void registerLineH(int y, int x1, int x2) {
            for (int x = x1; x <= x2; ++x) horiz_lines[y].push_back(x);
        }

        void registerLineV(int x, int y1, int y2) {
            for (int y = y1; y <= y2; ++y) vert_lines[x].push_back(y);
        }

        bool hasLineV(int x, int y) {
            auto it = vert_lines.find(x);
            if (it == vert_lines.end()) return false;
            return std::find(it->second.begin(), it->second.end(), y) != it->second.end();
        }

        bool hasLineH(int x, int y) {
            auto it = horiz_lines.find(y);
            if (it == horiz_lines.end()) return false;
            return std::find(it->second.begin(), it->second.end(), x) != it->second.end();
        }
    };

    struct Box {
        int width, height;
    };

    //      ▜            ▗
    //    █▌▐ █▌▛▛▌█▌▛▌▜▘▛▘
    //    ▙▖▐▖▙▖▌▌▌▙▖▌▌▐▖▄▌
    //

    // The base class for EVERYTHING in your UI
    class Element {
    public:
        virtual ~Element() = default;

        // How much space does this widget actually need?
        virtual Box getGeometry() = 0;

        // Draw yourself inside this specific allocated box on the screen
        virtual void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir = LayoutDir::None) = 0;
    };

    // A smart pointer alias to make syntax cleaner
    using element = std::shared_ptr<Element>;

    class Flex : public Element {
    private:
        element child;
        Mod alignment;
    public:
        Flex(element child_node, Mod align) : child(child_node), alignment(align) {}

        Mod getMode() const { return alignment; }
        element getChild() const { return child; }

        Box getGeometry() override {
            return child->getGeometry();
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir) override {
            Box target = child->getGeometry();

            // Defend bounds - Separated hfill and vfill components logic
            int final_w = (alignment == Mod::fill || alignment == Mod::hfill) ? allocated_w : target.width;
            int final_h = (alignment == Mod::fill || alignment == Mod::vfill) ? allocated_h : target.height;
            if (final_w > allocated_w) final_w = allocated_w;
            if (final_h > allocated_h) final_h = allocated_h;

            int target_x = x;
            int target_y = y;

            // Center calculations only kick in if we aren't completely filling the axis
            if (alignment == Mod::hcenter || alignment == Mod::center) {
                target_x += (allocated_w - final_w) / 2;
            }
            if (alignment == Mod::vcenter || alignment == Mod::center) {
                target_y += (allocated_h - final_h) / 2;
            }

            child->render(tui, target_x, target_y, final_w, final_h, parent_dir);
        }
    };

    class SizeOverride : public Element {
    private:
        element child;
        int fixed_w;
        int fixed_h;
    public:
        SizeOverride(element child_node, int w, int h)
            : child(child_node), fixed_w(w), fixed_h(h) {}

        element getChild() const { return child; }

        Box getGeometry() override {
            Box b = child->getGeometry();
            return { fixed_w >= 0 ? fixed_w : b.width, fixed_h >= 0 ? fixed_h : b.height };
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir) override {
            Box b = child->getGeometry();
            int target_w = fixed_w >= 0 ? fixed_w : allocated_w;
            int target_h = fixed_h >= 0 ? fixed_h : allocated_h;
            child->render(tui, x, y, std::min(target_w, allocated_w), std::min(target_h, allocated_h), parent_dir);
        }
    };

    struct width { int value; width(int v) : value(v) {} };
    struct height { int value; height(int v) : value(v) {} };

    class Border : public Element {
    private:
        element child;
    public:
        Border(element child_node) : child(child_node) {}

        element getChild() const { return child; }

        Box getGeometry() override {
            Box b = child->getGeometry();
            return { b.width + 2, b.height + 2 };
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir) override {
            if (allocated_w < 2 || allocated_h < 2) return;

            // Register structural layout lines first so setChar can spot intersections
            tui.registerLineH(y, x, x + allocated_w - 1);
            tui.registerLineH(y + allocated_h - 1, x, x + allocated_w - 1);
            tui.registerLineV(x, y, y + allocated_h - 1);
            tui.registerLineV(x + allocated_w - 1, y, y + allocated_h - 1);

            // Now draw the frame elements
            tui.setChar(x, y, "┌");
            tui.setChar(x + allocated_w - 1, y, "┐");
            tui.setChar(x, y + allocated_h - 1, "└");
            tui.setChar(x + allocated_w - 1, y + allocated_h - 1, "┘");

            for (int i = 1; i < allocated_w - 1; ++i) {
                tui.setChar(x + i, y, "─");
                tui.setChar(x + i, y + allocated_h - 1, "─");
            }

            for (int i = 1; i < allocated_h - 1; ++i) {
                tui.setChar(x, y + i, "│");
                tui.setChar(x + allocated_w - 1, y + i, "│");
            }

            child->render(tui, x + 1, y + 1, allocated_w - 2, allocated_h - 2, parent_dir);
        }
    };

    class Text : public Element {
    public:
        std::string content;
        Style style = normal;
        Colors fg = white;
        int bg = -1;

        Text(std::string text) : content(text) {}

        Box getGeometry() override {
            return { (int)content.length(), 1 }; // Width is text length, height is 1 row
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir) override {
            // Render text, cutting it off if the container is too small
            std::string visible = content.substr(0, allocated_w);
            tui.setString(x, y, visible, fg, bg, style);
        }
    };

    // Helper function to create the clean FTXUI syntax
    inline element text(std::string text) {
        return std::make_shared<Text>(text);
    }

    class AsciiCanvas : public Element {
    public:
        int img_w = 0;
        int img_h = 0;
        std::vector<Cell> pixels;
        bool use_custom_color = false;
        Colors custom_color_val = white;
        std::vector<std::vector<Cell>> frames;
        std::vector<int> delays;
        std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
        int total_duration = 0;

        AsciiCanvas(int w, int h, const std::vector<Cell>& p) : img_w(w), img_h(h), pixels(p) {}

        Box getGeometry() override {
            return { img_w, img_h };
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir) override {
            const std::vector<Cell>* current_pixels = &pixels;
            if (!frames.empty()) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                if (total_duration > 0) {
                    elapsed_ms %= total_duration;
                }
                int current_frame = 0;
                long long accumulated = 0;
                for (size_t i = 0; i < frames.size(); ++i) {
                    accumulated += delays[i];
                    if (elapsed_ms < accumulated) {
                        current_frame = i;
                        break;
                    }
                }
                current_pixels = &frames[current_frame];
            }

            for (int r = 0; r < std::min(img_h, allocated_h); ++r) {
                for (int c = 0; c < std::min(img_w, allocated_w); ++c) {
                    const Cell& pixel = (*current_pixels)[r * img_w + c];
                    int final_fg = use_custom_color ? static_cast<int>(custom_color_val) : pixel.fg;
                    tui.setChar(x + c, y + r, pixel.ch, final_fg, pixel.bg, pixel.style);
                }
            }
        }
    };

    inline element canvas(int w, int h, const std::vector<Cell>& pixels) {
        return std::make_shared<AsciiCanvas>(w, h, pixels);
    }

    // Helpers intern pentru detectarea elementelor cu modificator de umplere (fill)
    inline bool is_hfill_element(element el) {
        while (el) {
            if (auto flex_obj = std::dynamic_pointer_cast<Flex>(el)) {
                Mod mode = flex_obj->getMode();
                return mode == Mod::fill || mode == Mod::hfill;
            }
            if (auto border_obj = std::dynamic_pointer_cast<Border>(el)) {
                el = border_obj->getChild();
            } else if (auto size_obj = std::dynamic_pointer_cast<SizeOverride>(el)) {
                el = size_obj->getChild();
            } else {
                break;
            }
        }
        return false;
    }

    inline bool is_vfill_element(element el) {
        while (el) {
            if (auto flex_obj = std::dynamic_pointer_cast<Flex>(el)) {
                Mod mode = flex_obj->getMode();
                return mode == Mod::fill || mode == Mod::vfill;
            }
            if (auto border_obj = std::dynamic_pointer_cast<Border>(el)) {
                el = border_obj->getChild();
            } else if (auto size_obj = std::dynamic_pointer_cast<SizeOverride>(el)) {
                el = size_obj->getChild();
            } else {
                break;
            }
        }
        return false;
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
                Box b = child->getGeometry();
                if (b.width > max_w) max_w = b.width;
                total_h += b.height;
            }
            return { max_w, total_h };
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir = LayoutDir::None) override {
            int static_h = 0;
            int fill_count = 0;

            // Pasul 1: Calculăm spațiul cerut de elementele fixe și numărul de elemente flexibile
            for (auto& child : children) {
                if (is_vfill_element(child)) {
                    fill_count++;
                } else {
                    static_h += child->getGeometry().height;
                }
            }

            int remaining_space = allocated_h - static_h;
            int fill_share = (fill_count > 0 && remaining_space > 0) ? (remaining_space / fill_count) : 0;

            int current_y = y;
            for (auto& child : children) {
                Box b = child->getGeometry();

                int remaining_h = (y + allocated_h) - current_y;
                if (remaining_h <= 0) break;

                // Dacă e element de tip 'fill' pe axa verticală, îi dăm cota parte
                int desired_h = is_vfill_element(child) ? fill_share : b.height;
                int child_h = std::min(desired_h, remaining_h);

                child->render(tui, x, current_y, allocated_w, child_h, LayoutDir::Vertical);

                current_y += child_h;
            }
        }    };
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
                Box b = child->getGeometry();
                total_w += b.width;
                if (b.height > max_h) max_h = b.height;
            }
            return { total_w, max_h };
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir = LayoutDir::None) override {
            int static_w = 0;
            int fill_count = 0;

            // Pasul 1: Calculăm lățimea elementelor fixe și numărul de elemente flexibile
            for (auto& child : children) {
                if (is_hfill_element(child)) {
                    fill_count++;
                } else {
                    static_w += child->getGeometry().width;
                }
            }

            int remaining_space = allocated_w - static_w;
            int fill_share = (fill_count > 0 && remaining_space > 0) ? (remaining_space / fill_count) : 0;

            int current_x = x;
            for (auto& child : children) {
                Box b = child->getGeometry();

                int remaining_w = (x + allocated_w) - current_x;
                if (remaining_w <= 0) break;

                // Dacă e element de tip 'fill' pe axa orizontală, primește cota parte
                int desired_w = is_hfill_element(child) ? fill_share : b.width;
                int child_w = std::min(desired_w, remaining_w);

                child->render(tui, current_x, y, child_w, allocated_h, LayoutDir::Horizontal);

                current_x += child_w;
            }
        }    };
    inline element hbox(std::initializer_list<element> children) {
        return std::make_shared<HBox>(children);
    }

    class Separator : public Element {
    public:
        Box getGeometry() override {
            return { 1, 1 };
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir) override {
            if (parent_dir == LayoutDir::Vertical) {
                int mid_y = y + (allocated_h / 2);

                // Docking Fix: Extend registration 1 character left (-1) and right (+allocated_w)
                tui.registerLineH(mid_y, x - 1, x + allocated_w);

                // Draw line including the boundary edges to hit the parent frame
                for (int i = -1; i <= allocated_w; ++i) {
                    tui.setChar(x + i, mid_y, "─");
                }
            } else {
                int mid_x = x + (allocated_w / 2);

                // Docking Fix: Extend registration 1 character up (-1) and down (+allocated_h)
                tui.registerLineV(mid_x, y - 1, y + allocated_h);

                // Draw line including the boundary edges to hit the parent frame
                for (int j = -1; j <= allocated_h; ++j) {
                    tui.setChar(mid_x, y + j, "│");
                }
            }
        }
    };

    inline element separator() {
        return std::make_shared<Separator>();
    }

    class Gauge : public Element {
    public:
        float progress = 0.0f;
        Colors fg = white;
        int bg = -1;

        Gauge(float p) {
            progress = p < 0.0f ? 0.0f : (p > 1.0f ? 1.0f : p);
        }

        Box getGeometry() override {
            // Geometria minimă implicită (lățime mică pe 1 singur rând)
            return { 4, 1 };
        }

        void render(compositor& tui, int x, int y, int allocated_w, int allocated_h, LayoutDir parent_dir) override {
            if (allocated_h <= 0 || allocated_w <= 0) return;

            float total_filled = progress * allocated_w;
            int full_blocks = static_cast<int>(total_filled);
            float remainder = total_filled - full_blocks;

            // Caractere bloc din FTXUI pentru rezoluție fină sub-pixel
            std::string fractions[] = {" ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};
            int fraction_idx = static_cast<int>(remainder * 8.0f);

            for (int i = 0; i < allocated_w; ++i) {
                std::string ch = " ";
                if (i < full_blocks) {
                    ch = "█";
                } else if (i == full_blocks && fraction_idx > 0) {
                    ch = fractions[fraction_idx];
                }

                tui.setChar(x + i, y, ch, fg, bg, normal);
            }
        }
    };

    inline element gauge(float progress) {
        return std::make_shared<Gauge>(progress);
    }

    //              ▗
    //    ▛▌▛▌█▌▛▘▀▌▜▘▛▌▛▘▛▘
    //    ▙▌▙▌▙▖▌ █▌▐▖▙▌▌ ▄▌
    //     ▌

    // Helper utility to safely locate the underlying target styleable element
    inline element getStyleableTarget(element el) {
        while (el) {
            if (std::dynamic_pointer_cast<Text>(el) || std::dynamic_pointer_cast<Gauge>(el) || std::dynamic_pointer_cast<AsciiCanvas>(el)) {
                return el;
            }
            if (auto border_obj = std::dynamic_pointer_cast<Border>(el)) {
                el = border_obj->getChild();
            } else if (auto flex_obj = std::dynamic_pointer_cast<Flex>(el)) {
                el = flex_obj->getChild();
            } else if (auto size_obj = std::dynamic_pointer_cast<SizeOverride>(el)) {
                el = size_obj->getChild();
            } else {
                break;
            }
        }
        return el;
    }

    inline element operator/(element el, Style mod) {
        // Try to cast the generic element down to our Text class
        if (auto txt_obj = std::dynamic_pointer_cast<Text>(getStyleableTarget(el))) {
            switch (mod) {
                case bold:      txt_obj->style = bold; break;
                case italic:    txt_obj->style = italic; break;
                case underline: txt_obj->style = underline; break;
            }
        }
        return el; // Return the modified element so it can be nested!
    }

    inline element operator/(element el, Mod mod) {
        if (mod == border) {
            return std::make_shared<Border>(el);
        }
        if (mod == fill || mod == hfill || mod == vfill || mod == center || mod == hcenter || mod == vcenter) {
            return std::make_shared<Flex>(el, mod);
        }
        if (mod == color) {
            if (auto canvas_obj = std::dynamic_pointer_cast<AsciiCanvas>(getStyleableTarget(el))) {
                canvas_obj->use_custom_color = false;
            }
        }
        return el;
    }

    inline element operator/(element el, custom_color cc) {
        if (auto canvas_obj = std::dynamic_pointer_cast<AsciiCanvas>(getStyleableTarget(el))) {
            canvas_obj->use_custom_color = true;
            canvas_obj->custom_color_val = cc.c;
        }
        return el;
    }

    inline element operator/(element el, width w) {
        return std::make_shared<SizeOverride>(el, w.value, -1);
    }

    inline element operator/(element el, height h) {
        return std::make_shared<SizeOverride>(el, -1, h.value);
    }

    inline element operator/(element el, Colors s) {
        element target = getStyleableTarget(el);
        // Try to cast the generic element down to our Text class
        if (auto txt_obj = std::dynamic_pointer_cast<Text>(target)) {
            txt_obj->fg = s;
        }
        if (auto gauge_obj = std::dynamic_pointer_cast<Gauge>(target)) {
            gauge_obj->fg = s;
        }
        if (auto canvas_obj = std::dynamic_pointer_cast<AsciiCanvas>(target)) {
            canvas_obj->use_custom_color = true;
            canvas_obj->custom_color_val = s;
        }
        return el; // Return the modified element so it can be nested!
    }

    inline element operator/(element el, bg_color bg) {
        element target = getStyleableTarget(el);
        if (auto txt_obj = std::dynamic_pointer_cast<Text>(target)) {
            txt_obj->bg = bg.c;
        }
        if (auto gauge_obj = std::dynamic_pointer_cast<Gauge>(target)) {
            gauge_obj->bg = bg.c;
        }
        if (auto canvas_obj = std::dynamic_pointer_cast<AsciiCanvas>(target)) {
            for (auto& p : canvas_obj->pixels) {
                p.bg = bg.c;
            }
            for (auto& f : canvas_obj->frames) {
                for (auto& p : f) {
                    p.bg = bg.c;
                }
            }
        }
        return el;
    }

    // --- NEW IMAGE LOADER & CONVERTER OVERLOAD ---

    inline element canvas(const std::string& filepath, int target_w, int target_h) {
        int img_w = 0, img_h = 0, channels = 0;
        unsigned char* rgb_data = nullptr;
        bool loaded_via_webp = false;
        bool loaded_via_gif = false;
        std::vector<std::vector<Cell>> gif_frames;
        std::vector<int> gif_delays;

        // Check file extension to route WebP explicitly
        std::string lower_path = filepath;
        std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

        if (lower_path.size() >= 5 && lower_path.compare(lower_path.size() - 5, 5, ".webp") == 0) {
            std::ifstream file(filepath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                size_t size = file.tellg();
                file.seekg(0, std::ios::beg);
                std::vector<unsigned char> buffer(size);
                if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                    rgb_data = WebPDecodeRGBA(buffer.data(), size, &img_w, &img_h);
                    channels = 4;
                    loaded_via_webp = true;
                }
            }
        }

        // Handle standard GIF parsing via specific stb implementation
        if (!rgb_data && lower_path.size() >= 4 && lower_path.compare(lower_path.size() - 4, 4, ".gif") == 0) {
            std::ifstream file(filepath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                size_t size = file.tellg();
                file.seekg(0, std::ios::beg);
                std::vector<unsigned char> buffer(size);
                if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                    int w = 0, h = 0, layers = 0, comp = 0;
                    int* delays_ptr = nullptr;
                    unsigned char* gif_data = stbi_load_gif_from_memory(buffer.data(), size, &delays_ptr, &w, &h, &layers, &comp, 3);
                    if (gif_data) {
                        loaded_via_gif = true;
                        img_w = w;
                        img_h = h;

                        auto match_ansi256 = [](int r, int g, int b) -> int {
                            int r_idx = (r * 5) / 255;
                            int g_idx = (g * 5) / 255;
                            int b_idx = (b * 5) / 255;
                            return 16 + (36 * r_idx) + (6 * g_idx) + b_idx;
                        };
                        std::string ascii_ramp = " .:-=+*#%@";

                        for (int l = 0; l < layers; ++l) {
                            std::vector<Cell> converted_pixels(target_w * target_h);
                            unsigned char* frame_data = gif_data + (l * img_w * img_h * 3);

                            for (int y = 0; y < target_h; ++y) {
                                float src_y = (static_cast<float>(y) / target_h) * img_h;
                                int y0 = std::min(static_cast<int>(floor(src_y)), img_h - 1);
                                int y1 = std::min(y0 + 1, img_h - 1);
                                float weight_y = src_y - y0;

                                for (int x = 0; x < target_w; ++x) {
                                    float src_x = (static_cast<float>(x) / target_w) * img_w;
                                    int x0 = std::min(static_cast<int>(floor(src_x)), img_w - 1);
                                    int x1 = std::min(x0 + 1, img_w - 1);
                                    float weight_x = src_x - x0;

                                    float r = 0, g = 0, b = 0;
                                    int sample_points[4][2] = {{x0, y0}, {x1, y0}, {x0, y1}, {x1, y1}};
                                    float weights[4] = {
                                        (1.0f - weight_x) * (1.0f - weight_y),
                                        weight_x * (1.0f - weight_y),
                                        (1.0f - weight_x) * weight_y,
                                        weight_x * weight_y
                                    };

                                    for (int k = 0; k < 4; ++k) {
                                        int p_idx = (sample_points[k][1] * img_w + sample_points[k][0]) * 3;
                                        r += frame_data[p_idx] * weights[k];
                                        g += frame_data[p_idx + 1] * weights[k];
                                        b += frame_data[p_idx + 2] * weights[k];
                                    }

                                    int final_r = std::round(r);
                                    int final_g = std::round(g);
                                    int final_b = std::round(b);

                                    float luminance = 0.2126f * final_r + 0.7152f * final_g + 0.0722f * final_b;
                                    int ramp_idx = std::round((luminance / 255.0f) * (ascii_ramp.size() - 1));

                                    Cell& cell = converted_pixels[y * target_w + x];
                                    cell.ch = std::string(1, ascii_ramp[ramp_idx]);
                                    cell.fg = match_ansi256(final_r, final_g, final_b);
                                    cell.bg = -1;
                                    cell.style = normal;
                                }
                            }
                            gif_frames.push_back(converted_pixels);
                            gif_delays.push_back(delays_ptr[l]);
                        }
                        stbi_image_free(gif_data);
                    }
                }
            }
        }

        // Handle standard PNG, JPG parsing via standard backend if WebP or GIF wasn't triggered
        if (!rgb_data && !loaded_via_gif) {
            rgb_data = stbi_load(filepath.c_str(), &img_w, &img_h, &channels, 3);
            channels = 3;
        }

        if (!rgb_data && !loaded_via_gif) {
            // Graceful error fallback: Return a tiny empty canvas cell array
            return std::make_shared<AsciiCanvas>(1, 1, std::vector<Cell>{{"?", 9, -1, normal}});
        }

        if (loaded_via_gif) {
            auto canvas_ptr = std::make_shared<AsciiCanvas>(target_w, target_h, gif_frames.empty() ? std::vector<Cell>{} : gif_frames[0]);
            canvas_ptr->frames = std::move(gif_frames);
            canvas_ptr->delays = std::move(gif_delays);
            canvas_ptr->total_duration = 0;
            for (int d : canvas_ptr->delays) {
                canvas_ptr->total_duration += d;
            }
            return canvas_ptr;
        }

        std::vector<Cell> converted_pixels(target_w * target_h);
        std::string ascii_ramp = " .:-=+*#%@";

        // Quantization maps down to xterm 256 color specifications seamlessly
        auto match_ansi256 = [](int r, int g, int b) -> int {
            int r_idx = (r * 5) / 255;
            int g_idx = (g * 5) / 255;
            int b_idx = (b * 5) / 255;
            return 16 + (36 * r_idx) + (6 * g_idx) + b_idx;
        };

        // Bilinear interpolation downsampling to structural canvas coordinates
        for (int y = 0; y < target_h; ++y) {
            float src_y = (static_cast<float>(y) / target_h) * img_h;
            int y0 = std::min(static_cast<int>(floor(src_y)), img_h - 1);
            int y1 = std::min(y0 + 1, img_h - 1);
            float weight_y = src_y - y0;

            for (int x = 0; x < target_w; ++x) {
                float src_x = (static_cast<float>(x) / target_w) * img_w;
                int x0 = std::min(static_cast<int>(floor(src_x)), img_w - 1);
                int x1 = std::min(x0 + 1, img_w - 1);
                float weight_x = src_x - x0;

                float r = 0, g = 0, b = 0;
                int sample_points[4][2] = {{x0, y0}, {x1, y0}, {x0, y1}, {x1, y1}};
                float weights[4] = {
                    (1.0f - weight_x) * (1.0f - weight_y),
                    weight_x * (1.0f - weight_y),
                    (1.0f - weight_x) * weight_y,
                    weight_x * weight_y
                };

                for (int k = 0; k < 4; ++k) {
                    int p_idx = (sample_points[k][1] * img_w + sample_points[k][0]) * channels;
                    r += rgb_data[p_idx] * weights[k];
                    g += rgb_data[p_idx + 1] * weights[k];
                    b += rgb_data[p_idx + 2] * weights[k];
                }

                int final_r = std::round(r);
                int final_g = std::round(g);
                int final_b = std::round(b);

                // Perceptual luminance calculation to extract accurate density patterns
                float luminance = 0.2126f * final_r + 0.7152f * final_g + 0.0722f * final_b;
                int ramp_idx = std::round((luminance / 255.0f) * (ascii_ramp.size() - 1));

                Cell& cell = converted_pixels[y * target_w + x];
                cell.ch = std::string(1, ascii_ramp[ramp_idx]);
                cell.fg = match_ansi256(final_r, final_g, final_b);
                cell.bg = -1;
                cell.style = normal;
            }
        }

        if (loaded_via_webp) {
            free(rgb_data); // WebP uses typical standard allocator free paths
        } else {
            stbi_image_free(rgb_data);
        }

        return std::make_shared<AsciiCanvas>(target_w, target_h, converted_pixels);
    }

}

#endif // SNAPCELL_LIBRARY_HPP