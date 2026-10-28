// Generated from the former Actions::init() and ActionManager::initDefaults().
// This table is the single place where an action is declared.

#include "actiontable.h"
#include "inputmap.h"

namespace {

const QList<ActionSpec> &buildTable() {
    static const QList<ActionSpec> table = {
        {"nextImage", QVersionNumber(0,6,2), {
            "Right",
            "XButton2",
            "WheelDown",
        }},
        {"prevImage", QVersionNumber(0,6,2), {
            "Left",
            "XButton1",
            "WheelUp",
        }},
        {"toggleFullscreen", QVersionNumber(0,6,2), {
            "F",
            "F11",
            "LMB_DoubleClick",
        }},
        {"fitWindow", QVersionNumber(0,6,2), {
            "1",
        }},
        {"fitWidth", QVersionNumber(0,6,2), {
            "2",
        }},
        {"fitNormal", QVersionNumber(0,6,2), {
            "3",
        }},
        {"fitWindowStretch", QVersionNumber(1,0,3), {
            "4",
        }},
        {"toggleFitMode", QVersionNumber(0,6,2), {
            "Space",
        }},
        {"resize", QVersionNumber(0,6,2), {
            "R",
        }},
        {"rotateRight", QVersionNumber(0,6,2), {
            InputMap::keyNameCtrl() + "+R",
        }},
        {"rotateLeft", QVersionNumber(0,6,2), {
            InputMap::keyNameCtrl() + "+L",
        }},
        {"scrollUp", QVersionNumber(0,6,2), {
            "Up",
        }},
        {"scrollDown", QVersionNumber(0,6,2), {
            "Down",
        }},
        {"scrollLeft", QVersionNumber(0,6,2), {
        }},
        {"scrollRight", QVersionNumber(0,6,2), {
        }},
        {"zoomIn", QVersionNumber(0,6,2), {
            "=",
            InputMap::keyNameCtrl() + "+=",
            "+",
            InputMap::keyNameCtrl() + "++",
            InputMap::keyNameCtrl() + "+Up",
#ifdef __APPLE__
            InputMap::keyNameAlt() + "+Up",
#endif
        }},
        {"zoomOut", QVersionNumber(0,6,2), {
            "-",
            InputMap::keyNameCtrl() + "+-",
            InputMap::keyNameCtrl() + "+Down",
#ifdef __APPLE__
            InputMap::keyNameAlt() + "+Down",
#endif
        }},
        {"zoomInCursor", QVersionNumber(0,6,2), {
            InputMap::keyNameCtrl() + "+WheelUp",
        }},
        {"zoomOutCursor", QVersionNumber(0,6,2), {
            InputMap::keyNameCtrl() + "+WheelDown",
        }},
        {"open", QVersionNumber(0,6,2), {
            InputMap::keyNameCtrl() + "+O",
        }},
        {"save", QVersionNumber(0,6,2), {
            InputMap::keyNameCtrl() + "+S",
        }},
        {"saveAs", QVersionNumber(0,6,2), {
            InputMap::keyNameCtrl() + "+" + InputMap::keyNameShift() + "+S",
        }},
        {"setWallpaper", QVersionNumber(0,9,3), {
            InputMap::keyNameCtrl() + "+W",
        }},
        {"crop", QVersionNumber(0,6,2), {
            "X",
        }},
        {"removeFile", QVersionNumber(0,6,2), {
            InputMap::keyNameShift() + "+Del",
        }},
        {"copyFile", QVersionNumber(0,6,2), {
            "C",
        }},
        {"moveFile", QVersionNumber(0,6,2), {
            "M",
        }},
        {"jumpToFirst", QVersionNumber(0,6,2), {
            "Home",
        }},
        {"jumpToLast", QVersionNumber(0,6,2), {
            "End",
        }},
        {"openSettings", QVersionNumber(0,6,2), {
#ifdef __APPLE__
            InputMap::keyNameCtrl() + "+Comma",
#endif
#ifndef __APPLE__
            "P",
#endif
        }},
        {"closeFullScreenOrExit", QVersionNumber(0,6,2), {
            "Esc",
        }},
        {"exit", QVersionNumber(0,6,2), {
            "MiddleButton",
            InputMap::keyNameAlt() + "+X",
            InputMap::keyNameCtrl() + "+Q",
        }},
        {"flipH", QVersionNumber(0,6,3), {
            "H",
        }},
        {"flipV", QVersionNumber(0,6,3), {
            "V",
        }},
        {"folderView", QVersionNumber(0,6,85), {
            "Enter",
            "Backspace",
        }},
        {"pauseVideo", QVersionNumber(0,6,85), {
        }},
        {"frameStep", QVersionNumber(0,6,85), {
            ".",
        }},
        {"frameStepBack", QVersionNumber(0,6,85), {
            ",",
        }},
        {"documentView", QVersionNumber(0,6,88), {
        }},
        {"toggleFolderView", QVersionNumber(0,6,88), {
        }},
        {"moveToTrash", QVersionNumber(0,6,89), {
            "Del",
        }},
        {"reloadImage", QVersionNumber(0,7,80), {
            "F5",
        }},
        {"copyFileClipboard", QVersionNumber(0,7,80), {
            InputMap::keyNameCtrl() + "+C",
        }},
        {"copyPathClipboard", QVersionNumber(0,7,80), {
            InputMap::keyNameCtrl() + "+" + InputMap::keyNameShift() + "+C",
        }},
        {"renameFile", QVersionNumber(0,7,80), {
            "F2",
        }},
        {"contextMenu", QVersionNumber(0,7,81), {
            "RMB",
            "Menu",
        }},
        {"toggleTransparencyGrid", QVersionNumber(0,7,82), {
        }},
        {"sortByName", QVersionNumber(0,7,83), {
        }},
        {"sortByTime", QVersionNumber(0,7,83), {
        }},
        {"sortBySize", QVersionNumber(0,7,83), {
        }},
        {"toggleImageInfo", QVersionNumber(0,7,84), {
            "I",
        }},
        {"toggleShuffle", QVersionNumber(0,8,3), {
            InputMap::keyNameCtrl() + "+`",
        }},
        {"toggleScalingFilter", QVersionNumber(0,8,3), {
        }},
        {"toggleSlideshow", QVersionNumber(0,8,81), {
            "`",
        }},
        {"showInDirectory", QVersionNumber(0,8,82), {
            InputMap::keyNameCtrl() + "+D",
        }},
        {"goUp", QVersionNumber(0,9,2), {
        }},
        {"discardEdits", QVersionNumber(0,9,2), {
            InputMap::keyNameCtrl() + "+Z",
        }},
        {"nextDirectory", QVersionNumber(0,9,2), {
            InputMap::keyNameShift() + "+Right",
        }},
        {"prevDirectory", QVersionNumber(0,9,2), {
            InputMap::keyNameShift() + "+Left",
        }},
        {"lockZoom", QVersionNumber(0,9,2), {
        }},
        {"lockView", QVersionNumber(0,9,2), {
        }},
        {"print", QVersionNumber(1,0,0), {
            InputMap::keyNameCtrl() + "+P",
        }},
        {"toggleFullscreenInfoBar", QVersionNumber(1,0,0), {
            InputMap::keyNameShift() + "+F",
        }},
        {"pasteFile", QVersionNumber(1,0,3), {
            InputMap::keyNameCtrl() + "+V",
        }},
    };
    return table;
}

} // namespace

const QList<ActionSpec> &ActionTable::all() {
    return buildTable();
}

const ActionSpec *ActionTable::find(const QString &name) {
    for(const ActionSpec &spec : buildTable()) {
        if(spec.name == name)
            return &spec;
    }
    return nullptr;
}

bool ActionTable::contains(const QString &name) {
    return find(name) != nullptr;
}

QStringList ActionTable::names() {
    QStringList list;
    for(const ActionSpec &spec : buildTable())
        list.append(spec.name);
    list.sort();
    return list;
}

const QMap<QString, QString> &ActionTable::defaultShortcuts() {
    static const QMap<QString, QString> map = [] {
        QMap<QString, QString> m;
        for(const ActionSpec &spec : buildTable()) {
            for(const QString &key : spec.defaultShortcuts)
                m.insert(key, spec.name);
        }
        return m;
    }();
    return map;
}

QStringList ActionTable::conflictingDefaultShortcuts() {
    QStringList conflicts;
    QMap<QString, QString> seen;
    for(const ActionSpec &spec : buildTable()) {
        for(const QString &key : spec.defaultShortcuts) {
            if(seen.value(key) == spec.name)
                conflicts.append(key + " is listed twice for " + spec.name);
            else if(seen.contains(key))
                conflicts.append(key + " is claimed by both " + seen.value(key)
                                  + " and " + spec.name);
            else
                seen.insert(key, spec.name);
        }
    }
    return conflicts;
}
