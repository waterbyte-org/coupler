/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.0.0
 * File       ：DEM生成器测试
 * Date       ：03/23/2023
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#include "../src/couplefun.h"
#include "../interface/coupler.h"

int main()
{
	// DEM文件保存路径，如果不存在，则创建此文件
	const char* dem_save_path = "D:/sdk_v1.3.0/data/couple/custom.asc";

	// 已知模型的左下角坐标为(0,   0)，  左下移动4.5格
	// 已知模型的右上角坐标为(200, 150)，右上移动4.5格	
	const double x_bl = -22.5;
	const double y_bl = -22.5;
	const double x_tr = 222.5;
	const double y_tr = 172.5;

	// 假设网格尺寸为5m
	const double cell_size = 5.0;

	// 模型中节点地面高层，最小微7.65m，最大为8.0m，往两侧放宽0.1m
	const double min_elev = 7.55;
	const double max_elev = 8.1;

//	if (!create_dem(
	if (!createDEM(
		dem_save_path, x_bl, y_bl, x_tr, y_tr, cell_size, min_elev, max_elev))
		return 1;

	return 0;
}