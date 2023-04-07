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

	// 2. ����һά����ģ�ͣ�ģ��ʵʱģ�ͣ�
	const string inp_path = string(data_path) + "/model.inp";
	INetwork* net = createNetwork();
	if (!net->readFromFile(inp_path.c_str()))
		return disconnect_and_return(1000);
	if (!net->validateData())
		return disconnect_and_return(1001);
	net->initState();

	// 3. ��¡ʵʱģ��
	INetwork* net_copy = net->cloneNetwork();  // ��¡ʵʱģ��
	net_copy->copyStateAndStatsFrom(net);      // ����ʵʱ״̬

	// 4. ��������ռ���Ϣ����	
	IGeoprocess* geo = createGeoprocess();
	if (!geo->openGeoFile(inp_path.c_str()))
		return disconnect_and_return(1100);
	if (!geo->validateData())
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
	if (!flood->validateData()) 
		return err_id;
	flood->initState();

	// 6. �޸Ķ�άģ�����ʼ�ͽ���ʱ��
	//    ע��setup.csv�ļ��е���ʼ�ͽ���ʱ�䣬��ʵʱģ������ʱ��������ǰ���úã�
	//        ��ˣ�Ҫͨ��ISetup����ӿ�����
	ISetup* setup = flood->getSetup();
	const double start_time = net_copy->getEndRoutingDateTime(); // ���������Ƴ�
	setup->setStartTime(start_time);
	const double end_time = addDays(start_time, 0.25); // ����6h=0.25d
	setup->setEndTime(end_time);

	// 7. ִ����ϼ���
//	if (!online_couple(net_copy, flood, geo))
	if (!onlineCouple(net_copy, flood, geo))
		return disconnect_and_return(-555);

	// 8. ��ȡͳ�����ݣ��߽�����Ҫ���⴦��
	cout << "�ܽ�����Ϊ��" << flood->getRainVolume()   << "m3\n";
	cout << "��������Ϊ��" << flood->getInflowVolume() << "m3\n";
	cout << "�ܻ�ˮ��Ϊ��" << flood->getPondVolume()   << "m3\n";
	cout << "��������Ϊ��" << flood->getInfilVolume()  << "m3\n";

	// 9. �ͷ��ڴ�
	deleteNetwork(net);
	deleteNetwork(net_copy);
	deleteGeoprocess(geo);
	deleteFlood(flood);

	return disconnect_and_return(0);
}