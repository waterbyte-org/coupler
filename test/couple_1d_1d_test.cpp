/*
 *******************************************************************************
 * Project    ��Coupler
 * Version    ��1.2.0
 * File       �������ͺ������
 * Date       ��10/11/2024
 * Author     ����С��
 * Copyright  ������ˮ�ֽڿƼ����޹�˾
 *******************************************************************************
 */

#include "idinfo.h"
#include "globalfun.h"
#include "../src/couplefun.h"

#include <iostream>

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

	// 2. ��������ģ��
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

	// 3. ��������ģ��
	const string river_inp_path = string(data_path) + "/river.inp";
	INetwork* river_net = createNetwork();
	if (!river_net->readFromFile(river_inp_path.c_str()))
		return disconnect_and_return(3);
	if (!river_net->validateData())
		return disconnect_and_return(4);
	river_net->initState();

	// 4. �����������Ƽ���
	std::vector<std::pair<std::string, std::string>> connect_names;
	connect_names.emplace_back("O1", "river_in");

	// 5. ��ʼ���
	int err = connect_pipe_with_river(pipe_net, river_net, connect_names);
	if (err != 0)
		return err;

	deleteNetwork(pipe_net);
	deleteNetwork(river_net);

	return disconnect_and_return(0);
}