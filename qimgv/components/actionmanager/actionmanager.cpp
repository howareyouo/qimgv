#include "actionmanager.h"

ActionManager *actionManager = nullptr;

ActionManager::ActionManager(QObject *parent) : QObject(parent) {
}
//------------------------------------------------------------------------------
ActionManager::~ActionManager() {
    delete actionManager;
}
//------------------------------------------------------------------------------
ActionManager *ActionManager::getInstance() {
    if(!actionManager) {
        actionManager = new ActionManager();
        initShortcuts();
    }
    return actionManager;
}
//------------------------------------------------------------------------------
void ActionManager::initShortcuts() {
    actionManager->readShortcuts();
    if(actionManager->shortcuts.isEmpty()) {
        actionManager->resetDefaults();
    }
}
//------------------------------------------------------------------------------
void ActionManager::addShortcut(const QString &keys, const QString &action) {
    ActionType type = validateAction(action);
    if(type != ActionType::ACTION_INVALID) {
        actionManager->shortcuts.insert(keys, action);
    }
}
//------------------------------------------------------------------------------
void ActionManager::removeShortcut(const QString &keys) {
    actionManager->shortcuts.remove(keys);
}
//------------------------------------------------------------------------------
QStringList ActionManager::actionList() {
    return ActionTable::names();
}
//------------------------------------------------------------------------------
const QMap<QString, QString> &ActionManager::allShortcuts() {
    return actionManager->shortcuts;
}
//------------------------------------------------------------------------------
void ActionManager::removeAllShortcuts() {
    shortcuts.clear();
}
//------------------------------------------------------------------------------
// Removes all shortcuts for specified action. Slow (reverse map lookup).
void ActionManager::removeAllShortcuts(QString actionName) {
    if(validateAction(actionName) == ActionType::ACTION_INVALID)
        return;

    for (auto i = shortcuts.begin(); i != shortcuts.end();) {
        if(i.value() == actionName)
            i = shortcuts.erase(i);
        else
            ++i;
    }
}
//------------------------------------------------------------------------------
QString ActionManager::keyForNativeScancode(quint32 scanCode) {
    if(inputMap->keys().contains(scanCode)) {
        return inputMap->keys()[scanCode];
    }
    return "";
}
//------------------------------------------------------------------------------
void ActionManager::resetDefaults() {
    actionManager->shortcuts = ActionTable::defaultShortcuts();
}
//------------------------------------------------------------------------------
void ActionManager::resetDefaults(QString action) {
    actionManager->removeAllShortcuts(action);
    const ActionSpec *spec = ActionTable::find(action);
    if(!spec)
        return;
    for(const QString &keys : spec->defaultShortcuts) {
        shortcuts.insert(keys, spec->name);
        qDebug() << "[ActionManager] new action " << spec->name << " - assigning as [" << keys << "]";
    }
}
//------------------------------------------------------------------------------
void ActionManager::adjustFromVersion(QVersionNumber lastVer) {
    // swap Ctrl-P & P
    if(lastVer < QVersionNumber(0,9,2)) {
        actionManager->resetDefaults("print");
        actionManager->resetDefaults("openSettings");
    }
    // swap WheelUp/WheelDown. derp
    if(lastVer < QVersionNumber(1,0,1)) {
        qDebug() << "[actionManager]: swapping WheelUp/WheelDown";
        QMapIterator<QString, QString> i(shortcuts);
        QMap<QString, QString> swapped;
        while(i.hasNext()) {
            i.next();
            QString key = i.key();
            if(key.contains("WheelUp"))
                key.replace("WheelUp", "WheelDown");
            else if(key.contains("WheelDown"))
                key.replace("WheelDown", "WheelUp");
            swapped.insert(key, i.value());
        }
        shortcuts = swapped;
    }
    // add default shortcuts of actions introduced after the config was written
    for(const ActionSpec &spec : ActionTable::all()) {
        if(spec.introduced <= lastVer)
            continue;
        for(const QString &keys : spec.defaultShortcuts) {
            if(!shortcuts.contains(keys)) {
                shortcuts.insert(keys, spec.name);
                qDebug() << "[ActionManager] new action " << spec.name << " - assigning as [" << keys << "]";
            } else if(spec.name != actionForShortcut(keys)) {
                qDebug() << "[ActionManager] new action " << spec.name << " - shortcut [" << keys << "] already assigned to another action " << actionForShortcut(keys);
            }
        }
    }
    // apply
    saveShortcuts();
}
//------------------------------------------------------------------------------
void ActionManager::saveShortcuts() {
    settings->saveShortcuts(actionManager->shortcuts);
}
//------------------------------------------------------------------------------
QString ActionManager::actionForShortcut(const QString &keys) {
    // value(), not operator[]: a lookup must not insert an empty entry
    return actionManager->shortcuts.value(keys, "");
}

// returns first shortcut that is found
const QString ActionManager::shortcutForAction(QString action) {
    return shortcuts.key(action, "");
}

const QList<QString> ActionManager::shortcutsForAction(QString action) {
    return shortcuts.keys(action);
}
//------------------------------------------------------------------------------
void ActionManager::registerHandler(const QString &actionName, std::function<void()> handler) {
    if(!handler) {
        qWarning() << "[ActionManager] ignoring empty handler for action:" << actionName;
        return;
    }
    if(handlers.contains(actionName))
        qWarning() << "[ActionManager] action bound twice, keeping the last handler:" << actionName;
    handlers.insert(actionName, std::move(handler));
}
//------------------------------------------------------------------------------
QStringList ActionManager::actionsWithoutHandler() const {
    QStringList list;
    for(const ActionSpec &spec : ActionTable::all()) {
        if(!handlers.contains(spec.name))
            list.append(spec.name);
    }
    return list;
}
//------------------------------------------------------------------------------
QStringList ActionManager::handlersWithoutAction() const {
    QStringList list;
    for(auto it = handlers.constBegin(); it != handlers.constEnd(); ++it) {
        if(!ActionTable::contains(it.key()))
            list.append(it.key());
    }
    list.sort();
    return list;
}
//------------------------------------------------------------------------------
QStringList ActionManager::bindingProblems() const {
    QStringList problems;
    for(const QString &name : actionsWithoutHandler())
        problems.append("action \"" + name + "\" has no handler");
    for(const QString &name : handlersWithoutAction())
        problems.append("handler \"" + name + "\" is not a known action");
    for(const QString &conflict : ActionTable::conflictingDefaultShortcuts())
        problems.append("default shortcut conflict: " + conflict);
    return problems;
}
//------------------------------------------------------------------------------
bool ActionManager::invokeAction(const QString &actionName) {
    ActionType type = validateAction(actionName);
    if(type == ActionType::ACTION_SCRIPT) {
        emit runScript(actionName.mid(2)); // drop the "s:" prefix
        return true;
    }
    if(type == ActionType::ACTION_NORMAL) {
        auto it = handlers.constFind(actionName);
        if(it == handlers.constEnd()) {
            qWarning() << "[ActionManager] action has no handler:" << actionName;
            return true;
        }
        (*it)();
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
bool ActionManager::invokeActionForShortcut(const QString &shortcut) {
    if(!shortcut.isEmpty() && shortcuts.contains(shortcut)) {
        return invokeAction(actionManager->shortcuts[shortcut]);
    }
    return false;
}
//------------------------------------------------------------------------------
void ActionManager::validateShortcuts() {
    for (auto i = shortcuts.begin(); i != shortcuts.end();) {
        if(validateAction(i.value()) == ActionType::ACTION_INVALID)
            i = shortcuts.erase(i);
        else
            ++i;
    }
}
//------------------------------------------------------------------------------
inline
ActionType ActionManager::validateAction(const QString &actionName) {
    if(ActionTable::contains(actionName))
        return ActionType::ACTION_NORMAL;
    if(actionName.startsWith("s:")) {
        QString scriptName = actionName;
        scriptName.remove(0, 2);
        if(scriptManager->scriptExists(scriptName))
            return ActionType::ACTION_SCRIPT;
    }
    return ActionType::ACTION_INVALID;
}
//------------------------------------------------------------------------------
void ActionManager::readShortcuts() {
    settings->readShortcuts(shortcuts);
    actionManager->validateShortcuts();
}
//------------------------------------------------------------------------------
bool ActionManager::processEvent(QInputEvent *event) {
    return actionManager->invokeActionForShortcut(ShortcutBuilder::fromEvent(event));
}
//------------------------------------------------------------------------------
