#ifndef GUI_PROFILE_PANEL_H
#define GUI_PROFILE_PANEL_H

#include <wx/panel.h>

#include "logic.h"

class MainFrame;
class wxButton;
class wxListCtrl;
class wxListEvent;
class wxStaticText;
class wxTextCtrl;

// Вкладка «Профиль»: редактирование данных, баллы и история заказов.
class ProfilePanel : public wxPanel {
public:
    ProfilePanel(wxWindow* parent, MainFrame* frame, Logic& logic, userid_t userId);

    void RefreshData();

private:
    void BuildUi();
    void OnSave(wxCommandEvent& event);
    void OnCancelOrder(wxCommandEvent& event);
    void OnOrderSelected(wxListEvent& event);
    void UpdateCancelButton();
    bool SelectedOrder(orderid_t& orderId) const;

    MainFrame* frame_ = nullptr;
    Logic& logic_;
    userid_t userId_ = 0;

    wxTextCtrl* nameCtrl_ = nullptr;
    wxTextCtrl* phoneCtrl_ = nullptr;
    wxTextCtrl* addressCtrl_ = nullptr;
    wxStaticText* pointsLabel_ = nullptr;
    wxListCtrl* ordersList_ = nullptr;
    wxButton* cancelButton_ = nullptr;
};

#endif  // GUI_PROFILE_PANEL_H
