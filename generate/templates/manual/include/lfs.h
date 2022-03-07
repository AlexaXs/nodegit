#ifndef GITLFS_H
#define GITLFS_H

#include <nan.h>
#include "context.h"
#include "lock_master.h"
#include "lfs_types.h"
//#include <string>

extern "C" {
#include <git2.h>
}

// TODO: move the structures for lfs options to a different header 'lfs_options.h`
struct lfs_initialize_options {
  bool local {};
};

class GitLFS : public Nan::ObjectWrap {
   public:
    GitLFS(const GitLFS &other) = delete;
    GitLFS(GitLFS &&other) = delete;
    GitLFS& operator=(const GitLFS &other) = delete;
    GitLFS& operator=(GitLFS &&other) = delete;

    static void InitializeComponent(v8::Local<v8::Object> target, nodegit::Context *nodegitContext);

  private:
    struct InitializeBaton {
      int error_code;
      const git_error *error;
      git_repository *repo;
      std::unique_ptr<LfsCmd> cmd;
    };
    class InitializeWorker : public nodegit::AsyncWorker {
      public:
        InitializeWorker(
            InitializeBaton *_baton,
            Nan::Callback *callback
        ) : nodegit::AsyncWorker(callback, "nodegit:AsyncWorker:LFS:Initialize")
          , baton(_baton) {};
        InitializeWorker(const InitializeWorker &) = delete;
        InitializeWorker(InitializeWorker &&) = delete;
        InitializeWorker &operator=(const InitializeWorker &) = delete;
        InitializeWorker &operator=(InitializeWorker &&) = delete;
        ~InitializeWorker() {};
        void Execute();
        void HandleErrorCallback();
        void HandleOKCallback();
        nodegit::LockMaster AcquireLocks();

      private:
        InitializeBaton *baton {nullptr};
    };

    static NAN_METHOD(Initialize);


};

#endif  // GITLFS_H