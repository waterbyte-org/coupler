/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.0.0
 * File       ：部分在线耦合测试
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
	
	// 3. 执行耦合计算	
	INetwork* net_copy = net->cloneNetwork();  // 克隆实时模型
	net_copy->copyStateAndStatsFrom(net);      // 复制实时状态
//	err_id = partial_online_couple(
	err_id = partialOnlineCouple(
		net_copy, inp_path.c_str(), data_path, out_path, "setup.csv");
	if (err_id != 0)
		return disconnect_and_return(err_id);

	// 4. 释放内存
	deleteNetwork(net);
	deleteNetwork(net_copy);

	return disconnect_and_return(0);
}