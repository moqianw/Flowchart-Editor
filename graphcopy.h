#pragma once
#include "base.h"
namespace base {
	class CopyManager {
		static CopyManager* unique;
		CopyManager();
	public:
		QVector<Object*> childs;
		static CopyManager* getInstance();
		void add(Object* parent);
		void clear();

	};
}
