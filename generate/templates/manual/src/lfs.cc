#include <nan.h>

extern "C" {
  #include <git2.h>
}

#include "../include/nodegit.h"
#include "../include/context.h"
#include "../include/lock_master.h"
#include "../include/functions/copy.h"
#include "../include/lfs.h"
#include "../include/run_command.h"
#include "../include/repository.h"

using namespace v8;

void GitLFS::InitializeComponent(v8::Local<v8::Object> target, nodegit::Context *nodegitContext) {
  Nan::HandleScope scope;

  v8::Local<Object> lfs = Nan::New<Object>();

  Local<External> nodegitExternal = Nan::New<External>(nodegitContext);
  Nan::SetMethod(lfs, "initialize", Initialize, nodegitExternal);

  Nan::Set(target, Nan::New<String>("LFS").ToLocalChecked(), lfs);
  nodegitContext->SaveToPersistent("LFS", lfs);
}

NAN_METHOD(GitLFS::Initialize) {
  if (info.Length() == 0 || !info[0]->IsObject()) {
    return Nan::ThrowError("Repository repo is required.");
  }

  if (info.Length() >= 3 && !info[1]->IsNull() && !info[1]->IsUndefined() && !info[1]->IsObject()) {
    return Nan::ThrowError("Options must be an object, null, or undefined.");
  }

  if (!info[info.Length() - 1]->IsFunction()) {
    return Nan::ThrowError("Callback is required and must be a Function.");
  }

  InitializeBaton* baton = new InitializeBaton();

  baton->error_code = GIT_OK;
  baton->error = NULL;
  baton->repo = Nan::ObjectWrap::Unwrap<GitRepository>(Nan::To<v8::Object>(info[0]).ToLocalChecked())->GetValue();

  std::unique_ptr<nodegit::LfsCmdOptsInitialize> lfsCmdOpts = std::make_unique<nodegit::LfsCmdOptsInitialize>();
  if (info.Length() == 3 && info[1]->IsObject()) {
    v8::Local<v8::Object> options = Nan::To<v8::Object>(info[1]).ToLocalChecked();
    v8::Local<v8::Value> maybeBool = nodegit::safeGetField(options, "local");
    if (!maybeBool.IsEmpty() && !maybeBool->IsUndefined() && !maybeBool->IsNull()) {
      if (!maybeBool->IsBoolean()) {
        return Nan::ThrowError("Must pass Boolean to local");
      }
      lfsCmdOpts->local = static_cast<bool>(Nan::To<bool>(maybeBool).FromJust());
    }
  }
  baton->cmd = std::make_unique<nodegit::LfsCmd>(nodegit::LfsCmd::Type::kInitialize, std::move(lfsCmdOpts));

  Nan::Callback *callback = new Nan::Callback(v8::Local<Function>::Cast(info[info.Length() - 1]));
  InitializeWorker *worker = new InitializeWorker(baton, callback);

  worker->Reference<GitRepository>("repo", info[0]);

  nodegit::Context *nodegitContext = reinterpret_cast<nodegit::Context *>(info.Data().As<External>()->Value());
  nodegitContext->QueueWorker(worker);
  return;
}

nodegit::LockMaster GitLFS::InitializeWorker::AcquireLocks() {
  nodegit::LockMaster lockMaster(
    /*asyncAction: */true
          ,baton->repo
  );
  return lockMaster;
}

void GitLFS::InitializeWorker::Execute() {
  git_error_clear();

  // TODO: fill out
  int result = GIT_OK;
  baton->error_code = result;
  baton->cmd->SetEnv(nodegit::Cmd::Env::kCWD, git_repository_workdir(baton->repo));

  // TODO: remove cout
  if (nodegit::runcommand::exec(baton->cmd.get())) {
    std::cout << "exec cmd SUCCESS: <" << baton->cmd->out << ">" << std::endl;
  }
  else {
    std::cout << "exec cmd FAILED: <" << baton->cmd->errorMsg << ">" << std::endl;
    result = GIT_EUSER;
  }

  if (result != GIT_OK && git_error_last() != NULL) {
    baton->error = git_error_dup(git_error_last());
  }
}

void GitLFS::InitializeWorker::HandleErrorCallback() {
  if (!GetIsCancelled()) {
    v8::Local<v8::Object> err = Nan::To<v8::Object>(Nan::Error(ErrorMessage())).ToLocalChecked();
    Nan::Set(err, Nan::New("errorFunction").ToLocalChecked(), Nan::New("LFS.initialize").ToLocalChecked());
    v8::Local<v8::Value> argv[1] = {
      err
    };
    callback->Call(1, argv, async_resource);
  }

  if (baton->error) {
    if (baton->error->message) {
      free((void *)baton->error->message);
    }

    free((void *)baton->error);
  }

  delete baton;
}

void GitLFS::InitializeWorker::HandleOKCallback() {
    if (baton->error_code == GIT_OK) {
    v8::Local<v8::Value> result = Nan::Undefined();
   
    v8::Local<v8::Value> argv[2] = {
      Nan::Null(),
      result
    };
    callback->Call(2, argv, async_resource);
  } else {
    if (baton->error) {
      v8::Local<v8::Object> err;
      if (baton->error->message) {
        err = Nan::To<v8::Object>(Nan::Error(baton->error->message)).ToLocalChecked();
      } else {
        err = Nan::To<v8::Object>(Nan::Error("Method initialize has thrown an error.")).ToLocalChecked();
      }
      Nan::Set(err, Nan::New("errno").ToLocalChecked(), Nan::New(baton->error_code));
      Nan::Set(err, Nan::New("errorFunction").ToLocalChecked(), Nan::New("LFS.initialize").ToLocalChecked());
      v8::Local<v8::Value> argv[1] = {
        err
      };
      callback->Call(1, argv, async_resource);
      if (baton->error->message) {
        free((void *)baton->error->message);
      }
      free((void *)baton->error);
    } else if (baton->error_code < 0) {
      bool callbackFired = false;
      if (!callbackErrorHandle.IsEmpty()) {
        v8::Local<v8::Value> maybeError = Nan::New(callbackErrorHandle);
        if (!maybeError->IsNull() && !maybeError->IsUndefined()) {
          v8::Local<v8::Value> argv[1] = {
            maybeError
          };
          callback->Call(1, argv, async_resource);
          callbackFired = true;
        }
      }

      if (!callbackFired) {
        v8::Local<v8::Object> err = Nan::To<v8::Object>(Nan::Error("Method initialize has thrown an error.")).ToLocalChecked();
        Nan::Set(err, Nan::New("errno").ToLocalChecked(), Nan::New(baton->error_code));
        Nan::Set(err, Nan::New("errorFunction").ToLocalChecked(), Nan::New("LFS.initialize").ToLocalChecked());
        v8::Local<v8::Value> argv[1] = {
          err
        };
        callback->Call(1, argv, async_resource);
      }
    } else {
      callback->Call(0, NULL, async_resource);
    }
  }

  delete baton;
}