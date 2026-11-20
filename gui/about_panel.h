#ifndef GUI_ABOUT_PANEL_H
#define GUI_ABOUT_PANEL_H

#include <wx/panel.h>

#include <vector>

#include "logic.h"

class MainFrame;
class wxBoxSizer;
class wxScrolledWindow;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;

// Вкладка «О нас»: информация о кафе, рейтинг, отзывы и форма отзыва.
class AboutPanel : public wxPanel {
public:
    AboutPanel(wxWindow* parent, MainFrame* frame, Logic& logic, userid_t userId);

    void RefreshData();

private:
    void BuildUi();
    void RebuildReviews(const std::vector<Review>& reviews);
    void OnSubmitReview(wxCommandEvent& event);

    MainFrame* frame_ = nullptr;
    Logic& logic_;
    userid_t userId_ = 0;

    wxStaticText* ratingLabel_ = nullptr;
    wxScrolledWindow* reviewsScroll_ = nullptr;
    wxBoxSizer* reviewsSizer_ = nullptr;
    wxSpinCtrl* ratingSpin_ = nullptr;
    wxTextCtrl* reviewText_ = nullptr;
};

#endif  // GUI_ABOUT_PANEL_H
