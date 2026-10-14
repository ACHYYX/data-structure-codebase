#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

//顺序表默认初始容量
#define SL_DEFAULT_CAPACITY 4

typedef int SLDataType;

typedef struct SeqList
{
	SLDataType* arr;   //动态数组
	int size;          //有效元素个数
	int capacity;      //当前容量
}SL;

//初始化顺序表
void SLInit(SL* ps);

//销毁顺序表
void SLDestroy(SL* ps);

//返回顺序表中第i个下标位置元素的值
SLDataType GetElem(const SL* ps, int i);

//查找指定元素的下标，找不到返回-1
int LocateElem(const SL* ps, SLDataType x);

//在顺序表的第i个位置插入元素x
void SLInsert(SL* ps, int i, SLDataType x);

//删除顺序表中的第i个元素，返回被删除的值
SLDataType SLDelete(SL* ps, int i);

//打印顺序表中的元素
void SLPrint(const SL* ps);

//判断顺序表是否为空，空则返回true，否则返回false
bool SLEmpty(const SL* ps);

//获取顺序表中有效元素的个数
int SLSize(const SL* ps);

//头插元素x
void SLPushFront(SL* ps, SLDataType x);

//尾插元素x
void SLPushBack(SL* ps, SLDataType x);

//头删
void SLPopFront(SL* ps);

//尾删
void SLPopBack(SL* ps);
