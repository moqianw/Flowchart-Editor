#include "graphcopy.h"
namespace base {
	CopyManager* CopyManager::unique = nullptr;
	CopyManager::CopyManager() {

	}
	CopyManager* CopyManager::getInstance()
	{
		if (!unique) unique = new CopyManager();
		return unique;
	}
	void CopyManager::add(Object* parent)
	{
		unique->childs.push_back(parent->clone());
	}
	void CopyManager::clear()
	{
		QVector<Object*>().swap(unique->childs);
	}
}