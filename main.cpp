#include "snapcell.hpp"
using namespace tui;
using namespace std;

int main() {
    compositor tui;

    element screen = canvas("/home/nadoleanum/Pictures/giphy.gif", 20, 20) / blue / Mod::fill;

    bool running = true;
    while (running) {
        if (tui.getKey() == 'q') running = false;

        tui.clear();

        // Define your UI declaratively!
        element ui = vbox({
            hbox({
                text("dimensions are: " + to_string(tui.getWidth()) + "x" + to_string(tui.getHeight())) / center / Mod::fill / bg_color(black),
                separator(),
                text("--- MY DECLARATIVE TUI ---") / vcenter / Mod::fill ,
            }) / Mod::fill,
            separator(),
            text("Welcome to the engine.") / bold / green / bg_color(white),
            gauge(0.5f) / blue / bg_color(dark_blue) / border
        }) / border;

        // Render the root element starting at (0,0) spanning the full screen width/height

        screen->render(tui, 0, 0, tui.getWidth(), tui.getHeight());
        ui->render(tui, 0, 0, tui.getWidth(), tui.getHeight());


        tui.display();
        tui.delay_fps(60);
    }
    return 0;
}