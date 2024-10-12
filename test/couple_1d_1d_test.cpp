/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.2.0
 * File       ：管网和河网耦合
 * Date       ：10/11/2024
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#include "idinfo.h"
#include "globalfun.h"
#include "../src/couplefun.h"

#include <iostream>

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

	// 2. 创建管网模型
	const string pipe_inp_path = string(data_path) + "/model.inp";
	INetwork* pipe_net = createNetwork();
	if (!pipe_net->readFromFile(pipe_inp_path.c_str()))
	{
		char* error_text = new char[pipe_net->getErrorTextSize() + 1];
		pipe_net->getErrorText(error_text);
		cout << error_text << "\n";
		delete[] error_text;
		return disconnect_and_return(1);			
	}		
	if (!pipe_net->validateData())
	{ 
		char* error_text = new char[pipe_net->getErrorTextSize() + 1];
		pipe_net->getErrorText(error_text);
		cout << error_text << "\n";
		delete[] error_text;
		return disconnect_and_return(2);
	}
	pipe_net->initState();

	// 3. 创建河网模型
	const string river_inp_path = string(data_path) + "/river.inp";
	INetwork* river_net = createNetwork();
	if (!river_net->readFromFile(river_inp_path.c_str()))
		return disconnect_and_return(3);
	if (!river_net->validateData())
		return disconnect_and_return(4);
	river_net->initState();

	// 4. 创建关联名称集合
	std::vector<std::pair<std::string, std::string>> connect_names;
	connect_names.emplace_back("O1", "river_in");

	// 5. 开始耦合
	int err = connect_pipe_with_river(pipe_net, river_net, connect_names);
	if (err != 0)
		return err;

	deleteNetwork(pipe_net);
	deleteNetwork(river_net);

	return disconnect_and_return(0);
}