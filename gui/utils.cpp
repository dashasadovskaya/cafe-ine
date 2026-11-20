#include "gui/utils.h"

#include <wx/dcmemory.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/pen.h>
#include <wx/stdpaths.h>

#include <algorithm>
#include <cmath>

namespace gui {

wxString Utf8(const std::string& value) {
    if (value.empty()) {
        return wxString();
    }
    return wxString::FromUTF8(value.c_str(), value.size());
}

std::string ToUtf8(const wxString& value) {
    return value.utf8_string();
}

wxString Trimmed(const wxString& value) {
    wxString result = value;
    result.Trim(true).Trim(false);
    return result;
}

wxString FormatPrice(double value) {
    const double rounded = std::round(value * 100.0) / 100.0;
    if (std::fabs(rounded - std::round(rounded)) < 1e-9) {
        return wxString::Format("%.0f", rounded) + U8(" Br");
    }

    wxString text = wxString::Format("%.2f", rounded);
    text.Replace(".", ",");
    return text + U8(" Br");
}

bool IsValidBelarusPhone(const wxString& value) {
    wxString digits;
    for (wxString::const_iterator it = value.begin(); it != value.end(); ++it) {
        const wxUniChar ch = *it;
        if (ch >= '0' && ch <= '9') {
            digits += ch;
        }
    }
    return (digits.length() == 12 && digits.StartsWith("375")) ||
           (digits.length() == 11 && digits.StartsWith("80"));
}

wxString FormatRating(double rating) {
    wxString text = wxString::Format("%.1f", rating);
    text.Replace(".", ",");
    return text;
}

wxString RatingStars(int rating) {
    wxString result;
    for (int i = 0; i < 5; ++i) {
        result += (i < rating) ? U8("★") : U8("☆");
    }
    return result;
}

namespace {

wxString ExeDirectory() {
    const wxFileName executable(wxStandardPaths::Get().GetExecutablePath());
    return executable.GetPath();
}

wxString ResolveFile(const wxString& relativePath) {
    if (relativePath.empty()) {
        return wxString();
    }

    const wxFileName fileName(relativePath);
    if (fileName.IsAbsolute()) {
        return wxFileName::FileExists(relativePath) ? relativePath : wxString();
    }

    const wxString exeDir = ExeDirectory();
    const wxString separator = wxFileName::GetPathSeparator();
    const wxString candidates[] = {
        relativePath,
        exeDir + separator + relativePath,
    };

    for (const wxString& candidate : candidates) {
        if (wxFileName::FileExists(candidate)) {
            return candidate;
        }
    }
    return wxString();
}

}  // namespace

wxBitmap MakePlaceholderBitmap(const wxSize& size, const wxString& text) {
    const wxSize target(size.x > 0 ? size.x : 240, size.y > 0 ? size.y : 160);
    wxBitmap canvas(target.x, target.y);

    wxMemoryDC dc(canvas);
    dc.SetBackground(wxBrush(wxColour(245, 245, 245)));
    dc.Clear();
    dc.SetPen(wxPen(wxColour(215, 215, 215)));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0, 0, target.x, target.y);

    wxFont font = dc.GetFont();
    font.SetPointSize(12);
    dc.SetFont(font);
    dc.SetTextForeground(wxColour(140, 140, 140));

    const wxString label = text.empty() ? U8("Нет фото") : text;
    const wxSize textSize = dc.GetTextExtent(label);
    dc.DrawText(label,
                (target.x - textSize.x) / 2,
                (target.y - textSize.y) / 2);
    dc.SelectObject(wxNullBitmap);
    return canvas;
}

wxBitmap LoadItemBitmap(const wxString& relativePath, const wxSize& size) {
    const wxSize target(size.x > 0 ? size.x : 240, size.y > 0 ? size.y : 160);
    wxImage image;
    bool loaded = false;

    const wxString resolved = ResolveFile(relativePath);
    if (!resolved.empty() && image.LoadFile(resolved) && image.IsOk()) {
        loaded = true;
    }

    if (!loaded) {
        const wxString placeholder = ResolveFile(U8("images/placeholder.png"));
        if (!placeholder.empty() && image.LoadFile(placeholder) && image.IsOk()) {
            loaded = true;
        }
    }

    if (!loaded || !image.IsOk() || image.GetWidth() <= 0 || image.GetHeight() <= 0) {
        return MakePlaceholderBitmap(target, U8("Нет фото"));
    }

    const double scale = std::min(
        static_cast<double>(target.x) / image.GetWidth(),
        static_cast<double>(target.y) / image.GetHeight());
    const int width = std::max(1, static_cast<int>(image.GetWidth() * scale));
    const int height = std::max(1, static_cast<int>(image.GetHeight() * scale));

    const wxImage scaled = image.Scale(width, height, wxIMAGE_QUALITY_HIGH);
    wxBitmap canvas(target.x, target.y);

    wxMemoryDC dc(canvas);
    dc.SetBackground(wxBrush(wxColour(245, 245, 245)));
    dc.Clear();
    const wxBitmap bitmap(scaled);
    dc.DrawBitmap(bitmap, (target.x - width) / 2, (target.y - height) / 2, true);
    dc.SelectObject(wxNullBitmap);

    return canvas;
}

}  // namespace gui
