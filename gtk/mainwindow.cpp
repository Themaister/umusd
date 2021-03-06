#include "mainwindow.hpp"
#include <iostream>

template <class T, class... A>
inline T* managed(A&&... args)
{
   return Gtk::manage(new T(std::forward<A>(args)...));
}

MainWindow::MainWindow() :
   grid(3, 2), diag(*this, "Open File ...")
{
   set_title("uMusC");
   set_icon_from_file("/usr/share/icons/umusc.png");

   play.set_image(*managed<Gtk::Image>(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_BUTTON));
   stop.set_image(*managed<Gtk::Image>(Gtk::Stock::MEDIA_STOP, Gtk::ICON_SIZE_BUTTON));
   pause.set_image(*managed<Gtk::Image>(Gtk::Stock::MEDIA_PAUSE, Gtk::ICON_SIZE_BUTTON));
   open.set_image(*managed<Gtk::Image>(Gtk::Stock::OPEN, Gtk::ICON_SIZE_BUTTON));
   next.set_image(*managed<Gtk::Image>(Gtk::Stock::MEDIA_NEXT, Gtk::ICON_SIZE_BUTTON));
   prev.set_image(*managed<Gtk::Image>(Gtk::Stock::MEDIA_PREVIOUS, Gtk::ICON_SIZE_BUTTON));

   play.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_play_clicked));
   stop.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_stop_clicked));
   pause.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_pause_clicked));
   open.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_open_clicked));
   next.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_next_clicked));
   prev.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_prev_clicked));

   vbox.set_spacing(3);
   vbox.set_border_width(5);
   hbox.pack_start(open);
   hbox.pack_start(*managed<Gtk::VSeparator>());
   hbox.pack_start(prev);
   hbox.pack_start(play);
   hbox.pack_start(pause);
   hbox.pack_start(stop);
   hbox.pack_start(next);

   init_menu();
   vbox.pack_start(hbox, Gtk::PACK_SHRINK);
   vbox.pack_start(progress, Gtk::PACK_SHRINK);
   vbox.pack_start(grid);

   grid.set_row_spacings(5);
   grid.set_col_spacings(10);

   auto label = Gtk::manage(new Gtk::Label);
   label->set_markup("<b>Title</b>");
   label->set_alignment(0, 0.5);
   grid.attach(*label, 0, 1, 0, 1);
   label = Gtk::manage(new Gtk::Label);
   label->set_markup("<b>Artist</b>");
   label->set_alignment(0, 0.5);
   grid.attach(*label, 0, 1, 1, 2);
   label = Gtk::manage(new Gtk::Label);
   label->set_markup("<b>Album</b>");
   label->set_alignment(0, 0.5);
   grid.attach(*label, 0, 1, 2, 3);

   meta.title.set_alignment(0, 0.5);
   meta.artist.set_alignment(0, 0.5);
   meta.album.set_alignment(0, 0.5);
   grid.attach(meta.title,  1, 2, 0, 1);
   grid.attach(meta.artist, 1, 2, 1, 2);
   grid.attach(meta.album,  1, 2, 2, 3);
   meta.title.set_text("N/A");
   meta.artist.set_text("N/A");
   meta.album.set_text("N/A");

   progress.set_fraction(0.0);
   progress.set_text("N/A");
   signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_button_press));
   set_events(Gdk::BUTTON_PRESS_MASK);

   diag.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
   diag.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
   diag.set_select_multiple();

   add_filter("Media files", {"*.mp3", "*.flac", "*.ogg", "*.m4a", "*.wav"});
   add_filter("MP3 files", {"*.mp3"});
   add_filter("FLAC files", {"*.flac"});
   add_filter("Vorbis files", {"*.ogg"});
   add_filter("M4A files", {"*.m4a"});
   add_filter("WAV files", {"*.wav"});
   add_filter("Any file", {"*"});

   main_box.pack_start(menu, Gtk::PACK_SHRINK);
   main_box.pack_start(vbox);
   add(main_box);
   show_all();

   on_fork_clicked();
   Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &MainWindow::on_timer_tick), 1); 
   on_timer_tick();
}

void MainWindow::on_fork_clicked()
{
   try
   {
      Glib::spawn_command_line_async("umusd");
   }
   catch(const Glib::SpawnError &)
   {
      Gtk::MessageDialog diag(*this,
            "Failed to start umusd. It is probably not in $PATH.", true);

      Gtk::Image img(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG);
      diag.set_image(img);
      img.show();
      diag.run();
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

void MainWindow::on_kill_clicked()
{
   try
   {
      Connection con;
      con.command("DIE\r\n");
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

void MainWindow::init_menu()
{
   auto men = managed<Gtk::Menu>();
   auto item = managed<Gtk::MenuItem>("_File");
   item->set_use_underline();
   item->set_submenu(*men);
   menu.append(*item);
   item->show();

   auto action = managed<Gtk::MenuItem>("Quit");
   men->append(*action);
   action->signal_activate().connect(sigc::ptr_fun(Gtk::Main::quit));
   action->show();

   men = managed<Gtk::Menu>();
   item = managed<Gtk::MenuItem>("uMus_D");
   item->set_use_underline();
   item->set_submenu(*men);
   menu.append(*item);
   item->show();

   action = managed<Gtk::MenuItem>("Start");
   men->append(*action);
   action->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_fork_clicked));

   action = managed<Gtk::MenuItem>("Stop");
   men->append(*action);
   action->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_kill_clicked));

   men = managed<Gtk::Menu>();
   item = managed<Gtk::MenuItem>("_Help");
   item->set_use_underline();
   item->set_submenu(*men);
   menu.append(*item);
   item->show();

   action = managed<Gtk::MenuItem>("About");
   men->append(*action);
   action->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about));
   action->show();
}

void MainWindow::on_about()
{
   Gtk::MessageDialog diag(*this,
         "<b>uMusC</b>\n\n"
         "Small GTK+ frontend for uMusD\n"
         "<small>Copyright &#169; 2012 - Hans-Kristian Arntzen</small>", true);

   Gtk::Image img("/usr/share/icons/umusc.png");
   diag.set_image(img);
   img.show();
   diag.run();
}

bool MainWindow::on_button_press(GdkEventButton *btn)
{
   int x, y;
   progress.get_pointer(x, y);
   int width = progress.get_width();
   int height = progress.get_height();

   if (x < 0 || y < 0 || x >= width || y >= height)
      return true;

   float frac = static_cast<float>(x) / width;
   seek(frac);
   return true;
}

void MainWindow::seek(float rel)
{
   try
   {
      Connection con;
      auto res = con.command("POS\r\n");
      auto list = string_split(res, " ");
      if (list.size() != 2)
         return;

      unsigned len = std::strtoul(list[1].c_str(), nullptr, 0);
      con.command(stringify("SEEK \"", static_cast<int>(rel * len), "\"\r\n"));
      con.command("UNPAUSE\r\n");
      update_pos(con);
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

void MainWindow::add_filter(const std::string &name, const std::list<std::string> &ext)
{
   Gtk::FileFilter filt;
   filt.set_name(name);
   for (auto &str : ext)
      filt.add_pattern(str);
   diag.add_filter(filt);
}

bool MainWindow::on_timer_tick()
{
   try
   {
      Connection con;
      update_meta(con);
      update_pos(con);
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
      reset_meta_pos();
   }

   return true;
}

void MainWindow::reset_meta_pos()
{
   progress.set_text("N/A");
   progress.set_fraction(0);
   meta.title.set_text("");
   meta.artist.set_text("");
   meta.album.set_text("");
}

std::string MainWindow::sec_to_text(unsigned sec)
{
   std::string res;
   unsigned hours = sec / 3600;
   unsigned mins = (sec / 60) % 60;
   sec %= 60;

   if (hours)
      res += stringify(hours, ":");

   if (hours && mins < 10)
      res += '0';
   res += stringify(mins, ":");

   if (sec < 10)
      res += '0';
   res += stringify(sec);
   return res;
}

void MainWindow::update_pos(Connection &con)
{
   try
   {
      auto pos = con.command("POS\r\n");
      auto list = string_split(pos, " ");
      if (list.size() != 2)
      {
         progress.set_text("N/A");
         progress.set_fraction(0);
         return;
      }

      unsigned cur = std::strtoul(list[0].c_str(), nullptr, 0);
      unsigned len = std::strtoul(list[1].c_str(), nullptr, 0);
      if (cur > len || !len)
         throw std::logic_error("Current position is larger than total length.");

      progress.set_text(stringify(sec_to_text(cur), " / ", sec_to_text(len)));
      progress.set_fraction(static_cast<float>(cur) / len);
   }
   catch(const std::exception &e)
   {
      progress.set_text("N/A");
      progress.set_fraction(0);
   }
}

void MainWindow::update_meta(Connection &con)
{
   try
   {
      auto title = con.command("TITLE\r\n");
      if (title.empty())
         set_title("uMusC");
      else
         set_title(stringify("uMusC - ", title));
      meta.title.set_text(title);
      meta.artist.set_text(con.command("ARTIST\r\n"));
      meta.album.set_text(con.command("ALBUM\r\n"));
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
      meta.title.set_text("");
      meta.artist.set_text("");
      meta.album.set_text("");
   }
}

void MainWindow::play_add(const std::string &cmd, const std::vector<std::string> &paths)
{
   try
   {
      Connection con;

      std::string ret = stringify(cmd, " \"", string_join(paths, "\n"), "\"\r\n");
      ret = con.command(ret);

      if (ret != "OK")
         throw std::runtime_error(stringify("Connection: ", ret));

      update_meta(con);
      update_pos(con);
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

void MainWindow::play_file(const std::vector<std::string> &paths)
{
   play_add("PLAY", paths);
}

void MainWindow::queue_file(const std::vector<std::string> &paths)
{
   play_add("QUEUE", paths);
}

void MainWindow::on_play_clicked()
{
   play_file({});
}

void MainWindow::on_stop_clicked()
{
   try
   {
      Connection con;
      auto ret = con.command("STOP\r\n");
      if (ret != "OK")
         throw std::runtime_error(stringify("Connection: ", ret));
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

void MainWindow::on_pause_clicked()
{
   try
   {
      Connection con;
      auto ret = con.command("STATUS\r\n");
      if (ret == "PAUSED")
         con.command("UNPAUSE\r\n");
      else if (ret == "PLAYING")
         con.command("PAUSE\r\n");
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

void MainWindow::play_ctl(const std::string &cmd)
{
   try
   {
      Connection con;
      con.command(stringify(cmd, "\r\n"));
      update_meta(con);
      update_pos(con);
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

void MainWindow::on_next_clicked()
{
   play_ctl("NEXT");
}

void MainWindow::on_prev_clicked()
{
   play_ctl("PREV");
}

void MainWindow::on_open_clicked()
{
   if (diag.run() == Gtk::RESPONSE_ACCEPT)
   {
      diag.hide();
      std::vector<std::string> files;

      // This method sometimes throws awkward errors,
      // but doesn't seem to be a problem. :x
      try
      {
         files = diag.get_filenames();
      }
      catch(...)
      {}

      if (!files.empty())
      {
         queue_file({});
         play_file(files);
      }
   }
   else
      diag.hide();
}

