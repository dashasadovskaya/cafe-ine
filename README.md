# Кафе-Ин

Приложение для заказа блюд из кафе

## Функционал:

- Меню (наименование, вес/размер, состав, цена, картинка)
- Корзина для добавления и заказа блюд
- Профиль (история заказов, баллы)
- О нас(написать отзыв)
- Доставка

## Требования

- CMake **3.20 или новее**
- Компилятор с поддержкой **C++17** (MSVC, GCC или Clang)
- wxWidgets **3.2** (компоненты `core` и `base`)
- Для GUI нужна графическая сессия: X11/Wayland на Linux, обычный Desktop на Windows

## Сборка и запуск

### Linux

Установите зависимости. Например, для Arch:

```bash
sudo pacman -S wxwidgets-gtk3 cmake
```

Для Debian/Ubuntu:

```bash
sudo apt install build-essential cmake libwxgtk3.2-dev
```

Для Fedora:

```bash
sudo dnf install gcc-c++ cmake wxGTK-devel
```

> В старых версиях Ubuntu пакет wxWidgets может называться `libwxgtk3.0-gtk3-dev`.

Из корня репозитория выполните:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j$(nproc)
```

Запуск:

```bash
./build/cafe-in
```

Исполняемый файл находится по пути `build/cafe-in`. Приложение использует wxGTK, поэтому для запуска нужна графическая сессия (переменная `DISPLAY` или `WAYLAND_DISPLAY`).

### Windows (Visual Studio)

1. Установите **Visual Studio 2022** с рабочей нагрузкой *Desktop development with C++* (либо другой актуальной версией с MSVC).
2. Установите CMake (можно использовать версию, входящую в Visual Studio). Проверить версию можно командой `cmake --version`.
3. Скачайте с сайта [wxWidgets](https://wxwidgets.org/downloads/) кнопкой *Download Windows Binaries* архивы **Header Files** и **Development Files** и распакуйте оба в одну папку, например `C:\Libraries\wxWidgets`. Внутри должны быть каталоги `include` и `lib`.
4. Задайте переменную окружения `wxWidgets_ROOT` (CMake автоматически преобразует её в `wxWidgets_ROOT_DIR`):

   ```powershell
   setx wxWidgets_ROOT "C:\Libraries\wxWidgets"
   ```

   После этого перезапустите терминал и Visual Studio. Можно также не задавать переменную, а сразу передать путь при конфигурации через `-DwxWidgets_ROOT_DIR="C:\Libraries\wxWidgets"`.

В **Developer PowerShell for VS 2022** из корня репозитория выполните:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Запуск:

```powershell
.\build\Release\cafe-in.exe
```

Исполняемый файл находится по пути `build\Release\cafe-in.exe`. CMake автоматически копирует рядом с ним библиотеки wxWidgets (`*.dll`) и папку `images`, поэтому приложение можно запускать прямо из папки сборки.

> Разрядность `-A x64` должна совпадать с разрядностью скачанных библиотек wxWidgets. Если использованы 32-битные библиотеки, укажите `-A Win32`. Для Visual Studio 2019 генератор называется `"Visual Studio 16 2019"`.

## Тесты

После сборки можно запустить тесты логики и работы с БД:

```bash
ctest --test-dir build --output-on-failure
```

В Windows с генератором Visual Studio укажите конфигурацию:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Исполняемые файлы можно запускать и напрямую:

```bash
./build/test_logic       # консольная проверка бизнес-логики
./build/cafe_db_demo     # создаёт/открывает cafe-in.db в текущей папке
```

На Windows они находятся в `build\Release\` (для генератора Visual Studio).

## Изображения меню

Пути к картинкам заданы в `app/logic.cpp` в функции `seedDemoMenu()`, например `images/espresso.png`. Достаточно положить файлы с этими именами в папку `images/`:

- `espresso.png`, `cappuccino.png`, `latte.png`, `tea.png`
- `croissant.png`, `syrniki.png`, `carbonara.png`, `cheesecake.png`

CMake копирует всю папку `images/` рядом с исполняемым файлом при пересборке цели `cafe-in`. Если картинки добавлены **после** сборки, скопируйте их вручную в `build/images/` (на Windows — в `build\Release\images\`) или пересоберите проект, затем перезапустите приложение. Отсутствующие файлы заменяются на `images/placeholder.png`.

## Логика программы

Логика находится в `app/logic.h` и `app/logic.cpp` и не зависит от GUI:

- `Item` — блюдо меню: название, состав, вес/размер, цена, картинка, категория;
- `Cart` / `CartEntry` — корзина, количество и стоимость позиций;
- `User` — профиль, корзина, баллы и история заказов;
- `Order` — заказ, статус, доставка, скидка и начисленные баллы;
- `DeliveryInfo` — адрес, телефон и комментарий к доставке;
- `Review` — отзыв с оценкой от 1 до 5;
- `Logic` — единая точка входа для меню, корзины, заказов, пользователей и отзывов.

Правила баллов и доставки: 1 балл начисляется за каждые 100 Br, 1 балл = 1 Br скидки
(не более 50% стоимости блюд); доставка стоит 15 Br и бесплатна от 100 Br.
Значения можно изменить через `Logic::setDeliverySettings`.

## Распределение задач

- Даниил Друк: тим лид и логика программы;
- Бармин Стефан: база данных;
- Садовская Дарья: GUI;
