#include <iostream>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <array>

namespace nodegit {
  // First testing version to run commands in MacOS, Linux and Win32
  namespace runcommand {
    std::string exec(const std::string &cmd, const std::string &args, const std::string &cwd, bool redirectErr = true) {
      // TODO: sanitize path
      std::string cmdRedirectErr = redirectErr ? " 2>&1" : "";
      std::string finalCmd = "cd " + cwd + " && " + cmd + " " + args + cmdRedirectErr;
#ifdef WIN32
      auto pipe = _popen(finalCmd.c_str(), "r");
#else
      auto pipe = popen(finalCmd.c_str(), "r");
#endif
      
      if (!pipe) throw std::runtime_error("popen() failed!");

      std::array<char, 128> buffer {};
      std::string result {};
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
      
      // TODO: remove cout
      if (rc == EXIT_SUCCESS)
      {
        std::cout << "cmd run: " << finalCmd << std::endl;
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