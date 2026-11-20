#include <wx/wx.h>

#include <memory>

#include "gui/main_frame.h"
#include "logic.h"

// Точка входа GUI: приложение владеет единственным объектом Logic и
// создаёт главное окно. Вся остальная логика интерфейса находится в gui/.
class CafeApp : public wxApp {
public:
    bool OnInit() override;

private:
    std::unique_ptr<Logic> logic_;
};

bool CafeApp::OnInit() {
    if (!wxApp::OnInit()) {
        return false;
    }

    wxInitAllImageHandlers();

    logic_ = std::make_unique<Logic>();
    logic_->seedDemoMenu();

    auto* frame = new MainFrame(*logic_);
    frame->Show();
    return true;
}

wxIMPLEMENT_APP(CafeApp);
