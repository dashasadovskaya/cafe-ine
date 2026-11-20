#ifndef GUI_CHECKOUT_PANEL_H
#define GUI_CHECKOUT_PANEL_H

#include <wx/panel.h>

#include "logic.h"

class MainFrame;
class wxButton;
class wxSpinCtrl;
class wxSpinEvent;
class wxStaticText;
class wxTextCtrl;

// Вкладка «Оформление»: данные доставки, списание баллов и итог заказа.
class CheckoutPanel : public wxPanel {
public:
    CheckoutPanel(wxWindow* parent, MainFrame* frame, Logic& logic, userid_t userId);

    void RefreshData();

private:
    void BuildUi();
    void LoadProfileIntoForm(bool overwrite);
    void UpdateTotals();
    void OnFillFromProfile(wxCommandEvent& event);
    void OnPointsChanged(wxSpinEvent& event);
    void OnPlaceOrder(wxCommandEvent& event);

    MainFrame* frame_ = nullptr;
    Logic& logic_;
    userid_t userId_ = 0;

    wxTextCtrl* addressCtrl_ = nullptr;
    wxTextCtrl* phoneCtrl_ = nullptr;
    wxTextCtrl* recipientCtrl_ = nullptr;
    wxTextCtrl* commentCtrl_ = nullptr;
    wxSpinCtrl* pointsSpin_ = nullptr;

    wxStaticText* pointsHint_ = nullptr;
    wxStaticText* itemsLabel_ = nullptr;
    wxStaticText* deliveryLabel_ = nullptr;
    wxStaticText* discountLabel_ = nullptr;
    wxStaticText* totalLabel_ = nullptr;
    wxStaticText* earnedLabel_ = nullptr;
    wxButton* placeButton_ = nullptr;
};

#endif  // GUI_CHECKOUT_PANEL_H
