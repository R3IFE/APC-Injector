//config.h - configuration variables + save/load
#pragma once

#include <string>
#include <fstream>

// ============================================================
// CONFIG VARIABLES
// ============================================================

// Manual wallhack identification (slider-based)
int countstride1 = -1;
int countstride2 = -1;
int countstride3 = -1;
int countstride4 = -1;
int countstride5 = -1;
int countExIStride1 = -1;
int countExIStride2 = -1;
int countExIStride3 = -1;
int countExIStride4 = -1;
int countGRootConstantBuffer = -2;
int countGRootDescriptor = -2;
int countIndexCount = -1;
int countfindrendertarget = -1;

bool enabletemporaryids = false;
int countcurrentRootSigID = -1;
int countcurrentRootSigID2 = -1;
int countcurrentIndexAddress = -1;
int countcurrentIndexAddress2 = -1;
int countcurrentIndexAddress3 = -1;
int countcurrentPSOAddress = -1;

bool enablefilters = false;
bool filterrendertarget = false;
int countfilterrendertarget = -1;
int countfilterrendertarget2 = -1;
bool filterGRootConstantBuffer = false;
int countfilterGRootConstantBuffer = -2;
int countfilterGRootConstantBuffer2 = -2;
int countfilterGRootConstantBuffer3 = -2;
bool filterGRootDescriptor = false;
int countfilterGRootDescriptor = -2;
int countfilterGRootDescriptor2 = -2;
int countfilterGRootDescriptor3 = -2;
bool filterindexformat = false;
int countfilterindexformat = -1;
bool filternumViews = false;
int countfilternumViews = -1;
bool filterIndexCountPerInstance = false;
int countfilterIndexCountPerInstance = -1;
int countfilterIndexCountPerInstance2 = -1;

bool enableignores = false;
bool ignorerendertarget = false;
int countignorerendertarget = -1;
bool ignoreGRootConstantBuffer = false;
int countignoreGRootConstantBuffer = -2;
int countignoreGRootConstantBuffer2 = -2;
int countignoreGRootConstantBuffer3 = -2;
bool ignoreGRootDescriptor = false;
int countignoreGRootDescriptor = -2;
int countignoreGRootDescriptor2 = -2;
int countignoreGRootDescriptor3 = -2;
bool ignorenumViews = false;
int countignorenumViews = -1;
bool ignoreIndexCountPerInstance = false;
int countignoreIndexCountPerInstance = -1;

bool reversedDepth = false;
bool DisableOcclusionCulling = true;
bool enablecolor = false;
int cri0 = 0, cri1 = 0, cri2 = 0, cri3 = 0, cri4 = 0;
int cri5 = 0, cri6 = 0, cri7 = 0, cri8 = 0, cri9 = 0, cri10 = 0;

bool ExecuteIndirectisCalled = 0;

// ============================================================
// SAVE / LOAD
// ============================================================

using namespace std;

void SaveConfig()
{
    ofstream fout("d3d12wh.ini", ios::trunc);

    fout << "countstride1 " << countstride1 << endl;
    fout << "countstride2 " << countstride2 << endl;
    fout << "countstride3 " << countstride3 << endl;
    fout << "countstride4 " << countstride4 << endl;
    fout << "countstride5 " << countstride5 << endl;
    fout << "exinstride1 " << countExIStride1 << endl;
    fout << "exinstride2 " << countExIStride2 << endl;
    fout << "exinstride3 " << countExIStride3 << endl;
    fout << "exinstride4 " << countExIStride4 << endl;
    fout << "countGRootConstantBuffer " << countGRootConstantBuffer << endl;
    fout << "countGRootDescriptor " << countGRootDescriptor << endl;
    fout << "countIndexCount " << countIndexCount << endl;
    fout << "countfindrendertarget " << countfindrendertarget << endl;
    fout << "enabletemporaryids " << enabletemporaryids << endl;
    fout << "countcurrentRootSigID " << countcurrentRootSigID << endl;
    fout << "countcurrentRootSigID2 " << countcurrentRootSigID2 << endl;
    fout << "countcurrentIndexAddress " << countcurrentIndexAddress << endl;
    fout << "countcurrentIndexAddress2 " << countcurrentIndexAddress2 << endl;
    fout << "countcurrentIndexAddress3 " << countcurrentIndexAddress3 << endl;
    fout << "countcurrentPSOAddress " << countcurrentPSOAddress << endl;
    fout << "enablefilters " << enablefilters << endl;
    fout << "filterrendertarget " << filterrendertarget << endl;
    fout << "countfilterrendertarget " << countfilterrendertarget << endl;
    fout << "countfilterrendertarget2 " << countfilterrendertarget2 << endl;
    fout << "filterGRootConstantBuffer " << filterGRootConstantBuffer << endl;
    fout << "countfilterGRootConstantBuffer " << countfilterGRootConstantBuffer << endl;
    fout << "countfilterGRootConstantBuffer2 " << countfilterGRootConstantBuffer2 << endl;
    fout << "countfilterGRootConstantBuffer3 " << countfilterGRootConstantBuffer3 << endl;
    fout << "filterGRootDescriptor " << filterGRootDescriptor << endl;
    fout << "countfilterGRootDescriptor " << countfilterGRootDescriptor << endl;
    fout << "countfilterGRootDescriptor2 " << countfilterGRootDescriptor2 << endl;
    fout << "countfilterGRootDescriptor3 " << countfilterGRootDescriptor3 << endl;
    fout << "filterindexformat " << filterindexformat << endl;
    fout << "countfilterindexformat " << countfilterindexformat << endl;
    fout << "filternumViews " << filternumViews << endl;
    fout << "countfilternumViews " << countfilternumViews << endl;
    fout << "filterIndexCountPerInstance " << filterIndexCountPerInstance << endl;
    fout << "countfilterIndexCountPerInstance " << countfilterIndexCountPerInstance << endl;
    fout << "countfilterIndexCountPerInstance2 " << countfilterIndexCountPerInstance2 << endl;
    fout << "enableignores " << enableignores << endl;
    fout << "ignorerendertarget " << ignorerendertarget << endl;
    fout << "countignorerendertarget " << countignorerendertarget << endl;
    fout << "ignoreGRootConstantBuffer " << ignoreGRootConstantBuffer << endl;
    fout << "countignoreGRootConstantBuffer " << countignoreGRootConstantBuffer << endl;
    fout << "countignoreGRootConstantBuffer2 " << countignoreGRootConstantBuffer2 << endl;
    fout << "countignoreGRootConstantBuffer3 " << countignoreGRootConstantBuffer3 << endl;
    fout << "ignoreGRootDescriptor " << ignoreGRootDescriptor << endl;
    fout << "countignoreGRootDescriptor " << countignoreGRootDescriptor << endl;
    fout << "countignoreGRootDescriptor2 " << countignoreGRootDescriptor2 << endl;
    fout << "countignoreGRootDescriptor3 " << countignoreGRootDescriptor3 << endl;
    fout << "ignorenumViews " << ignorenumViews << endl;
    fout << "countignorenumViews " << countignorenumViews << endl;
    fout << "ignoreIndexCountPerInstance " << ignoreIndexCountPerInstance << endl;
    fout << "countignoreIndexCountPerInstance " << countignoreIndexCountPerInstance << endl;
    fout << "reversedDepth " << reversedDepth << endl;
    fout << "DisableOcclusionCulling " << DisableOcclusionCulling << endl;
    fout << "enablecolor " << enablecolor << endl;
    fout << "coloroffset " << coloroffset << endl;
    fout << "cri0 " << cri0 << endl;
    fout << "cri1 " << cri1 << endl;
    fout << "cri2 " << cri2 << endl;
    fout << "cri3 " << cri3 << endl;
    fout << "cri4 " << cri4 << endl;
    fout << "cri5 " << cri5 << endl;
    fout << "cri6 " << cri6 << endl;
    fout << "cri7 " << cri7 << endl;
    fout << "cri8 " << cri8 << endl;
    fout << "cri9 " << cri9 << endl;
    fout << "cri10 " << cri10 << endl;
    fout << "g_asEnabled " << g_asEnabled << endl;
    fout << "g_asMinCmd " << g_asMinCmd << endl;
    fout << "g_asMaxCmd " << g_asMaxCmd << endl;
    fout << "g_asRequireCount " << g_asRequireCount << endl;
    fout << "g_asStride " << g_asStride << endl;
    fout << "g_asNumRTVs " << g_asNumRTVs << endl;
    fout << "g_asDrawType " << g_asDrawType << endl;
    fout << "g_asMinVPWidth " << g_asMinVPWidth << endl;
    fout << "g_asMaxVPWidth " << g_asMaxVPWidth << endl;
    fout << "g_asSkipInterval " << g_asSkipInterval << endl;
    fout << "g_rtWallhackOn " << g_rtWallhackOn << endl;
    fout << "g_rtHideMode " << g_rtHideMode << endl;
    fout << "g_rtTarget " << g_rtTarget << endl;
    fout << "g_rtFilterByTarget " << g_rtFilterByTarget << endl;
    fout << "g_rtTargetHash " << g_rtTargetHash << endl;
    fout << "g_rtFilterByHash " << g_rtFilterByHash << endl;

    fout.close();
}

void LoadConfig()
{
    ifstream fin("d3d12wh.ini", ios::in);

    string key;
    while (fin >> key)
    {
        if (key == "countstride1")                  fin >> countstride1;
        else if (key == "countstride2")             fin >> countstride2;
        else if (key == "countstride3")             fin >> countstride3;
        else if (key == "countstride4")             fin >> countstride4;
        else if (key == "countstride5")             fin >> countstride5;
        else if (key == "exinstride1")              fin >> countExIStride1;
        else if (key == "exinstride2")              fin >> countExIStride2;
        else if (key == "exinstride3")              fin >> countExIStride3;
        else if (key == "exinstride4")              fin >> countExIStride4;
        else if (key == "countGRootConstantBuffer")  fin >> countGRootConstantBuffer;
        else if (key == "countGRootDescriptor")      fin >> countGRootDescriptor;
        else if (key == "countIndexCount")           fin >> countIndexCount;
        else if (key == "countfindrendertarget")     fin >> countfindrendertarget;
        else if (key == "enabletemporaryids")       fin >> enabletemporaryids;
        else if (key == "countcurrentRootSigID")    fin >> countcurrentRootSigID;
        else if (key == "countcurrentRootSigID2")   fin >> countcurrentRootSigID2;
        else if (key == "countcurrentIndexAddress") fin >> countcurrentIndexAddress;
        else if (key == "countcurrentIndexAddress2")fin >> countcurrentIndexAddress2;
        else if (key == "countcurrentIndexAddress3")fin >> countcurrentIndexAddress3;
        else if (key == "countcurrentPSOAddress")   fin >> countcurrentPSOAddress;
        else if (key == "enablefilters")            fin >> enablefilters;
        else if (key == "filterrendertarget")       fin >> filterrendertarget;
        else if (key == "countfilterrendertarget")  fin >> countfilterrendertarget;
        else if (key == "countfilterrendertarget2") fin >> countfilterrendertarget2;
        else if (key == "filterGRootConstantBuffer")     fin >> filterGRootConstantBuffer;
        else if (key == "countfilterGRootConstantBuffer") fin >> countfilterGRootConstantBuffer;
        else if (key == "countfilterGRootConstantBuffer2")fin >> countfilterGRootConstantBuffer2;
        else if (key == "countfilterGRootConstantBuffer3")fin >> countfilterGRootConstantBuffer3;
        else if (key == "filterGRootDescriptor")     fin >> filterGRootDescriptor;
        else if (key == "countfilterGRootDescriptor") fin >> countfilterGRootDescriptor;
        else if (key == "countfilterGRootDescriptor2")fin >> countfilterGRootDescriptor2;
        else if (key == "countfilterGRootDescriptor3")fin >> countfilterGRootDescriptor3;
        else if (key == "filterindexformat")        fin >> filterindexformat;
        else if (key == "countfilterindexformat")   fin >> countfilterindexformat;
        else if (key == "filternumViews")           fin >> filternumViews;
        else if (key == "countfilternumViews")      fin >> countfilternumViews;
        else if (key == "filterIndexCountPerInstance")      fin >> filterIndexCountPerInstance;
        else if (key == "countfilterIndexCountPerInstance")  fin >> countfilterIndexCountPerInstance;
        else if (key == "countfilterIndexCountPerInstance2") fin >> countfilterIndexCountPerInstance2;
        else if (key == "enableignores")            fin >> enableignores;
        else if (key == "ignorerendertarget")       fin >> ignorerendertarget;
        else if (key == "countignorerendertarget")  fin >> countignorerendertarget;
        else if (key == "ignoreGRootConstantBuffer")     fin >> ignoreGRootConstantBuffer;
        else if (key == "countignoreGRootConstantBuffer") fin >> countignoreGRootConstantBuffer;
        else if (key == "countignoreGRootConstantBuffer2")fin >> countignoreGRootConstantBuffer2;
        else if (key == "countignoreGRootConstantBuffer3")fin >> countignoreGRootConstantBuffer3;
        else if (key == "ignoreGRootDescriptor")     fin >> ignoreGRootDescriptor;
        else if (key == "countignoreGRootDescriptor") fin >> countignoreGRootDescriptor;
        else if (key == "countignoreGRootDescriptor2")fin >> countignoreGRootDescriptor2;
        else if (key == "countignoreGRootDescriptor3")fin >> countignoreGRootDescriptor3;
        else if (key == "ignorenumViews")           fin >> ignorenumViews;
        else if (key == "countignorenumViews")      fin >> countignorenumViews;
        else if (key == "ignoreIndexCountPerInstance")      fin >> ignoreIndexCountPerInstance;
        else if (key == "countignoreIndexCountPerInstance")  fin >> countignoreIndexCountPerInstance;
        else if (key == "reversedDepth")            fin >> reversedDepth;
        else if (key == "DisableOcclusionCulling")  fin >> DisableOcclusionCulling;
        else if (key == "enablecolor")              fin >> enablecolor;
        else if (key == "coloroffset")              fin >> coloroffset;
        else if (key == "cri0")  fin >> cri0;
        else if (key == "cri1")  fin >> cri1;
        else if (key == "cri2")  fin >> cri2;
        else if (key == "cri3")  fin >> cri3;
        else if (key == "cri4")  fin >> cri4;
        else if (key == "cri5")  fin >> cri5;
        else if (key == "cri6")  fin >> cri6;
        else if (key == "cri7")  fin >> cri7;
        else if (key == "cri8")  fin >> cri8;
        else if (key == "cri9")  fin >> cri9;
        else if (key == "cri10") fin >> cri10;
        else if (key == "g_asEnabled") fin >> g_asEnabled;
        else if (key == "g_asMinCmd") fin >> g_asMinCmd;
        else if (key == "g_asMaxCmd") fin >> g_asMaxCmd;
        else if (key == "g_asRequireCount") fin >> g_asRequireCount;
        else if (key == "g_asStride") fin >> g_asStride;
        else if (key == "g_asNumRTVs") fin >> g_asNumRTVs;
        else if (key == "g_asDrawType") fin >> g_asDrawType;
        else if (key == "g_asMinVPWidth") fin >> g_asMinVPWidth;
        else if (key == "g_asMaxVPWidth") fin >> g_asMaxVPWidth;
        else if (key == "g_asSkipInterval") fin >> g_asSkipInterval;
        else if (key == "g_rtWallhackOn") fin >> g_rtWallhackOn;
        else if (key == "g_rtHideMode") fin >> g_rtHideMode;
        else if (key == "g_rtTarget") fin >> g_rtTarget;
        else if (key == "g_rtFilterByTarget") fin >> g_rtFilterByTarget;
        else if (key == "g_rtTargetHash") fin >> g_rtTargetHash;
        else if (key == "g_rtFilterByHash") fin >> g_rtFilterByHash;
        else if (key == "g_playerChams") { bool dummy; fin >> dummy; } // legacy, ignored
        else { string dummy; fin >> dummy; }
    }

    fin.close();
}
