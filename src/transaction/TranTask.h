#pragma once
#include <atomic>
#include "../core/ActionType.h"

namespace storage {
	using namespace std;

  class TranTask
  {
	protected:
		static atomic<uint64_t> atomicId;
		ActionType actionType;
		//Statement statement;
		//List<IDataValue[]> lstArDv;
		//volatile long taskId;
		//volatile int rowNum;
		//CompletableFuture<Integer> futureTask = new CompletableFuture<>();
		//public StatementTask(ActionType actionType, Statement statement, List<IDataValue[]> lstArDv) {
		//	this.actionType = actionType;
		//	this.statement = statement;
		//	this.lstArDv = lstArDv;
		//	this.taskId = atomicId.getAndIncrement();
		//}

		//public StatementTask(ActionType actionType, Statement statement, IDataValue[] arDv) {
		//	this.actionType = actionType;
		//	this.statement = statement;
		//	lstArDv = new ArrayList<>();
		//	lstArDv.add(arDv);
		//	this.taskId = atomicId.getAndIncrement();
		//}

		//public ActionType getActionType() {
		//	return actionType;
		//}

		//public Statement getStatement() {
		//	return statement;
		//}

		//public List<IDataValue[]> getObjectValues() {
		//	return lstArDv;
		//}

		//public long getTaskId() {
		//	return taskId;
		//}

		//public void addRowsNumber(int rows) {
		//	rowNum += rows;
		//}

		//public int getRowsNumber() {
		//	return rowNum;
		//}

		//public Future<Integer> getFutureTask() {
		//	return futureTask;
		//}
  };
}

