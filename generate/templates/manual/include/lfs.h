#ifndef GITLFS_H
#define GITLFS_H

#include <nan.h>
#include "context.h"

class GitLFS : public Nan::ObjectWrap {
   public:
    GitLFS(const GitLFS &other) = delete;
    GitLFS(GitLFS &&other) = delete;
    GitLFS& operator=(const GitLFS &other) = delete;
    GitLFS& operator=(GitLFS &&other) = delete;

    static void InitializeComponent(v8::Local<v8::Object> target, nodegit::Context *nodegitContext);
};

#endif  // GITLFS_H