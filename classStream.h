#pragma once
#include "base.h"
#include <QFile>
#include <QDataStream>
class classStream
{
public:
	static void save(QVector<base::Object*>& obj, QString filename);
	static QVector<base::Object*>& load(QString filename);
};

