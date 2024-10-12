/*
 *******************************************************************************
 * Project    ：Coupler
 * Version    ：1.2.0
 * File       ：账号信息
 * Date       ：10/11/2024
 * Author     ：梁小光
 * Copyright  ：福州水字节科技有限公司
 *******************************************************************************
 */

#pragma once

constexpr const char* user_id   = "szj000006";
constexpr const char* user_name = "Mr.Liang";

#if _WIN32
constexpr const char* sdk_path  = "D:/sdk_v1.7.0/";
constexpr const char* data_path = "D:/sdk_v1.7.0/data/coupler";
constexpr const char* out_path  = "D:/sdk_v1.7.0/data/coupler/out";
constexpr const char* key_path  = "D:/sdk_v1.7.0/key/szj000006_PublicKey.key";
#else  
constexpr const char* sdk_path  = "/mnt/d/sdk_v1.7.0/";
constexpr const char* data_path = "/mnt/d/sdk_v1.7.0/data/coupler";
constexpr const char* out_path  = "/mnt/d/sdk_v1.7.0/data/coupler/out";
constexpr const char* key_path  = "/mnt/d/sdk_v1.7.0/key/szj000006_PublicKey.key";
#endif