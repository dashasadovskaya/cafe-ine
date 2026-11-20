#include "gui/cart_panel.h"

#include <wx/wx.h>

#include <exception>

#include "gui/main_frame.h"
#include "gui/utils.h"

CartPanel::CartPanel(wxWindow* parent, MainFrame* frame, Logic& logic, userid_t userId)
    : wxPanel(parent, wxID_ANY), frame_(frame), logic_(logic), userId_(userId) {
    BuildUi();
}

void CartPanel::BuildUi() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    scroll_ = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxVSCROLL);
    scroll_->SetScrollRate(0, 12);
    rowsSizer_ = new wxBoxSizer(wxVERTICAL);
    scroll_->SetSizer(rowsSizer_);
    root->Add(scroll_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    emptyLabel_ = new wxStaticText(
        this, wxID_ANY,
        gui::U8("Корзина пуста.\nДобавьте блюда из вкладки «Меню»."),
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    emptyLabel_->SetForegroundColour(wxColour(120, 120, 120));
    root->Add(emptyLabel_, 1, wxALIGN_CENTER | wxALL, 24);
    emptyLabel_->Hide();

    auto* summary = new wxStaticBoxSizer(wxVERTICAL, this, gui::U8("Итого"));
    auto* summaryGrid = new wxFlexGridSizer(2, 6, 12);
    summaryGrid->AddGrowableCol(1, 1);

    const auto addSummaryRow = [this, summaryGrid](const wxString& title,
                                                   wxStaticText*& value,
                                                   bool bold) {
        summaryGrid->Add(new wxStaticText(this, wxID_ANY, title), 0,
                         wxALIGN_CENTER_VERTICAL);
        value = new wxStaticText(this, wxID_ANY, gui::FormatPrice(0.0),
                                 wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
        if (bold) {
            wxFont font = value->GetFont();
            font.SetWeight(wxFONTWEIGHT_BOLD);
            font.SetPointSize(font.GetPointSize() + 1);
            value->SetFont(font);
        }
        summaryGrid->Add(value, 1, wxEXPAND);
    };

    addSummaryRow(gui::U8("Сумма товаров:"), itemsTotalLabel_, false);
    addSummaryRow(gui::U8("Доставка:"), deliveryLabel_, false);
    addSummaryRow(gui::U8("Итого:"), totalLabel_, true);
    summary->Add(summaryGrid, 0, wxEXPAND | wxALL, 8);

    auto* note = new wxStaticText(
        this, wxID_ANY,
        gui::U8("Скидку баллами можно указать при оформлении заказа."));
    note->SetForegroundColour(wxColour(120, 120, 120));
    summary->Add(note, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(summary, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    clearButton_ = new wxButton(this, wxID_ANY, gui::U8("Очистить корзину"));
    clearButton_->Bind(wxEVT_BUTTON, &CartPanel::OnClear, this);
    buttons->Add(clearButton_, 0, wxALIGN_CENTER_VERTICAL);

    buttons->AddStretchSpacer();

    checkoutButton_ = new wxButton(this, wxID_ANY, gui::U8("Оформить заказ"));
    checkoutButton_->Bind(wxEVT_BUTTON, &CartPanel::OnCheckout, this);
    buttons->Add(checkoutButton_, 0, wxALIGN_CENTER_VERTICAL);

    root->Add(buttons, 0, wxEXPAND | wxALL, 12);

    SetSizer(root);
}

void CartPanel::RefreshData() {
    try {
        const Cart cart = logic_.viewCart(userId_);
        RebuildRows(cart);

        const double itemsTotal = cart.getTotal();
        const double delivery = logic_.calculateDeliveryCost(userId_);

        itemsTotalLabel_->SetLabel(gui::FormatPrice(itemsTotal));
        if (cart.isEmpty() || delivery <= 0.0) {
            deliveryLabel_->SetLabel(cart.isEmpty() ? gui::FormatPrice(0.0)
                                                    : gui::U8("Бесплатно"));
        } else {
            deliveryLabel_->SetLabel(gui::FormatPrice(delivery));
        }
        totalLabel_->SetLabel(gui::FormatPrice(itemsTotal + delivery));

        const bool isEmpty = cart.isEmpty();
        clearButton_->Enable(!isEmpty);
        checkoutButton_->Enable(!isEmpty);
        scroll_->Show(!isEmpty);
        emptyLabel_->Show(isEmpty);

        Layout();
    } catch (const std::exception&) {
        clearButton_->Enable(false);
        checkoutButton_->Enable(false);
    }
}

void CartPanel::RebuildRows(const Cart& cart) {
    scroll_->Freeze();
    rowsSizer_->Clear(true);
    for (const CartEntry& entry : cart.getEntries()) {
        rowsSizer_->Add(CreateRow(entry), 0, wxEXPAND | wxALL, 6);
    }
    scroll_->Layout();
    scroll_->FitInside();
    scroll_->Thaw();
}

wxPanel* CartPanel::CreateRow(const CartEntry& entry) {
    const itemid_t itemId = entry.getItem().getId();

    auto* row = new wxPanel(scroll_, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxBORDER_SIMPLE);
    row->SetBackgroundColour(*wxWHITE);

    auto* sizer = new wxBoxSizer(wxHORIZONTAL);

    auto* name = new wxStaticText(row, wxID_ANY, gui::Utf8(entry.getItem().getName()));
    wxFont nameFont = name->GetFont();
    nameFont.SetWeight(wxFONTWEIGHT_BOLD);
    name->SetFont(nameFont);
    sizer->Add(name, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    auto* portion = new wxStaticText(row, wxID_ANY,
                                     gui::Utf8(entry.getItem().getPortion()));
    portion->SetForegroundColour(wxColour(120, 120, 120));
    sizer->Add(portion, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

    auto* minus = new wxButton(row, wxID_ANY, "-", wxDefaultPosition, wxSize(34, -1));
    minus->Bind(wxEVT_BUTTON, [this, itemId](wxCommandEvent&) {
        ChangeQuantity(itemId, -1);
    });
    sizer->Add(minus, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    auto* quantity = new wxStaticText(row, wxID_ANY,
                                      wxString::Format("%d", entry.getQuantity()),
                                      wxDefaultPosition, wxSize(36, -1),
                                      wxALIGN_CENTER_HORIZONTAL);
    sizer->Add(quantity, 0, wxALIGN_CENTER_VERTICAL);

    auto* plus = new wxButton(row, wxID_ANY, "+", wxDefaultPosition, wxSize(34, -1));
    plus->Bind(wxEVT_BUTTON, [this, itemId](wxCommandEvent&) {
        ChangeQuantity(itemId, 1);
    });
    sizer->Add(plus, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);

    auto* subtotal = new wxStaticText(row, wxID_ANY,
                                      gui::FormatPrice(entry.getSubtotal()),
                                      wxDefaultPosition, wxSize(110, -1),
                                      wxALIGN_RIGHT);
    sizer->Add(subtotal, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    auto* remove = new wxButton(row, wxID_ANY, gui::U8("Удалить"));
    remove->Bind(wxEVT_BUTTON, [this, itemId](wxCommandEvent&) {
        RemoveItem(itemId);
    });
    sizer->Add(remove, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

    row->SetSizer(sizer);
    return row;
}

void CartPanel::ChangeQuantity(itemid_t itemId, int delta) {
    try {
        const Cart cart = logic_.viewCart(userId_);
        const int newQuantity = cart.getQuantity(itemId) + delta;
        logic_.setCartItemQuantity(userId_, itemId, newQuantity > 0 ? newQuantity : 0);
        frame_->RefreshLater();
    } catch (const std::exception&) {
        wxMessageBox(gui::U8("Не удалось изменить количество."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

void CartPanel::RemoveItem(itemid_t itemId) {
    try {
        logic_.setCartItemQuantity(userId_, itemId, 0);
        frame_->RefreshLater();
    } catch (const std::exception&) {
        wxMessageBox(gui::U8("Не удалось удалить позицию."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

void CartPanel::OnClear(wxCommandEvent&) {
    const int answer = wxMessageBox(
        gui::U8("Удалить все позиции из корзины?"),
        gui::U8("Очистка корзины"),
        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this);
    if (answer != wxYES) {
        return;
    }

    try {
        logic_.clearUserCart(userId_);
        frame_->RefreshLater();
    } catch (const std::exception&) {
        wxMessageBox(gui::U8("Не удалось очистить корзину."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

void CartPanel::OnCheckout(wxCommandEvent&) {
    try {
        if (logic_.viewCart(userId_).isEmpty()) {
            wxMessageBox(gui::U8("Корзина пуста. Добавьте блюда из меню."),
                         gui::U8("Оформление заказа"),
                         wxOK | wxICON_INFORMATION, this);
            return;
        }
    } catch (const std::exception&) {
        wxMessageBox(gui::U8("Не удалось прочитать корзину."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }

    frame_->ShowCheckoutTab();
}
