#ifndef NODEGIT_LFS_CMD_H
#define NODEGIT_LFS_CMD_H

#include <memory>
#include <string>
#include "cmd.h"

namespace nodegit {
/**
 * \struct LfsCmdOpts
 * Abstract structure to store options for the LFS commands.
 */
struct LfsCmdOpts {
  LfsCmdOpts() = default;
  virtual ~LfsCmdOpts() = default;
  LfsCmdOpts(const LfsCmdOpts &other) = delete;
  LfsCmdOpts(LfsCmdOpts &&other) = delete;
  LfsCmdOpts& operator=(const LfsCmdOpts &other) = delete;
  LfsCmdOpts& operator=(LfsCmdOpts &&other) = delete;

  virtual std::string BuildArgs() const = 0;
};

/**
 * \struct LfsCmdOptsInitialize
 */
struct LfsCmdOptsInitialize final : public LfsCmdOpts {
  LfsCmdOptsInitialize() = default;
  ~LfsCmdOptsInitialize() = default;
  LfsCmdOptsInitialize(const LfsCmdOptsInitialize &other) = delete;
  LfsCmdOptsInitialize(LfsCmdOptsInitialize &&other) = delete;
  LfsCmdOptsInitialize& operator=(const LfsCmdOptsInitialize &other) = delete;
  LfsCmdOptsInitialize& operator=(LfsCmdOptsInitialize &&other) = delete;

  std::string BuildArgs() const;

  bool local {false};
};

/**
 * \class LfsCmd
 * An LFS command to execute.
 */
class LfsCmd : public Cmd {
public:
  static const char* kStrLfsCmd;

  // Enumeration describing the type of LFS command
  enum class Type {kNone, kInitialize};

  LfsCmd(Type lfsCmdType, std::unique_ptr<LfsCmdOpts> &&opts);
  LfsCmd() = delete;
  ~LfsCmd() = default;
  LfsCmd(const LfsCmd &other) = delete;
  LfsCmd(LfsCmd &&other) = delete;
  LfsCmd& operator=(const LfsCmd &other) = delete;
  LfsCmd& operator=(LfsCmd &&other) = delete;

  std::string Command() const;
  std::string Args() const;

private:
  Type m_lfsCmdType {Type::kNone};
  std::unique_ptr<LfsCmdOpts> m_opts {};
};
}       // namespace nodegit
#endif  // NODEGIT_LFS_CMD_H