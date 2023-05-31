#pragma once
#pragma once

#include <iomanip>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
using namespace std;

struct BBox//二维矩形
{
	double xmin;
	double xmax;
	double ymin;
	double ymax;
};

struct Point
{
	double x;
	double y;
};

class Geometry
{
public:
	virtual string wkt() const = 0;

	virtual void Build(char *buf, int size) = 0;//根据传入的char数组创建Geometry对象的函数

protected:
	vector<vector<Point>> geom;
};

class CPoint :public Geometry//一个点
{
private:
	double x, y; // 点的横纵坐标
public:
	CPoint(double _x = 0.0, double _y = 0.0) : x(_x), y(_y) {};

	virtual string wkt() const //wkt() 函数返回该点的 WKT 表示
	{
		return "POINT(" + to_string(x) + " " + to_string(y) + ")";
	}
	virtual void Build(char *buf, int size) //根据传入的字符数组 buf 创建一个 CPoint 对象,通过解析 buf 中的字节流来获取点的横纵坐标
											//解析字节流是将一个连续的二进制数据序列拆分成有意义的数据元素的过程
	{
		x = *(double*)buf;
		y = *(double*)(buf + sizeof(double));//指向下一个双精度浮点数 向后移动八个字节	
	}
};

class CPolyline :public Geometry//一条折线
{
public:
	virtual string wkt() const {
		if (geom.size() == 1)
		{
			ostringstream ofs;
			ofs << "LINESTRING(";
			//LINESTRING(3 4, 10 50, 20 25)
			for (int i = 0; i < geom[0].size(); i++)
			{
				ofs << geom[0][i].x << " " << geom[0][i].y;
				if (i < geom[0].size() - 1)
					ofs << ",";
				else
					ofs << ")";
			}
			return ofs.str();
		}
		else if (geom.size() > 1) {
			ostringstream ofs;
			ofs << "MULTILINESTRING(";
			for (int i = 0; i < geom.size(); i++) {
				ofs << "(";
				for (int j = 0; j < geom[i].size(); j++)
				{
					ofs << geom[i][j].x << " " << geom[i][j].y;
					if (j < geom[i].size() - 1)
						ofs << ",";
					else
						ofs << ")";
				}
				if (i < geom.size() - 1)
					ofs << ",";
				else
					ofs << ")";
			}
			return ofs.str();
		}
		else {
			return "";
		}
	}
	virtual void Build(char *buf, int size)//根据数组存储的点和线段的信息将其转换为vector<vector<Point>>的形式保存
	{
		//buf是指向字符数组的指针，包含点和线的信息，size是确保不读取超过缓冲区的大小的信息
		int numParts = *(int*)(buf + 36);//线的数量
		int numPoints = *(int*)(buf + 40);//点的数量
		int *Parts = (int*)(buf + 44);//Parts是一个大小为numParts的整数数组，每个元素表示geom中的一条线段的第一个点的索引。
		Point *pt = (Point*)(buf + 44 + 4 * numParts);//pt是一个大小为numPoints的Point结构体数组，其中包含所有的点坐标。
		for (int i = 0; i < numParts; i++)//遍历每条线段
		{
			int nextPart = numPoints;//每条线段都是以前一条线段的下一个点作为起点的
			if (i < numParts - 1)
				nextPart = Parts[i + 1];
			vector<Point> line;//保存该线段所包含的点
			for (int j = Parts[i]; j < nextPart; j++)
				line.push_back(pt[j]);
			geom.push_back(line);
		}
	}
};

class CPolygon :public CPolyline
{
public:
	virtual string wkt() const
	{
		//POLYGON((1 1,5 1,5 5,1 5,1 1),(2 2,2 3,3 3,3 2,2 2))
		ostringstream ofs;
		//ofs<<setiosflags(ios_base::scientific);
		ofs << setiosflags(ios_base::fixed);
		ofs << "POLYGON(";
		//LINESTRING(3 4, 10 50, 20 25)
		for (int i = 0; i < geom.size(); i++)
		{
			ofs << "(";
			for (int j = 0; j < geom[i].size(); j++)
			{
				ofs << setprecision(10) << geom[i][j].x << " " << setprecision(10) << geom[i][j].y;
				if (j < geom[i].size() - 1)
					ofs << ",";
				else
					ofs << ")";
			}
			if (i < geom.size() - 1)
				ofs << ",";
			else
				ofs << ")";
		}
		return ofs.str();
	};
};

class Shapefile
{
	ifstream ifs;
public:
	bool Open(const char *fname);//打开指定shape file文件返回操作是否成功
	int  GetShapeType();
	string GetShapeTypeString() { return ""; }//获取几何类型返回字符串类型格式

	bool GetBoundingBox(BBox &box);//获取shape file边界框的信息

	class iterator//内部类 内部迭代器允许用户遍历shape file文件的几何要素
	{
		ifstream &ifs;
		int  off;
	public:
		iterator(ifstream &ifs, int off) :ifs(ifs), off(off) {};
		void operator++(int)//重载运算函数，表示向后移动迭代器指针，读取下一个几何要素
		{
			ifs.seekg(off + 4);//
			int size = GetBigInt() * 2;//得到当前读取的数据块大小
			off += 8;
			off += size;
		};

		Geometry* operator*()//获取迭代器当前位置的几何要素转换为相应的Geometry对象
		{
			ifs.seekg(off + 8);
			int type = GetLittleInt();
			Geometry *pt = NULL;
			if (type == 1)
				pt = new CPoint;//1  1
			else if (type == 3)
				pt = new CPolyline;//n ,n0,n1....
			else if (type == 5)
				pt = new CPolygon;//n,n0,n2

			ifs.seekg(off + 4);
			int size = GetBigInt() * 2;
			char *ptBuf = new char[size];
			ifs.seekg(off + 8);
			ifs.read(ptBuf, size);

			pt->Build(ptBuf, size);

			delete[]ptBuf;
			return pt;
		};

		bool operator!=(iterator &oth)
		{
			return off != oth.off;
		};

	private:
		int GetBigInt()
		{
			//return (i >> 24 & 0xff) | (i >> 8 & 0xff00) | (i << 8 & 0xff0000) | (i << 24);
			char tmp[4], x[4];
			ifs.read(tmp, sizeof(int));
			x[0] = tmp[3];
			x[1] = tmp[2];
			x[2] = tmp[1];
			x[3] = tmp[0];
			return *(int*)x;
		}
		int GetLittleInt()
		{
			int i;
			ifs.read((char*)&i, sizeof(int));
			return i;
		}
		double GetDouble()
		{
			double d;
			ifs.read((char*)&d, sizeof(double));
			return d;
		}
	};
	iterator begin() { return iterator(ifs, 100); };
	iterator end() {
		ifs.seekg(0, ios::end);
		int size0 = ifs.tellg();
		return iterator(ifs, size0);
	};
private:
	int GetBigInt()
	{
		//return (i >> 24 & 0xff) | (i >> 8 & 0xff00) | (i << 8 & 0xff0000) | (i << 24);

		char tmp[4], x[4];
		ifs.read(tmp, sizeof(int));
		x[0] = tmp[3];
		x[1] = tmp[2];
		x[2] = tmp[1];
		x[3] = tmp[0];
		return *(int*)x;
	}
	int GetLittleInt()
	{
		int i;
		ifs.read((char*)&i, sizeof(int));
		return i;
	}
	double GetDouble()
	{
		double d;
		ifs.read((char*)&d, sizeof(double));
		return d;
	}
};

class my_expection
{
public:
	const char* what()const
	{
		return "文件记录不是圆、矩形、三角形，文件格式有误";
	}

};

class Textfile {
	ifstream ifs; // 用于打开txt文件的输入流对象
public:
	bool Open(const char* fname)
	{ // 打开指定txt文件，返回操作是否成功
		ifs.open(fname);
		return ifs.is_open(); // 如果成功打开，则返回true；否则，返回false
	}

	class Triangle { // 内部类：Triangle，含有三个点坐标（x,y）
	public:
		double x1, y1, x2, y2, x3, y3;
		Triangle(double x1, double y1, double x2, double y2, double x3, double y3) :
			x1(x1), y1(y1), x2(x2), y2(y2), x3(x3), y3(y3) {}
	};

	class Circle { // 内部类：Circle 点坐标和半径
	public:
		double x, y, r;
		Circle(double x, double y, double r) : x(x), y(y), r(r) {}
	};

	class Rectangle { // 内部类：Rectangle 包含左上角和右下角两个点坐标
	public:
		double x1, y1, x2, y2;
		Rectangle(double x1, double y1, double x2, double y2) : x1(x1), y1(y1), x2(x2), y2(y2) {}
	};

	vector<Triangle> GetTriangles() // 获取txt文件中的所有三角形
	{
		vector<Triangle> triangles;
		string type;

		while (ifs >> type) { // 逐行读取txt文件，直到全部读取完成
			if (type == "Triangle")
			{ // 如果该行的类型为Triangle，则获取三个点坐标并存储
				double x1, y1, x2, y2, x3, y3;
				ifs >> x1 >> y1 >> x2 >> y2 >> x3 >> y3;
				triangles.emplace_back(x1, y1, x2, y2, x3, y3);
			}

			if (type != "Triangle"&&type != "Circle"&&type != "Rectangle")
			{
				throw my_expection();
			}

			else { // 如果该行的类型不是Triangle，则直接跳过该行
				getline(ifs, type);
			}
		}
		int count = 0;
		for (const auto& triangle : triangles)
		{
			cout << "Triangle vertices: " << endl;
			cout << "(" << triangle.x1 << ", " << triangle.y1 << ")" << endl;
			cout << "(" << triangle.x2 << ", " << triangle.y2 << ")" << endl;
			cout << "(" << triangle.x3 << ", " << triangle.y3 << ")" << endl;
			count++;
		}
		cout << " 三角形的个数是：" << count << endl;
		return triangles;
	}

	vector<Circle> GetCircles() { // 获取txt文件中的所有圆形
		vector<Circle> circles;
		string type;
		int num = 0;
		while (ifs >> type) { // 逐行读取txt文件，直到全部读取完成
			if (type == "Circle") { // 如果该行的类型为Circle，则获取圆心坐标和半径并存储
				double x, y, r;
				ifs >> x >> y >> r;
				circles.emplace_back(x, y, r);

			}
			if (type != "Triangle"&&type != "Circle"&&type != "Rectangle")
			{
				throw my_expection();
			}
			else { // 如果该行的类型不是Circle，则直接跳过该行
				getline(ifs, type);
			}
		}
		//vector<Circle> circles = GetCircles();

		for (const auto& circle : circles) {
			cout << "Circle center: (" << circle.x << ", " << circle.y << "), radius: " << circle.r << endl;
			num++;
		}
		cout << " 圆的个数是：" << num << endl;
		return circles;
	}

	vector<Rectangle> GetRectangles() { // 获取txt文件中的所有矩形
		vector<Rectangle> rectangles;
		string type;
		int hum = 0;
		while (ifs >> type) { // 逐行读取txt文件，直到全部读取完成
			if (type == "Rectangle")
			{ // 如果该行的类型为Rectangle，则获取左上角和右下角坐标并存储
				double x1, y1, x2, y2;
				ifs >> x1 >> y1 >> x2 >> y2;
				rectangles.emplace_back(x1, y1, x2, y2);
			}

			if (type != "Triangle"&&type != "Circle"&&type != "Rectangle")
			{
				throw my_expection();
			}

			else { // 如果该行的类型不是Rectangle，则直接跳过该行
				getline(ifs, type);
			}
		}
		//vector<Rectangle> rectangles = GetRectangles();
		for (const auto& rect : rectangles) {
			cout << "Left-bottom vertex: (" << rect.x1 << ", " << rect.y1 << ")" << endl;
			cout << "Right-top vertex: (" << rect.x2 << ", " << rect.y2 << ")" << endl;
			hum++;
		}
		cout << " 矩形的个数是：" << hum << endl;
		return rectangles;

	}

private:
	// 以下三个函数用于按照指定格式从txt文件中读取数据，代码与Shapefile类中相关内容相同
	int GetBigInt() {
		char tmp[4], x[4];
		ifs.read(tmp, sizeof(int));
		x[0] = tmp[3];
		x[1] = tmp[2];
		x[2] = tmp[1];
		x[3] = tmp[0];
		return *(int*)x;
	}
};

ostream& operator<<(ostream& out, BBox &box)
{
	out << "(" << box.xmin << ",";
	out << box.xmax << ",";
	out << box.ymin << ",";
	out << box.ymax << ")";
	return out;
}


bool Shapefile::Open(const char *fname)
{
	ifs.open(fname, ios::binary);
	if (!ifs)
		return false;

	ifs.seekg(0);
	if (GetBigInt() != 9994)
		return false;

	ifs.seekg(24);
	int size = GetBigInt() * 2;

	ifs.seekg(0, ios::end);
	int size0 = ifs.tellg();
	if (size != size0)
		return false;

	return true;
}

int  Shapefile::GetShapeType()
{
	ifs.seekg(32);
	return GetLittleInt();
}

bool Shapefile::GetBoundingBox(BBox &box)//得到参数为box的矩形的左上角和右下角
{
	ifs.seekg(36);
	box.xmin = GetDouble();
	ifs.seekg(44);
	box.ymin = GetDouble();

	ifs.seekg(52);
	box.xmax = GetDouble();
	ifs.seekg(60);
	box.ymax = GetDouble();
	return true;
}
