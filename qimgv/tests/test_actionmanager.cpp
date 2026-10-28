#include <QtTest>

#include "components/actionmanager/actionmanager.h"
#include "settings.h"
#include "utils/actiontable.h"
#include "utils/inputmap.h"

class Test_ActionManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void invokeUnknownActionFails();
    void registeredHandlerRuns();
    void allBindingsConsistent();
    void allKnownActionsInvoke();
    void handlerWithoutActionDetected();
    void resetDefaultsMatchesTable();
    void resetDefaultsForSingleAction();
    void actionListMatchesTable();
    void adjustFromVersionAddsNewActions();

private:
    ActionManager *am = nullptr;
};

void Test_ActionManager::initTestCase() {
    // same initialization order as main()
    inputMap = InputMap::getInstance();
    settings = Settings::getInstance();
    am = ActionManager::getInstance();
    QVERIFY(am != nullptr);
    QVERIFY(ActionManager::getInstance() == am);
}

void Test_ActionManager::invokeUnknownActionFails() {
    QCOMPARE(am->invokeAction("thisActionDoesNotExist"), false);
}

void Test_ActionManager::registeredHandlerRuns() {
    bool ran = false;
    am->registerHandler("goUp", [&]{ ran = true; });
    QCOMPARE(am->invokeAction("goUp"), true);
    QVERIFY(ran);
}

void Test_ActionManager::allBindingsConsistent() {
    for(const QString &name : ActionTable::names())
        am->registerHandler(name, []{});
    QVERIFY(am->actionsWithoutHandler().isEmpty());
    QVERIFY(am->handlersWithoutAction().isEmpty());
    QVERIFY(am->bindingProblems().isEmpty());
}

void Test_ActionManager::allKnownActionsInvoke() {
    for(const QString &name : ActionTable::names())
        QCOMPARE(am->invokeAction(name), true);
}

void Test_ActionManager::handlerWithoutActionDetected() {
    am->registerHandler("bogusAction", []{});
    QVERIFY(am->handlersWithoutAction().contains("bogusAction"));
    QCOMPARE(am->bindingProblems().size(), 1);
}

void Test_ActionManager::resetDefaultsMatchesTable() {
    am->resetDefaults();
    const QMap<QString, QString> &actual = am->allShortcuts();
    const QMap<QString, QString> &expected = ActionTable::defaultShortcuts();
    QCOMPARE(actual.size(), expected.size());
    for(auto i = expected.constBegin(); i != expected.constEnd(); ++i)
        QCOMPARE(actual.value(i.key()), i.value());
}

void Test_ActionManager::resetDefaultsForSingleAction() {
    am->removeAllShortcuts();
    am->resetDefaults("print");
    QCOMPARE(am->allShortcuts().size(), 1);
    QVERIFY(am->shortcutsForAction("print").contains(InputMap::keyNameCtrl() + "+P"));
    // unknown actions leave the map untouched
    am->resetDefaults("bogusAction");
    QCOMPARE(am->allShortcuts().size(), 1);
}

void Test_ActionManager::actionListMatchesTable() {
    QCOMPARE(am->actionList(), ActionTable::names());
}

void Test_ActionManager::adjustFromVersionAddsNewActions() {
    am->resetDefaults();
    QString paste = InputMap::keyNameCtrl() + "+V";
    am->removeShortcut(paste); // pasteFile was introduced in 1.0.3
    QVERIFY(am->actionForShortcut(paste).isEmpty());
    am->adjustFromVersion(QVersionNumber(1, 0, 2));
    QCOMPARE(am->actionForShortcut(paste), QString("pasteFile"));
    QCOMPARE(am->allShortcuts().size(), ActionTable::defaultShortcuts().size());
}

QTEST_MAIN(Test_ActionManager)
#include "test_actionmanager.moc"
