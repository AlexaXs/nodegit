#ifndef NODEGIT_CMD_H
#define NODEGIT_CMD_H

#include <string>
#include <map>

namespace nodegit {
/**
 * \class Cmd
 * Abstract class holding a command to execute.
 * 
 * Instances of Cmd will store all the information needed to run the command,
 * including environment variables to set, like CWD.
 */
class Cmd {
public:
  /* Enumeration describing the environment variables that Cmd will handle
  * - kCWD: Current Working Directory.
  */
  enum class Env {kCWD};

  Cmd() = default;
  virtual ~Cmd() = default;
  Cmd(const Cmd &other) = delete;
  Cmd(Cmd &&other) = delete;
  Cmd& operator=(const Cmd &other) = delete;
  Cmd& operator=(Cmd &&other) = delete;

  virtual std::string Command() const = 0;    // return the command to execute
  virtual std::string Args() const = 0;   // return the arguments of the command if any

  void SetEnv(Env env, const std::string &value) {
    m_env[env] = value;
  }
  std::string GetEnv(Env env) const {
    auto value = m_env.find(env);
    if (value != m_env.end()) { return value->second; }
    else { return std::string(""); }
  }
  void SetRedirectStdErr(bool redirectStdErr) {
    m_redirectStdErr = redirectStdErr;
  }
  bool GetRedirectStdErr() const {
    return m_redirectStdErr;
  }

  // TODO: turn private, and restrict set/get access with pre-validation if necessary
  std::string out {};        // cmd output. TODO: stream here?
  std::string errorMsg {};   // error message if the command execution failed

private:
  std::map<Env, std::string> m_env {};  // environment variables for this command
  bool m_redirectStdErr {true};  // whether or not to redirect stderr to stdout
};
} // namespace nodegit
#endif  // NODEGIT_CMD_H