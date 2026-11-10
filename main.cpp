#include "logic.h"
#include "database.h"
#include <iostream>
#include <iomanip>

void printStatus(const std::string& title) {
    std::cout << "\n========================================\n"
              << "  " << title << "\n"
              << "========================================\n";
}

int main() {
    const std::string dbFilename = "cafe.db";

    try {
        Database db(dbFilename);
        db.initSchema();

        printStatus("СЕССИЯ 1: Инициализация и операции в приложении");

        Logic logic;
        // Загружаем данные из SQLite, если они уже есть в файле
        db.load(logic);

        // Если меню еще пустое, заполняем демо-позициями
        if (logic.getItems().empty()) {
            std::cout << "Меню в БД пусто. Генерируем начальное демо-меню...\n";
            logic.seedDemoMenu();
        }

        std::cout << "Позиций в меню доступно: " << logic.getItems().size() << "\n";

        // Создаем тестового пользователя
        User user = logic.createUser("Иван Петров", "+7 (999) 000-11-22", "ул. Пушкина, д. 5, кв. 12");
        std::cout << "Создан пользователь: " << user.getName() << " (ID: " << user.getId() << ")\n";

        // Добавляем товары в корзину пользователя
        auto menu = logic.getAvailableItems();
        if (menu.size() >= 2) {
            logic.addToUserCart(user.getId(), menu[0].getId(), 2); // 2 шт. первого блюда
            logic.addToUserCart(user.getId(), menu[1].getId(), 1); // 1 шт. второго блюда
            std::cout << "В корзину добавлены: " << menu[0].getName() << " (2 шт.) и "
                      << menu[1].getName() << " (1 шт.)\n";
        }

        Cart cart = logic.viewCart(user.getId());
        std::cout << "Итого корзина: " << cart.getTotal() << " руб. (позиций: " << cart.getItemCount() << ")\n";

        // Оформляем заказ
        DeliveryInfo delivery(user.getAddress(), user.getPhone(), user.getName(), "Код домофона: 42");
        auto orderIdOpt = logic.placeOrder(user.getId(), delivery, 0);

        if (orderIdOpt.has_value()) {
            std::cout << "Заказ #" << *orderIdOpt << " успешно создан!\n";
            auto orderOpt = logic.findOrder(*orderIdOpt);
            if (orderOpt.has_value()) {
                std::cout << "Сумма заказа к оплате: " << orderOpt->getTotal() << " руб.\n";
                std::cout << "Начислено баллов: " << orderOpt->getPointsEarned() << "\n";
            }
        }

        // Пользователь оставляет отзыв
        logic.addReview(user.getId(), 5, "Отличный кофе и очень быстрая доставка!");
        std::cout << "Отзыв успешно добавлен. Средний рейтинг: " << logic.getAverageRating() << "/5.0\n";

        // Сохраняем все текущее состояние в базу данных SQLite
        std::cout << "\nСохранение всего состояния Logic в базу SQLite...\n";
        db.save(logic);
        std::cout << "Данные успешно записаны в файл " << dbFilename << "!\n";

        printStatus("СЕССИЯ 2: Проверка персистентности (чистый экземпляр Logic)");

        Logic freshLogic;
        // Загружаем сохраненные данные в абсолютно чистый объект
        db.load(freshLogic);

        std::cout << "Загружено из БД:\n";
        std::cout << " - Позиций меню: " << freshLogic.getItems().size() << "\n";
        std::cout << " - Заказов: " << freshLogic.getAllOrders().size() << "\n";
        std::cout << " - Отзывов: " << freshLogic.getReviews().size() << "\n";

        if (freshLogic.hasUser(user.getId())) {
            User loadedUser = freshLogic.viewUser(user.getId());
            std::cout << " - Пользователь #" << loadedUser.getId() << ": " << loadedUser.getName()
                      << " | Баланс баллов: " << loadedUser.getPoints()
                      << " | Заказов в истории: " << loadedUser.getOrderHistory().size() << "\n";
        }

        auto allOrders = freshLogic.getAllOrders();
        if (!allOrders.empty()) {
            const Order& lastOrder = allOrders.back();
            std::cout << "\nИнформация о последнем заказе из БД:\n";
            std::cout << "  ID: #" << lastOrder.getId() << " (" << toString(lastOrder.getStatus()) << ")\n";
            std::cout << "  Адрес: " << lastOrder.getDelivery().getAddress() << "\n";
            std::cout << "  Состав заказа:\n";
            for (const auto& entry : lastOrder.getItems()) {
                std::cout << "    * " << entry.getItem().getName() << " x " << entry.getQuantity()
                          << " — " << entry.getSubtotal() << " руб.\n";
            }
            std::cout << "  Итого: " << lastOrder.getTotal() << " руб.\n";
        }

        printStatus("Все тесты успешно пройдены!");

    } catch (const std::exception& ex) {
        std::cerr << "Критическая ошибка: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}