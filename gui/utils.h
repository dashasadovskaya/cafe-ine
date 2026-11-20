#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include <wx/bitmap.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

#include <string>

namespace gui {

// Русский текст в интерфейсе: исходники в UTF-8, поэтому строковые
// литералы создаём явно через FromUTF8.
inline wxString U8(const char* text) {
    return wxString::FromUTF8(text);
}

// Преобразование строк Logic (std::string в UTF-8) в wxString и обратно.
wxString Utf8(const std::string& value);
std::string ToUtf8(const wxString& value);
wxString Trimmed(const wxString& value);

// Деньги: "350 Br", а при наличии копеек — "12,50 Br" (белорусские рубли).
wxString FormatPrice(double value);

// Белорусский номер: +375 и 9 цифр (допускается национальный формат 80...).
bool IsValidBelarusPhone(const wxString& value);

// Рейтинг: "4,5" и "★★★★☆".
wxString FormatRating(double rating);
wxString RatingStars(int rating);

// Загружает картинку блюда (путь может быть относительным). Если файл не
// найден или повреждён, возвращает аккуратную заглушку нужного размера.
wxBitmap LoadItemBitmap(const wxString& relativePath, const wxSize& size);
wxBitmap MakePlaceholderBitmap(const wxSize& size, const wxString& text = wxString());

}  // namespace gui

#endif  // GUI_UTILS_H
