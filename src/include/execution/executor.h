#pragma once

#include "storage/table/tuple.h"
#include "execution/execution_context.h"

namespace dbengine {

class Executor {
public:
    // Prevents implicit type conversion. 
    explicit Executor(ExecutionContext *context) : context_(context) {}

    virtual ~Executor() = default;

    virtual void Init() = 0;

    virtual bool Next(Tuple &tuple, RID &rid) = 0;

protected:
    ExecutionContext *context_;
};

}
