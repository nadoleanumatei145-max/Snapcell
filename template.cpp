#include "snapcell.hpp"

int main() {
    tui::compositor app;

    app.set_ascii_palette(" .:-*#%");

    int mouse_x = 0, mouse_y = 0;
    std::string status = "Sistem Pregatit";
    bool image_loaded = true;

    while (true) {
        tui::Event ev = app.getEvent();

        if (ev.type == tui::Event::KeyEv) {
            if (ev.key == 'q' || ev.key == tui::key::esc) {
                break;
            }
            if (ev.key == '1') app.use_true_color();
            if (ev.key == '2') app.set_solid_color(tui::cyan);
            if (ev.key == '3') app.set_tint_color(255, 100, 50);
            
            if (ev.key.is_char()) {
                status = "Tasta: " + std::string(1, ev.key.to_char());
            }
        } 
        else if (ev.type == tui::Event::MouseEv) {
            mouse_x = ev.mouse.x;
            mouse_y = ev.mouse.y;

            if (ev.mouse.button == tui::MouseButton::Left && ev.mouse.type == tui::MouseEventType::Press) {
                status = "Click L (" + std::to_string(mouse_x) + ", " + std::to_string(mouse_y) + ")";
            } else if (ev.mouse.button == tui::MouseButton::WheelUp) {
                status = "Scroll Up";
            } else if (ev.mouse.button == tui::MouseButton::WheelDown) {
                status = "Scroll Down";
            }
        }

        app.clear();

        if (image_loaded) {
            image_loaded = app.draw_image("assets/banner.png", 2, 1, 20, 8);
        }

        auto layout = tui::vbox({
            tui::hbox({
                tui::text(" SnapCell Demo ") / tui::bold / tui::yellow / tui::border,
                tui::text(" Mod: 1-RGB 2-Solid 3-Tint ") / tui::italic / tui::cyan / tui::border
            }) / tui::hfill,

            tui::vbox({
                tui::text("Status: " + status) / tui::bold / tui::green,
                tui::text("Mouse: X=" + std::to_string(mouse_x) + " Y=" + std::to_string(mouse_y)) / tui::white,
                tui::text("Iesire: Q / ESC") / tui::underline / tui::red
            }) / tui::fill / tui::center / tui::border
        });

        layout->allocated_width = app.getWidth();
        layout->allocated_height = app.getHeight();
        layout->render(app);

        app.display();
        app.delay_fps(60);
    }

    return 0;
}
