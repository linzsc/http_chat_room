#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "include/sha1.h"  // 包含你提供的 SHA-1 实现
#include "include/base64.h"  // 包含你提供的 Base64 编码实现

// WebSocket GUID
static const char *ws_guid = "258EAFA5-69A2-4FC6-A914-279E4C974253";

// 计算 Sec-WebSocket-Accept
char *compute_sec_websocket_accept(const char *sec_websocket_key) {
    // 创建一个临时缓冲区，用于拼接 Sec-WebSocket-Key 和 GUID
    char key_with_guid[256];
    snprintf(key_with_guid, sizeof(key_with_guid), "%s%s", sec_websocket_key, ws_guid);

    // 计算 SHA-1 哈希
    unsigned char hash[20];
    SHA1::SHA1HashBytes((const unsigned char *)key_with_guid, strlen(key_with_guid), hash);
    std::cout<<"SHA1 hash: "<<hash<<std::endl;
    // 编码为 Base64
    char encoded[100];  // Base64 编码后的字符串长度会增加
    Base64encode(encoded, (const char *)hash, 20);

    return strdup(encoded);  // 返回动态分配的字符串，调用者负责释放
}

int main(int argc, char **argv) {
    // 示例：客户端提供的 Sec-WebSocket-Key
    //const char *sec_websocket_key = "G0qsTTVgel/mQ0tjqqdmKQ==";
    const char *sec_websocket_key ;
    if(argc>1){
        sec_websocket_key = argv[1];
    }
   
    char *sec_websocket_accept = compute_sec_websocket_accept(sec_websocket_key);

    // 输出结果
    printf("Sec-WebSocket-Accept: %s\n", sec_websocket_accept);

    // 释放动态分配的内存
    free(sec_websocket_accept);

    return 0;
}