#include "player.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char *argv[])
{
   try
   {
      Player p;
      p.run();
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
   }
}

