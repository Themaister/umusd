#include "mainwindow.hpp"
#include <iostream>

MainWindow::MainWindow() :
   play(Gtk::Stock::MEDIA_PLAY), stop(Gtk::Stock::MEDIA_STOP),
   pause(Gtk::Stock::MEDIA_PAUSE), open(Gtk::Stock::OPEN),
   grid(3, 2), diag(*this, "Open File ...")
{
   spawn();
   set_title("uMusC");
   set_icon_from_file("/usr/share/icons/umusc.png");
   set_border_width(5);

   play.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_play_clicked));
   stop.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_stop_clicked));
   pause.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_pause_clicked));
   open.signal_clicked().connect(
         sigc::mem_fun(*this, &MainWindow::on_open_clicked));

   vbox.set_spacing(3);
   hbox.pack_start(open);
   hbox.pack_start(*Gtk::manage(new Gtk::VSeparator));
   hbox.pack_start(play);
   hbox.pack_start(pause);
   hbox.pack_start(stop);

   init_menu();
   vbox.pack_start(menu);
   vbox.pack_start(hbox);
   vbox.pack_start(progress);
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

   add_filter("Media files", {"*.mp3", "*.flac", "*.ogg", "*.m4a", "*.wav"});
   add_filter("MP3 files", {"*.mp3"});
   add_filter("FLAC files", {"*.flac"});
   add_filter("Vorbis files", {"*.ogg"});
   add_filter("M4A files", {"*.m4a"});
   add_filter("WAV files", {"*.wav"});
   add_filter("Any file", {"*"});

   add(vbox);
   show_all();

   Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &MainWindow::on_timer_tick), 1); 
   on_timer_tick();
}

template <class T, class... A>
inline T* managed(A&&... args)
{
   return Gtk::manage(new T(std::forward<A>(args)...));
}

void MainWindow::spawn()
{
   try
   {
      Glib::spawn_command_line_async("umusd");
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

void MainWindow::init_menu()
{
   auto men = managed<Gtk::Menu>();
   auto item = managed<Gtk::MenuItem>("File");
   item->set_submenu(*men);
   menu.append(*item);
   item->show();

   auto action = managed<Gtk::MenuItem>("Quit");
   men->append(*action);
   action->signal_activate().connect(sigc::ptr_fun(Gtk::Main::quit));
   action->show();

   men = managed<Gtk::Menu>();
   item = managed<Gtk::MenuItem>("Help");
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
         "Copyright (C) 2012 - Hans-Kristian Arntzen", true);
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
         throw std::logic_error("Current position is larger than total length.\n");

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
      meta.title.set_text(con.command("TITLE\r\n"));
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

void MainWindow::play_file()
{
   try
   {
      Connection con;
      auto ret = con.command(stringify("PLAY \"", last_file, "\"\r\n"));
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

void MainWindow::on_play_clicked()
{
   if (last_file.empty())
      return on_open_clicked();
   else
      play_file();
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

void MainWindow::on_open_clicked()
{
   if (diag.run() == Gtk::RESPONSE_ACCEPT)
   {
      std::string file;
      try // This method sometimes throws awkward errors, but doesn't seem to be a problem. :x
      {
         file = diag.get_filename();
      }
      catch(...)
      {}

      last_file = file;
      play_file();
   }

   diag.hide();
}

