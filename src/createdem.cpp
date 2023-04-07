/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.0.0
 * File       ：DEM生成器
 * Date       ：03/23/2023
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#include <random>
#include <fstream>
#include <sstream>
#include <cassert>

#include "couplefun.h"

bool create_dem(const char* dem_file, double x_bl, double y_bl, double x_tr, 
	double y_tr, double cell_size, double min_elev, double max_elev)
//------------------------------------------------------------------------------
// 参数：dem_file            = DEM文件路径
//      (x_bl, y_bl)         = 左下角坐标
//      (x_tr, y_tr)         = 右上角坐标
//       cell_size           = 单个网格尺寸
//      (min_elev, max_elev) = 网格高程随机范围
//------------------------------------------------------------------------------
{
	// 创建或者打开文件
	std::ofstream out(dem_file);
	if (!out) return false;

	// 左下必须小于右上
	if (x_bl >= x_tr || y_bl >= y_tr || cell_size <= 0.0)
		return false;

	// 写入头部
	const int n_cols = int((x_tr - x_bl) / cell_size + 0.5);
	const int n_rows = int((y_tr - y_bl) / cell_size + 0.5);
	std::ostringstream oss;
	oss.unsetf(std::ios::floatfield);
	oss.precision(12);	
	oss << "ncols \t\t"      << n_cols    << "\n";
	oss << "nrows \t\t"      << n_rows    << "\n";
	oss << "xllcorner \t"    << x_bl      << "\n";
	oss << "yllcorner \t"    << y_bl      << "\n";
	oss << "cellsize \t"     << cell_size << "\n";
	oss << "NODATA_value \t" << -9999.0;

	// 随机生成主体数据			
	std::random_device rd;  
	std::mt19937 gen(rd()); 
	std::uniform_real_distribution<> dis(min_elev, max_elev);
	oss.unsetf(std::ios::floatfield);
	oss.precision(6);
	for (int i = 0; i < n_rows; ++i)
	{
		oss << "\n";
		for (int j = 0; j < n_cols; ++j)
			oss << dis(gen) << " ";
	}

	// 写入文件
	out << oss.str();
	out.close();

	return true;
}