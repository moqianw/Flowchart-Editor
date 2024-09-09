#include "OperationRecord.h"
namespace base{
	OperationRecord* OperationRecord::unique = nullptr;
	OperationRecord::OperationRecord() {

	}
	OperationRecord* OperationRecord::getInstance()
	{
		if (!unique) unique = new OperationRecord();
		return unique;
	}

	void OperationRecord::recodeCommand(QVector<Object*> parent, Command com)
	{
		if (com == Create||com == Trans) {
			Record* a = new Record(com);
			for (auto* it : parent) {
				a->graph.push_back(it);
			}
			bodys.push_back(a);
			if(com == Create)	return;
		}

		QVector<Object*> p;
		for (auto* it : parent) {
			p.push_back(it->clone());
		}
		if (com == Delete) {
			bodys.push_back(new Record(parent, com));
			unique->deleted.push_back(p);
		}
		else if (com == Trans) {
			unique->moved.push_back(p);
			unique->whichmoved.push_back(parent);
		}
	}

	void OperationRecord::cancelCommand()
	{
		if (bodys.isEmpty()) return;
		Record* record = bodys.back();
		base::GraphicalManager* manager = base::GraphicalManager::getInstance();
		if (record->com == Delete) {
			for (auto* it : deleted.back()) {
				manager->graphics.push_back(it->clone());
				manager->view->scene()->addItem(manager->graphics.back());
			}
			deleted.pop_back();
		}
		else if(record->com == Trans) {
			for (int i = 0; i < moved.back().size(); i++) {
				manager->graphics.push_back(moved.back()[i]->clone());

				for (int j = 0; j < bodys.size() - 1; j++) {
					for (auto*& it : bodys[j]->graph) {
						if (it == whichmoved.back()[i]) {
							it = manager->graphics.back();
						}
					}
				}
				for (int j = 0; j < whichmoved.size() - 1; j++) {
					for (auto*& it : whichmoved[j]) {
						if (it == whichmoved.back()[i]) {
							it = manager->graphics.back();
						}
					}
				}


				manager->view->scene()->addItem(manager->graphics.back());
			}

			for (auto* it : record->graph) {
				auto t = std::find(manager->graphics.begin(), manager->graphics.end(), it);
				if (t != manager->graphics.end()) {
					manager->graphics.erase(t);
					delete it;
				}
			}
			whichmoved.pop_back();
			moved.pop_back();
		}
		else {
			for (auto* it : record->graph) {
				auto t = std::find(manager->graphics.begin(), manager->graphics.end(), it);
				if (t != manager->graphics.end()) {
					manager->graphics.erase(t);
					delete it;
				}
			}
		}
		base::OptionBox::getInstance()->clear();
		bodys.pop_back();
	}

	void OperationRecord::clear()
	{
		bodys.clear();
		for (auto& t : deleted) {
			for (auto*& q: t) {
				delete q;
			}
			t.clear();
		}
		for (auto& t : moved) {
			for (auto*& q : t) {
				delete q;
			}
			t.clear();
		}
		moved.clear();
		for (auto& t : whichmoved) {
			t.clear();
		}
		whichmoved, clear();
	}

	OperationRecord::Record::Record(QVector<Object*> graph, Command com):com(com)
	{
		for (auto* it : graph) {
			this->graph.push_back(it->clone());
		}
		
	}
	OperationRecord::Record::Record(Command com) :com(com)
	{

	}

	OperationRecord::Record::~Record()
	{
		for (int i = 0; i < this->graph.size(); i++) {
			graph.pop_back();
		}
	}

}
