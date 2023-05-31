#include "QtWidgetsApplication.h"
#include <QtWidgets/QApplication>
#include"main.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtWidgetsApplication w;

	Textfile txt;
	Textfile txt1;
	Textfile txt2;
	int sum = 0;
	Shapefile shp;
	/*Shapefile shp1;
	Shapefile shp2;*/
	if (!shp.Open("D:\\大学\\面向对象课设报告\\中国地图ARCGIS\\省级行政区.shp"))//面
	//if (!shp.Open("D:\\大学\\面向对象课设报告\\中国地图ARCGIS\\国界线.shp"))//线
	//if (!shp.Open("D:\\大学\\面向对象课设报告\\中国地图ARCGIS\\主要铁路。.shp"))
															 //if (!shp.Open("D:\\大学\\面向对象课设报告\\中国地图ARCGIS\\地级城市驻地.shp")) 点
	{
		cout << shp.GetShapeType() << endl;
		cout << shp.GetShapeTypeString() << endl;
		BBox box;
		shp.GetBoundingBox(box);//一组点构成的最小矩形边界框
		cout << box << endl;//重载输出流
		for (Shapefile::iterator it = shp.begin(); it != shp.end(); it++)
		{
			sum++;
			Geometry *pt = *it;
			cout << pt->wkt() << endl;
			delete pt;
		}
		cout << "有" << sum << "条记录" << endl;
	}

	for (Shapefile::iterator it = shp.begin(); it != shp.end(); it++)
	{
		sum++;
		Geometry *pt = *it;
		cout << pt->wkt() << endl;
		delete pt;
	}
	cout << "有" << sum << "条记录" << endl;


	string  file_name = "D:\\大学\\面向对象课设报告\\geometry_data.txt";
	ifstream ifs(file_name.c_str());
	int count = 0;
	string temp_count;

	while (!ifs.eof()) {
		getline(ifs, temp_count, '\n');
		count += !temp_count.empty();
	}
	cout << "总共记录条数是" << count << endl;

	try
	{
		if (txt.Open("D:\\大学\\面向对象课设报告\\geometry_data.txt"))//(txt.Open("D:\\大学\\面向对象课设报告\\geometry_data.txt")) 
		{
			txt.GetTriangles();
		}
		if (txt1.Open("D:\\大学\\面向对象课设报告\\geometry_data.txt"))
		{
			txt1.GetCircles();
		}
		if (txt2.Open("D:\\大学\\面向对象课设报告\\geometry_data.txt"))
		{
			txt2.GetRectangles();
		}
	}
	catch (my_expection& e) { cout << e.what() << endl; }

	string input;
	cout << "请输入内容：";
	getline(cin, input);
	ofstream outfile("D:\\大学\\面向对象课设报告\\geometry_data.txt", ios::app);
	if (!outfile.is_open())
	{
		cout << "文件打开失败" << endl;
		return -1;
	}
	else
	{
		outfile << "\n" << input; //写入文件
		outfile.close();
		cout << "文件写入成功！" << endl;
		return 0;
	}
	getchar();
	return 0;
    w.show();
    return a.exec();

}
