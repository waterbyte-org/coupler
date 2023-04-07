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

	// 2. 创建一维在线模型（模仿实时模型）
	const string inp_path = string(data_path) + "/model.inp";
	INetwork* net = createNetwork();
	if (!net->readFromFile(inp_path.c_str()))
		return disconnect_and_return(1000);
	if (!net->validateData())
		return disconnect_and_return(1001);
	net->initState();

	// 3. 克隆实时模型
	INetwork* net_copy = net->cloneNetwork();  // 克隆实时模型
	net_copy->copyStateAndStatsFrom(net);      // 复制实时状态

	// 4. 创建地理空间信息对象	
	IGeoprocess* geo = createGeoprocess();
	if (!geo->openGeoFile(inp_path.c_str()))
		return disconnect_and_return(1100);
	if (!geo->validateData())
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
	if (!flood->validateData()) 
		return err_id;
	flood->initState();

	// 6. 修改二维模拟的起始和结束时间
	//    注：setup.csv文件中的起始和结束时间，在实时模型运行时，很难提前设置好！
	//        因此，要通过ISetup对象接口设置
	ISetup* setup = flood->getSetup();
	const double start_time = net_copy->getEndRoutingDateTime(); // 可以往后推迟
	setup->setStartTime(start_time);
	const double end_time = addDays(start_time, 0.25); // 往后6h=0.25d
	setup->setEndTime(end_time);

	// 7. 执行耦合计算
//	if (!online_couple(net_copy, flood, geo))
	if (!onlineCouple(net_copy, flood, geo))
		return disconnect_and_return(-555);

	// 8. 获取统计数据（边界数据要特殊处理）
	cout << "总降雨量为：" << flood->getRainVolume()   << "m3\n";
	cout << "总入流量为：" << flood->getInflowVolume() << "m3\n";
	cout << "总积水量为：" << flood->getPondVolume()   << "m3\n";
	cout << "总下渗量为：" << flood->getInfilVolume()  << "m3\n";

	// 9. 释放内存
	deleteNetwork(net);
	deleteNetwork(net_copy);
	deleteGeoprocess(geo);
	deleteFlood(flood);

	return disconnect_and_return(0);
}