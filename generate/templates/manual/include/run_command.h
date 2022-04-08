#ifndef NODEGIT_RUN_CMD_H
#define NODEGIT_RUN_CMD_H

#include <iostream>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <array>
#include "cmd.h"

namespace nodegit {
  // TODO:
  // This is just a first testing version to run commands in MacOS, Linux and Win32
  namespace runcommand {
    /**
     * \brief Executes a command.
     * 
     * \param cmd object storing all the information needed to execute a command. Result
     * of the execution will also be stored here.
     * \return true if command execution succeeded; false otherwise.
     */
    bool exec(Cmd *cmd) {
      // TODO: sanitize path
      assert (cmd != nullptr);
      std::string cwd = cmd->GetEnv(Cmd::Env::kCWD);
      std::string cmdCwd = cwd.empty() ? std::string("") : std::string("cd ").append(cwd).append(" && ");
      std::string cmdArgs = cmd->Args();
      std::string finalCmdArgs = cmdArgs.empty() ? std::string("") : std::string(" ").append(cmdArgs);
      std::string cmdRedirectErr = cmd->GetRedirectStdErr() ? " 2>&1" : std::string("");
      std::string finalCmd = cmdCwd + cmd->Command() + finalCmdArgs + cmdRedirectErr;

#ifdef WIN32
      auto pipe = _popen(finalCmd.c_str(), "r");
#else
      auto pipe = popen(finalCmd.c_str(), "r");
#endif
      if (!pipe) {
        cmd->errorMsg = "popen() failed!";
        return false;
      }

      std::array<char, 128> buffer {};
      while (!feof(pipe)) {
        if (fgets(buffer.data(), 128, pipe) != nullptr)
          cmd->out += buffer.data();
      }
#ifdef WIN32
      auto rc = _pclose(pipe);
#else
      auto rc = pclose(pipe);
#endif
      
      if (rc != EXIT_SUCCESS) {
        cmd->errorMsg = "pclose() failed!";
        return false;
      }
      return true;
    }
  } // namespace runcommand
} // namespace nodegit
#endif  // NODEGIT_RUN_CMD_H