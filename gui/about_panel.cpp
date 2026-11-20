#include "gui/about_panel.h"

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

#include <algorithm>
#include <cmath>
#include <exception>

#include "gui/main_frame.h"
#include "gui/utils.h"

AboutPanel::AboutPanel(wxWindow* parent, MainFrame* frame, Logic& logic,
                       userid_t userId)
    : wxPanel(parent, wxID_ANY), frame_(frame), logic_(logic), userId_(userId) {
    BuildUi();
}

void AboutPanel::BuildUi() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* title = new wxStaticText(this, wxID_ANY, gui::U8("О кафе «Кафе-Ин»"));
    wxFont titleFont = title->GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);
    title->SetFont(titleFont);
    root->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    auto* description = new wxStaticText(
        this, wxID_ANY,
        gui::U8("Кафе-Ин — уютное место, где можно заказать кофе, завтраки, "
                "горячие блюда и десерты.\n"
                "Мы готовим из свежих продуктов и доставляем заказы по Минску.\n"
                "Баллы: 1 балл = 1 Br скидки (не более 50% стоимости блюд), "
                "доставка бесплатна от 1000 Br."));
    description->Wrap(760);
    description->SetForegroundColour(wxColour(70, 70, 70));
    root->Add(description, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    ratingLabel_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
    wxFont ratingFont = ratingLabel_->GetFont();
    ratingFont.SetWeight(wxFONTWEIGHT_BOLD);
    ratingFont.SetPointSize(ratingFont.GetPointSize() + 1);
    ratingLabel_->SetFont(ratingFont);
    root->Add(ratingLabel_, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    auto* reviewsBox = new wxStaticBoxSizer(wxVERTICAL, this, gui::U8("Отзывы"));
    reviewsScroll_ = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                          wxDefaultSize, wxVSCROLL);
    reviewsScroll_->SetScrollRate(0, 12);
    reviewsSizer_ = new wxBoxSizer(wxVERTICAL);
    reviewsScroll_->SetSizer(reviewsSizer_);
    reviewsBox->Add(reviewsScroll_, 1, wxEXPAND | wxALL, 8);
    root->Add(reviewsBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    auto* formBox = new wxStaticBoxSizer(wxVERTICAL, this,
                                         gui::U8("Оставить отзыв"));
    auto* ratingRow = new wxBoxSizer(wxHORIZONTAL);
    ratingRow->Add(new wxStaticText(this, wxID_ANY, gui::U8("Оценка:")), 0,
                   wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    ratingSpin_ = new wxSpinCtrl(this, wxID_ANY, "5", wxDefaultPosition,
                                 wxSize(90, -1), wxSP_ARROW_KEYS, 1, 5, 5);
    ratingRow->Add(ratingSpin_, 0, wxALIGN_CENTER_VERTICAL);
    formBox->Add(ratingRow, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    reviewText_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                 wxSize(-1, 70), wxTE_MULTILINE);
    formBox->Add(reviewText_, 1, wxEXPAND | wxALL, 8);

    auto* submitButton = new wxButton(this, wxID_ANY,
                                      gui::U8("Отправить отзыв"));
    submitButton->Bind(wxEVT_BUTTON, &AboutPanel::OnSubmitReview, this);
    formBox->Add(submitButton, 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    root->Add(formBox, 0, wxEXPAND | wxALL, 12);

    SetSizer(root);
}

void AboutPanel::RefreshData() {
    try {
        const std::vector<Review> reviews = logic_.getReviews();
        const double average = logic_.getAverageRating();

        if (reviews.empty()) {
            ratingLabel_->SetLabel(gui::U8("Средний рейтинг: пока нет отзывов"));
        } else {
            const int stars = static_cast<int>(std::lround(average));
            ratingLabel_->SetLabel(
                gui::U8("Средний рейтинг: ") + gui::FormatRating(average) +
                gui::U8("  ") + gui::RatingStars(stars) +
                gui::U8("  (отзывов: ") +
                wxString::Format("%d", static_cast<int>(reviews.size())) +
                gui::U8(")"));
        }

        RebuildReviews(reviews);
        Layout();
    } catch (const std::exception&) {
        ratingLabel_->SetLabel(gui::U8("Средний рейтинг: нет данных"));
    }
}

void AboutPanel::RebuildReviews(const std::vector<Review>& reviews) {
    const int wrapWidth = std::max(300, reviewsScroll_->GetClientSize().x - 40);

    reviewsScroll_->Freeze();
    reviewsSizer_->Clear(true);

    if (reviews.empty()) {
        auto* empty = new wxStaticText(
            reviewsScroll_, wxID_ANY,
            gui::U8("Пока нет отзывов. Станьте первым!"));
        empty->SetForegroundColour(wxColour(120, 120, 120));
        reviewsSizer_->Add(empty, 0, wxALL, 12);
    } else {
        for (auto it = reviews.rbegin(); it != reviews.rend(); ++it) {
            const Review& review = *it;
            auto* row = new wxPanel(reviewsScroll_, wxID_ANY);
            row->SetBackgroundColour(*wxWHITE);

            auto* sizer = new wxBoxSizer(wxVERTICAL);
            auto* header = new wxBoxSizer(wxHORIZONTAL);

            auto* author = new wxStaticText(row, wxID_ANY,
                                            gui::Utf8(review.getAuthorName()));
            wxFont authorFont = author->GetFont();
            authorFont.SetWeight(wxFONTWEIGHT_BOLD);
            author->SetFont(authorFont);
            header->Add(author, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

            auto* stars = new wxStaticText(row, wxID_ANY,
                                           gui::RatingStars(review.getRating()));
            stars->SetForegroundColour(wxColour(200, 150, 0));
            header->Add(stars, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

            header->AddStretchSpacer();

            auto* date = new wxStaticText(row, wxID_ANY,
                                          gui::Utf8(review.getCreatedAt()));
            date->SetForegroundColour(wxColour(120, 120, 120));
            header->Add(date, 0, wxALIGN_CENTER_VERTICAL);

            sizer->Add(header, 0, wxEXPAND | wxALL, 6);

            const wxString text = review.getText().empty()
                                      ? gui::U8("Без текста")
                                      : gui::Utf8(review.getText());
            auto* body = new wxStaticText(row, wxID_ANY, text);
            body->Wrap(wrapWidth);
            sizer->Add(body, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);

            row->SetSizer(sizer);
            reviewsSizer_->Add(row, 0, wxEXPAND | wxALL, 6);
            reviewsSizer_->Add(new wxStaticLine(reviewsScroll_), 0,
                               wxEXPAND | wxLEFT | wxRIGHT, 6);
        }
    }

    reviewsScroll_->Layout();
    reviewsScroll_->FitInside();
    reviewsScroll_->Thaw();
}

void AboutPanel::OnSubmitReview(wxCommandEvent&) {
    const wxString text = gui::Trimmed(reviewText_->GetValue());
    if (text.empty()) {
        wxMessageBox(gui::U8("Введите текст отзыва."),
                     gui::U8("Новый отзыв"),
                     wxOK | wxICON_WARNING, this);
        reviewText_->SetFocus();
        return;
    }

    try {
        const int rating = ratingSpin_->GetValue();
        const std::optional<reviewid_t> reviewId =
            logic_.addReview(userId_, rating, gui::ToUtf8(text));
        if (!reviewId.has_value()) {
            wxMessageBox(gui::U8("Не удалось добавить отзыв. "
                                 "Проверьте оценку (1–5)."),
                         gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
            return;
        }

        wxMessageBox(gui::U8("Спасибо! Отзыв добавлен."), gui::U8("Отзыв"),
                     wxOK | wxICON_INFORMATION, this);
        reviewText_->Clear();
        ratingSpin_->SetValue(5);
        frame_->RefreshLater();
    } catch (const std::exception&) {
        wxMessageBox(gui::U8("Не удалось добавить отзыв из-за внутренней ошибки."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}
