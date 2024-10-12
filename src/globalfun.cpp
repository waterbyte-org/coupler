/*
 *******************************************************************************
 * Project    ��Coupler
 * Version    ��1.2.0
 * File       ��ȫ�ֺ���
 * Date       ��10/12/2024
 * Author     ����С��
 * Copyright  ������ˮ�ֽڿƼ����޹�˾
 *******************************************************************************
 */

#include "globalfun.h"

bool is_report_time(double report_time, double current_time)
//------------------------------------------------------------------------------
// ������next_report_time = ��һ������ʱ��
//       current_time     = ��ǰ����ĩ��ʱ�䣨��ʱ��״̬�ոո��£�
// Ŀ�ģ���鵱ǰʱ���ǲ��Ǳ���ʱ�䣿����ǣ�ͬ�����±���ʱ��
//------------------------------------------------------------------------------
{
	return (report_time == current_time);
}

void create_inflow_events(INodeSet* nodes, ICoordinates* nodes_xy,
	IMap* map, IGrid* grid, IInflowManager* inflows, double current_time)
//------------------------------------------------------------------------------
// ������nodes        = �ڵ���󼯺�
//       nodes_xy     = �ڵ�������꼯��
//       grid         = DEM�������
//       inflows      = ����������󣨽��սڵ�������
//       current_time = ��ǰʱ��
//------------------------------------------------------------------------------
{
	// ���inflows�����ͷ��ڲ�����ռ�õ��ڴ�ռ䣩
	inflows->clearInflowEvent();

	// �ҵ������ڵ㣬������Ӧ�������󣬲������inflows	
	double x, y;              // �����ڵ�����
	char   name[30];          // �����ڵ�����
	IBox* box = nullptr;    // �����ڵ����ڵľ�������
	INode* node = nullptr;    // �����ڵ����
	const int node_counts = nodes->getNodeCounts(); // һάģ�ͽڵ�����	
	for (int i = 0; i < node_counts; ++i)
	{
		node = nodes->getNodeObjectAndName(name, i);
		assert(node != nullptr);

		// ����ýڵ�û������������
		const double overflow = node->getOverFlow(); // m3/s
		if (overflow == 0.0) continue;

		// �ýڵ�������ʱ����Ϊ��ά����������
		// 1. �ҵ������ڵ�����(x, y)
		nodes_xy->getCoordinateByName(name, &x, &y, map);

		// 2. ���������ýڵ�ľ����������
		box = grid->createBox(x, y);

		// 3. ���������¼�
		IInflowEvent* inflow = createIsolatedInflowEvent(grid);

		// 4. ����������Χ���������������һ��Ԫ����		
		inflow->setBox(box);
		deleteIsolatedBox(box); // �ǵ��ͷ��ڴ棬��ΪsetBox��δת������Ȩ

		// 5. ���ʱ���������ݣ���һ����
		inflow->addData(overflow, current_time); // (m3/s, d)

		// 6. ����������������
		inflows->addInflowEvent(inflow); // ��������Ȩת�ƣ������ͷ�inflow�ڴ�
	}
}

int check_pipe_net(INodeSet* pipe_nodes,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// Ŀ�ģ�����������������ŷſ��Ƿ���ڣ���Ϊʱ����������
//------------------------------------------------------------------------------
{
	for (const auto& [name, ignore] : connect_names)
	{
		if (INode* node = pipe_nodes->getNodeObject(name.c_str()))
		{
			if (!node->isOutfall())
				return 1; // �ýڵ㲻���ŷſڽڵ�	
			IOutfall* out_node = dynamic_cast<IOutfall*>(node);
			if (!out_node->isTimeseriesType() ||
				out_node->getTimeseries() == nullptr)
				return 2; // βˮ����ʱ����������			
		}
		else
			return 3; // ���������ڸýڵ�����
	}
	return 0;
}

int check_river_net(INodeSet* river_nodes,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// Ŀ�ģ����������ܹ��������Ľڵ��Ƿ���ڣ��ҽ���Ϊ�ⲿֱ������
//------------------------------------------------------------------------------
{
	for (const auto& [ignore, name] : connect_names)
	{
		if (INode* node = river_nodes->getNodeObject(name.c_str()))
		{
			if (node->isOutfall())
				return 4; // �ýڵ����ŷſڽڵ�			
			if (node->getExternDirectFlow() == nullptr)
				return 5; // ���������ⲿֱ������
		}
		else
			return 6; // ���������ڸýڵ�����
	}
	return 0;
}

bool is_start_time_same(INetwork* pipe_net, INetwork* river_net)
//------------------------------------------------------------------------------
// Ŀ�ģ��������ģ����ʼʱ���Ƿ���ͬ
//------------------------------------------------------------------------------
{
	return (pipe_net->getStartDateTime() == river_net->getStartDateTime());
}

bool is_end_time_same(INetwork* pipe_net, INetwork* river_net)
//------------------------------------------------------------------------------
// Ŀ�ģ��������ģ�͵�ǰ���������ն�ʱ���Ƿ���ͬ
//------------------------------------------------------------------------------
{
	return (pipe_net->getEndRoutingDateTime() == 
		   river_net->getEndRoutingDateTime());
}

bool is_report_step_same(INetwork* pipe_net, INetwork* river_net)
//------------------------------------------------------------------------------
// Ŀ�ģ��������ģ�ͱ���ʱ�䲽���Ƿ���ͬ
//------------------------------------------------------------------------------
{
	return (pipe_net->getOption()->getReportStep() ==
		   river_net->getOption()->getReportStep());
}

void pipe_to_river(INodeSet* pipe_nodes, INodeSet* river_nodes, double t,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// Ŀ�ģ����º���ģ�ͽڵ�����������³�ʼ��
//------------------------------------------------------------------------------
{
	double pipe_outflow = 0.0;
	INode*       pipe_node    = nullptr;
	INode*       river_node   = nullptr;
	ITimeseries* river_edf_ts = nullptr;
	for (const auto& [pipe_name, river_name] : connect_names)
	{
		// �ҵ������ڵ��
		pipe_node  = pipe_nodes->getNodeObject(pipe_name.c_str());
		river_node = river_nodes->getNodeObject(river_name.c_str());
		assert(pipe_node  != nullptr);
		assert(river_node != nullptr);
		assert(pipe_node->isOutfall());
		assert(!river_node->isOutfall());

		// ��ȡ�ӵ�ˮλ		
		pipe_outflow = pipe_node->getOutFlow();

		// ���óɹܵ��ŷſ�ˮλ�������³�ʼ��		
		river_edf_ts = river_node->getExternDirectFlow()->getTimeseries();
		assert(river_edf_ts != nullptr);
		river_edf_ts->addDataElement(t, pipe_outflow);
		river_edf_ts->initStateAfterModified(t);
	}
}

void river_to_pipe(INodeSet* river_nodes, INodeSet* pipe_nodes, double t,
	const std::vector<std::pair<std::string, std::string>>& connect_names)
//------------------------------------------------------------------------------
// Ŀ�ģ����¹���ģ���ŷſ�ˮλ�������³�ʼ��
//------------------------------------------------------------------------------
{
	double river_head = 0.0;
	INode* pipe_node     = nullptr;
	INode* river_node    = nullptr;
	ITimeseries* pipe_ts = nullptr;
	for (const auto& [pipe_name, river_name] : connect_names)
	{
		// �ҵ������ڵ��
		pipe_node  = pipe_nodes->getNodeObject(pipe_name.c_str());
		river_node = river_nodes->getNodeObject(river_name.c_str());
		assert(pipe_node  != nullptr);
		assert(river_node != nullptr);
		assert(pipe_node->isOutfall());
		assert(!river_node->isOutfall());

		// ��ȡ�ӵ�ˮλ
		river_head = river_node->getHead();

		// ���óɹܵ��ŷſ�ˮλ�������³�ʼ��		
		pipe_ts = (dynamic_cast<IOutfall*>(pipe_node))->getTimeseries();
		assert(pipe_ts != nullptr);
		pipe_ts->addDataElement(t, river_head);
		pipe_ts->initStateAfterModified(t);
	}
}