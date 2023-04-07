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

	// 2. ִ����ϼ���
	const string inp_path = string(data_path) + "/model.inp";
//	err_id = offline_couple(inp_path.c_str(), data_path, out_path, "setup.csv");
	err_id = offlineCouple(inp_path.c_str(), data_path, out_path, "setup.csv");
	if (err_id != 0)
		return disconnect_and_return(err_id);
	
	return disconnect_and_return(0);
}