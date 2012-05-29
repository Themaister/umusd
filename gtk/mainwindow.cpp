#include "mainwindow.hpp"
#include <iostream>

MainWindow::MainWindow() :
   play(Gtk::Stock::MEDIA_PLAY), stop(Gtk::Stock::MEDIA_STOP),
   pause(Gtk::Stock::MEDIA_PAUSE), open(Gtk::Stock::OPEN)
{
   set_title("uMusC");
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

   vbox.pack_start(hbox);
   vbox.pack_start(progress);

   progress.set_fraction(0.0);
   progress.set_text("N/A");

   add(vbox);
   show_all();
}

void MainWindow::play_file()
{
   try
   {
      Connection con;
      auto ret = con.command(stringify("PLAY \"", last_file, "\"\r\n"));
      if (ret != "OK")
         throw std::runtime_error(stringify("Connection: ", ret));
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
   std::cerr << "Pause" << std::endl;
}

void MainWindow::on_open_clicked()
{
   GtkFileChooserDialog *c_dia = GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new("Open File ...",
         GTK_WINDOW(gobj()), GTK_FILE_CHOOSER_ACTION_OPEN,
         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, nullptr));

   Gtk::FileChooserDialog *diag = Glib::wrap(c_dia);

   Gtk::FileFilter filt;
   filt.set_name("Any file");
   filt.add_pattern("*");
   diag->add_filter(filt);

   if (diag->run() == Gtk::RESPONSE_ACCEPT)
   {
      auto file = diag->get_filename();
      std::cerr << "Path: " << file << std::endl;
      last_file = file;
      play_file();
   }

   delete diag;
}

