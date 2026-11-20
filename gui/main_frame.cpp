#include "gui/main_frame.h"

#include <wx/notebook.h>
#include <wx/sizer.h>

#include "gui/about_panel.h"
#include "gui/cart_panel.h"
#include "gui/checkout_panel.h"
#include "gui/menu_panel.h"
#include "gui/profile_panel.h"
#include "gui/utils.h"

MainFrame::MainFrame(Logic& logic)
    : wxFrame(nullptr,
              wxID_ANY,
              gui::U8("Кафе-Ин — заказ блюд"),
              wxDefaultPosition,
              wxSize(1100, 760)),
      logic_(logic) {
    SetMinSize(wxSize(900, 650));

    // Авторизации в Logic нет, поэтому работаем от имени одного демо-пользователя.
    const User user = logic_.createUser("Гость", "+375 29 123-45-67",
                                        "г. Минск, ул. Ленина, 10, кв. 5");
    userId_ = user.getId();
    // Немного баллов, чтобы можно было проверить списание в оформлении заказа.
    logic_.addUserPoints(userId_, 100);

    notebook_ = new wxNotebook(this, wxID_ANY);

    menuPanel_ = new MenuPanel(notebook_, this, logic_, userId_);
    cartPanel_ = new CartPanel(notebook_, this, logic_, userId_);
    checkoutPanel_ = new CheckoutPanel(notebook_, this, logic_, userId_);
    profilePanel_ = new ProfilePanel(notebook_, this, logic_, userId_);
    aboutPanel_ = new AboutPanel(notebook_, this, logic_, userId_);

    notebook_->AddPage(menuPanel_, gui::U8("Меню"), true);
    notebook_->AddPage(cartPanel_, gui::U8("Корзина"));
    notebook_->AddPage(checkoutPanel_, gui::U8("Оформление"));
    notebook_->AddPage(profilePanel_, gui::U8("Профиль"));
    notebook_->AddPage(aboutPanel_, gui::U8("О нас"));

    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(notebook_, 1, wxEXPAND);
    SetSizer(root);

    notebook_->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &MainFrame::OnPageChanged, this);

    Centre();
    RefreshAll();
}

Logic& MainFrame::GetLogic() {
    return logic_;
}

userid_t MainFrame::GetUserId() const {
    return userId_;
}

void MainFrame::RefreshAll() {
    if (menuPanel_) {
        menuPanel_->RefreshData();
    }
    if (cartPanel_) {
        cartPanel_->RefreshData();
    }
    if (checkoutPanel_) {
        checkoutPanel_->RefreshData();
    }
    if (profilePanel_) {
        profilePanel_->RefreshData();
    }
    if (aboutPanel_) {
        aboutPanel_->RefreshData();
    }
}

void MainFrame::RefreshLater() {
    CallAfter([this]() { RefreshAll(); });
}

void MainFrame::ShowCheckoutTab() {
    const int page = notebook_->FindPage(checkoutPanel_);
    if (page != wxNOT_FOUND) {
        notebook_->SetSelection(page);
    }
}

void MainFrame::ShowProfileTab() {
    const int page = notebook_->FindPage(profilePanel_);
    if (page != wxNOT_FOUND) {
        notebook_->SetSelection(page);
    }
}

void MainFrame::OnPageChanged(wxBookCtrlEvent& event) {
    event.Skip();
    RefreshAll();
}
