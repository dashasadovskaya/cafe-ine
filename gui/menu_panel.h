#ifndef GUI_MENU_PANEL_H
#define GUI_MENU_PANEL_H

#include <wx/panel.h>

#include <vector>

#include "logic.h"

class MainFrame;
class wxChoice;
class wxFlexGridSizer;
class wxScrolledWindow;
class wxStaticText;
class wxTextCtrl;

// Вкладка «Меню»: карточки блюд, фильтр по категориям и поиск.
class MenuPanel : public wxPanel {
public:
    MenuPanel(wxWindow* parent, MainFrame* frame, Logic& logic, userid_t userId);

    void RefreshData();

private:
    void BuildUi();
    wxPanel* CreateCard(const Item& item);
    std::vector<Item> CollectItems() const;
    void RebuildCards();
    void AddItemToCart(itemid_t itemId);

    MainFrame* frame_ = nullptr;
    Logic& logic_;
    userid_t userId_ = 0;

    wxChoice* categoryChoice_ = nullptr;
    wxTextCtrl* searchCtrl_ = nullptr;
    wxScrolledWindow* scroll_ = nullptr;
    wxFlexGridSizer* cardSizer_ = nullptr;
    wxStaticText* emptyLabel_ = nullptr;
    std::vector<ItemCategory> categories_;
};

#endif  // GUI_MENU_PANEL_H
