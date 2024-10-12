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

#include "globalfun.h"

bool is_report_time(double report_time, double current_time)
//------------------------------------------------------------------------------
// 参数：next_report_time = 下一个报告时间
//       current_time     = 当前区间末端时间（该时刻状态刚刚更新）
// 目的：检查当前时间是不是报告时间？如果是，同步更新报告时间
//------------------------------------------------------------------------------
{
	return (report_time == current_time);
}

void create_inflow_events(INodeSet* nodes, ICoordinates* nodes_xy,
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
	IBox* box = nullptr;    // 溢流节点所在的矩形区域
	INode* node = nullptr;    // 溢流节点对象
	const int node_counts = nodes->getNodeCounts(); // 一维模型节点总数	
	for (int i = 0; i < node_counts; ++i)
	{
		node = nodes->getNodeObjectAndName(name, i);
		assert(node != nullptr);

		// 如果该节点没有溢流，跳过
		const double overflow = node->getOverFlow(); // m3/s
		if (overflow == 0.0) continue;

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

int check_pipe_net(INodeSet* pipe_nodes,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// 目的：检查管网排向河网的排放口是否存在，且为时间序列类型
//------------------------------------------------------------------------------
{
	for (const auto& [name, ignore] : connect_names)
	{
		if (INode* node = pipe_nodes->getNodeObject(name.c_str()))
		{
			if (!node->isOutfall())
				return 1; // 该节点不是排放口节点	
			IOutfall* out_node = dynamic_cast<IOutfall*>(node);
			if (!out_node->isTimeseriesType() ||
				out_node->getTimeseries() == nullptr)
				return 2; // 尾水不是时间序列类型			
		}
		else
			return 3; // 管网不存在该节点名称
	}
	return 0;
}

int check_river_net(INodeSet* river_nodes,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// 目的：检查河网接受管网流量的节点是否存在，且进流为外部直接入流
//------------------------------------------------------------------------------
{
	for (const auto& [ignore, name] : connect_names)
	{
		if (INode* node = river_nodes->getNodeObject(name.c_str()))
		{
			if (node->isOutfall())
				return 4; // 该节点是排放口节点			
			if (node->getExternDirectFlow() == nullptr)
				return 5; // 进流不是外部直接入流
		}
		else
			return 6; // 河网不存在该节点名称
	}
	return 0;
}

bool is_start_time_same(INetwork* pipe_net, INetwork* river_net)
//------------------------------------------------------------------------------
// 目的：检查两个模型起始时间是否相同
//------------------------------------------------------------------------------
{
	return (pipe_net->getStartDateTime() == river_net->getStartDateTime());
}

bool is_end_time_same(INetwork* pipe_net, INetwork* river_net)
//------------------------------------------------------------------------------
// 目的：检查两个模型当前演算区间终端时间是否相同
//------------------------------------------------------------------------------
{
	return (pipe_net->getEndRoutingDateTime() == 
		   river_net->getEndRoutingDateTime());
}

bool is_report_step_same(INetwork* pipe_net, INetwork* river_net)
//------------------------------------------------------------------------------
// 目的：检查两个模型报告时间步长是否相同
//------------------------------------------------------------------------------
{
	return (pipe_net->getOption()->getReportStep() ==
		   river_net->getOption()->getReportStep());
}

void pipe_to_river(INodeSet* pipe_nodes, INodeSet* river_nodes, double t,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// 目的：更新河网模型节点进流，并重新初始化
//------------------------------------------------------------------------------
{
	double pipe_outflow = 0.0;
	INode*       pipe_node    = nullptr;
	INode*       river_node   = nullptr;
	ITimeseries* river_edf_ts = nullptr;
	for (const auto& [pipe_name, river_name] : connect_names)
	{
		// 找到关联节点对
		pipe_node  = pipe_nodes->getNodeObject(pipe_name.c_str());
		river_node = river_nodes->getNodeObject(river_name.c_str());
		assert(pipe_node  != nullptr);
		assert(river_node != nullptr);
		assert(pipe_node->isOutfall());
		assert(!river_node->isOutfall());

		// 获取河道水位		
		pipe_outflow = pipe_node->getOutFlow();

		// 设置成管道排放口水位，并重新初始化		
		river_edf_ts = river_node->getExternDirectFlow()->getTimeseries();
		assert(river_edf_ts != nullptr);
		river_edf_ts->addDataElement(t, pipe_outflow);
		river_edf_ts->initStateAfterModified(t);
	}
}

void river_to_pipe(INodeSet* river_nodes, INodeSet* pipe_nodes, double t,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// 目的：更新管网模型排放口水位，并重新初始化
//------------------------------------------------------------------------------
{
	double river_head = 0.0;
	INode* pipe_node     = nullptr;
	INode* river_node    = nullptr;
	ITimeseries* pipe_ts = nullptr;
	for (const auto& [pipe_name, river_name] : connect_names)
	{
		// 找到关联节点对
		pipe_node  = pipe_nodes->getNodeObject(pipe_name.c_str());
		river_node = river_nodes->getNodeObject(river_name.c_str());
		assert(pipe_node  != nullptr);
		assert(river_node != nullptr);
		assert(pipe_node->isOutfall());
		assert(!river_node->isOutfall());

		// 获取河道水位
		river_head = river_node->getHead();

		// 设置成管道排放口水位，并重新初始化		
		pipe_ts = (dynamic_cast<IOutfall*>(pipe_node))->getTimeseries();
		assert(pipe_ts != nullptr);
		pipe_ts->addDataElement(t, river_head);
		pipe_ts->initStateAfterModified(t);
	}
}