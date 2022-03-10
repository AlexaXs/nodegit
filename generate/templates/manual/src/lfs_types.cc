#include "../include/lfs_types.h"

const char* LfsCmd::kStrLfsCmd = "git lfs";

/*** LfsCmdOptsInitialize ***/

// LfsCmdOptsInitialize::BuildArgs
std::string LfsCmdOptsInitialize::BuildArgs() const {
  std::string strArgs {};

  if (local) {
    strArgs.append(" --local");
  }
  return strArgs;
}

/*** LfsCmd ***/

// LfsCmd::LfsCmd
LfsCmd::LfsCmd(Type lfsCmdType, std::unique_ptr<LfsCmdOpts> &&opts) :
  m_lfsCmdType(lfsCmdType), m_opts(std::move(opts)) {
}

// LfsCmd::Cmd
std::string LfsCmd::Cmd() const {
  std::string strCmd {LfsCmd::kStrLfsCmd};

  switch (m_lfsCmdType) {
    case Type::kInitialize:
      strCmd.append(" install");
      break;
    default:
      break;
  }
  return strCmd;
}

// LfsCmd::Args
std::string LfsCmd::Args() const {
  return m_opts->BuildArgs();
}
