#include <iostream>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <array>

namespace nodegit {
  // First testing version to run commands in MacOS, Linux and Win32
  namespace runcommand {
    std::string exec(const char* cmd) {
      std::array<char, 128> buffer;
      std::string result;
#ifdef WIN32
      auto pipe = _popen(cmd, "r");
#else
      auto pipe = popen(cmd, "r");
#endif
      
      if (!pipe) throw std::runtime_error("popen() failed!");
      
      while (!feof(pipe))
      {
        if (fgets(buffer.data(), 128, pipe) != nullptr)
          result += buffer.data();
      }
#ifdef WIN32
      auto rc = _pclose(pipe);
#else
      auto rc = pclose(pipe);
#endif
      
      if (rc == EXIT_SUCCESS)
      {
        std::cout << "SUCCESS\n";
      }
      else
      {
        std::cout << "FAILED\n";
      }
      
      return result;
    }
  }
}