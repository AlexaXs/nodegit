#include "../include/lfs_cmd.h"

const char* nodegit::LfsCmd::kStrLfsCmd = "git lfs";

/**
 * LfsCmd::LfsCmd
 * 
 * Constructor from type and options.
 */
nodegit::LfsCmd::LfsCmd(Type lfsCmdType, std::unique_ptr<nodegit::LfsCmdOpts> &&opts) :
  m_lfsCmdType(lfsCmdType), m_opts(std::move(opts)) {
}

/**
 * nodegit::LfsCmd::Command
 * 
 * \return string with LFS command to execute.
 */
std::string nodegit::LfsCmd::Command() const {
  std::string strCmd {nodegit::LfsCmd::kStrLfsCmd};

  switch (m_lfsCmdType) {
    default:
      break;
  }
  return strCmd;
}

/**
 * nodegit::LfsCmd::Args
 * 
 * \return string with arguments for this LFS command.
 */
std::string nodegit::LfsCmd::Args() const {
  return m_opts->BuildArgs();
}
