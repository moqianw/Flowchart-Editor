#pragma once
#include "base.h"
namespace base {
	class OperationRecord
	{
		OperationRecord();
		static OperationRecord* unique;
	public:
		enum Command {
			Create,//´´½¨
			Trans,//±ä»»
			Delete//É¾³ý
			
		};
		struct Record {
			QVector<Object*> graph;
			Command com;
			Record(QVector<Object*> graph, Command com);
			Record(Command com);
			~Record();
		};
		QVector<Record*> bodys;
		QVector<QVector<Object*>> deleted;
		QVector<QVector<Object*>> moved;
		QVector<QVector<Object*>> whichmoved;
		static OperationRecord* getInstance();
		void recodeCommand(QVector<Object*> parent, Command com);
		void cancelCommand();
		void clear();
	};


}
