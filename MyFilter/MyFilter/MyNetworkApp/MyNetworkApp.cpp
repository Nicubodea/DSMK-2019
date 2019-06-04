// MyNetworkApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//



#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")

#include <windows.h>
#include <fwpmu.h>
#include <stdio.h>

#pragma comment(lib, "Fwpuclnt.lib")
#include <iostream>
#include <vector>


#define FILE0_PATH L"C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe"

struct FILTRU_CU_ID
{
    FWPM_FILTER0* filtru;
    UINT64 id;
};

std::vector<FILTRU_CU_ID*> filtre;


void create_filter_for_host(int host, HANDLE engineHandle, FWP_BYTE_BLOB* app)
{
    FWPM_FILTER0 *newFilter;
    FILTRU_CU_ID* filtru;
    UINT64 id;
    FWPM_FILTER_CONDITION0 *newConditions;
    FWP_V4_ADDR_AND_MASK addrMask;

    DWORD noConditions = 2;

    DWORD result;

    addrMask.addr = host;
    addrMask.mask = 0xFFFFFFFF;
    
    newFilter = (FWPM_FILTER0 *)malloc(sizeof(FWPM_FILTER0));

    memset(newFilter, 0, sizeof(FWPM_FILTER0));

    newConditions = (FWPM_FILTER_CONDITION0*)malloc(sizeof(FWPM_FILTER_CONDITION0) * noConditions);

    newFilter->layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4;
    newFilter->action.type = FWP_ACTION_BLOCK;

    newFilter->displayData.name = (wchar_t*)L"UN FILTRU BUN TINE DOCTORU DEPARTE";

    newFilter->numFilterConditions = noConditions;

    newConditions[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
    newConditions[0].matchType = FWP_MATCH_EQUAL;
    newConditions[0].conditionValue.type = FWP_V4_ADDR_MASK;
    newConditions[0].conditionValue.v4AddrMask = &addrMask;

    newConditions[1].fieldKey = FWPM_CONDITION_ALE_APP_ID;
    newConditions[1].matchType = FWP_MATCH_EQUAL;
    newConditions[1].conditionValue.type = FWP_BYTE_BLOB_TYPE;
    newConditions[1].conditionValue.byteBlob = app;

    newFilter->filterCondition = newConditions;

    result = FwpmFilterAdd(engineHandle, newFilter, NULL, &id);

    if (result != ERROR_SUCCESS)
    {
        printf("FwpmFilterAdd0 failed. Return value: %d.\n", result);
        free(newConditions);
        free(newFilter);
    }
    else
    {
        printf("Filter added successfully.\n");
        filtru = (FILTRU_CU_ID*)malloc(sizeof(FILTRU_CU_ID));
        filtru->filtru = newFilter;
        filtru->id = id;
        filtre.push_back(filtru);
    }
}


int main()
{
    FWP_BYTE_BLOB *fwpApplicationByteBlob;
    int conCount = 0;
    DWORD result = ERROR_SUCCESS;
    HANDLE engineHandle = NULL;
    struct addrinfo *resultaddr = NULL;
    struct addrinfo hints;
    int errcode;
    const char* host = "www.aol.com";
    DWORD iResult;
    WSADATA wsaData;


    fwpApplicationByteBlob = (FWP_BYTE_BLOB*)malloc(sizeof(FWP_BYTE_BLOB));

    printf("Retrieving application identifier for filter testing.\n");
    result = FwpmGetAppIdFromFileName0(FILE0_PATH, &fwpApplicationByteBlob);
    if (result != ERROR_SUCCESS)
    {
        printf("FwpmGetAppIdFromFileName failed (%d).\n", result);
        return 0;
    }

    result = ERROR_SUCCESS;

    printf("Opening the filter engine.\n");

    result = FwpmEngineOpen0(
        NULL,
        RPC_C_AUTHN_WINNT,
        NULL,
        NULL,
        &engineHandle);

    if (result != ERROR_SUCCESS)
    {
        printf("FwpmEngineOpen0 failed. Return value: %d.\n", result);
    }
    else
    {
        printf("Filter engine opened successfully.\n");
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        goto cleanup_and_exit;
    }

    errcode = getaddrinfo(host, NULL, &hints, &resultaddr);

    if (errcode != 0)
    {
        printf("errcode: %d\n", errcode);
        goto cleanup_and_exit;
    }

    printf("Host: %s\n", host);
    while (resultaddr)
    {
        int adresa;

        switch (resultaddr->ai_family)
        {
        case AF_INET:
            adresa = ((struct sockaddr_in *) resultaddr->ai_addr)->sin_addr.S_un.S_addr;
            break;
        default:
        {
            goto cleanup_and_exit;
        }
        }

        create_filter_for_host(ntohl(adresa), engineHandle, fwpApplicationByteBlob);

        resultaddr = resultaddr->ai_next;
    }

    getchar();

cleanup_and_exit:
    for (unsigned int i = 0; i < filtre.size(); i++)
    {
        FwpmFilterDeleteById(engineHandle, filtre[i]->id);
        free(filtre[i]->filtru->filterCondition);
        free(filtre[i]->filtru);
        free(filtre[i]);
    }

    WSACleanup();

    FwpmEngineClose(engineHandle);
 
    return 0;
}