/*
 *******************************************************************************
 * Project    ��Coupler
 * Version    ��1.0.0
 * File       ��������ϲ���
 * Date       ��03/23/2023
 * Author     ����С��
 * Copyright  ������ˮ�ֽڿƼ����޹�˾
 *******************************************************************************
 */

#include <iostream>

#include "idinfo.h"
#include "globalfun.h"
#include "../src/couplefun.h"
#include "../interface/coupler.h"

int main()
{
	using namespace std;

	// 1. ���ӷ�����
	int err_id = initIDCheckInstance(user_id, user_name, key_path);
	if (err_id != 0)
	{
		cout << "���ӷ�����ʧ�ܣ�" << "\n";
		return disconnect_and_return(err_id);
	}
	cout << "���ӷ������ɹ���" << "\n";

	// 2. ����һάģ�ͣ�ģ��ʵʱģ�ͣ�
	const string pipe_inp_path = string(data_path) + "/model.inp";
	INetwork* pipe_net = createNetwork();
	if (!pipe_net->readFromFile(pipe_inp_path.c_str()))
		return disconnect_and_return(1000);
	if (!pipe_net->validateData())
		return disconnect_and_return(1001);
	pipe_net->initState();
	const string river_inp_path = string(data_path) + "/river.inp";
	INetwork* river_net = createNetwork();
	if (!river_net->readFromFile(river_inp_path.c_str()))
		return disconnect_and_return(3);
	if (!river_net->validateData())
		return disconnect_and_return(4);
	river_net->initState();

	// 3. ��¡ʵʱģ��
	INetwork* pipe_net_copy = pipe_net->cloneNetwork(1);   // ��¡ʵʱģ��
	pipe_net_copy->copyStateAndStatsFrom(pipe_net);        // ����ʵʱ״̬
	INetwork* river_net_copy = river_net->cloneNetwork(1); // ��¡ʵʱģ��
	river_net_copy->copyStateAndStatsFrom(river_net);      // ����ʵʱ״̬

	// 4. ��������ռ���Ϣ����	
	IGeoprocess* pipe_geo = createGeoprocess();
	if (!pipe_geo->openGeoFile(pipe_inp_path.c_str()))
		return disconnect_and_return(1100);
	if (!pipe_geo->validateData())
		return disconnect_and_return(1101);

	// 5. ����ʵʱ���Է�������
	IFlood* flood = createFlood();
	flood->setWorkingDirectory(data_path);
	flood->setOutputDirectory(out_path);
	err_id = flood->readSetupAndDemData("setup.csv");
	if (err_id != 0)
		return err_id;
	err_id = flood->readEventData(); 
	if (err_id != 0)
		return err_id;
	flood->generateData();
	if (!flood->validateEvents()) 
		return err_id;
	flood->initState();

	// 6. �޸Ķ�άģ�����ʼ�ͽ���ʱ��
	//    ע��setup.csv�ļ��е���ʼ�ͽ���ʱ�䣬��ʵʱģ������ʱ��������ǰ���úã�
	//        ��ˣ�Ҫͨ��ISetup����ӿ�����
	ISetup* setup = flood->getSetup();
	const double start_time = pipe_net_copy->getEndRoutingDateTime(); 
	setup->setStartTime(start_time);
	const double end_time = addDays(start_time, 0.25);           // ����6h=0.25d
	setup->setEndTime(end_time);

	// 7. ִ����ϼ���
	std::vector<std::pair<std::string, std::string>> connect_names;
	connect_names.emplace_back("O1", "river_in");
	int ret = online_couple_1d_1d_2d(
		pipe_net_copy, river_net_copy, flood, pipe_geo, connect_names);
	if (ret != 0)
		return disconnect_and_return(ret);

	// 8. ��ȡͳ�����ݣ��߽�����Ҫ���⴦��
	cout << "�ܽ�����Ϊ��" << flood->getRainVolume()   << "m3\n";
	cout << "��������Ϊ��" << flood->getInflowVolume() << "m3\n";
	cout << "�ܻ�ˮ��Ϊ��" << flood->getPondVolume()   << "m3\n";
	cout << "��������Ϊ��" << flood->getInfilVolume()  << "m3\n";

	// 9. �ͷ��ڴ�
	deleteNetwork(pipe_net);
	deleteNetwork(pipe_net_copy);
	deleteNetwork(river_net);
	deleteNetwork(river_net_copy);
	deleteGeoprocess(pipe_geo);
	deleteFlood(flood);

	return disconnect_and_return(0);
}