#include "gui/checkout_panel.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>

#include <algorithm>
#include <cmath>
#include <exception>

#include "gui/main_frame.h"
#include "gui/utils.h"

CheckoutPanel::CheckoutPanel(wxWindow* parent, MainFrame* frame, Logic& logic,
                             userid_t userId)
    : wxPanel(parent, wxID_ANY), frame_(frame), logic_(logic), userId_(userId) {
    BuildUi();
    LoadProfileIntoForm(false);
}

void CheckoutPanel::BuildUi() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* title = new wxStaticText(this, wxID_ANY, gui::U8("Оформление заказа"));
    wxFont titleFont = title->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);
    title->SetFont(titleFont);
    root->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    auto* deliveryBox = new wxStaticBoxSizer(wxVERTICAL, this, gui::U8("Доставка"));
    auto* form = new wxFlexGridSizer(2, 8, 12);
    form->AddGrowableCol(1, 1);

    const auto addField = [this, form](const wxString& label, wxTextCtrl* control) {
        form->Add(new wxStaticText(this, wxID_ANY, label), 0,
                  wxALIGN_CENTER_VERTICAL);
        form->Add(control, 1, wxEXPAND);
    };

    addressCtrl_ = new wxTextCtrl(this, wxID_ANY);
    phoneCtrl_ = new wxTextCtrl(this, wxID_ANY);
    recipientCtrl_ = new wxTextCtrl(this, wxID_ANY);
    commentCtrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                  wxSize(-1, 60), wxTE_MULTILINE);

    addressCtrl_->SetHint(gui::U8("г. Минск, ул. Ленина, 10, кв. 5"));
    phoneCtrl_->SetHint(gui::U8("+375 29 123-45-67"));
    recipientCtrl_->SetHint(gui::U8("Имя получателя"));
    commentCtrl_->SetHint(gui::U8("Комментарий к заказу"));

    addField(gui::U8("Адрес *"), addressCtrl_);
    addField(gui::U8("Телефон *"), phoneCtrl_);
    addField(gui::U8("Получатель"), recipientCtrl_);
    addField(gui::U8("Комментарий"), commentCtrl_);
    deliveryBox->Add(form, 0, wxEXPAND | wxALL, 8);

    auto* fillButton = new wxButton(this, wxID_ANY,
                                    gui::U8("Заполнить из профиля"));
    fillButton->Bind(wxEVT_BUTTON, &CheckoutPanel::OnFillFromProfile, this);
    deliveryBox->Add(fillButton, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(deliveryBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* pointsBox = new wxStaticBoxSizer(wxVERTICAL, this, gui::U8("Баллы"));
    pointsHint_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    pointsHint_->SetForegroundColour(wxColour(120, 120, 120));
    pointsBox->Add(pointsHint_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* pointsRow = new wxBoxSizer(wxHORIZONTAL);
    pointsRow->Add(new wxStaticText(this, wxID_ANY, gui::U8("Списать баллов:")),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    pointsSpin_ = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition,
                                 wxSize(110, -1), wxSP_ARROW_KEYS, 0, 0, 0);
    pointsSpin_->Bind(wxEVT_SPINCTRL, &CheckoutPanel::OnPointsChanged, this);
    pointsRow->Add(pointsSpin_, 0, wxALIGN_CENTER_VERTICAL);
    pointsBox->Add(pointsRow, 0, wxALL, 8);
    root->Add(pointsBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* summaryBox = new wxStaticBoxSizer(wxVERTICAL, this, gui::U8("Итого"));
    auto* summaryGrid = new wxFlexGridSizer(2, 6, 12);
    summaryGrid->AddGrowableCol(1, 1);

    const auto addSummaryRow = [this, summaryGrid](const wxString& label,
                                                    wxStaticText*& value,
                                                    bool bold) {
        summaryGrid->Add(new wxStaticText(this, wxID_ANY, label), 0,
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

    addSummaryRow(gui::U8("Сумма товаров:"), itemsLabel_, false);
    addSummaryRow(gui::U8("Доставка:"), deliveryLabel_, false);
    addSummaryRow(gui::U8("Скидка баллами:"), discountLabel_, false);
    addSummaryRow(gui::U8("Итого:"), totalLabel_, true);
    addSummaryRow(gui::U8("Баллы за заказ:"), earnedLabel_, false);
    summaryBox->Add(summaryGrid, 0, wxEXPAND | wxALL, 8);
    root->Add(summaryBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->AddStretchSpacer();
    placeButton_ = new wxButton(this, wxID_ANY, gui::U8("Оформить заказ"));
    placeButton_->SetDefault();
    placeButton_->Bind(wxEVT_BUTTON, &CheckoutPanel::OnPlaceOrder, this);
    buttons->Add(placeButton_, 0, wxALIGN_CENTER_VERTICAL);
    root->Add(buttons, 0, wxEXPAND | wxALL, 12);

    SetSizer(root);
}

void CheckoutPanel::RefreshData() {
    try {
        const User user = logic_.viewUser(userId_);
        const Cart cart = logic_.viewCart(userId_);

        const double itemsTotal = cart.getTotal();
        const int maxByShare = static_cast<int>(
            std::floor(itemsTotal * Logic::MAX_POINTS_DISCOUNT_SHARE));
        const int maxDiscount = std::max(0, std::min(user.getPoints(), maxByShare));

        const int current = pointsSpin_->GetValue();
        pointsSpin_->SetRange(0, maxDiscount);
        if (current > maxDiscount) {
            pointsSpin_->SetValue(maxDiscount);
        }

        if (cart.isEmpty()) {
            pointsHint_->SetLabel(
                gui::U8("Корзина пуста — списание баллов пока недоступно."));
        } else {
            pointsHint_->SetLabel(
                gui::U8("Доступно баллов: ") + wxString::Format("%d", user.getPoints()) +
                gui::U8(". Максимальная скидка: ") + gui::FormatPrice(maxDiscount) +
                gui::U8(" (не более 50% суммы блюд)."));
        }

        placeButton_->Enable(!cart.isEmpty());
        LoadProfileIntoForm(false);
        UpdateTotals();
    } catch (const std::exception&) {
        placeButton_->Enable(false);
    }
}

void CheckoutPanel::LoadProfileIntoForm(bool overwrite) {
    try {
        const User user = logic_.viewUser(userId_);
        if (overwrite || addressCtrl_->GetValue().empty()) {
            addressCtrl_->SetValue(gui::Utf8(user.getAddress()));
        }
        if (overwrite || phoneCtrl_->GetValue().empty()) {
            phoneCtrl_->SetValue(gui::Utf8(user.getPhone()));
        }
        if (overwrite || recipientCtrl_->GetValue().empty()) {
            recipientCtrl_->SetValue(gui::Utf8(user.getName()));
        }
    } catch (const std::exception&) {
        // Профиль недоступен — оставляем поля как есть.
    }
}

void CheckoutPanel::UpdateTotals() {
    try {
        const User user = logic_.viewUser(userId_);
        const Cart cart = logic_.viewCart(userId_);

        const double itemsTotal = cart.getTotal();
        const double delivery = logic_.calculateDeliveryCost(userId_);

        const int maxByShare = static_cast<int>(
            std::floor(itemsTotal * Logic::MAX_POINTS_DISCOUNT_SHARE));
        const int maxDiscount = std::max(0, std::min(user.getPoints(), maxByShare));

        int points = pointsSpin_->GetValue();
        if (points < 0) {
            points = 0;
        }
        if (points > maxDiscount) {
            points = maxDiscount;
            pointsSpin_->SetValue(points);
        }

        const double total = itemsTotal + delivery - static_cast<double>(points);
        const int earned = static_cast<int>(std::floor(total / 100.0)) *
                           Logic::POINTS_PER_100_RUBLES;

        itemsLabel_->SetLabel(gui::FormatPrice(itemsTotal));
        if (itemsTotal <= 0.0) {
            deliveryLabel_->SetLabel(gui::FormatPrice(0.0));
        } else if (delivery > 0.0) {
            deliveryLabel_->SetLabel(gui::FormatPrice(delivery));
        } else {
            deliveryLabel_->SetLabel(gui::U8("Бесплатно"));
        }

        discountLabel_->SetLabel(
            points > 0 ? "-" + gui::FormatPrice(static_cast<double>(points))
                       : gui::FormatPrice(0.0));
        totalLabel_->SetLabel(gui::FormatPrice(total));
        earnedLabel_->SetLabel(wxString::Format("%d", earned));

        Layout();
    } catch (const std::exception&) {
        // Если пользователь или корзина недоступны, оставляем нули.
    }
}

void CheckoutPanel::OnFillFromProfile(wxCommandEvent&) {
    LoadProfileIntoForm(true);
}

void CheckoutPanel::OnPointsChanged(wxSpinEvent&) {
    UpdateTotals();
}

void CheckoutPanel::OnPlaceOrder(wxCommandEvent&) {
    try {
        const Cart cart = logic_.viewCart(userId_);
        if (cart.isEmpty()) {
            wxMessageBox(gui::U8("Корзина пуста. Добавьте блюда из меню."),
                         gui::U8("Оформление заказа"),
                         wxOK | wxICON_INFORMATION, this);
            return;
        }

        const wxString address = gui::Trimmed(addressCtrl_->GetValue());
        const wxString phone = gui::Trimmed(phoneCtrl_->GetValue());
        if (address.empty()) {
            wxMessageBox(gui::U8("Укажите адрес доставки."),
                         gui::U8("Проверьте данные"),
                         wxOK | wxICON_WARNING, this);
            addressCtrl_->SetFocus();
            return;
        }
        if (phone.empty()) {
            wxMessageBox(gui::U8("Укажите телефон для связи."),
                         gui::U8("Проверьте данные"),
                         wxOK | wxICON_WARNING, this);
            phoneCtrl_->SetFocus();
            return;
        }
        if (!gui::IsValidBelarusPhone(phone)) {
            wxMessageBox(gui::U8("Укажите белорусский номер в формате "
                                 "+375 XX XXX-XX-XX."),
                         gui::U8("Проверьте данные"),
                         wxOK | wxICON_WARNING, this);
            phoneCtrl_->SetFocus();
            return;
        }

        const DeliveryInfo delivery(
            gui::ToUtf8(address),
            gui::ToUtf8(phone),
            gui::ToUtf8(gui::Trimmed(recipientCtrl_->GetValue())),
            gui::ToUtf8(gui::Trimmed(commentCtrl_->GetValue())));

        if (!delivery.isValid()) {
            wxMessageBox(gui::U8("Данные доставки заполнены неверно. "
                                 "Проверьте адрес и телефон."),
                         gui::U8("Проверьте данные"),
                         wxOK | wxICON_WARNING, this);
            return;
        }

        const User user = logic_.viewUser(userId_);
        const double itemsTotal = cart.getTotal();
        const int maxByShare = static_cast<int>(
            std::floor(itemsTotal * Logic::MAX_POINTS_DISCOUNT_SHARE));
        const int maxDiscount = std::max(0, std::min(user.getPoints(), maxByShare));

        int points = pointsSpin_->GetValue();
        if (points < 0) {
            points = 0;
        }
        if (points > maxDiscount) {
            wxMessageBox(
                gui::U8("Слишком много баллов для списания. Максимум сейчас: ") +
                    wxString::Format("%d", maxDiscount) + gui::U8("."),
                gui::U8("Проверьте баллы"), wxOK | wxICON_WARNING, this);
            pointsSpin_->SetValue(maxDiscount);
            UpdateTotals();
            return;
        }

        const std::optional<orderid_t> orderId =
            logic_.placeOrder(userId_, delivery, points);
        if (!orderId.has_value()) {
            wxMessageBox(
                gui::U8("Не удалось оформить заказ. Проверьте корзину и данные "
                        "доставки, возможно, часть блюд стала недоступна."),
                gui::U8("Ошибка оформления"), wxOK | wxICON_ERROR, this);
            frame_->RefreshLater();
            return;
        }

        wxString message = gui::U8("Заказ №") +
                           wxString::Format("%d", *orderId) +
                           gui::U8(" успешно оформлен.");
        if (const std::optional<Order> order = logic_.findOrder(*orderId)) {
            message += gui::U8("\nСумма: ") + gui::FormatPrice(order->getTotal()) +
                       gui::U8(".");
            message += gui::U8("\nНачислено баллов: ") +
                       wxString::Format("%d", order->getPointsEarned()) +
                       gui::U8(".");
        }
        wxMessageBox(message, gui::U8("Заказ оформлен"),
                     wxOK | wxICON_INFORMATION, this);

        pointsSpin_->SetValue(0);
        frame_->RefreshAll();
        frame_->ShowProfileTab();
    } catch (const std::exception&) {
        wxMessageBox(gui::U8("Не удалось оформить заказ из-за внутренней ошибки."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}
