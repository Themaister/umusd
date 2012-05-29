#include <gtkmm.h>
#include "connection.hpp"
#include <list>

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
      bool on_timer_tick();

      Gtk::VBox vbox;
      Gtk::ProgressBar progress;

      std::string last_file;

      Gtk::Table grid;
      struct
      {
         Gtk::Label title, artist, album;
      } meta;

      Gtk::FileChooserDialog diag;
      void add_filter(const std::string &name, const std::list<std::string> &ext);

      void play_file();
      void update_meta(Connection &con);
      void update_pos(Connection &con);
      void reset_meta_pos();

      static std::string sec_to_text(unsigned sec);
};

