/*
 *******************************************************************************
 * Project    ��Coupler
 * Version    ��1.0.0
 * File       ������1ά��2ά
 * Date       ��03/23/2023
 * Author     ����С��
 * Copyright  ������ˮ�ֽڿƼ����޹�˾
 *******************************************************************************
 */

#include <cassert>
#include <iostream>

#include "couplefun.h"

static bool is_report_time(
	double& next_report_time, double current_time, int report_step)
//------------------------------------------------------------------------------
// ������next_report_time = ��һ������ʱ��
//       current_time     = ��ǰ����ĩ��ʱ�䣨��ʱ��״̬�ոո��£�
//       report_step      = ����ʱ�䲽����s
// Ŀ�ģ���鵱ǰʱ���ǲ��Ǳ���ʱ�䣿����ǣ�ͬ�����±���ʱ��
//------------------------------------------------------------------------------
{
	if (next_report_time != current_time)
		return false;

	// ����next_report_time
	next_report_time = addSeconds(next_report_time, report_step);
	return true;
}

static void create_inflow_events(INodeSet* nodes, ICoordinates* nodes_xy, 
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
	IBox*  box  = nullptr;    // �����ڵ����ڵľ�������
	INode* node = nullptr;    // �����ڵ����
	const int node_counts = nodes->getNodeCounts(); // һάģ�ͽڵ�����
	for (int i = 0; i < node_counts; ++i)
	{
		node = nodes->getNodeObjectAndName(name, i);
		assert(node != nullptr);

		// ����ýڵ�û������������
		const double overflow = node->getOverFlow(); // m3/s
		if (overflow == 0.0) continue;

		//if (strcmp("J9", name) == 0)
		//	std::cout << "�ڵ�" << name << "��ǰ����ǿ��Ϊ��" << overflow << "\n";

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

bool online_couple(INetwork* inited_net, IFlood* inited_flood, IGeoprocess* geo,
	double pv_threshold)
//------------------------------------------------------------------------------
// ������inited_net   = һ���Ѿ���ʼ����һάģ��ģ��
//       inited_flood = һ���Ѿ���ʼ����ʵʱ���Է�������
//       geo          = ����ռ���Ϣ����
//       pv_threshold = �����ֵ����ʱ����ֵ�����ڸ�ֵ��0.0����
// Ҫ��һάģ�͵ı���ʱ�䲽��ӦΪ��άģ�͵ĸ�������������
//------------------------------------------------------------------------------
{
	// 1. ���Ȼ�ȡISetup����
	ISetup* setup = inited_flood->getSetup();

	// 2. ���ʱ���Ƿ����Ҫȷ��һάģ�ͻ�û���㵽��άģ�͵Ŀ�ʼ��
	//    �����ȷ��һάģ�͵ĵ�ǰ��������ĩ��ʱ��Ϊ��άģ�͵�ģ����ʼʱ�䡣
	//    ע�������ò��ּ�飬�㷨Ҳ���������С����ǣ������ǰ�Ѿ���ˮ������Щ��
	//        ˮ�����ڶ�άģ���м��Դ���ͨ�׽��������޷��ڶ�άģ���ʼ���׶�ָ
	//        ����״��ˮ��ȣ�����ˣ�ֻ��Ҫ���ڶ�ά����֮ǰ��һά��û����������
	//        ��Ҫ���û��Լ�����֤��
	const double start_t = inited_net->getEndRoutingDateTime();
	if (start_t > setup->getStartTime())
		return false;

	// 3. �Ƿ����ʵʱ���Է���Ȩ�ޣ��ٶ��ͻ�����һάģ����Ȩ�ޣ�
	int err_id = getRtFloodStatus();
	if (err_id != 0)
		return false;

	// 4. ��ȡһ���ֺ���Ҫ����ʹ�õĶ���
	INodeSet*       nodes    = inited_net->getNodeSet();
	ICoordinates*   nodes_xy = geo->getCoordinates();
	IMap*           map      = geo->getMap();
	IGrid*          grid     = inited_flood->getGrid();
	IInflowManager* inflows  = inited_flood->getInflowManager();	

	// 5. ����ִ��һά���㣬��Ƕ���ά����	
	const int report_step = inited_net->getOption()->getReportStep();
	double next_report_time = start_t;
	while (inited_net->getEndRoutingDateTime() < setup->getEndTime())
	{
		// ����Ƿ���Ҫ�Ͷ�ά������ϣ���һ�ο϶���ϣ�
		const double current_time = inited_net->getEndRoutingDateTime();		
		if (is_report_time(next_report_time, current_time, report_step) &&
			inited_flood->getCurrentTime() <= current_time)
		{			
			// 1. ��������¼�
			create_inflow_events(
				nodes, nodes_xy, map, grid, inflows, current_time);

			// 2. �������
			if (!inited_flood->validateData())
				return false;

			// 3. ���¼���������Ϊ������λ�ÿ��ܷ����仯��
			inited_flood->updateComputeRegion();

			// 4. ���¶�άģ��״̬			
			while (inited_flood->getCurrentTime() < next_report_time)			
				inited_flood->updateState();	
			// ���һά�ı���ʱ�䲽���Ƕ�ά�ĸ������ڵ��������������¶��Գ���
			// �������¶��Բ�������
			assert(inited_flood->getCurrentTime() == next_report_time);

			// 5. �����άģ��ĵ�ǰʱ�̽��
			// ע�������ļ�·����Ҫ��ǰָ����
			if (!inited_flood->outputDepthRaster())
				return false;
			if (!inited_flood->outputVelocityAngleRaster())
				return false;
		}

		// ����һάģ��״̬		
		err_id = inited_net->updateState();
		if (err_id != 0)
			return false;
	}

	// 6. �����άģ��ķ�ֵʱ�̽��
	// ע�������ļ�·����Ҫ��ǰָ����
	if (!inited_flood->outputPeakDepthRaster())
		return false;
	if (!inited_flood->outputPeakVelocityRaster(pv_threshold))
		return false;

	// 7. �����ô���1
	inited_net->updateSystemStats();
	INodeStatsSet* inss = inited_net->getSubNet(0)->getNodeStatsSet();
	std::cout << "�����뿪ϵͳ��ˮ����" << inss->getTotalOverFlow() << "m3\n";
	double pond_volume = 0.0;
	INodeStats* ins = nullptr;
	for (int i = 0; i < inss->getNodeStatsCounts(); ++i)
	{
		ins = inss->getNodeStats(i);
		pond_volume += ins->getPondedVolume();
	}
	std::cout << "�ڵ��ˮ����" << pond_volume << "m3\n";

	//// 8. �����ô���2
	//std::cout << "�ܽ�����Ϊ��" << inited_flood->getRainVolume()   << "m3\n";
	//std::cout << "��������Ϊ��" << inited_flood->getInflowVolume() << "m3\n";
	//std::cout << "�ܻ�ˮ��Ϊ��" << inited_flood->getPondVolume()   << "m3\n";
	//std::cout << "��������Ϊ��" << inited_flood->getInfilVolume()  << "m3\n";

	return true;
}

int partial_online_couple(INetwork* net_copy, const char* geo_file, 
	const char* work_directory, const char* out_directory, 
	const char* setup_file, double pv_threshold)
//------------------------------------------------------------------------------
// ������net_copy       = �������е�һάģ�͵Ŀ�¡��
//       geo_file       = ��������ռ���Ϣ��inp�ļ�
//       work_directory = ��Ŷ�άģ�͵��������ݵ��ļ���
//       out_directory  = ��Ŷ�άģ�͵�������ݵ��ļ���
//       setup_file     = ��άģ�͵Ĺ����ļ����ڲ�����ָ��DEM�ļ����ƣ�
//       pv_threshold   = �����ֵ����ʱ����ֵ�����ڸ�ֵ��0.0����
// Ҫ��һάģ�͵ı���ʱ�䲽��ӦΪ��άģ�͵ĸ�������������
//------------------------------------------------------------------------------
{
	// 1. ��������ռ���Ϣ����	
	IGeoprocess* geo = createGeoprocess();
	if (!geo->openGeoFile(geo_file))
		return 1100;
	if (!geo->validateData())
		return 1101;

	// 2. ����ʵʱ���Է�������
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

	// 3. ��������
	if (!online_couple(net_copy, flood, geo, pv_threshold))
		return 1200;

	// 4. �ͷ��ڴ�
	deleteFlood(flood);
	deleteGeoprocess(geo);

	return 0;
}

int offline_couple(const char* inp_file, const char* work_directory,
	const char* out_directory, const char* setup_file, double pv_threshold)
//------------------------------------------------------------------------------
// ������inp_file       = һάģ�͵�inp�ļ�����������ռ���Ϣ��
//       work_directory = ��Ŷ�άģ�͵��������ݵ��ļ���
//       out_directory  = ��Ŷ�άģ�͵�������ݵ��ļ���
//       setup_file     = ��άģ�͵Ĺ����ļ����ڲ�����ָ��DEM�ļ����ƣ�
//       pv_threshold   = �����ֵ����ʱ����ֵ�����ڸ�ֵ��0.0����
// Ҫ��һάģ�͵ı���ʱ�䲽��ӦΪ��άģ�͵ĸ�������������
//------------------------------------------------------------------------------
{
	// 1. ����һάģ�����
	INetwork* net = createNetwork();
	if (!net->readFromFile(inp_file))
		return 1000;
	if (!net->validateData())
		return 1001;
	net->initState();

	// 2. ��������
	const int err_id = partial_online_couple(net, inp_file, work_directory,
		out_directory, setup_file, pv_threshold);

	// 3. �ͷ��ڴ�
	deleteNetwork(net);

	return err_id;
}