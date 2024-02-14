#include <iostream>

#include "main-ui.hpp"

// using namespace ftxui;
using namespace std;

int main() 
{

    try
    {
      DisplayMainUI( true);
    }
    catch(const std::exception& e)
    {
      std::cout << e.what() << '\n';
    }
    catch (...) 
    {
        // Catch any other exception (should be the last catch block)
        std::cout << "Caught an unknown exception!" << std::endl;
    }
    
  return 0;
}
