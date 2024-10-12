/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.2.0
 * File       ：耦合接口
 * Date       ：10/12/2024
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#pragma once

#include "rtflood/flood.h"
#include "geoprocess/geoprocess.h"
#include "swmmkernel/swmmkernel.h"

#include <vector>
#include <string>
#include <utility>

extern bool create_dem(const char* dem_file, double x_bl, double y_bl,
	double x_tr, double y_tr, double cell_size,
	double min_elev, double max_elev);

extern bool online_couple(INetwork* inited_net, IFlood* inited_flood, 
	IGeoprocess* geo);

extern int partial_online_couple(INetwork* net_copy, const char* geo_file,
	const char* work_directory, const char* out_directory, 
	const char* setup_file);

extern int offline_couple(const char* inp_file, const char* work_directory,
	const char* out_directory, const char* setup_file);

extern int connect_pipe_with_river(INetwork* pipe_net, INetwork* river_net,
	const std::vector<std::pair<std::string, std::string>>& connect_names);

extern int online_couple_1d_1d_2d(INetwork* pipe_net, INetwork* river_net, 
	IFlood* flood, IGeoprocess* pipe_geo,
	const std::vector<std::pair<std::string, std::string>>& connect_names);