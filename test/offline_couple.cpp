/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.0.0
 * File       ：离线耦合测试
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

	// 2. 执行耦合计算
	const string inp_path = string(data_path) + "/model.inp";
//	err_id = offline_couple(inp_path.c_str(), data_path, out_path, "setup.csv");
	err_id = offlineCouple(inp_path.c_str(), data_path, out_path, "setup.csv");
	if (err_id != 0)
		return disconnect_and_return(err_id);
	
	return disconnect_and_return(0);
}