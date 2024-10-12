/*
 *******************************************************************************
 * Project    ��Coupler
 * Version    ��1.2.0
 * File       ������������������2ά
 * Date       ��10/12/2024
 * Author     ����С��
 * Copyright  ������ˮ�ֽڿƼ����޹�˾
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
// ������inited_net   = һ���Ѿ���ʼ����һάģ��ģ��
//       inited_flood = һ���Ѿ���ʼ����ʵʱ���Է�������
//       geo          = ����ռ���Ϣ����
// Ҫ��һάģ�͵ı���ʱ�䲽��ӦΪ��άģ�͵ĸ�������������
//------------------------------------------------------------------------------
{
	// 1. ���Ȼ�ȡ����
	ISetup*   setup       = flood->getSetup();
	INodeSet* pipe_nodes  = pipe_net->getNodeSet();
	INodeSet* river_nodes = river_net->getNodeSet();

	// 2. �������	
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
	// ���ʱ���Ƿ����Ҫȷ��һάģ�ͻ�û���㵽��άģ�͵Ŀ�ʼ��
	//    �����ȷ��һάģ�͵ĵ�ǰ��������ĩ��ʱ��Ϊ��άģ�͵�ģ����ʼʱ�䡣
	//    ע�������ò��ּ�飬�㷨Ҳ���������С����ǣ������ǰ�Ѿ���ˮ������Щ��
	//        ˮ�����ڶ�άģ���м��Դ���ͨ�׽��������޷��ڶ�άģ���ʼ���׶�ָ
	//        ����״��ˮ��ȣ�����ˣ�ֻ��Ҫ���ڶ�ά����֮ǰ��һά��û����������
	//        ��Ҫ���û��Լ�����֤��
	if (pipe_net->getEndRoutingDateTime() > setup->getStartTime())
		return 10;

	// 3. �Ƿ����ʵʱ���Է���Ȩ�ޣ��ٶ��ͻ�����һάģ����Ȩ�ޣ�
	int status = getRtFloodStatus();
	if (status != 0)
		return status;

	// 4. ��ȡһ���ֺ���Ҫ����ʹ�õĶ���	
	ICoordinates*   nodes_xy = pipe_geo->getCoordinates();
	IMap*           map      = pipe_geo->getMap();
	IGrid*          grid     = flood->getGrid();
	IInflowManager* inflows  = flood->getInflowManager();	
	// ͨ���ڴ��ȡ���
	int*   idx   = nullptr;
	float* depth = nullptr;
	float* vel   = nullptr;
	float* ang   = nullptr;	
	int    size;

	// 5. ִ��һά���㣬��Ƕ���ά����	
	const int report_step = pipe_net->getOption()->getReportStep();
	double   old_report_t = pipe_net->getEndRoutingDateTime();
	double   new_report_t = addSeconds(old_report_t, report_step); // ��һ����ʱ��	
	std::atomic_flag finish_2d = ATOMIC_FLAG_INIT;
	bool             err_2d    = false;
	while (pipe_net->getEndRoutingDateTime() < pipe_net->getEndDateTime())
	{
		// 1������ά�����Ƿ����
		if (err_2d) return 11;

		// 2�����������¼�				
		finish_2d.wait(true);
		create_inflow_events(
			pipe_nodes, nodes_xy, map, grid, inflows, old_report_t);

		// 3���첽ִ�ж�ά����
		finish_2d.test_and_set(); // = true
		std::jthread t1([&finish_2d, flood, new_report_t,
			&err_2d, &size, &idx, &depth, &vel, &ang]
			{
				// a���������
				if (!flood->validateEvents())
				{
					err_2d = true;
					finish_2d.clear();
					return;
				}

				// b�����¼���������Ϊ������λ�ÿ��ܷ����仯��
				flood->updateComputeRegion();

				// c�����¶�άģ��״̬			
				while (flood->getCurrentTime() < new_report_t)
					flood->updateState();
				// ���һά�ı���ʱ�䲽���Ƕ�ά�ĸ������ڵ��������������¶��Գ���
				// �������¶��Բ�������
				assert(flood->getCurrentTime() == new_report_t);

				// d�������άģ��ĵ�ǰʱ�̽��
				// ע�������ļ�·����Ҫ��ǰָ����
				size = flood->outputRasterEx(&idx, &depth, &vel, &ang);	
				std::cout << "��ǰʱ������ˮ��������" << size << "\n";
				if (!flood->outputRaster())
				{
					err_2d = true;
					finish_2d.clear();
					return ;
				}

				finish_2d.clear();
			}
		);
			
		// 4�����¹���ģ��״̬ 
		while (pipe_net->getEndRoutingDateTime() < new_report_t)
		{
			status = pipe_net->updateState();
			if (status != 0)
				return status;
		}

		// 5�����º���ģ�͵Ľ���Ϊ����ģ�͵�ǰ���
		pipe_to_river(pipe_nodes, river_nodes, old_report_t, connect_names);

		// 6�����º���ģ��״̬			
		while (river_net->getEndRoutingDateTime() < new_report_t)
		{
			status = river_net->updateState();
			if (status != 0)
				return status;
		}

		// ��������ģ��������
		{
			INode* pipe_out = pipe_nodes->getNodeObject("O1");
			INode* river_out = river_nodes->getNodeObject("river_out");
			std::cout << pipe_out->getOutFlow() << ", "
				<< river_out->getOutFlow() << "\n";
		}

		// 7�����¹���ģ�͵�βˮˮλΪ����ģ�ͱ�����	
		river_to_pipe(river_nodes, pipe_nodes, new_report_t, connect_names);

		// 8�����±���ʱ��
		old_report_t = new_report_t;
		new_report_t = addSeconds(old_report_t, report_step);
	}

	// 6. ����һάģ�͵�ϵͳͳ����
	//    ʡ��

	// 7. �����άģ��ķ�ֵʱ�̽��
	// ע�������ļ�·����Ҫ��ǰָ����
	size = flood->outputPeakRasterEx(&idx, &depth, &vel);
	std::cout << "��ֵ��Чˮ��������" << size << "\n";
	if (!flood->outputPeakRaster())
		return 12;

	return 0;
}

//int partial_online_couple(INetwork* net_copy, const char* geo_file, 
//	const char* work_directory, const char* out_directory, 
//	const char* setup_file)
////------------------------------------------------------------------------------
//// ������net_copy       = �������е�һάģ�͵Ŀ�¡��
////       geo_file       = ��������ռ���Ϣ��inp�ļ�
////       work_directory = ��Ŷ�άģ�͵��������ݵ��ļ���
////       out_directory  = ��Ŷ�άģ�͵�������ݵ��ļ���
////       setup_file     = ��άģ�͵Ĺ����ļ����ڲ�����ָ��DEM�ļ����ƣ�
//// Ҫ��һάģ�͵ı���ʱ�䲽��ӦΪ��άģ�͵ĸ�������������
////------------------------------------------------------------------------------
//{
//	// 1. ��������ռ���Ϣ����	
//	IGeoprocess* geo = createGeoprocess();
//	if (!geo->openGeoFile(geo_file))
//		return 1100;
//	if (!geo->validateData())
//		return 1101;
//
//	// 2. ����ʵʱ���Է�������
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
//	// 3. ��������
//	if (!online_couple(net_copy, flood, geo))
//		return 1200;
//
//	// 4. �ͷ��ڴ�
//	deleteFlood(flood);
//	deleteGeoprocess(geo);
//
//	return 0;
//}
//
//int offline_couple(const char* inp_file, const char* work_directory,
//	const char* out_directory, const char* setup_file)
////------------------------------------------------------------------------------
//// ������inp_file       = һάģ�͵�inp�ļ�����������ռ���Ϣ��
////       work_directory = ��Ŷ�άģ�͵��������ݵ��ļ���
////       out_directory  = ��Ŷ�άģ�͵�������ݵ��ļ���
////       setup_file     = ��άģ�͵Ĺ����ļ����ڲ�����ָ��DEM�ļ����ƣ�
//// Ҫ��һάģ�͵ı���ʱ�䲽��ӦΪ��άģ�͵ĸ�������������
////------------------------------------------------------------------------------
//{
//	// 1. ����һάģ�����
//	INetwork* net = createNetwork();
//	if (!net->readFromFile(inp_file))
//		return 1000;
//	if (!net->validateData())
//		return 1001;
//	net->initState();
//
//	// 2. ��������
//	const int err_id = partial_online_couple(
//		net, inp_file, work_directory, out_directory, setup_file);
//
//	// 3. �ͷ��ڴ�
//	deleteNetwork(net);
//
//	return err_id;
//}