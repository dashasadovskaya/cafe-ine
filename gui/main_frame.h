#ifndef GUI_MAIN_FRAME_H
#define GUI_MAIN_FRAME_H

#include <wx/frame.h>

#include "logic.h"

class wxBookCtrlEvent;
class wxNotebook;
class MenuPanel;
class CartPanel;
class CheckoutPanel;
class ProfilePanel;
class AboutPanel;

// Главное окно: владеет навигацией и предоставляет экранам доступ к
// единственному объекту Logic и текущему пользователю.
class MainFrame : public wxFrame {
public:
    explicit MainFrame(Logic& logic);

    Logic& GetLogic();
    userid_t GetUserId() const;

    // Перечитывает данные Logic на всех экранах.
    void RefreshAll();
    // То же, но после завершения текущего обработчика события: так можно
    // безопасно перестраивать панель, из кнопки которой пришло событие.
    void RefreshLater();

    void ShowCheckoutTab();
    void ShowProfileTab();

private:
    void OnPageChanged(wxBookCtrlEvent& event);

    Logic& logic_;
    userid_t userId_ = 0;

    wxNotebook* notebook_ = nullptr;
    MenuPanel* menuPanel_ = nullptr;
    CartPanel* cartPanel_ = nullptr;
    CheckoutPanel* checkoutPanel_ = nullptr;
    ProfilePanel* profilePanel_ = nullptr;
    AboutPanel* aboutPanel_ = nullptr;
};

#endif  // GUI_MAIN_FRAME_H
