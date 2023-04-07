/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.0.0
 * File       ：关联1维和2维
 * Date       ：03/23/2023
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#include <cassert>
#include <iostream>

#include "couplefun.h"

static bool is_report_time(
	double& next_report_time, double current_time, int report_step)
//------------------------------------------------------------------------------
// 参数：next_report_time = 下一个报告时间
//       current_time     = 当前区间末端时间（该时刻状态刚刚更新）
//       report_step      = 报告时间步长，s
// 目的：检查当前时间是不是报告时间？如果是，同步更新报告时间
//------------------------------------------------------------------------------
{
	if (next_report_time != current_time)
		return false;

	// 更新next_report_time
	next_report_time = addSeconds(next_report_time, report_step);
	return true;
}

static void create_inflow_events(INodeSet* nodes, ICoordinates* nodes_xy, 
	IMap* map, IGrid* grid, IInflowManager* inflows, double current_time)
//------------------------------------------------------------------------------
// 参数：nodes        = 节点对象集合
//       nodes_xy     = 节点对象坐标集合
//       grid         = DEM网格对象
//       inflows      = 入流管理对象（接收节点溢流）
//       current_time = 当前时刻
//------------------------------------------------------------------------------
{
	// 清空inflows（将释放内部对象占用的内存空间）
	inflows->clearInflowEvent();

	// 找到溢流节点，创建对应入流对象，并添加至inflows	
	double x, y;              // 溢流节点坐标
	char   name[30];          // 溢流节点名称
	IBox*  box  = nullptr;    // 溢流节点所在的矩形区域
	INode* node = nullptr;    // 溢流节点对象
	const int node_counts = nodes->getNodeCounts(); // 一维模型节点总数
	for (int i = 0; i < node_counts; ++i)
	{
		node = nodes->getNodeObjectAndName(name, i);
		assert(node != nullptr);

		// 如果该节点没有溢流，跳过
		const double overflow = node->getOverFlow(); // m3/s
		if (overflow == 0.0) continue;

		//if (strcmp("J9", name) == 0)
		//	std::cout << "节点" << name << "当前溢流强度为：" << overflow << "\n";

		// 该节点有溢流时，作为二维的入流处理
		// 1. 找到溢流节点坐标(x, y)
		nodes_xy->getCoordinateByName(name, &x, &y, map);

		// 2. 创建包含该节点的矩形区域对象
		box = grid->createBox(x, y);

		// 3. 创建入流事件
		IInflowEvent* inflow = createIsolatedInflowEvent(grid);

		// 4. 设置入流范围（矩形区域仅包含一个元胞）		
		inflow->setBox(box);
		deleteIsolatedBox(box); // 记得释放内存，因为setBox并未转移所有权

		// 5. 添加时间序列数据（仅一个）
		inflow->addData(overflow, current_time); // (m3/s, d)

		// 6. 添加至入流管理对象
		inflows->addInflowEvent(inflow); // 发生所有权转移，无需释放inflow内存
	}
}

bool online_couple(INetwork* inited_net, IFlood* inited_flood, IGeoprocess* geo,
	double pv_threshold)
//------------------------------------------------------------------------------
// 参数：inited_net   = 一个已经初始化的一维模拟模型
//       inited_flood = 一个已经初始化的实时内涝分析对象
//       geo          = 地理空间信息对象
//       pv_threshold = 保存峰值流速时的阈值，低于该值按0.0处理
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

	// 5. 继续执行一维演算，并嵌入二维演算	
	const int report_step = inited_net->getOption()->getReportStep();
	double next_report_time = start_t;
	while (inited_net->getEndRoutingDateTime() < setup->getEndTime())
	{
		// 检查是否需要和二维进行耦合（第一次肯定耦合）
		const double current_time = inited_net->getEndRoutingDateTime();		
		if (is_report_time(next_report_time, current_time, report_step) &&
			inited_flood->getCurrentTime() <= current_time)
		{			
			// 1. 添加溢流事件
			create_inflow_events(
				nodes, nodes_xy, map, grid, inflows, current_time);

			// 2. 检查数据
			if (!inited_flood->validateData())
				return false;

			// 3. 更新计算区域（因为溢流点位置可能发生变化）
			inited_flood->updateComputeRegion();

			// 4. 更新二维模拟状态			
			while (inited_flood->getCurrentTime() < next_report_time)			
				inited_flood->updateState();	
			// 如果一维的报告时间步长是二维的更新周期的整数倍，则以下断言成立
			// 否则，以下断言不成立！
			assert(inited_flood->getCurrentTime() == next_report_time);

			// 5. 保存二维模拟的当前时刻结果
			// 注：保存文件路径需要提前指定！
			if (!inited_flood->outputDepthRaster())
				return false;
			if (!inited_flood->outputVelocityAngleRaster())
				return false;
		}

		// 更新一维模型状态		
		err_id = inited_net->updateState();
		if (err_id != 0)
			return false;
	}

	// 6. 保存二维模拟的峰值时刻结果
	// 注：保存文件路径需要提前指定！
	if (!inited_flood->outputPeakDepthRaster())
		return false;
	if (!inited_flood->outputPeakVelocityRaster(pv_threshold))
		return false;

	// 7. 测试用代码1
	inited_net->updateSystemStats();
	INodeStatsSet* inss = inited_net->getSubNet(0)->getNodeStatsSet();
	std::cout << "永久离开系统总水量：" << inss->getTotalOverFlow() << "m3\n";
	double pond_volume = 0.0;
	INodeStats* ins = nullptr;
	for (int i = 0; i < inss->getNodeStatsCounts(); ++i)
	{
		ins = inss->getNodeStats(i);
		pond_volume += ins->getPondedVolume();
	}
	std::cout << "节点积水量：" << pond_volume << "m3\n";

	//// 8. 测试用代码2
	//std::cout << "总降雨量为：" << inited_flood->getRainVolume()   << "m3\n";
	//std::cout << "总入流量为：" << inited_flood->getInflowVolume() << "m3\n";
	//std::cout << "总积水量为：" << inited_flood->getPondVolume()   << "m3\n";
	//std::cout << "总下渗量为：" << inited_flood->getInfilVolume()  << "m3\n";

	return true;
}

int partial_online_couple(INetwork* net_copy, const char* geo_file, 
	const char* work_directory, const char* out_directory, 
	const char* setup_file, double pv_threshold)
//------------------------------------------------------------------------------
// 参数：net_copy       = 正在运行的一维模型的克隆体
//       geo_file       = 包含地理空间信息的inp文件
//       work_directory = 存放二维模型的输入数据的文件夹
//       out_directory  = 存放二维模型的输出数据的文件夹
//       setup_file     = 二维模型的构建文件（内部必须指定DEM文件名称）
//       pv_threshold   = 保存峰值流速时的阈值，低于该值按0.0处理
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
	if (!flood->validateData()) 
		return err_id;
	flood->initState();

	// 3. 开启运行
	if (!online_couple(net_copy, flood, geo, pv_threshold))
		return 1200;

	// 4. 释放内存
	deleteFlood(flood);
	deleteGeoprocess(geo);

	return 0;
}

int offline_couple(const char* inp_file, const char* work_directory,
	const char* out_directory, const char* setup_file, double pv_threshold)
//------------------------------------------------------------------------------
// 参数：inp_file       = 一维模型的inp文件（必须包含空间信息）
//       work_directory = 存放二维模型的输入数据的文件夹
//       out_directory  = 存放二维模型的输出数据的文件夹
//       setup_file     = 二维模型的构建文件（内部必须指定DEM文件名称）
//       pv_threshold   = 保存峰值流速时的阈值，低于该值按0.0处理
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
	const int err_id = partial_online_couple(net, inp_file, work_directory,
		out_directory, setup_file, pv_threshold);

	// 3. 释放内存
	deleteNetwork(net);

	return err_id;
}