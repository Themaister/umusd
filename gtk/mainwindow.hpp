#include <gtkmm.h>
#include "connection.hpp"
#include <list>

class MainWindow : public Gtk::Window
{
   public:
      MainWindow();
      
   private:
      Gtk::HBox hbox;
      Gtk::Button play, stop, pause, open, next, prev;
      void on_play_clicked();
      void on_stop_clicked();
      void on_pause_clicked();
      void on_open_clicked();
      void on_next_clicked();
      void on_prev_clicked();
      bool on_timer_tick();
      bool on_button_press(GdkEventButton *btn);
      void on_about();

      Gtk::MenuBar menu;
      Gtk::VBox main_box;
      Gtk::VBox vbox;
      Gtk::ProgressBar progress;

      Gtk::Table grid;
      struct
      {
         Gtk::Label title, artist, album;
      } meta;

      Gtk::FileChooserDialog diag;
      void add_filter(const std::string &name, const std::list<std::string> &ext);

      void spawn();
      void init_menu();

      void play_ctl(const std::string &cmd);
      void play_add(const std::string &cmd, const std::vector<std::string> &path);
      void play_file(const std::vector<std::string> &path);
      void queue_file(const std::vector<std::string> &path);
      void update_meta(Connection &con);
      void update_pos(Connection &con);
      void reset_meta_pos();
      void seek(float rel);

      static std::string sec_to_text(unsigned sec);
};

