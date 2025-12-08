#pragma once

#include "execution/executor.h"
#include "execution/expression.h"
#include "catalog/schema.h"
#include <memory>

namespace dbengine {

class FilterExecutor : public Executor {
public:
    FilterExecutor(ExecutionContext *context,
                   std::unique_ptr<Executor> child,
                   std::unique_ptr<Expression> predicate,
                   const std::string &table_name)
        : Executor(context),
          child_(std::move(child)),
          predicate_(std::move(predicate)),
          table_name_(table_name) {}

    ~FilterExecutor() override = default;

    void Init() override {
        schema_ = context_->GetSchema(table_name_);
        if (schema_ == nullptr) {
            throw std::runtime_error("Schema not found: " + table_name_);
        }
        child_->Init();
    }

    bool Next(Tuple &tuple, RID &rid) override {
        while (child_->Next(tuple, rid)) {
            Value result = predicate_->Evaluate(tuple, schema_);

            if (result.GetAsInt() != 0) {
                return true;
            }
        }
        return false;
    }

private:
    std::unique_ptr<Executor> child_;
    std::unique_ptr<Expression> predicate_;
    std::string table_name_;
    Schema *schema_;
};

}
