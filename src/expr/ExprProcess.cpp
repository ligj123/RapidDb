
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

bool ExprSelect::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprInsert::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprUpdate::Preprocess(Session *session) {
  // TO DO
  return false;
}

bool ExprDelete::Preprocess(Session *session) {
  // TO DO
  return false;
}
} // namespace storage