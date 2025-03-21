#ifndef PACKET_H
#define PACKET_H
#include "stdint.h"
#include "octoTree.h"
#include "auxiliary_tool.h"
#include "MySystem.h"
#define MAPPING_REQUEST_PAYLOAD_LENGTH 1
#define AUTOFLY_PACKET_HEAD_LENGTH 4
#define AUTOFLY_PACKET_MUT 60 - AUTOFLY_PACKET_HEAD_LENGTH

#define BROADCAST_ID 0xFF

typedef enum
{
    // request
    MAPPING_REQ = 0x10, // mapping
    EXPLORE_REQ = 0x20, // explore
    PATH_REQ = 0x30,    // path
    CLUSTER_REQ = 0x40, // cluster

    TERMINATE = 0xFF, // terminate

    // response
    EXPLORE_RESP = 0x2A,
    PATH_RESP = 0x3A,
    CLUSTER_RESP = 0x4A,

    POSITION_BROADCAST = 0x50
} packetType_t; // 报文类型

typedef struct
{
    uint8_t len : 4;
    uint8_t mergedNums : 4;
    coordinate_t startPoint;
    coordinate_t endPoint[6];
} __attribute__((packed)) mapping_req_payload_t; // 6*7+1 = 43

typedef struct
{
    // uint8_t weights_100[6];
    uavRange_t uavRange;
} __attribute__((packed)) explore_req_payload_t; // 48

typedef struct
{
    coordinateF_t nextpoint;
} __attribute__((packed)) explore_resp_payload_t; // 12

typedef struct
{
    uint16_t seq;
    mapping_req_payload_t mappingRequestPayload[MAPPING_REQUEST_PAYLOAD_LENGTH];
} __attribute__((packed)) mapping_req_packet_t; // 43*MAPPING_REQUEST_PAYLOAD_LENGTH+2

typedef struct
{
    uint16_t seq;
    explore_req_payload_t exploreRequestPayload;
} __attribute__((packed)) explore_req_packet_t; // 48+2

typedef struct
{
    uint16_t seq;
    explore_resp_payload_t exploreResponsePayload;
} __attribute__((packed)) explore_resp_packet_t; // 12+2

typedef struct{
    coordinateF_t curPoint;
} __attribute__((packed)) position_broad_packet_t;

typedef struct
{
    uint8_t sourceId;
    uint8_t destinationId;
    uint8_t packetType;
    uint8_t length;
    uint8_t data[AUTOFLY_PACKET_MUT];
} __attribute__((packed)) Autofly_packet_t; // 60

#endif