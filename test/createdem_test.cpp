/*
 *******************************************************************************
 * Project    ��Coupler
 * Version    ��1.0.0
 * File       ��DEM����������
 * Date       ��03/23/2023
 * Author     ����С��
 * Copyright  ������ˮ�ֽڿƼ����޹�˾
 *******************************************************************************
 */

#include "../src/couplefun.h"
#include "../interface/coupler.h"

int main()
{
	// DEM�ļ�����·������������ڣ��򴴽����ļ�
	const char* dem_save_path = "D:/sdk_v1.3.0/data/couple/custom.asc";

	// ��֪ģ�͵����½�����Ϊ(0,   0)��  �����ƶ�4.5��
	// ��֪ģ�͵����Ͻ�����Ϊ(200, 150)�������ƶ�4.5��	
	const double x_bl = -22.5;
	const double y_bl = -22.5;
	const double x_tr = 222.5;
	const double y_tr = 172.5;

	// ��������ߴ�Ϊ5m
	const double cell_size = 5.0;

	// ģ���нڵ����߲㣬��С΢7.65m�����Ϊ8.0m��������ſ�0.1m
	const double min_elev = 7.55;
	const double max_elev = 8.1;

//	if (!create_dem(
	if (!createDEM(
		dem_save_path, x_bl, y_bl, x_tr, y_tr, cell_size, min_elev, max_elev))
		return 1;

	return 0;
}