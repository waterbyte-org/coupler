/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.2.0
 * File       ：关联管网和河网
 * Date       ：10/12/2024
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#include "globalfun.h"

#include <iostream>

//static int check_pipe_net(INodeSet* pipe_nodes, 
//	const std::vector<std::pair<std::string, std::string>>& connect_names)
////------------------------------------------------------------------------------
//// 目的：检查管网排向河网的排放口是否存在，且为时间序列类型
////------------------------------------------------------------------------------
//{
//	for (const auto& [name, ignore] : connect_names)
//	{
//		if (INode* node = pipe_nodes->getNodeObject(name.c_str()))
//		{
//			if (!node->isOutfall())
//				return 1; // 该节点不是排放口节点	
//			IOutfall* out_node = dynamic_cast<IOutfall*>(node);
//			if (!out_node->isTimeseriesType() || 
//				 out_node->getTimeseries() == nullptr)
//				return 2; // 尾水不是时间序列类型			
//		}
//		else
//			return 3; // 管网不存在该节点名称
//	}
//	return 0;
//}
//
//static int check_river_net(INodeSet* river_nodes,
//	const std::vector<std::pair<std::string, std::string>>& connect_names)
////------------------------------------------------------------------------------
//// 目的：检查河网接受管网流量的节点是否存在，且进流为外部直接入流
////------------------------------------------------------------------------------
//{
//	for (const auto& [ignore, name] : connect_names)
//	{
//		if (INode* node = river_nodes->getNodeObject(name.c_str()))
//		{
//			if (node->isOutfall())
//				return 4; // 该节点是排放口节点			
//			if (node->getExternDirectFlow() == nullptr)
//				return 5; // 进流不是外部直接入流
//		}
//		else
//			return 6; // 河网不存在该节点名称
//	}
//	return 0;
//}
//
//static bool is_start_time_same(INetwork* pipe_net, INetwork* river_net)
////------------------------------------------------------------------------------
//// 目的：检查两个模型起始时间是否相同
////------------------------------------------------------------------------------
//{
//	return (pipe_net->getStartDateTime() == river_net->getStartDateTime());
//}
//
//static bool is_report_step_same(INetwork* pipe_net, INetwork* river_net)
////------------------------------------------------------------------------------
//// 目的：检查两个模型报告时间步长是否相同
////------------------------------------------------------------------------------
//{
//	return (pipe_net->getOption()->getReportStep() ==
//		   river_net->getOption()->getReportStep());
//}
//
//static void pipe_to_river(
//	INodeSet* pipe_nodes, INodeSet* river_nodes, double t,
//	const std::vector<std::pair<std::string, std::string>>& connect_names)
////------------------------------------------------------------------------------
//// 目的：更新河网模型节点进流，并重新初始化
////------------------------------------------------------------------------------
//{
//	double pipe_outflow = 0.0;
//	INode* pipe_node    = nullptr;
//	INode* river_node   = nullptr;
//	ITimeseries* river_edf_ts = nullptr;
//	for (const auto& [pipe_name, river_name] : connect_names)
//	{
//		// 找到关联节点对
//		pipe_node  = pipe_nodes->getNodeObject(pipe_name.c_str());
//		river_node = river_nodes->getNodeObject(river_name.c_str());
//		assert(pipe_node  != nullptr);
//		assert(river_node != nullptr);
//		assert(pipe_node->isOutfall());
//		assert(!river_node->isOutfall());
//
//		// 获取河道水位		
//		pipe_outflow = pipe_node->getOutFlow();		
//
//		// 设置成管道排放口水位，并重新初始化		
//		river_edf_ts = river_node->getExternDirectFlow()->getTimeseries();
//		assert(river_edf_ts != nullptr);
//		river_edf_ts->addDataElement(t, pipe_outflow);
//		river_edf_ts->initStateAfterModified(t);
//	}
//}
//
//static void river_to_pipe(
//	INodeSet* river_nodes, INodeSet* pipe_nodes, double t,
//	const std::vector<std::pair<std::string, std::string>>& connect_names)
////------------------------------------------------------------------------------
//// 目的：更新管网模型排放口水位，并重新初始化
////------------------------------------------------------------------------------
//{
//	double river_head = 0.0;
//	INode* pipe_node = nullptr;
//	INode* river_node = nullptr;
//	ITimeseries* pipe_ts = nullptr;
//	for (const auto& [pipe_name, river_name] : connect_names)
//	{
//		// 找到关联节点对
//		pipe_node = pipe_nodes->getNodeObject(pipe_name.c_str());
//		river_node = river_nodes->getNodeObject(river_name.c_str());
//		assert(pipe_node != nullptr);
//		assert(river_node != nullptr);
//		assert(pipe_node->isOutfall());
//		assert(!river_node->isOutfall());
//
//		// 获取河道水位
//		river_head = river_node->getHead();
//
//		// 设置成管道排放口水位，并重新初始化		
//		pipe_ts = (dynamic_cast<IOutfall*>(pipe_node))->getTimeseries();
//		assert(pipe_ts != nullptr);
//		pipe_ts->addDataElement(t, river_head);
//		pipe_ts->initStateAfterModified(t);
//	}
//}

int connect_pipe_with_river(INetwork* pipe_net, INetwork* river_net,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// 参数：pipe_net      = 已初始化的实时管网模型
//       river_net     = 已初始化的实时河网模型（也可以是主干管网模型）
//       connect_names = 管网和河网的重叠节点名称对
// 说明：1）connect_names的用途是在管网和河网之间交互数据。
//       2）connect_names中的节点对，第1个是管网节点，第2个是河网节点，它们在空
//       间上完全重合。
//       3）pipe_net向river_net提供进流，river_net对pipe_net造成顶托。只要满足这
//       个条件的两个一维模型均可以采用本函数耦合。
//       4）connect_names的第1个名称，均为pipe_net的排放口节点，且排放口尾水类型
//       为时间序列（实时模型本就如此！）。
//       5）connect_names的第2个名称，均为river_net的非排放口节点，且该节点进水
//       类型为外部直接入流。
//       6) 通过拆分管网和河网，一维模型能得到尽可能多的多线程加速能力。
//       7）报告时间点应该完全重合
//------------------------------------------------------------------------------
{
	// 1. 检查数据
	INodeSet* pipe_nodes  = pipe_net->getNodeSet();
	INodeSet* river_nodes = river_net->getNodeSet();
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

	// 2. 耦合运行
	int status = 0;
	// 报告时间和报告时间步长，河网/管网的该值相同		
	const int report_step = pipe_net->getOption()->getReportStep();
	double old_report_t   = pipe_net->getEndRoutingDateTime();
	double new_report_t   = addSeconds(old_report_t, report_step); // 下一报告时间
	while (pipe_net->getEndRoutingDateTime() < pipe_net->getEndDateTime())
	{
		// 3. 更新管网模型状态 
		while (pipe_net->getEndRoutingDateTime() < new_report_t)
		{
			status = pipe_net->updateState();
			if (status != 0)
				return status;
		}

		// 4. 更新河网模型的进流为管网模型当前结果
		pipe_to_river(pipe_nodes, river_nodes, old_report_t, connect_names);

		// 5. 更新河网模型状态			
		while (river_net->getEndRoutingDateTime() < new_report_t)
		{
			status = river_net->updateState();
			if (status != 0)
				return status;
		}

		// 测试两个模型外排量
		{
			INode* pipe_out  =  pipe_nodes->getNodeObject("O1");
			INode* river_out = river_nodes->getNodeObject("river_out");
			std::cout <<  pipe_out->getOutFlow() << ", "
				      << river_out->getOutFlow() << "\n";			
		}

		// 6. 更新管网模型的尾水水位为河网模型报告结果	
		river_to_pipe(river_nodes, pipe_nodes, new_report_t, connect_names);

		// 7. 更新报告时间
		old_report_t = new_report_t;
		new_report_t = addSeconds(old_report_t, report_step);    	
	}

	return 0;
}