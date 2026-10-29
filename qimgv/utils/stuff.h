#pragma once

#include <QString>

#ifdef _WIN32
    #define StdString std::wstring
    #define CharType wchar_t
#else
    #define StdString std::string
    #define CharType char
#endif

int clamp(int x, int lower, int upper);
int probeOS();
StdString toStdString(QString str);
QString fromStdString(StdString str);

class QWidget;
// paint the native window frame (title bar) in dark or light mode.
// no-op where the window frame is not drawn by the system.
void applyWindowTitleBarTheme(QWidget *widget, bool dark);
