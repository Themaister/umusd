#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
   Gtk::Main kit(argc, argv);

   MainWindow win;
   Gtk::Main::run(win);
}

