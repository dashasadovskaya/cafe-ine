#ifndef DATABASE_H
#define DATABASE_H

#include "logic.h"
#include <string>
#include <vector>
#include <stdexcept>

struct sqlite3;
struct sqlite3_stmt;

class DatabaseError : public std::runtime_error {
public:
    explicit DatabaseError(const std::string& message) : std::runtime_error(message) {}
};

class Database {
public:
    explicit Database(const std::string& dbPath);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void open(const std::string& dbPath);
    void close();
    void initSchema();

    // Полная синхронизация состояния с объектом Logic
    void save(const Logic& logic);
    void load(Logic& logic);

    // Точечные операции выборки
    std::vector<Item> loadMenu();
    std::vector<Order> loadOrders();
    std::vector<Review> loadReviews();

    void execute(const std::string& sql);

private:
    sqlite3* db_ = nullptr;

    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();

    static std::string getColumnText(sqlite3_stmt* stmt, int col);
};

#endif // DATABASE_H