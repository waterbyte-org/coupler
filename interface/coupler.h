/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.1.0
 * File       ：耦合接口
 * Date       ：09/19/2023
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#ifdef DLL_EXPORT
#define DLL_API _declspec(dllexport) 
#else                                                                           
#define DLL_API _declspec(dllimport) 
#endif 
#else
#define __stdcall
#define DLL_API __attribute__((visibility("default")))
#endif

struct IFlood;
struct INetwork;
struct IGeoprocess;

#ifdef __cplusplus
extern "C" {
#endif

	DLL_API bool __stdcall createDEM(const char* dem_file, double x_bl, 
		double y_bl, double x_tr, double y_tr, double cell_size, 
		double min_elev, double max_elev);

	DLL_API bool __stdcall onlineCouple(INetwork* inited_net, 
		IFlood* inited_flood, IGeoprocess* geo);

	DLL_API int __stdcall partialOnlineCouple(INetwork* net_copy, 
		const char* geo_file, const char* work_directory, 
		const char* out_directory, const char* setup_file);

	DLL_API int __stdcall offlineCouple(const char* inp_file, 
		const char* work_directory, const char* out_directory, 
		const char* setup_file);

#ifdef __cplusplus
}
#endif

