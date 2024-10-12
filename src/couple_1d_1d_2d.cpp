/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.2.0
 * File       ：关联管网、河网和2维
 * Date       ：10/12/2024
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#include "globalfun.h"

#include <atomic>
#include <thread>
#include <iostream>

int online_couple_1d_1d_2d(INetwork* pipe_net, INetwork* river_net, 
	IFlood* flood, IGeoprocess* pipe_geo, 
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// 参数：inited_net   = 一个已经初始化的一维模拟模型
//       inited_flood = 一个已经初始化的实时内涝分析对象
//       geo          = 地理空间信息对象
// 要求：一维模型的报告时间步长应为二维模型的更新周期整数倍
//------------------------------------------------------------------------------
{
	// 1. 首先获取对象
	ISetup*   setup       = flood->getSetup();
	INodeSet* pipe_nodes  = pipe_net->getNodeSet();
	INodeSet* river_nodes = river_net->getNodeSet();

	// 2. 检查数据	
	if (int ret = check_pipe_net(pipe_nodes, connect_names); ret != 0)
		return ret;
	if (int ret = check_river_net(river_nodes, connect_names); ret != 0)
		return ret;
	if (!is_start_time_same(pipe_net, river_net))
		return 7;
	if (!is_report_step_same(pipe_net, river_net))
		return 8;
	if (!is_end_time_same(pipe_net, river_net))
		return 9;
	// 检查时间是否合理，要确保一维模型还没演算到二维模型的开始。
	//    最好能确保一维模型的当前演算区间末端时间为二维模型的模拟起始时间。
	//    注：不做该部分检查，算法也能正常运行。但是，如果此前已经积水，则这些积
	//        水不能在二维模型中加以处理（通俗讲，就是无法在二维模拟初始化阶段指
	//        定现状积水深度）。因此，只能要求在二维演算之前，一维还没有溢流。这
	//        点要由用户自己来保证。
	if (pipe_net->getEndRoutingDateTime() > setup->getStartTime())
		return 10;

	// 3. 是否具有实时内涝分析权限（假定客户具有一维模拟算权限）
	int status = getRtFloodStatus();
	if (status != 0)
		return status;

	// 4. 获取一部分后续要反复使用的对象	
	ICoordinates*   nodes_xy = pipe_geo->getCoordinates();
	IMap*           map      = pipe_geo->getMap();
	IGrid*          grid     = flood->getGrid();
	IInflowManager* inflows  = flood->getInflowManager();	
	// 通过内存获取结果
	int*   idx   = nullptr;
	float* depth = nullptr;
	float* vel   = nullptr;
	float* ang   = nullptr;	
	int    size;

	// 5. 执行一维演算，并嵌入二维演算	
	const int report_step = pipe_net->getOption()->getReportStep();
	double   old_report_t = pipe_net->getEndRoutingDateTime();
	double   new_report_t = addSeconds(old_report_t, report_step); // 下一报告时间	
	std::atomic_flag finish_2d = ATOMIC_FLAG_INIT;
	bool             err_2d    = false;
	while (pipe_net->getEndRoutingDateTime() < pipe_net->getEndDateTime())
	{
		// 1）检查二维演算是否出错
		if (err_2d) return 11;

		// 2）创建溢流事件				
		finish_2d.wait(true);
		create_inflow_events(
			pipe_nodes, nodes_xy, map, grid, inflows, old_report_t);

		// 3）异步执行二维演算
		finish_2d.test_and_set(); // = true
		std::jthread t1([&finish_2d, flood, new_report_t,
			&err_2d, &size, &idx, &depth, &vel, &ang]
			{
				// a）检查数据
				if (!flood->validateEvents())
				{
					err_2d = true;
					finish_2d.clear();
					return;
				}

				// b）更新计算区域（因为溢流点位置可能发生变化）
				flood->updateComputeRegion();

				// c）更新二维模拟状态			
				while (flood->getCurrentTime() < new_report_t)
					flood->updateState();
				// 如果一维的报告时间步长是二维的更新周期的整数倍，则以下断言成立
				// 否则，以下断言不成立！
				assert(flood->getCurrentTime() == new_report_t);

				// d）保存二维模拟的当前时刻结果
				// 注：保存文件路径需要提前指定！
				size = flood->outputRasterEx(&idx, &depth, &vel, &ang);	
				std::cout << "当前时刻有限水深数量：" << size << "\n";
				if (!flood->outputRaster())
				{
					err_2d = true;
					finish_2d.clear();
					return ;
				}

				finish_2d.clear();
			}
		);
			
		// 4）更新管网模型状态 
		while (pipe_net->getEndRoutingDateTime() < new_report_t)
		{
			status = pipe_net->updateState();
			if (status != 0)
				return status;
		}

		// 5）更新河网模型的进流为管网模型当前结果
		pipe_to_river(pipe_nodes, river_nodes, old_report_t, connect_names);

		// 6）更新河网模型状态			
		while (river_net->getEndRoutingDateTime() < new_report_t)
		{
			status = river_net->updateState();
			if (status != 0)
				return status;
		}

		// 测试两个模型外排量
		{
			INode* pipe_out = pipe_nodes->getNodeObject("O1");
			INode* river_out = river_nodes->getNodeObject("river_out");
			std::cout << pipe_out->getOutFlow() << ", "
				<< river_out->getOutFlow() << "\n";
		}

		// 7）更新管网模型的尾水水位为河网模型报告结果	
		river_to_pipe(river_nodes, pipe_nodes, new_report_t, connect_names);

		// 8）更新报告时间
		old_report_t = new_report_t;
		new_report_t = addSeconds(old_report_t, report_step);
	}

	// 6. 更新一维模型的系统统计量
	//    省略

	// 7. 保存二维模拟的峰值时刻结果
	// 注：保存文件路径需要提前指定！
	size = flood->outputPeakRasterEx(&idx, &depth, &vel);
	std::cout << "峰值有效水深数量：" << size << "\n";
	if (!flood->outputPeakRaster())
		return 12;

	return 0;
}

//int partial_online_couple(INetwork* net_copy, const char* geo_file, 
//	const char* work_directory, const char* out_directory, 
//	const char* setup_file)
////------------------------------------------------------------------------------
//// 参数：net_copy       = 正在运行的一维模型的克隆体
////       geo_file       = 包含地理空间信息的inp文件
////       work_directory = 存放二维模型的输入数据的文件夹
////       out_directory  = 存放二维模型的输出数据的文件夹
////       setup_file     = 二维模型的构建文件（内部必须指定DEM文件名称）
//// 要求：一维模型的报告时间步长应为二维模型的更新周期整数倍
////------------------------------------------------------------------------------
//{
//	// 1. 创建地理空间信息对象	
//	IGeoprocess* geo = createGeoprocess();
//	if (!geo->openGeoFile(geo_file))
//		return 1100;
//	if (!geo->validateData())
//		return 1101;
//
//	// 2. 创建实时内涝分析对象
//	IFlood* flood = createFlood();
//	flood->setWorkingDirectory(work_directory);
//	flood->setOutputDirectory(out_directory);	
//	int err_id = flood->readSetupAndDemData(setup_file);
//	if (err_id != 0)
//		return err_id;
//	err_id = flood->readEventData(); 
//	if (err_id != 0)
//		return err_id;
//	flood->generateData(); 
//	if (!flood->validateEvents()) 
//		return err_id;
//	flood->initState();
//
//	// 3. 开启运行
//	if (!online_couple(net_copy, flood, geo))
//		return 1200;
//
//	// 4. 释放内存
//	deleteFlood(flood);
//	deleteGeoprocess(geo);
//
//	return 0;
//}
//
//int offline_couple(const char* inp_file, const char* work_directory,
//	const char* out_directory, const char* setup_file)
////------------------------------------------------------------------------------
//// 参数：inp_file       = 一维模型的inp文件（必须包含空间信息）
////       work_directory = 存放二维模型的输入数据的文件夹
////       out_directory  = 存放二维模型的输出数据的文件夹
////       setup_file     = 二维模型的构建文件（内部必须指定DEM文件名称）
//// 要求：一维模型的报告时间步长应为二维模型的更新周期整数倍
////------------------------------------------------------------------------------
//{
//	// 1. 创建一维模拟对象
//	INetwork* net = createNetwork();
//	if (!net->readFromFile(inp_file))
//		return 1000;
//	if (!net->validateData())
//		return 1001;
//	net->initState();
//
//	// 2. 开启运行
//	const int err_id = partial_online_couple(
//		net, inp_file, work_directory, out_directory, setup_file);
//
//	// 3. 释放内存
//	deleteNetwork(net);
//
//	return err_id;
//}