#include "storage/disk/disk_manager.h"
#include "storage/buffer/buffer_pool_manager.h"
#include "storage/table/table_heap.h"
#include "catalog/schema.h"
#include "type/value.h"
#include "execution/execution_context.h"
#include "execution/seq_scan_executor.h"
#include "execution/insert_executor.h"
#include "execution/filter_executor.h"
#include "execution/expression.h"

#include <iostream>
#include <vector>
#include <memory>

using namespace dbengine;

void PrintTestHeader(const std::string &test_name) {
    std::cout << "\n==== " << test_name << " ====" << std::endl;
}

void PrintTuple(const Tuple &tuple, const Schema *schema) {
    std::cout << "(";
    for (uint32_t i = 0; i < schema->GetColumnCount(); ++i) {
        uint32_t offset = schema->GetColumnOffset(i);
        const Column &col = schema->GetColumn(i);
        Value val = Value::DeserializeFrom(tuple.GetData() + offset, col.GetType(), col.GetFixedLength());

        if (col.GetType() == TypeId::INTEGER) {
            std::cout << val.GetAsInt();
        } else if (col.GetType() == TypeId::VARCHAR) {
            std::cout << "\"" << val.GetAsString() << "\"";
        }

        if (i < schema->GetColumnCount() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")";
}

void TestInsertAndSeqScan() {
    PrintTestHeader("Test 1: INSERT and Sequential Scan");

    std::remove("test_query_exec.db");
    DiskManager disk_manager("test_query_exec.db");
    BufferPoolManager bpm(50, &disk_manager);

    TableHeap table_heap(&bpm);

    std::vector<Column> columns = {
        Column("id", TypeId::INTEGER),
        Column("age", TypeId::INTEGER)
    };
    Schema schema(columns);

    ExecutionContext exec_context(&bpm);
    exec_context.RegisterTable("students", &table_heap, &schema);

    std::cout << "Inserting 5 student records..." << std::endl;

    std::vector<std::vector<Value>> values = {
        {Value(1), Value(20)},
        {Value(2), Value(22)},
        {Value(3), Value(19)},
        {Value(4), Value(21)},
        {Value(5), Value(23)}
    };

    InsertExecutor insert_exec(&exec_context, "students", values);
    insert_exec.Init();

    Tuple tuple;
    RID rid;
    int count = 0;
    while (insert_exec.Next(tuple, rid)) {
        count++;
    }
    std::cout << "Inserted " << count << " records" << std::endl;

    std::cout << "\nScanning all students:" << std::endl;
    SeqScanExecutor scan_exec(&exec_context, "students");
    scan_exec.Init();

    count = 0;
    while (scan_exec.Next(tuple, rid)) {
        std::cout << "  Student ";
        PrintTuple(tuple, &schema);
        std::cout << " [RID: " << rid.GetPageId() << ":" << rid.GetSlotNum() << "]" << std::endl;
        count++;
    }
    std::cout << "Total records scanned: " << count << std::endl;

    std::cout << "[SUCCESS] Test 1 passed!" << std::endl;
}

void TestFilterExecution() {
    PrintTestHeader("Test 2: Filter Execution (WHERE clause)");

    std::remove("test_query_filter.db");
    DiskManager disk_manager("test_query_filter.db");
    BufferPoolManager bpm(50, &disk_manager);

    TableHeap table_heap(&bpm);

    std::vector<Column> columns = {
        Column("id", TypeId::INTEGER),
        Column("score", TypeId::INTEGER)
    };
    Schema schema(columns);

    ExecutionContext exec_context(&bpm);
    exec_context.RegisterTable("grades", &table_heap, &schema);

    std::cout << "Inserting grade records..." << std::endl;

    std::vector<std::vector<Value>> values = {
        {Value(1), Value(85)},
        {Value(2), Value(92)},
        {Value(3), Value(78)},
        {Value(4), Value(95)},
        {Value(5), Value(88)},
        {Value(6), Value(73)},
        {Value(7), Value(91)}
    };

    InsertExecutor insert_exec(&exec_context, "grades", values);
    insert_exec.Init();

    Tuple tuple;
    RID rid;
    while (insert_exec.Next(tuple, rid)) {}

    std::cout << "Query: SELECT * FROM grades WHERE score > 85" << std::endl;

    auto scan_exec = std::make_unique<SeqScanExecutor>(&exec_context, "grades");

    auto left_expr = std::make_unique<ColumnExpression>(1);
    auto right_expr = std::make_unique<ConstantExpression>(Value(85));
    auto predicate = std::make_unique<ComparisonExpression>(
        ExpressionType::COMPARE_GREATER_THAN,
        std::move(left_expr),
        std::move(right_expr)
    );

    FilterExecutor filter_exec(&exec_context, std::move(scan_exec), std::move(predicate), "grades");
    filter_exec.Init();

    std::cout << "Results:" << std::endl;
    int count = 0;
    while (filter_exec.Next(tuple, rid)) {
        std::cout << "  ";
        PrintTuple(tuple, &schema);
        std::cout << std::endl;
        count++;
    }
    std::cout << "Total records matching filter: " << count << std::endl;

    std::cout << "[SUCCESS] Test 2 passed!" << std::endl;
}

void TestMultipleFilters() {
    PrintTestHeader("Test 3: Multiple Filter Conditions");

    std::remove("test_query_multi.db");
    DiskManager disk_manager("test_query_multi.db");
    BufferPoolManager bpm(50, &disk_manager);

    TableHeap table_heap(&bpm);

    std::vector<Column> columns = {
        Column("employee_id", TypeId::INTEGER),
        Column("salary", TypeId::INTEGER)
    };
    Schema schema(columns);

    ExecutionContext exec_context(&bpm);
    exec_context.RegisterTable("employees", &table_heap, &schema);

    std::cout << "Inserting employee records..." << std::endl;

    std::vector<std::vector<Value>> values = {
        {Value(101), Value(45000)},
        {Value(102), Value(55000)},
        {Value(103), Value(62000)},
        {Value(104), Value(48000)},
        {Value(105), Value(70000)},
        {Value(106), Value(58000)},
        {Value(107), Value(52000)},
        {Value(108), Value(75000)}
    };

    InsertExecutor insert_exec(&exec_context, "employees", values);
    insert_exec.Init();

    Tuple tuple;
    RID rid;
    while (insert_exec.Next(tuple, rid)) {}

    std::cout << "Query: SELECT * FROM employees WHERE salary >= 50000 AND salary < 70000" << std::endl;

    auto scan_exec = std::make_unique<SeqScanExecutor>(&exec_context, "employees");

    auto left1 = std::make_unique<ColumnExpression>(1);
    auto right1 = std::make_unique<ConstantExpression>(Value(50000));
    auto predicate1 = std::make_unique<ComparisonExpression>(
        ExpressionType::COMPARE_GREATER_THAN,
        std::move(left1),
        std::move(right1)
    );

    auto filter1 = std::make_unique<FilterExecutor>(
        &exec_context,
        std::move(scan_exec),
        std::move(predicate1),
        "employees"
    );

    auto left2 = std::make_unique<ColumnExpression>(1);
    auto right2 = std::make_unique<ConstantExpression>(Value(70000));
    auto predicate2 = std::make_unique<ComparisonExpression>(
        ExpressionType::COMPARE_LESS_THAN,
        std::move(left2),
        std::move(right2)
    );

    FilterExecutor filter2(&exec_context, std::move(filter1), std::move(predicate2), "employees");
    filter2.Init();

    std::cout << "Results:" << std::endl;
    int count = 0;
    while (filter2.Next(tuple, rid)) {
        std::cout << "  ";
        PrintTuple(tuple, &schema);
        std::cout << std::endl;
        count++;
    }
    std::cout << "Total records matching filters: " << count << std::endl;

    std::cout << "[SUCCESS] Test 3 passed!" << std::endl;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "   Query Execution Engine Tests        " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        TestInsertAndSeqScan();
        TestFilterExecution();
        TestMultipleFilters();

        std::cout << "\n========================================" << std::endl;
        std::cout << "   ALL TESTS PASSED!                   " << std::endl;
        std::cout << "========================================\n" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
