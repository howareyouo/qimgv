#include "components/scriptmanager/scriptmanager.h"

// Minimal stand-in for the unit tests. ActionManager only ever calls
// scriptExists(), and only for "s:"-prefixed names, which the tests avoid.
// The real ScriptManager drags in Image/DocumentInfo, which the tests
// neither need nor want to link.
ScriptManager *scriptManager = nullptr;

bool ScriptManager::scriptExists(QString scriptName) {
    Q_UNUSED(scriptName)
    return false;
}
