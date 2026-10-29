#include "stuff.h"

#ifdef _WIN32
#include <QWidget>
#include <windows.h>
#endif

int clamp(int x, int lower, int upper) {
    return qMin(upper, qMax(x, lower));
}

// 0 - mac, 1 - linux, 2 - windows, 3 - other
int probeOS() {
#ifdef TARGET_OS_MAC
    return 0;
#elif defined(__linux__) || defined(__FreeBSD__)
    return 1;
#elif defined _WIN32 || defined _WIN64
    return 2;
#else
    return 3;
#endif
}

StdString toStdString(QString str) {
#ifdef _WIN32
    return str.toStdWString();
#else
    return str.toStdString();
#endif
}

QString fromStdString(StdString str) {
#ifdef _WIN32
    return QString::fromStdWString(str);
#else
    return QString::fromStdString(str);
#endif
}

void applyWindowTitleBarTheme(QWidget *widget, bool dark) {
#ifdef _WIN32
    if(!widget)
        return;
    typedef HRESULT (WINAPI *SetWindowAttributePtr)(HWND, DWORD, LPCVOID, DWORD);
    // dwmapi is resolved at runtime, the attribute is not available on older systems
    static SetWindowAttributePtr setWindowAttribute = []() -> SetWindowAttributePtr {
        HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
        if(!dwmapi)
            return nullptr;
        return reinterpret_cast<SetWindowAttributePtr>(GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
    }();
    if(!setWindowAttribute)
        return;
    const HWND handle = reinterpret_cast<HWND>(widget->winId());
    const BOOL value = dark ? TRUE : FALSE;
    // DWMWA_USE_IMMERSIVE_DARK_MODE: 20 on windows 10 2004 and later, 19 before that
    if(FAILED(setWindowAttribute(handle, 20, &value, sizeof(value))))
        setWindowAttribute(handle, 19, &value, sizeof(value));
    // repaint the non-client area, otherwise the old frame color stays until the
    // window is reactivated
    SetWindowPos(handle, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#else
    Q_UNUSED(widget)
    Q_UNUSED(dark)
#endif
}
