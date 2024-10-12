/*
 *******************************************************************************
 * Project    ��Coupler
 * Version    ��1.2.0
 * File       ������1ά��2ά
 * Date       ��10/08/2024
 * Author     ����С��
 * Copyright  ������ˮ�ֽڿƼ����޹�˾
 *******************************************************************************
 */

#include "globalfun.h"

#include <atomic>
#include <thread>
#include <iostream>

bool online_couple(INetwork* inited_net, IFlood* inited_flood, IGeoprocess* geo)
//------------------------------------------------------------------------------
// ������inited_net   = һ���Ѿ���ʼ����һάģ��ģ��
//       inited_flood = һ���Ѿ���ʼ����ʵʱ���Է�������
//       geo          = ����ռ���Ϣ����
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
	// ͨ���ڴ��ȡ���
	int*   idx   = nullptr;
	float* depth = nullptr;
	float* vel   = nullptr;
	float* ang   = nullptr;
	int    size;

	// 5. ����ִ��һά���㣬��Ƕ���ά����	
	const int report_step      = inited_net->getOption()->getReportStep();
	double    next_report_time = start_t;
	bool      new_inflow       = false;
	std::atomic_flag finish_2d = ATOMIC_FLAG_INIT;
	bool             err_2d    = false;
	while (inited_net->getEndRoutingDateTime() < setup->getEndTime())
	{
		// ����ά�����Ƿ����
		if (err_2d) return false;

		// �����µ���������������һάģ��ı���ʱ�䣩�����������¼�		
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

		// ����Ƿ���Ҫ�Ͷ�ά������ϣ���һ�ο϶���ϣ�
		if (new_inflow)
		{
			new_inflow = false;
			finish_2d.test_and_set(); // = true
		
			std::jthread t1([&finish_2d, inited_flood, next_report_time, 
				&err_2d, &size, &idx, &depth, &vel, &ang]
				{
				// 1. �������
				if (!inited_flood->validateEvents())
				{ 
					err_2d = true;	
					finish_2d.clear();
					return;
				}

				// 2. ���¼���������Ϊ������λ�ÿ��ܷ����仯��
				inited_flood->updateComputeRegion();

				// 3. ���¶�άģ��״̬			
				while (inited_flood->getCurrentTime() < next_report_time)
					inited_flood->updateState();
				// ���һά�ı���ʱ�䲽���Ƕ�ά�ĸ������ڵ��������������¶��Գ���
				// �������¶��Բ�������
				assert(inited_flood->getCurrentTime() == next_report_time);				

				// 4. �����άģ��ĵ�ǰʱ�̽��
				// ע�������ļ�·����Ҫ��ǰָ����
				size = inited_flood->outputRasterEx(&idx, &depth, &vel, &ang);
				std::cout << "��ǰʱ������ˮ��������" << size << "\n";
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

		// ����һάģ��״̬		
		err_id = inited_net->updateState();
		if (err_id != 0)
			return false;		
	}

	// 6. �����άģ��ķ�ֵʱ�̽��
	// ע�������ļ�·����Ҫ��ǰָ����
	size = inited_flood->outputPeakRasterEx(&idx, &depth, &vel);
	std::cout << "��ֵ��Чˮ��������" << size << "\n";
	if (!inited_flood->outputPeakRaster())
		return false;

	return true;
}

int partial_online_couple(INetwork* net_copy, const char* geo_file, 
	const char* work_directory, const char* out_directory, 
	const char* setup_file)
//------------------------------------------------------------------------------
// ������net_copy       = �������е�һάģ�͵Ŀ�¡��
//       geo_file       = ��������ռ���Ϣ��inp�ļ�
//       work_directory = ��Ŷ�άģ�͵��������ݵ��ļ���
//       out_directory  = ��Ŷ�άģ�͵�������ݵ��ļ���
//       setup_file     = ��άģ�͵Ĺ����ļ����ڲ�����ָ��DEM�ļ����ƣ�
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
	if (!flood->validateEvents()) 
		return err_id;
	flood->initState();

	// 3. ��������
	if (!online_couple(net_copy, flood, geo))
		return 1200;

	// 4. �ͷ��ڴ�
	deleteFlood(flood);
	deleteGeoprocess(geo);

	return 0;
}

int offline_couple(const char* inp_file, const char* work_directory,
	const char* out_directory, const char* setup_file)
//------------------------------------------------------------------------------
// ������inp_file       = һάģ�͵�inp�ļ�����������ռ���Ϣ��
//       work_directory = ��Ŷ�άģ�͵��������ݵ��ļ���
//       out_directory  = ��Ŷ�άģ�͵�������ݵ��ļ���
//       setup_file     = ��άģ�͵Ĺ����ļ����ڲ�����ָ��DEM�ļ����ƣ�
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
	const int err_id = partial_online_couple(
		net, inp_file, work_directory, out_directory, setup_file);

	// 3. �ͷ��ڴ�
	deleteNetwork(net);

	return err_id;
}