#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVersionNumber>

/*
 * Single source of truth for the action set.
 *
 * An action used to be described in four unrelated places that had to be kept
 * in sync by hand: the name list in utils/actions.cpp, a signal in
 * ActionManager, a defaults.insert() call, and a connect() in
 * Core::initActions(). Missing one of them failed silently at runtime.
 *
 * Now every action is declared exactly once, here. ActionManager derives its
 * default shortcut map and its validation from this table, and the action ->
 * handler bindings are checked against it at startup.
 */
struct ActionSpec {
    QString name;
    // version that introduced the action; used to migrate shortcut configs
    // written by older releases
    QVersionNumber introduced;
    QStringList defaultShortcuts;
};

class ActionTable {
public:
    ActionTable() = delete;

    static const QList<ActionSpec> &all();

    // nullptr when the name is not a known action
    static const ActionSpec *find(const QString &name);
    static bool contains(const QString &name);

    // sorted by name, which is the order the shortcut editor expects
    static QStringList names();

    // <shortcut, action>, equal to ActionManager's default shortcut set
    static const QMap<QString, QString> &defaultShortcuts();

    // Two actions claiming the same default shortcut is a bug in this table,
    // not something to resolve at runtime. Empty when the table is sane.
    static QStringList conflictingDefaultShortcuts();
};
