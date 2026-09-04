#include "snapcell.hpp"
using namespace tui;

int main() {
    compositor tui;
    if (!is_native_tty()) {
        // Paletă UTF-8 bogată pentru terminal emulators (Kitty, Alacritty, Foot)
        tui.set_ascii_palette("█");
    } else {
        tui.set_ascii_palette(" _:*#%&@");
    }
    tui.set_tint_color(0, 255, 0);
    tui.use_true_color();
    bool running = true;
    while (running) {
        auto k = tui.getKey();

        if (k == 'q' || ( k == 'x' && k.ctrl )) {
            running = false;
        }

        tui.clear();

        // Define your UI declaratively!
        element ui = vbox({
            text("Welcome to the engine.")
            / bold
            / red
            / hfill
            / right,
            text("Acesta este spatiul maxim")
            / fill
            / right / vcenter,
            text("footer")
        })
        / border
        / width( tui.getWidth())
        / height( tui.getHeight());

        if (!tui.draw_image("/home/nadoleanum/Pictures/halftone-black-hole-3840x2160-v0-whmp08jhbyca1.png", 0, 0, tui.getWidth(), tui.getHeight())) {
            tui.setString(0, 0, "nu s-a putut incarca", red);
        }
        //ui->render(tui);

        tui.display();
        tui.delay_fps(60);
    }
    return 0;
}