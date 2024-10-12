/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.2.0
 * File       ：关联1维和2维
 * Date       ：10/08/2024
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#include "globalfun.h"

#include <atomic>
#include <thread>
#include <iostream>

bool online_couple(INetwork* inited_net, IFlood* inited_flood, IGeoprocess* geo)
//------------------------------------------------------------------------------
// 参数：inited_net   = 一个已经初始化的一维模拟模型
//       inited_flood = 一个已经初始化的实时内涝分析对象
//       geo          = 地理空间信息对象
// 要求：一维模型的报告时间步长应为二维模型的更新周期整数倍
//------------------------------------------------------------------------------
{
	// 1. 首先获取ISetup对象
	ISetup* setup = inited_flood->getSetup();

	// 2. 检查时间是否合理，要确保一维模型还没演算到二维模型的开始。
	//    最好能确保一维模型的当前演算区间末端时间为二维模型的模拟起始时间。
	//    注：不做该部分检查，算法也能正常运行。但是，如果此前已经积水，则这些积
	//        水不能在二维模型中加以处理（通俗讲，就是无法在二维模拟初始化阶段指
	//        定现状积水深度）。因此，只能要求在二维演算之前，一维还没有溢流。这
	//        点要由用户自己来保证。
	const double start_t = inited_net->getEndRoutingDateTime();
	if (start_t > setup->getStartTime())
		return false;

	// 3. 是否具有实时内涝分析权限（假定客户具有一维模拟算权限）
	int err_id = getRtFloodStatus();
	if (err_id != 0)
		return false;

	// 4. 获取一部分后续要反复使用的对象
	INodeSet*       nodes    = inited_net->getNodeSet();
	ICoordinates*   nodes_xy = geo->getCoordinates();
	IMap*           map      = geo->getMap();
	IGrid*          grid     = inited_flood->getGrid();
	IInflowManager* inflows  = inited_flood->getInflowManager();	
	// 通过内存获取结果
	int*   idx   = nullptr;
	float* depth = nullptr;
	float* vel   = nullptr;
	float* ang   = nullptr;
	int    size;

	// 5. 继续执行一维演算，并嵌入二维演算	
	const int report_step      = inited_net->getOption()->getReportStep();
	double    next_report_time = start_t;
	bool      new_inflow       = false;
	std::atomic_flag finish_2d = ATOMIC_FLAG_INIT;
	bool             err_2d    = false;
	while (inited_net->getEndRoutingDateTime() < setup->getEndTime())
	{
		// 检查二维演算是否出错
		if (err_2d) return false;

		// 存在新的入流（即到达了一维模拟的报告时间），创建溢流事件		
		const double current_time = inited_net->getEndRoutingDateTime();		
		if (is_report_time(next_report_time, current_time) &&
			inited_flood->getCurrentTime() <= current_time)
		{
			finish_2d.wait(true);
			create_inflow_events(
				nodes, nodes_xy, map, grid, inflows, current_time);
			new_inflow = true;
			next_report_time = addSeconds(next_report_time, report_step);
		}

		// 检查是否需要和二维进行耦合（第一次肯定耦合）
		if (new_inflow)
		{
			new_inflow = false;
			finish_2d.test_and_set(); // = true
		
			std::jthread t1([&finish_2d, inited_flood, next_report_time, 
				&err_2d, &size, &idx, &depth, &vel, &ang]
				{
				// 1. 检查数据
				if (!inited_flood->validateEvents())
				{ 
					err_2d = true;	
					finish_2d.clear();
					return;
				}

				// 2. 更新计算区域（因为溢流点位置可能发生变化）
				inited_flood->updateComputeRegion();

				// 3. 更新二维模拟状态			
				while (inited_flood->getCurrentTime() < next_report_time)
					inited_flood->updateState();
				// 如果一维的报告时间步长是二维的更新周期的整数倍，则以下断言成立
				// 否则，以下断言不成立！
				assert(inited_flood->getCurrentTime() == next_report_time);				

				// 4. 保存二维模拟的当前时刻结果
				// 注：保存文件路径需要提前指定！
				size = inited_flood->outputRasterEx(&idx, &depth, &vel, &ang);
				std::cout << "当前时刻有限水深数量：" << size << "\n";
				if (!inited_flood->outputRaster())
				{ 
					err_2d = true;	
					finish_2d.clear();
					return;
				}

				finish_2d.clear();
				}
			);
		}

		// 更新一维模型状态		
		err_id = inited_net->updateState();
		if (err_id != 0)
			return false;		
	}

	// 6. 保存二维模拟的峰值时刻结果
	// 注：保存文件路径需要提前指定！
	size = inited_flood->outputPeakRasterEx(&idx, &depth, &vel);
	std::cout << "峰值有效水深数量：" << size << "\n";
	if (!inited_flood->outputPeakRaster())
		return false;

	return true;
}

int partial_online_couple(INetwork* net_copy, const char* geo_file, 
	const char* work_directory, const char* out_directory, 
	const char* setup_file)
//------------------------------------------------------------------------------
// 参数：net_copy       = 正在运行的一维模型的克隆体
//       geo_file       = 包含地理空间信息的inp文件
//       work_directory = 存放二维模型的输入数据的文件夹
//       out_directory  = 存放二维模型的输出数据的文件夹
//       setup_file     = 二维模型的构建文件（内部必须指定DEM文件名称）
// 要求：一维模型的报告时间步长应为二维模型的更新周期整数倍
//------------------------------------------------------------------------------
{
	// 1. 创建地理空间信息对象	
	IGeoprocess* geo = createGeoprocess();
	if (!geo->openGeoFile(geo_file))
		return 1100;
	if (!geo->validateData())
		return 1101;

	// 2. 创建实时内涝分析对象
	IFlood* flood = createFlood();
	flood->setWorkingDirectory(work_directory);
	flood->setOutputDirectory(out_directory);	
	int err_id = flood->readSetupAndDemData(setup_file);
	if (err_id != 0)
		return err_id;
	err_id = flood->readEventData(); 
	if (err_id != 0)
		return err_id;
	flood->generateData(); 
	if (!flood->validateEvents()) 
		return err_id;
	flood->initState();

	// 3. 开启运行
	if (!online_couple(net_copy, flood, geo))
		return 1200;

	// 4. 释放内存
	deleteFlood(flood);
	deleteGeoprocess(geo);

	return 0;
}

int offline_couple(const char* inp_file, const char* work_directory,
	const char* out_directory, const char* setup_file)
//------------------------------------------------------------------------------
// 参数：inp_file       = 一维模型的inp文件（必须包含空间信息）
//       work_directory = 存放二维模型的输入数据的文件夹
//       out_directory  = 存放二维模型的输出数据的文件夹
//       setup_file     = 二维模型的构建文件（内部必须指定DEM文件名称）
// 要求：一维模型的报告时间步长应为二维模型的更新周期整数倍
//------------------------------------------------------------------------------
{
	// 1. 创建一维模拟对象
	INetwork* net = createNetwork();
	if (!net->readFromFile(inp_file))
		return 1000;
	if (!net->validateData())
		return 1001;
	net->initState();

	// 2. 开启运行
	const int err_id = partial_online_couple(
		net, inp_file, work_directory, out_directory, setup_file);

	// 3. 释放内存
	deleteNetwork(net);

	return err_id;
}