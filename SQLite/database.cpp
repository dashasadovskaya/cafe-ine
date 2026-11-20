#include "database.h"
#include "sqlite3.h"

#include <iostream>
#include <sstream>

namespace {

class Statement {
public:
    Statement(sqlite3* db, const std::string& query) {
        int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt_, nullptr);
        if (rc != SQLITE_OK) {
            throw DatabaseError("Ошибка компиляции SQL (" + query + "): " + sqlite3_errmsg(db));
        }
    }

    ~Statement() {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
    }

    sqlite3_stmt* get() const { return stmt_; }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

private:
    sqlite3_stmt* stmt_ = nullptr;
};

} // namespace

Database::Database(const std::string& dbPath) {
    open(dbPath);
}

Database::~Database() {
    close();
}

void Database::open(const std::string& dbPath) {
    close();
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        close();
        throw DatabaseError("Не удалось открыть базу SQLite: " + err);
    }
    execute("PRAGMA foreign_keys = ON;");
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string errorStr = errMsg ? errMsg : "Неизвестная ошибка";
        sqlite3_free(errMsg);
        throw DatabaseError("Ошибка исполнения SQL: " + errorStr);
    }
}

void Database::beginTransaction() {
    execute("BEGIN TRANSACTION;");
}

void Database::commitTransaction() {
    execute("COMMIT;");
}

void Database::rollbackTransaction() {
    execute("ROLLBACK;");
}

std::string Database::getColumnText(sqlite3_stmt* stmt, int col) {
    const unsigned char* text = sqlite3_column_text(stmt, col);
    return text ? reinterpret_cast<const char*>(text) : "";
}

void Database::initSchema() {
    const std::string ddl = R"(
        CREATE TABLE IF NOT EXISTS menu_items (
            id          INTEGER PRIMARY KEY,
            name        TEXT NOT NULL,
            description TEXT,
            price       REAL NOT NULL,
            portion     TEXT,
            image_path  TEXT,
            category    INTEGER NOT NULL,
            available   INTEGER NOT NULL DEFAULT 1
        );

        CREATE TABLE IF NOT EXISTS users (
            id      INTEGER PRIMARY KEY,
            name    TEXT NOT NULL,
            phone   TEXT,
            address TEXT,
            points  INTEGER NOT NULL DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS cart_items (
            user_id  INTEGER NOT NULL,
            item_id  INTEGER NOT NULL,
            quantity INTEGER NOT NULL,
            PRIMARY KEY (user_id, item_id),
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY (item_id) REFERENCES menu_items(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS orders (
            id             INTEGER PRIMARY KEY,
            user_id        INTEGER NOT NULL,
            items_total    REAL NOT NULL,
            delivery_cost  REAL NOT NULL,
            points_spent   INTEGER NOT NULL,
            points_earned  INTEGER NOT NULL,
            address        TEXT NOT NULL,
            phone          TEXT NOT NULL,
            recipient      TEXT,
            comment        TEXT,
            status         INTEGER NOT NULL,
            created_at     TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(id)
        );

        CREATE TABLE IF NOT EXISTS order_items (
            order_id   INTEGER NOT NULL,
            item_id    INTEGER NOT NULL,
            name       TEXT NOT NULL,
            price      REAL NOT NULL,
            quantity   INTEGER NOT NULL,
            portion    TEXT,
            category   INTEGER NOT NULL,
            image_path TEXT,
            PRIMARY KEY (order_id, item_id),
            FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS reviews (
            id          INTEGER PRIMARY KEY,
            user_id     INTEGER NOT NULL,
            author_name TEXT NOT NULL,
            rating      INTEGER NOT NULL,
            text        TEXT,
            created_at  TEXT NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(id)
        );

        CREATE TABLE IF NOT EXISTS settings (
            key   TEXT PRIMARY KEY,
            value REAL NOT NULL
        );
    )";

    execute(ddl);
}

void Database::save(const Logic& logic) {
    beginTransaction();
    try {
        // Очистка таблиц перед полной выгрузкой состояния
        execute("DELETE FROM order_items;");
        execute("DELETE FROM orders;");
        execute("DELETE FROM cart_items;");
        execute("DELETE FROM reviews;");
        execute("DELETE FROM users;");
        execute("DELETE FROM menu_items;");
        execute("DELETE FROM settings;");

        // 1. Сохранение меню
        {
            Statement stmt(db_, "INSERT INTO menu_items (id, name, description, price, portion, image_path, category, available) "
                                "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
            for (const Item& item : logic.menu_) {
                sqlite3_reset(stmt.get());
                sqlite3_bind_int(stmt.get(), 1, item.getId());
                sqlite3_bind_text(stmt.get(), 2, item.getName().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt.get(), 3, item.getDescription().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_double(stmt.get(), 4, item.getPrice());
                sqlite3_bind_text(stmt.get(), 5, item.getPortion().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt.get(), 6, item.getImagePath().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt.get(), 7, static_cast<int>(item.getCategory()));
                sqlite3_bind_int(stmt.get(), 8, item.isAvailable() ? 1 : 0);
                sqlite3_step(stmt.get());
            }
        }

        // 2. Сохранение пользователей и корзин
        {
            Statement stmtUser(db_, "INSERT INTO users (id, name, phone, address, points) VALUES (?, ?, ?, ?, ?);");
            Statement stmtCart(db_, "INSERT INTO cart_items (user_id, item_id, quantity) VALUES (?, ?, ?);");

            for (const auto& [userId, user] : logic.users_) {
                sqlite3_reset(stmtUser.get());
                sqlite3_bind_int(stmtUser.get(), 1, user.getId());
                sqlite3_bind_text(stmtUser.get(), 2, user.getName().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmtUser.get(), 3, user.getPhone().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmtUser.get(), 4, user.getAddress().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmtUser.get(), 5, user.getPoints());
                sqlite3_step(stmtUser.get());

                for (const CartEntry& entry : user.getCart().getEntries()) {
                    sqlite3_reset(stmtCart.get());
                    sqlite3_bind_int(stmtCart.get(), 1, userId);
                    sqlite3_bind_int(stmtCart.get(), 2, entry.getItem().getId());
                    sqlite3_bind_int(stmtCart.get(), 3, entry.getQuantity());
                    sqlite3_step(stmtCart.get());
                }
            }
        }

        // 3. Сохранение заказов и состава заказов
        {
            Statement stmtOrder(db_, "INSERT INTO orders (id, user_id, items_total, delivery_cost, points_spent, points_earned, "
                                     "address, phone, recipient, comment, status, created_at) "
                                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");

            Statement stmtItem(db_, "INSERT INTO order_items (order_id, item_id, name, price, quantity, portion, category, image_path) "
                                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?);");

            for (const Order& order : logic.orders_) {
                sqlite3_reset(stmtOrder.get());
                sqlite3_bind_int(stmtOrder.get(), 1, order.getId());
                sqlite3_bind_int(stmtOrder.get(), 2, order.getUserId());
                sqlite3_bind_double(stmtOrder.get(), 3, order.getItemsTotal());
                sqlite3_bind_double(stmtOrder.get(), 4, order.getDeliveryCost());
                sqlite3_bind_int(stmtOrder.get(), 5, order.getPointsSpent());
                sqlite3_bind_int(stmtOrder.get(), 6, order.getPointsEarned());
                sqlite3_bind_text(stmtOrder.get(), 7, order.getDelivery().getAddress().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmtOrder.get(), 8, order.getDelivery().getPhone().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmtOrder.get(), 9, order.getDelivery().getRecipient().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmtOrder.get(), 10, order.getDelivery().getComment().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmtOrder.get(), 11, static_cast<int>(order.getStatus()));
                sqlite3_bind_text(stmtOrder.get(), 12, order.getCreatedAt().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmtOrder.get());

                for (const CartEntry& entry : order.getItems()) {
                    sqlite3_reset(stmtItem.get());
                    sqlite3_bind_int(stmtItem.get(), 1, order.getId());
                    sqlite3_bind_int(stmtItem.get(), 2, entry.getItem().getId());
                    sqlite3_bind_text(stmtItem.get(), 3, entry.getItem().getName().c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_double(stmtItem.get(), 4, entry.getItem().getPrice());
                    sqlite3_bind_int(stmtItem.get(), 5, entry.getQuantity());
                    sqlite3_bind_text(stmtItem.get(), 6, entry.getItem().getPortion().c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(stmtItem.get(), 7, static_cast<int>(entry.getItem().getCategory()));
                    sqlite3_bind_text(stmtItem.get(), 8, entry.getItem().getImagePath().c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_step(stmtItem.get());
                }
            }
        }

        // 4. Сохранение отзывов
        {
            Statement stmt(db_, "INSERT INTO reviews (id, user_id, author_name, rating, text, created_at) "
                                "VALUES (?, ?, ?, ?, ?, ?);");
            for (const Review& review : logic.reviews_) {
                sqlite3_reset(stmt.get());
                sqlite3_bind_int(stmt.get(), 1, review.getId());
                sqlite3_bind_int(stmt.get(), 2, review.getUserId());
                sqlite3_bind_text(stmt.get(), 3, review.getAuthorName().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt.get(), 4, review.getRating());
                sqlite3_bind_text(stmt.get(), 5, review.getText().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt.get(), 6, review.getCreatedAt().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt.get());
            }
        }

        // 5. Сохранение настроек доставки
        {
            Statement stmt(db_, "INSERT INTO settings (key, value) VALUES (?, ?);");

            sqlite3_reset(stmt.get());
            sqlite3_bind_text(stmt.get(), 1, "delivery_base_cost", -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt.get(), 2, logic.deliveryBaseCost_);
            sqlite3_step(stmt.get());

            sqlite3_reset(stmt.get());
            sqlite3_bind_text(stmt.get(), 1, "free_delivery_from", -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt.get(), 2, logic.freeDeliveryFrom_);
            sqlite3_step(stmt.get());
        }

        commitTransaction();
    } catch (...) {
        rollbackTransaction();
        throw;
    }
}

void Database::load(Logic& logic) {
    // Очистка локального in-memory состояния перед загрузкой из базы
    logic.menu_.clear();
    logic.users_.clear();
    logic.orders_.clear();
    logic.reviews_.clear();
    logic.nextItemId_ = 1;
    logic.nextUserId_ = 1;
    logic.nextOrderId_ = 1;
    logic.nextReviewId_ = 1;

    // 1. Загрузка меню
    logic.menu_ = loadMenu();
    for (const auto& item : logic.menu_) {
        logic.nextItemId_ = std::max(logic.nextItemId_, item.getId() + 1);
    }

    // 2. Загрузка пользователей
    {
        Statement stmt(db_, "SELECT id, name, phone, address, points FROM users;");
        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            userid_t id = sqlite3_column_int(stmt.get(), 0);
            std::string name = getColumnText(stmt.get(), 1);
            std::string phone = getColumnText(stmt.get(), 2);
            std::string address = getColumnText(stmt.get(), 3);
            int points = sqlite3_column_int(stmt.get(), 4);

            User user(id, name, phone, address, points);
            logic.users_.emplace(id, std::move(user));
            logic.nextUserId_ = std::max(logic.nextUserId_, id + 1);
        }
    }

    // 3. Загрузка корзин пользователей
    {
        Statement stmt(db_, "SELECT user_id, item_id, quantity FROM cart_items;");
        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            userid_t userId = sqlite3_column_int(stmt.get(), 0);
            itemid_t itemId = sqlite3_column_int(stmt.get(), 1);
            int quantity = sqlite3_column_int(stmt.get(), 2);

            auto userIt = logic.users_.find(userId);
            const Item* itemPtr = logic.findMenuItem(itemId);

            if (userIt != logic.users_.end() && itemPtr != nullptr) {
                userIt->second.getCart().addItem(*itemPtr, quantity);
            }
        }
    }

    // 4. Загрузка заказов
    logic.orders_ = loadOrders();
    for (const auto& order : logic.orders_) {
        logic.nextOrderId_ = std::max(logic.nextOrderId_, order.getId() + 1);
        auto userIt = logic.users_.find(order.getUserId());
        if (userIt != logic.users_.end()) {
            userIt->second.addOrderToHistory(order.getId());
        }
    }

    // 5. Загрузка отзывов
    logic.reviews_ = loadReviews();
    for (const auto& review : logic.reviews_) {
        logic.nextReviewId_ = std::max(logic.nextReviewId_, review.getId() + 1);
    }

    // 6. Загрузка настроек доставки
    {
        Statement stmt(db_, "SELECT key, value FROM settings;");
        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            std::string key = getColumnText(stmt.get(), 0);
            double val = sqlite3_column_double(stmt.get(), 1);
            if (key == "delivery_base_cost") {
                logic.deliveryBaseCost_ = val;
            } else if (key == "free_delivery_from") {
                logic.freeDeliveryFrom_ = val;
            }
        }
    }
}

std::vector<Item> Database::loadMenu() {
    std::vector<Item> items;
    Statement stmt(db_, "SELECT id, name, description, price, portion, image_path, category, available FROM menu_items ORDER BY id;");

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        itemid_t id = sqlite3_column_int(stmt.get(), 0);
        std::string name = getColumnText(stmt.get(), 1);
        std::string description = getColumnText(stmt.get(), 2);
        double price = sqlite3_column_double(stmt.get(), 3);
        std::string portion = getColumnText(stmt.get(), 4);
        std::string imagePath = getColumnText(stmt.get(), 5);
        ItemCategory category = static_cast<ItemCategory>(sqlite3_column_int(stmt.get(), 6));
        bool available = (sqlite3_column_int(stmt.get(), 7) != 0);

        items.emplace_back(id, name, description, price, portion, imagePath, category, available);
    }
    return items;
}

std::vector<Order> Database::loadOrders() {
    std::vector<Order> orders;

    Statement stmtOrders(db_, "SELECT id, user_id, items_total, delivery_cost, points_spent, points_earned, "
                              "address, phone, recipient, comment, status, created_at FROM orders ORDER BY id;");

    Statement stmtItems(db_, "SELECT item_id, name, price, quantity, portion, category, image_path FROM order_items WHERE order_id = ?;");

    while (sqlite3_step(stmtOrders.get()) == SQLITE_ROW) {
        orderid_t id = sqlite3_column_int(stmtOrders.get(), 0);
        userid_t userId = sqlite3_column_int(stmtOrders.get(), 1);
        double itemsTotal = sqlite3_column_double(stmtOrders.get(), 2);
        double deliveryCost = sqlite3_column_double(stmtOrders.get(), 3);
        int pointsSpent = sqlite3_column_int(stmtOrders.get(), 4);
        int pointsEarned = sqlite3_column_int(stmtOrders.get(), 5);
        std::string address = getColumnText(stmtOrders.get(), 6);
        std::string phone = getColumnText(stmtOrders.get(), 7);
        std::string recipient = getColumnText(stmtOrders.get(), 8);
        std::string comment = getColumnText(stmtOrders.get(), 9);
        OrderStatus status = static_cast<OrderStatus>(sqlite3_column_int(stmtOrders.get(), 10));
        std::string createdAt = getColumnText(stmtOrders.get(), 11);

        DeliveryInfo delivery(address, phone, recipient, comment);

        // Загрузка состава заказа
        sqlite3_reset(stmtItems.get());
        sqlite3_bind_int(stmtItems.get(), 1, id);

        std::vector<CartEntry> orderEntries;
        while (sqlite3_step(stmtItems.get()) == SQLITE_ROW) {
            itemid_t itemId = sqlite3_column_int(stmtItems.get(), 0);
            std::string name = getColumnText(stmtItems.get(), 1);
            double price = sqlite3_column_double(stmtItems.get(), 2);
            int quantity = sqlite3_column_int(stmtItems.get(), 3);
            std::string portion = getColumnText(stmtItems.get(), 4);
            ItemCategory category = static_cast<ItemCategory>(sqlite3_column_int(stmtItems.get(), 5));
            std::string imagePath = getColumnText(stmtItems.get(), 6);

            Item snapshotItem(itemId, name, "", price, portion, imagePath, category, true);
            orderEntries.emplace_back(snapshotItem, quantity);
        }

        Order order(id, userId, orderEntries, itemsTotal, deliveryCost, pointsSpent, pointsEarned, delivery, createdAt);
        order.setStatus(status);
        orders.push_back(std::move(order));
    }

    return orders;
}

std::vector<Review> Database::loadReviews() {
    std::vector<Review> reviews;
    Statement stmt(db_, "SELECT id, user_id, author_name, rating, text, created_at FROM reviews ORDER BY id;");

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        reviewid_t id = sqlite3_column_int(stmt.get(), 0);
        userid_t userId = sqlite3_column_int(stmt.get(), 1);
        std::string author = getColumnText(stmt.get(), 2);
        int rating = sqlite3_column_int(stmt.get(), 3);
        std::string text = getColumnText(stmt.get(), 4);
        std::string createdAt = getColumnText(stmt.get(), 5);

        reviews.emplace_back(id, userId, author, rating, text, createdAt);
    }
    return reviews;
}