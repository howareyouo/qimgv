#pragma once

#include <QObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QHash>
#include <QMap>
#include <QDebug>
#include <QStringList>

#include <functional>

#include "utils/actiontable.h"
#include "shortcutbuilder.h"
#include "components/scriptmanager/scriptmanager.h"
#include "settings.h"

enum ActionType {
    ACTION_INVALID,
    ACTION_NORMAL,
    ACTION_SCRIPT
};

class ActionManager : public QObject {
    Q_OBJECT
public:
    static ActionManager* getInstance();
    ~ActionManager();
    bool processEvent(QInputEvent*);
    void addShortcut(const QString &keys, const QString &action);
    void resetDefaults();
    void resetDefaults(QString action);
    QString actionForShortcut(const QString &keys);
    const QString shortcutForAction(QString action);
    const QList<QString> shortcutsForAction(QString action);
    QStringList actionList();
    const QMap<QString,QString>& allShortcuts();
    void removeShortcut(const QString &keys);
    void removeAllShortcuts();
    void removeAllShortcuts(QString actionName);
    QString keyForNativeScancode(quint32 scanCode);
    void adjustFromVersion(QVersionNumber lastVer);
    void saveShortcuts();

    /*
     * Binds an action name to the code that runs it. Names come from
     * ActionTable, so a typo here shows up in bindingProblems() instead of
     * silently doing nothing the first time the user presses the key.
     */
    void registerHandler(const QString &actionName, std::function<void()> handler);
    // ActionTable entries with no registered handler
    QStringList actionsWithoutHandler() const;
    // registered handlers whose name is not an ActionTable entry
    QStringList handlersWithoutAction() const;
    // everything that is inconsistent, one human readable line per problem
    QStringList bindingProblems() const;

public slots:
    bool invokeAction(const QString &actionName);

private:
    explicit ActionManager(QObject *parent = nullptr);
    QMap<QString, QString> shortcuts; // <shortcut, action>
    QHash<QString, std::function<void()>> handlers;

    static void initShortcuts();
    QString modifierKeys(QEvent *event);
    bool invokeActionForShortcut(const QString &action);
    void validateShortcuts();
    void readShortcuts();
    ActionType validateAction(const QString &actionName);

signals:
    void runScript(const QString&);
};

extern ActionManager *actionManager;
