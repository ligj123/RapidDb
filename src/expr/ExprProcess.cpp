
#include "../manager/Session.h"
#include "ExprDdl.h"
#include "ExprStatement.h"

namespace storage {
using namespace std;

bool ExprCreateTable::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprDropTable::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprShowTables::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprTrunTable::Preprocess(Session *session) {
  // TO DO
  return false;
}
bool ExprTransaction::Preprocess(Session *session) {
  // TO DO
  return false;
}

} // namespace storage