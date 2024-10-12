/*
 *******************************************************************************
 * Project    ��Coupler
 * Version    ��1.2.0
 * File       �����������ͺ���
 * Date       ��10/12/2024
 * Author     ����С��
 * Copyright  ������ˮ�ֽڿƼ����޹�˾
 *******************************************************************************
 */

#include "globalfun.h"

#include <iostream>

//static int check_pipe_net(INodeSet* pipe_nodes, 
//	const std::vector<std::pair<std::string, std::string>>& connect_names)
////------------------------------------------------------------------------------
//// Ŀ�ģ�����������������ŷſ��Ƿ���ڣ���Ϊʱ����������
////------------------------------------------------------------------------------
//{
//	for (const auto& [name, ignore] : connect_names)
//	{
//		if (INode* node = pipe_nodes->getNodeObject(name.c_str()))
//		{
//			if (!node->isOutfall())
//				return 1; // �ýڵ㲻���ŷſڽڵ�	
//			IOutfall* out_node = dynamic_cast<IOutfall*>(node);
//			if (!out_node->isTimeseriesType() || 
//				 out_node->getTimeseries() == nullptr)
//				return 2; // βˮ����ʱ����������			
//		}
//		else
//			return 3; // ���������ڸýڵ�����
//	}
//	return 0;
//}
//
//static int check_river_net(INodeSet* river_nodes,
//	const std::vector<std::pair<std::string, std::string>>& connect_names)
////------------------------------------------------------------------------------
//// Ŀ�ģ����������ܹ��������Ľڵ��Ƿ���ڣ��ҽ���Ϊ�ⲿֱ������
////------------------------------------------------------------------------------
//{
//	for (const auto& [ignore, name] : connect_names)
//	{
//		if (INode* node = river_nodes->getNodeObject(name.c_str()))
//		{
//			if (node->isOutfall())
//				return 4; // �ýڵ����ŷſڽڵ�			
//			if (node->getExternDirectFlow() == nullptr)
//				return 5; // ���������ⲿֱ������
//		}
//		else
//			return 6; // ���������ڸýڵ�����
//	}
//	return 0;
//}
//
//static bool is_start_time_same(INetwork* pipe_net, INetwork* river_net)
////------------------------------------------------------------------------------
//// Ŀ�ģ��������ģ����ʼʱ���Ƿ���ͬ
////------------------------------------------------------------------------------
//{
//	return (pipe_net->getStartDateTime() == river_net->getStartDateTime());
//}
//
//static bool is_report_step_same(INetwork* pipe_net, INetwork* river_net)
////------------------------------------------------------------------------------
//// Ŀ�ģ��������ģ�ͱ���ʱ�䲽���Ƿ���ͬ
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
//// Ŀ�ģ����º���ģ�ͽڵ�����������³�ʼ��
////------------------------------------------------------------------------------
//{
//	double pipe_outflow = 0.0;
//	INode* pipe_node    = nullptr;
//	INode* river_node   = nullptr;
//	ITimeseries* river_edf_ts = nullptr;
//	for (const auto& [pipe_name, river_name] : connect_names)
//	{
//		// �ҵ������ڵ��
//		pipe_node  = pipe_nodes->getNodeObject(pipe_name.c_str());
//		river_node = river_nodes->getNodeObject(river_name.c_str());
//		assert(pipe_node  != nullptr);
//		assert(river_node != nullptr);
//		assert(pipe_node->isOutfall());
//		assert(!river_node->isOutfall());
//
//		// ��ȡ�ӵ�ˮλ		
//		pipe_outflow = pipe_node->getOutFlow();		
//
//		// ���óɹܵ��ŷſ�ˮλ�������³�ʼ��		
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
//// Ŀ�ģ����¹���ģ���ŷſ�ˮλ�������³�ʼ��
////------------------------------------------------------------------------------
//{
//	double river_head = 0.0;
//	INode* pipe_node = nullptr;
//	INode* river_node = nullptr;
//	ITimeseries* pipe_ts = nullptr;
//	for (const auto& [pipe_name, river_name] : connect_names)
//	{
//		// �ҵ������ڵ��
//		pipe_node = pipe_nodes->getNodeObject(pipe_name.c_str());
//		river_node = river_nodes->getNodeObject(river_name.c_str());
//		assert(pipe_node != nullptr);
//		assert(river_node != nullptr);
//		assert(pipe_node->isOutfall());
//		assert(!river_node->isOutfall());
//
//		// ��ȡ�ӵ�ˮλ
//		river_head = river_node->getHead();
//
//		// ���óɹܵ��ŷſ�ˮλ�������³�ʼ��		
//		pipe_ts = (dynamic_cast<IOutfall*>(pipe_node))->getTimeseries();
//		assert(pipe_ts != nullptr);
//		pipe_ts->addDataElement(t, river_head);
//		pipe_ts->initStateAfterModified(t);
//	}
//}

int connect_pipe_with_river(INetwork* pipe_net, INetwork* river_net,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// ������pipe_net      = �ѳ�ʼ����ʵʱ����ģ��
//       river_net     = �ѳ�ʼ����ʵʱ����ģ�ͣ�Ҳ���������ɹ���ģ�ͣ�
//       connect_names = �����ͺ������ص��ڵ����ƶ�
// ˵����1��connect_names����;���ڹ����ͺ���֮�佻�����ݡ�
//       2��connect_names�еĽڵ�ԣ���1���ǹ����ڵ㣬��2���Ǻ����ڵ㣬�����ڿ�
//       ������ȫ�غϡ�
//       3��pipe_net��river_net�ṩ������river_net��pipe_net��ɶ��С�ֻҪ������
//       ������������һάģ�;����Բ��ñ�������ϡ�
//       4��connect_names�ĵ�1�����ƣ���Ϊpipe_net���ŷſڽڵ㣬���ŷſ�βˮ����
//       Ϊʱ�����У�ʵʱģ�ͱ�����ˣ�����
//       5��connect_names�ĵ�2�����ƣ���Ϊriver_net�ķ��ŷſڽڵ㣬�Ҹýڵ��ˮ
//       ����Ϊ�ⲿֱ��������
//       6) ͨ����ֹ����ͺ�����һάģ���ܵõ������ܶ�Ķ��̼߳���������
//       7������ʱ���Ӧ����ȫ�غ�
//------------------------------------------------------------------------------
{
	// 1. �������
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

	// 2. �������
	int status = 0;
	// ����ʱ��ͱ���ʱ�䲽��������/�����ĸ�ֵ��ͬ		
	const int report_step = pipe_net->getOption()->getReportStep();
	double old_report_t   = pipe_net->getEndRoutingDateTime();
	double new_report_t   = addSeconds(old_report_t, report_step); // ��һ����ʱ��
	while (pipe_net->getEndRoutingDateTime() < pipe_net->getEndDateTime())
	{
		// 3. ���¹���ģ��״̬ 
		while (pipe_net->getEndRoutingDateTime() < new_report_t)
		{
			status = pipe_net->updateState();
			if (status != 0)
				return status;
		}

		// 4. ���º���ģ�͵Ľ���Ϊ����ģ�͵�ǰ���
		pipe_to_river(pipe_nodes, river_nodes, old_report_t, connect_names);

		// 5. ���º���ģ��״̬			
		while (river_net->getEndRoutingDateTime() < new_report_t)
		{
			status = river_net->updateState();
			if (status != 0)
				return status;
		}

		// ��������ģ��������
		{
			INode* pipe_out  =  pipe_nodes->getNodeObject("O1");
			INode* river_out = river_nodes->getNodeObject("river_out");
			std::cout <<  pipe_out->getOutFlow() << ", "
				      << river_out->getOutFlow() << "\n";			
		}

		// 6. ���¹���ģ�͵�βˮˮλΪ����ģ�ͱ�����	
		river_to_pipe(river_nodes, pipe_nodes, new_report_t, connect_names);

		// 7. ���±���ʱ��
		old_report_t = new_report_t;
		new_report_t = addSeconds(old_report_t, report_step);    	
	}

	return 0;
}