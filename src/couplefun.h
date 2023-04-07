/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.0.0
 * File       ：关联1维和2维的接口
 * Date       ：03/23/2023
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#pragma once

#include "../../../sdk_v1.3.0/include/rtflood/flood.h"
#include "../../../sdk_v1.3.0/include/geoprocess/geoprocess.h"
#include "../../../sdk_v1.3.0/include/swmmkernel/swmmkernel.h"

extern bool create_dem(const char* dem_file, double x_bl, double y_bl,
	double x_tr, double y_tr, double cell_size,
	double min_elev, double max_elev);

extern bool online_couple(INetwork* inited_net, IFlood* inited_flood, 
	IGeoprocess* geo, double pv_threshold = 0.01);

extern int partial_online_couple(INetwork* net_copy, const char* geo_file,
	const char* work_directory, const char* out_directory, 
	const char* setup_file, double pv_threshold = 0.01);

extern int offline_couple(const char* inp_file, const char* work_directory,
	const char* out_directory, const char* setup_file, 
	double pv_threshold = 0.01);