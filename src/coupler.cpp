/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.0.0
 * File       ：耦合接口
 * Date       ：03/23/2023
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define DLL_EXPORT
#endif

#include "couplefun.h"
#include "../interface/coupler.h"

DLL_API bool __stdcall createDEM(const char* dem_file, double x_bl, double y_bl,
	double x_tr, double y_tr, double cell_size, double min_elev, double max_elev)
{
	return create_dem(
		dem_file, x_bl, y_bl, x_tr, y_tr, cell_size, min_elev, max_elev);
}

DLL_API bool __stdcall onlineCouple(INetwork* inited_net, IFlood* inited_flood, 
	IGeoprocess* geo, double pv_threshold)
{
	return online_couple(inited_net, inited_flood, geo, pv_threshold);
}

DLL_API int __stdcall partialOnlineCouple(INetwork* net_copy, 
	const char* geo_file, const char* work_directory, const char* out_directory, 
	const char* setup_file, double pv_threshold)
{
	return partial_online_couple(net_copy, geo_file, work_directory, 
		out_directory, setup_file, pv_threshold);
}

DLL_API int __stdcall offlineCouple(const char* inp_file, 
	const char* work_directory, const char* out_directory, 
	const char* setup_file, double pv_threshold)
{
	return offline_couple(inp_file, work_directory, out_directory,
		setup_file, pv_threshold);
}