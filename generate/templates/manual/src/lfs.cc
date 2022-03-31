#include <nan.h>
#include "../include/nodegit.h"
#include "../include/context.h"
#include "../include/lfs.h"

using namespace v8;

void GitLFS::InitializeComponent(v8::Local<v8::Object> target, nodegit::Context *nodegitContext) {
  Nan::HandleScope scope;

  v8::Local<Object> lfs = Nan::New<Object>();

  Nan::Set(target, Nan::New<String>("LFS").ToLocalChecked(), lfs);
  nodegitContext->SaveToPersistent("LFS", lfs);
}