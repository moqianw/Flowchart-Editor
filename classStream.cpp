#include "classStream.h"

void classStream::save(QVector<base::Object*>& obj, QString filename)
{
	try {
		if (obj.isEmpty()) {
			throw std::runtime_error("class empty");
		}
		QFile file(filename);
		if (!file.open(QIODevice::WriteOnly)) {
			throw std::runtime_error("Cannot open file: " + filename.toStdString());
		}

		QDataStream out(&file);

		out << static_cast<quint32>(obj.size());

		for (auto& it : obj) {
			it->save(out);
		}
	}
	catch (const std::exception& e) {
		qDebug() << "Error during saving vector of objects:" << e.what();
	}
	
}

QVector<base::Object*>& classStream::load(QString filename)
{
	QVector<base::Object*>* objects = new QVector<base::Object*>;

	try {
		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly)) {
			throw std::runtime_error("Cannot open file: " + filename.toStdString());
		}

		QDataStream in(&file);

		// 读取对象的数量
		quint32 size;
		in >> size;
		// 读取每个对象
		for (quint32 i = 0; i < size; ++i) {

			QString graphicsName;
			in >> graphicsName;
			base::Object* obj = nullptr;
			QRectF showrectf;
			in >> showrectf;
			qDebug() << showrectf;
			if (graphicsName == "Ellipse") {

				obj = new base::Ellipse(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
			}
			if (graphicsName == "Elliesp") {
				obj = new base::Ellipse(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
			}
			else if (graphicsName == "Rectangle") {
				obj = new base::Rectangle(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
			}
			else if (graphicsName == "AllowLine") {
				QPointF p1;
				QPointF p2;
				in >> p1;
				in >> p2;
				obj = new base::AllowLine(p1.x(), p1.y(), p2.x(), p2.y());
			}
			else if (graphicsName == "Diamond") {
				obj = new base::Diamond(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
			}
			else if (graphicsName == "RoundedRectangle") {
				obj = new base::RoundedRectangle(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height(), 10, 10);
			}
			else if (graphicsName == "Parallelogram") {
				obj = new base::Parallelogram(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height(), 80);
			}
			else if (graphicsName == "Start_or_Terminator") {
				obj = new base::Start_or_Terminator(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
			}
			else if (graphicsName == "Subprocess") {
				obj = new base::Subprocess(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height(), 30);
			}
			else if (graphicsName == "Database") {
				obj = new base::Database(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height(), 0.3);
			}
			else if (graphicsName == "Document") {
				obj = new base::Document(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height(), 0.15);
			}
			else if (graphicsName == "DataStorage") {
				obj = new base::DataStorage(showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height(), 15);
			}
			else if (graphicsName == "Textpointer") {
				obj = new base::Textpointer(base::GraphicalManager::getInstance()->view->scene(), showrectf.x(), showrectf.y(), showrectf.width(), showrectf.height());
			}
			if (obj) {
				int penwidth;
				QColor color;
				QBrush brush;
				in >> color;
				in >> penwidth;
				in >> brush;

				obj->setColor(color);
				obj->setWidth(penwidth);
				obj->setBrush(brush);

				QPointF position;
				in >> position;
				obj->setPos(position);

				QTransform transformMatrix;
				in >> transformMatrix;
				obj->setTransform(transformMatrix);
				objects->push_back(obj);
			}

		}

		file.close();
	}
	catch (const std::exception& e) {
		qDebug() << "Error during loading objects:" << e.what();
	}

	return *objects;
}
