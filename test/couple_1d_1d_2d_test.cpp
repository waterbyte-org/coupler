/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.0.0
 * File       ：在线耦合测试
 * Date       ：03/23/2023
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
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

	// 1. 连接服务器
	int err_id = initIDCheckInstance(user_id, user_name, key_path);
	if (err_id != 0)
	{
		cout << "连接服务器失败！" << "\n";
		return disconnect_and_return(err_id);
	}
	cout << "连接服务器成功！" << "\n";

	// 2. 创建一维模型（模仿实时模型）
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

	// 3. 克隆实时模型
	INetwork* pipe_net_copy = pipe_net->cloneNetwork(1);   // 克隆实时模型
	pipe_net_copy->copyStateAndStatsFrom(pipe_net);        // 复制实时状态
	INetwork* river_net_copy = river_net->cloneNetwork(1); // 克隆实时模型
	river_net_copy->copyStateAndStatsFrom(river_net);      // 复制实时状态

	// 4. 创建地理空间信息对象	
	IGeoprocess* pipe_geo = createGeoprocess();
	if (!pipe_geo->openGeoFile(pipe_inp_path.c_str()))
		return disconnect_and_return(1100);
	if (!pipe_geo->validateData())
		return disconnect_and_return(1101);

	// 5. 创建实时内涝分析对象
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

	// 6. 修改二维模拟的起始和结束时间
	//    注：setup.csv文件中的起始和结束时间，在实时模型运行时，很难提前设置好！
	//        因此，要通过ISetup对象接口设置
	ISetup* setup = flood->getSetup();
	const double start_time = pipe_net_copy->getEndRoutingDateTime(); 
	setup->setStartTime(start_time);
	const double end_time = addDays(start_time, 0.25);           // 往后6h=0.25d
	setup->setEndTime(end_time);

	// 7. 执行耦合计算
	std::vector<std::pair<std::string, std::string>> connect_names;
	connect_names.emplace_back("O1", "river_in");
	int ret = online_couple_1d_1d_2d(
		pipe_net_copy, river_net_copy, flood, pipe_geo, connect_names);
	if (ret != 0)
		return disconnect_and_return(ret);

	// 8. 获取统计数据（边界数据要特殊处理）
	cout << "总降雨量为：" << flood->getRainVolume()   << "m3\n";
	cout << "总入流量为：" << flood->getInflowVolume() << "m3\n";
	cout << "总积水量为：" << flood->getPondVolume()   << "m3\n";
	cout << "总下渗量为：" << flood->getInfilVolume()  << "m3\n";

	// 9. 释放内存
	deleteNetwork(pipe_net);
	deleteNetwork(pipe_net_copy);
	deleteNetwork(river_net);
	deleteNetwork(river_net_copy);
	deleteGeoprocess(pipe_geo);
	deleteFlood(flood);

	return disconnect_and_return(0);
}