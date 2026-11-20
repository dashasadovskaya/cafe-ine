#ifndef GUI_CART_PANEL_H
#define GUI_CART_PANEL_H

#include <wx/panel.h>

#include "logic.h"

class MainFrame;
class wxBoxSizer;
class wxButton;
class wxScrolledWindow;
class wxStaticText;

// Вкладка «Корзина»: позиции, количество, суммы и переход к оформлению.
class CartPanel : public wxPanel {
public:
    CartPanel(wxWindow* parent, MainFrame* frame, Logic& logic, userid_t userId);

    void RefreshData();

private:
    void BuildUi();
    wxPanel* CreateRow(const CartEntry& entry);
    void RebuildRows(const Cart& cart);
    void ChangeQuantity(itemid_t itemId, int delta);
    void RemoveItem(itemid_t itemId);
    void OnClear(wxCommandEvent& event);
    void OnCheckout(wxCommandEvent& event);

    MainFrame* frame_ = nullptr;
    Logic& logic_;
    userid_t userId_ = 0;

    wxScrolledWindow* scroll_ = nullptr;
    wxBoxSizer* rowsSizer_ = nullptr;
    wxStaticText* emptyLabel_ = nullptr;
    wxStaticText* itemsTotalLabel_ = nullptr;
    wxStaticText* deliveryLabel_ = nullptr;
    wxStaticText* totalLabel_ = nullptr;
    wxButton* clearButton_ = nullptr;
    wxButton* checkoutButton_ = nullptr;
};

#endif  // GUI_CART_PANEL_H
