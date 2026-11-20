#include "gui/profile_panel.h"

#include <wx/wx.h>
#include <wx/listctrl.h>

#include <exception>

#include "gui/main_frame.h"
#include "gui/utils.h"

namespace {

wxString ComposeItems(const Order& order) {
    wxString result;
    for (const CartEntry& entry : order.getItems()) {
        if (!result.empty()) {
            result += gui::U8(", ");
        }
        result += gui::Utf8(entry.getItem().getName()) + gui::U8(" ×") +
                  wxString::Format("%d", entry.getQuantity());
    }
    return result;
}

wxString ComposePoints(const Order& order) {
    wxString result;
    if (order.getPointsSpent() > 0) {
        result += wxString::Format("-%d", order.getPointsSpent());
    }
    if (order.getPointsEarned() > 0) {
        if (!result.empty()) {
            result += gui::U8(", ");
        }
        result += wxString::Format("+%d", order.getPointsEarned());
    }
    return result.empty() ? wxString("0") : result;
}

}  // namespace

ProfilePanel::ProfilePanel(wxWindow* parent, MainFrame* frame, Logic& logic,
                           userid_t userId)
    : wxPanel(parent, wxID_ANY), frame_(frame), logic_(logic), userId_(userId) {
    BuildUi();
}

void ProfilePanel::BuildUi() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* profileBox = new wxStaticBoxSizer(wxVERTICAL, this, gui::U8("Профиль"));
    auto* form = new wxFlexGridSizer(2, 8, 12);
    form->AddGrowableCol(1, 1);

    const auto addField = [this, form](const wxString& label, wxTextCtrl* control) {
        form->Add(new wxStaticText(this, wxID_ANY, label), 0,
                  wxALIGN_CENTER_VERTICAL);
        form->Add(control, 1, wxEXPAND);
    };

    nameCtrl_ = new wxTextCtrl(this, wxID_ANY);
    phoneCtrl_ = new wxTextCtrl(this, wxID_ANY);
    addressCtrl_ = new wxTextCtrl(this, wxID_ANY);
    nameCtrl_->SetHint(gui::U8("Имя"));
    phoneCtrl_->SetHint(gui::U8("+375 29 123-45-67"));
    addressCtrl_->SetHint(gui::U8("г. Минск, ул. Ленина, 10, кв. 5"));
    addField(gui::U8("Имя *"), nameCtrl_);
    addField(gui::U8("Телефон"), phoneCtrl_);
    addField(gui::U8("Адрес"), addressCtrl_);
    profileBox->Add(form, 0, wxEXPAND | wxALL, 8);

    auto* saveButton = new wxButton(this, wxID_ANY,
                                    gui::U8("Сохранить изменения"));
    saveButton->Bind(wxEVT_BUTTON, &ProfilePanel::OnSave, this);
    profileBox->Add(saveButton, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(profileBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* pointsBox = new wxStaticBoxSizer(wxHORIZONTAL, this, gui::U8("Баллы"));
    pointsLabel_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    wxFont pointsFont = pointsLabel_->GetFont();
    pointsFont.SetWeight(wxFONTWEIGHT_BOLD);
    pointsFont.SetPointSize(pointsFont.GetPointSize() + 1);
    pointsLabel_->SetFont(pointsFont);
    pointsBox->Add(pointsLabel_, 1, wxALIGN_CENTER_VERTICAL | wxALL, 8);
    root->Add(pointsBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* historyBox = new wxStaticBoxSizer(wxVERTICAL, this,
                                            gui::U8("История заказов"));
    ordersList_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxLC_REPORT | wxLC_SINGLE_SEL);
    ordersList_->InsertColumn(0, gui::U8("№"), wxLIST_FORMAT_LEFT, 50);
    ordersList_->InsertColumn(1, gui::U8("Дата"), wxLIST_FORMAT_LEFT, 130);
    ordersList_->InsertColumn(2, gui::U8("Состав"), wxLIST_FORMAT_LEFT, 360);
    ordersList_->InsertColumn(3, gui::U8("Сумма"), wxLIST_FORMAT_RIGHT, 110);
    ordersList_->InsertColumn(4, gui::U8("Статус"), wxLIST_FORMAT_LEFT, 100);
    ordersList_->InsertColumn(5, gui::U8("Баллы"), wxLIST_FORMAT_LEFT, 100);
    ordersList_->Bind(wxEVT_LIST_ITEM_SELECTED, &ProfilePanel::OnOrderSelected, this);
    ordersList_->Bind(wxEVT_LIST_ITEM_DESELECTED, &ProfilePanel::OnOrderSelected, this);
    historyBox->Add(ordersList_, 1, wxEXPAND | wxALL, 8);

    auto* historyButtons = new wxBoxSizer(wxHORIZONTAL);
    historyButtons->AddStretchSpacer();
    cancelButton_ = new wxButton(this, wxID_ANY, gui::U8("Отменить заказ"));
    cancelButton_->Enable(false);
    cancelButton_->Bind(wxEVT_BUTTON, &ProfilePanel::OnCancelOrder, this);
    historyButtons->Add(cancelButton_, 0, wxALIGN_CENTER_VERTICAL);
    historyBox->Add(historyButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    root->Add(historyBox, 1, wxEXPAND | wxALL, 12);

    SetSizer(root);
}

void ProfilePanel::RefreshData() {
    try {
        const User user = logic_.viewUser(userId_);
        nameCtrl_->SetValue(gui::Utf8(user.getName()));
        phoneCtrl_->SetValue(gui::Utf8(user.getPhone()));
        addressCtrl_->SetValue(gui::Utf8(user.getAddress()));
        pointsLabel_->SetLabel(gui::U8("Баланс баллов: ") +
                               wxString::Format("%d", user.getPoints()));

        ordersList_->Freeze();
        ordersList_->DeleteAllItems();

        const std::vector<Order> orders = logic_.getUserOrders(userId_);
        for (auto it = orders.rbegin(); it != orders.rend(); ++it) {
            const Order& order = *it;
            const long index = ordersList_->InsertItem(
                ordersList_->GetItemCount(),
                wxString::Format("%d", order.getId()));
            ordersList_->SetItem(index, 1, gui::Utf8(order.getCreatedAt()));
            ordersList_->SetItem(index, 2, ComposeItems(order));
            ordersList_->SetItem(index, 3, gui::FormatPrice(order.getTotal()));
            ordersList_->SetItem(index, 4,
                                 gui::Utf8(toString(order.getStatus())));
            ordersList_->SetItem(index, 5, ComposePoints(order));
            ordersList_->SetItemData(index, static_cast<long>(order.getId()));
        }

        ordersList_->Thaw();
        UpdateCancelButton();
        Layout();
    } catch (const std::exception&) {
        ordersList_->DeleteAllItems();
        cancelButton_->Enable(false);
    }
}

void ProfilePanel::OnSave(wxCommandEvent&) {
    const wxString name = gui::Trimmed(nameCtrl_->GetValue());
    if (name.empty()) {
        wxMessageBox(gui::U8("Имя не может быть пустым."),
                     gui::U8("Проверьте профиль"),
                     wxOK | wxICON_WARNING, this);
        nameCtrl_->SetFocus();
        return;
    }

    const wxString phone = gui::Trimmed(phoneCtrl_->GetValue());
    if (!phone.empty() && !gui::IsValidBelarusPhone(phone)) {
        wxMessageBox(gui::U8("Укажите белорусский номер в формате "
                             "+375 XX XXX-XX-XX."),
                     gui::U8("Проверьте профиль"),
                     wxOK | wxICON_WARNING, this);
        phoneCtrl_->SetFocus();
        return;
    }

    try {
        User user = logic_.viewUser(userId_);
        user.setName(gui::ToUtf8(name));
        user.setPhone(gui::ToUtf8(phone));
        user.setAddress(gui::ToUtf8(gui::Trimmed(addressCtrl_->GetValue())));

        if (!logic_.updateUser(user)) {
            wxMessageBox(gui::U8("Не удалось сохранить профиль."),
                         gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
            return;
        }

        wxMessageBox(gui::U8("Профиль сохранён."), gui::U8("Профиль"),
                     wxOK | wxICON_INFORMATION, this);
        frame_->RefreshAll();
    } catch (const std::exception&) {
        wxMessageBox(gui::U8("Не удалось сохранить профиль из-за внутренней ошибки."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

bool ProfilePanel::SelectedOrder(orderid_t& orderId) const {
    const long index = ordersList_->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                wxLIST_STATE_SELECTED);
    if (index == -1) {
        return false;
    }
    orderId = static_cast<orderid_t>(ordersList_->GetItemData(index));
    return orderId > 0;
}

void ProfilePanel::UpdateCancelButton() {
    orderid_t orderId = 0;
    bool canCancel = false;
    if (SelectedOrder(orderId)) {
        if (const std::optional<Order> order = logic_.findOrder(orderId)) {
            const OrderStatus status = order->getStatus();
            canCancel = status != OrderStatus::Cancelled &&
                        status != OrderStatus::Completed;
        }
    }
    cancelButton_->Enable(canCancel);
}

void ProfilePanel::OnOrderSelected(wxListEvent&) {
    UpdateCancelButton();
}

void ProfilePanel::OnCancelOrder(wxCommandEvent&) {
    orderid_t orderId = 0;
    if (!SelectedOrder(orderId)) {
        wxMessageBox(gui::U8("Выберите заказ в списке."),
                     gui::U8("Отмена заказа"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    const std::optional<Order> order = logic_.findOrder(orderId);
    if (!order.has_value()) {
        wxMessageBox(gui::U8("Заказ не найден."), gui::U8("Ошибка"),
                     wxOK | wxICON_ERROR, this);
        frame_->RefreshLater();
        return;
    }

    if (order->getStatus() == OrderStatus::Cancelled ||
        order->getStatus() == OrderStatus::Completed) {
        wxMessageBox(gui::U8("Этот заказ уже нельзя отменить."),
                     gui::U8("Отмена заказа"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    const int answer = wxMessageBox(
        gui::U8("Отменить заказ №") + wxString::Format("%d", orderId) +
            gui::U8("?"),
        gui::U8("Отмена заказа"),
        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this);
    if (answer != wxYES) {
        return;
    }

    if (logic_.cancelOrder(orderId)) {
        wxMessageBox(gui::U8("Заказ отменён. Баллы пересчитаны."),
                     gui::U8("Заказ отменён"),
                     wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(gui::U8("Не удалось отменить заказ."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
    frame_->RefreshLater();
}
