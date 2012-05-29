#include <gtkmm.h>
#include "connection.hpp"

class MainWindow : public Gtk::Window
{
   public:
      MainWindow();
      
   private:
      Gtk::HBox hbox;
      Gtk::Button play, stop, pause, open;
      void on_play_clicked();
      void on_stop_clicked();
      void on_pause_clicked();
      void on_open_clicked();

      Gtk::VBox vbox;
      Gtk::ProgressBar progress;

      Glib::ustring last_file;

      void play_file();
};


