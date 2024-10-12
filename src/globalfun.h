/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.2.0
 * File       ：全局函数
 * Date       ：10/12/2024
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#pragma once

#include "couplefun.h"

#include <cassert>

extern bool is_report_time(double next_report_time, double current_time);

extern void create_inflow_events(INodeSet* nodes, ICoordinates* nodes_xy,
		IMap* map, IGrid* grid, IInflowManager* inflows, double current_time);

extern int check_pipe_net(INodeSet* pipe_nodes,
	const std::vector<std::pair<std::string, std::string>>& connect_names);

extern int check_river_net(INodeSet* river_nodes,
	const std::vector<std::pair<std::string, std::string>>& connect_names);

extern bool is_start_time_same(INetwork* pipe_net, INetwork* river_net);

extern bool is_end_time_same(INetwork* pipe_net, INetwork* river_net);

extern bool is_report_step_same(INetwork* pipe_net, INetwork* river_net);

extern void pipe_to_river(
	INodeSet* pipe_nodes, INodeSet* river_nodes, double t,
	const std::vector<std::pair<std::string, std::string>>& connect_names);

extern void river_to_pipe(
	INodeSet* river_nodes, INodeSet* pipe_nodes, double t,
	const std::vector<std::pair<std::string, std::string>>& connect_names);