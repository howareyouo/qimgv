#include <QtTest>

#include "utils/actiontable.h"
#include "utils/inputmap.h"

class Test_ActionTable : public QObject {
    Q_OBJECT

private slots:
    void actionCount();
    void namesAreUnique();
    void namesAreSorted();
    void everyActionHasAVersion();
    void noDefaultShortcutConflicts();
    void defaultShortcutsComplete();
    void knownDefaultBindings();
    void findAndContains();
};

void Test_ActionTable::actionCount() {
    QCOMPARE(ActionTable::all().size(), 64);
    QCOMPARE(ActionTable::names().size(), 64);
}

void Test_ActionTable::namesAreUnique() {
    QStringList names = ActionTable::names();
    QSet<QString> unique;
    for(const QString &name : names)
        unique.insert(name);
    QCOMPARE(unique.size(), names.size());
}

void Test_ActionTable::namesAreSorted() {
    QStringList names = ActionTable::names();
    QStringList sorted = names;
    sorted.sort();
    QCOMPARE(names, sorted);
}

void Test_ActionTable::everyActionHasAVersion() {
    for(const ActionSpec &spec : ActionTable::all())
        QVERIFY2(!spec.introduced.isNull(), qPrintable(spec.name));
}

void Test_ActionTable::noDefaultShortcutConflicts() {
    QCOMPARE(ActionTable::conflictingDefaultShortcuts(), QStringList());
}

void Test_ActionTable::defaultShortcutsComplete() {
    const QMap<QString, QString> &defaults = ActionTable::defaultShortcuts();
    for(const ActionSpec &spec : ActionTable::all()) {
        for(const QString &keys : spec.defaultShortcuts)
            QCOMPARE(defaults.value(keys), spec.name);
    }
#ifdef __APPLE__
    QCOMPARE(defaults.size(), 69);
#else
    QCOMPARE(defaults.size(), 67);
#endif
}

void Test_ActionTable::knownDefaultBindings() {
    const QMap<QString, QString> &defaults = ActionTable::defaultShortcuts();
    QCOMPARE(defaults.value("Space"), QString("toggleFitMode"));
    QCOMPARE(defaults.value("Del"), QString("moveToTrash"));
    QCOMPARE(defaults.value("F11"), QString("toggleFullscreen"));
    QCOMPARE(defaults.value("WheelUp"), QString("prevImage"));
    QCOMPARE(defaults.value("WheelDown"), QString("nextImage"));
    QCOMPARE(defaults.value("XButton1"), QString("prevImage"));
    QCOMPARE(defaults.value("XButton2"), QString("nextImage"));
    QCOMPARE(defaults.value("MiddleButton"), QString("exit"));
    QCOMPARE(defaults.value("Esc"), QString("closeFullScreenOrExit"));
    QCOMPARE(defaults.value(InputMap::keyNameCtrl() + "+R"), QString("rotateRight"));
    QCOMPARE(defaults.value(InputMap::keyNameCtrl() + "+L"), QString("rotateLeft"));
    QCOMPARE(defaults.value(InputMap::keyNameCtrl() + "+O"), QString("open"));
    QCOMPARE(defaults.value(InputMap::keyNameCtrl() + "+S"), QString("save"));
    QCOMPARE(defaults.value(InputMap::keyNameCtrl() + "+P"), QString("print"));
#ifdef __APPLE__
    QCOMPARE(defaults.value(InputMap::keyNameCtrl() + "+Comma"), QString("openSettings"));
#else
    QCOMPARE(defaults.value("P"), QString("openSettings"));
#endif
}

void Test_ActionTable::findAndContains() {
    const ActionSpec *spec = ActionTable::find("zoomIn");
    QVERIFY(spec != nullptr);
    QCOMPARE(spec->name, QString("zoomIn"));
    QVERIFY(ActionTable::contains("goUp"));
    QVERIFY(ActionTable::find("bogusAction") == nullptr);
    QVERIFY(!ActionTable::contains("bogusAction"));
    QVERIFY(!ActionTable::names().contains("bogusAction"));
}

QTEST_MAIN(Test_ActionTable)
#include "test_actiontable.moc"
