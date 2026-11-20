#include "gui/menu_panel.h"

#include <wx/wx.h>

#include <exception>

#include "gui/main_frame.h"
#include "gui/utils.h"

MenuPanel::MenuPanel(wxWindow* parent, MainFrame* frame, Logic& logic, userid_t userId)
    : wxPanel(parent, wxID_ANY), frame_(frame), logic_(logic), userId_(userId) {
    BuildUi();
}

void MenuPanel::BuildUi() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* filters = new wxBoxSizer(wxHORIZONTAL);
    filters->Add(new wxStaticText(this, wxID_ANY, gui::U8("Категория:")),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    categoryChoice_ = new wxChoice(this, wxID_ANY);
    categoryChoice_->Append(gui::U8("Все категории"));
    const ItemCategory allCategories[] = {
        ItemCategory::Coffee,
        ItemCategory::Tea,
        ItemCategory::Drinks,
        ItemCategory::Breakfast,
        ItemCategory::MainCourse,
        ItemCategory::Dessert,
        ItemCategory::Snack,
        ItemCategory::Other,
    };
    for (const ItemCategory category : allCategories) {
        categories_.push_back(category);
        categoryChoice_->Append(gui::Utf8(toString(category)));
    }
    categoryChoice_->SetSelection(0);
    categoryChoice_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { RefreshData(); });
    filters->Add(categoryChoice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

    filters->Add(new wxStaticText(this, wxID_ANY, gui::U8("Поиск:")),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    searchCtrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                 wxSize(280, -1), wxTE_PROCESS_ENTER);
    searchCtrl_->SetHint(gui::U8("Название, состав или категория"));
    searchCtrl_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { RefreshData(); });
    searchCtrl_->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { RefreshData(); });
    filters->Add(searchCtrl_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    auto* findButton = new wxButton(this, wxID_ANY, gui::U8("Найти"));
    findButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { RefreshData(); });
    filters->Add(findButton, 0, wxALIGN_CENTER_VERTICAL);

    root->Add(filters, 0, wxEXPAND | wxALL, 12);

    scroll_ = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxVSCROLL);
    scroll_->SetScrollRate(0, 12);
    cardSizer_ = new wxFlexGridSizer(3, 16, 16);
    cardSizer_->AddGrowableCol(0, 1);
    cardSizer_->AddGrowableCol(1, 1);
    cardSizer_->AddGrowableCol(2, 1);
    scroll_->SetSizer(cardSizer_);
    root->Add(scroll_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    emptyLabel_ = new wxStaticText(
        this, wxID_ANY,
        gui::U8("Ничего не найдено.\nПопробуйте изменить категорию или запрос."),
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    emptyLabel_->SetForegroundColour(wxColour(120, 120, 120));
    root->Add(emptyLabel_, 1, wxALIGN_CENTER | wxALL, 24);
    emptyLabel_->Hide();

    SetSizer(root);
}

void MenuPanel::RefreshData() {
    RebuildCards();
}

std::vector<Item> MenuPanel::CollectItems() const {
    wxString query = searchCtrl_->GetValue();
    query.Trim(true).Trim(false);

    std::vector<Item> items = query.empty()
                                  ? logic_.getAvailableItems()
                                  : logic_.searchItems(gui::ToUtf8(query));

    std::vector<Item> available;
    available.reserve(items.size());
    for (const Item& item : items) {
        if (item.isAvailable()) {
            available.push_back(item);
        }
    }

    const int selection = categoryChoice_->GetSelection();
    if (selection > 0 && selection <= static_cast<int>(categories_.size())) {
        const ItemCategory category = categories_[selection - 1];
        std::vector<Item> filtered;
        filtered.reserve(available.size());
        for (const Item& item : available) {
            if (item.getCategory() == category) {
                filtered.push_back(item);
            }
        }
        available.swap(filtered);
    }

    return available;
}

void MenuPanel::RebuildCards() {
    const std::vector<Item> items = CollectItems();

    scroll_->Freeze();
    cardSizer_->Clear(true);
    for (const Item& item : items) {
        cardSizer_->Add(CreateCard(item), 0, wxEXPAND | wxALL, 8);
    }

    const bool isEmpty = items.empty();
    scroll_->Show(!isEmpty);
    emptyLabel_->Show(isEmpty);

    scroll_->Layout();
    scroll_->FitInside();
    scroll_->Thaw();
    Layout();
}

wxPanel* MenuPanel::CreateCard(const Item& item) {
    auto* card = new wxPanel(scroll_, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxBORDER_SIMPLE);
    card->SetBackgroundColour(*wxWHITE);

    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* image = new wxStaticBitmap(
        card, wxID_ANY,
        gui::LoadItemBitmap(gui::Utf8(item.getImagePath()), wxSize(240, 140)));
    sizer->Add(image, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8);

    auto* name = new wxStaticText(card, wxID_ANY, gui::Utf8(item.getName()));
    wxFont nameFont = name->GetFont();
    nameFont.SetWeight(wxFONTWEIGHT_BOLD);
    nameFont.SetPointSize(nameFont.GetPointSize() + 1);
    name->SetFont(nameFont);
    sizer->Add(name, 0, wxLEFT | wxRIGHT, 10);

    auto* meta = new wxStaticText(
        card, wxID_ANY,
        gui::Utf8(item.getPortion()) + gui::U8(" • ") +
            gui::Utf8(toString(item.getCategory())));
    meta->SetForegroundColour(wxColour(120, 120, 120));
    sizer->Add(meta, 0, wxLEFT | wxRIGHT | wxTOP, 10);

    auto* description = new wxStaticText(card, wxID_ANY,
                                         gui::Utf8(item.getDescription()));
    description->SetForegroundColour(wxColour(90, 90, 90));
    description->Wrap(230);
    sizer->Add(description, 0, wxLEFT | wxRIGHT | wxTOP, 10);

    sizer->AddStretchSpacer();

    auto* price = new wxStaticText(card, wxID_ANY,
                                   gui::FormatPrice(item.getPrice()));
    wxFont priceFont = price->GetFont();
    priceFont.SetWeight(wxFONTWEIGHT_BOLD);
    priceFont.SetPointSize(priceFont.GetPointSize() + 1);
    price->SetFont(priceFont);
    sizer->Add(price, 0, wxLEFT | wxRIGHT, 10);

    auto* addButton = new wxButton(card, wxID_ANY, gui::U8("В корзину"));
    addButton->Bind(wxEVT_BUTTON,
                    [this, itemId = item.getId()](wxCommandEvent&) {
                        AddItemToCart(itemId);
                    });
    sizer->Add(addButton, 0, wxEXPAND | wxALL, 10);

    card->SetSizer(sizer);
    card->SetMinSize(wxSize(260, 360));
    return card;
}

void MenuPanel::AddItemToCart(itemid_t itemId) {
    try {
        if (logic_.addToUserCart(userId_, itemId, 1)) {
            // Карточка перестраивается, поэтому обновляемся после события.
            frame_->RefreshLater();
        } else {
            wxMessageBox(gui::U8("Это блюдо сейчас недоступно."),
                         gui::U8("Меню"), wxOK | wxICON_WARNING, this);
        }
    } catch (const std::exception&) {
        wxMessageBox(gui::U8("Не удалось добавить блюдо в корзину."),
                     gui::U8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}
