#define _CRT_SECURE_NO_WARNINGS

#include "SeqList.h"

//初始化顺序表
void SLInit(SL* ps)
{
	assert(ps);

	SLDataType* tem = (SLDataType*)malloc(sizeof(SLDataType) * SL_DEFAULT_CAPACITY);
	if (tem == NULL)
	{
		perror("malloc");
		exit(1);
	}
	ps->arr = tem;
	ps->size = 0;
	ps->capacity = SL_DEFAULT_CAPACITY;
}

//销毁顺序表
void SLDestroy(SL* ps)
{
	assert(ps);

	free(ps->arr);
	ps->arr = NULL;
	ps->capacity = ps->size = 0;
}

//返回顺序表中第i个下标位置元素的值
SLDataType GetElem(const SL* ps, int i)
{
	assert(ps);
	assert(i >= 0 && i < ps->size);

	return ps->arr[i];
}

//查找指定元素的下标，找不到返回-1
int LocateElem(const SL* ps, SLDataType x)
{
	assert(ps);

	for (int i = 0; i < ps->size; i++)
	{
		if (ps->arr[i] == x)
		{
			return i;
		}
	}
	return -1;
}

//在顺序表的第i个位置插入元素x
void SLInsert(SL* ps, int i, SLDataType x)
{
	assert(ps);
	assert(i >= 0 && i <= ps->size);

	//Release版本中assert会失效，这里再兜底一次，避免越界写
	if (i < 0 || i > ps->size)
	{
		return;
	}

	if (ps->size == ps->capacity)
	{
		int newCapacity = ps->capacity ? ps->capacity * 2 : SL_DEFAULT_CAPACITY;
		SLDataType* tem = (SLDataType*)realloc(ps->arr, sizeof(SLDataType) * newCapacity);
		if (tem == NULL)
		{
			perror("realloc");
			exit(1);
		}
		ps->arr = tem;
		ps->capacity = newCapacity;
	}

	for (int j = ps->size; j > i; j--)
	{
		ps->arr[j] = ps->arr[j - 1];
	}
	ps->arr[i] = x;
	ps->size++;
}

//删除顺序表中的第i个元素，返回被删除的值
SLDataType SLDelete(SL* ps, int i)
{
	assert(ps);
	assert(i >= 0 && i < ps->size);

	//Release版本中assert会失效，这里再兜底一次，避免size被减成负数
	if (i < 0 || i >= ps->size)
	{
		return (SLDataType)0;
	}

	SLDataType ret = ps->arr[i];
	for (int j = i; j < ps->size - 1; j++)
	{
		ps->arr[j] = ps->arr[j + 1];
	}
	ps->size--;
	ps->arr[ps->size] = 0;   //清理残留值，方便调试

	return ret;
}
//把顺序表中第i个下标的元素修改为x
void SLModify(SL* ps, int i, SLDataType x)
{
	assert(ps);
	assert(i >= 0 && i < ps->size);

	//Release版本中assert会失效，这里再兜底一次，避免越界写
	if (i < 0 || i >= ps->size)
	{
		return;
	}

	ps->arr[i] = x;
}

//打印顺序表中的元素
void SLPrint(const SL* ps)
{
	assert(ps);

	for (int i = 0; i < ps->size; i++)
	{
		printf("%d ", ps->arr[i]);
	}
	printf("\n");
}

//判断顺序表是否为空，空则返回true，否则返回false
bool SLEmpty(const SL* ps)
{
	assert(ps);

	return ps->size == 0;
}

//获取顺序表中有效元素的个数
int SLSize(const SL* ps)
{
	assert(ps);

	return ps->size;
}

//头插元素x
void SLPushFront(SL* ps, SLDataType x)
{
	assert(ps);

	SLInsert(ps, 0, x);
}

//尾插元素x
void SLPushBack(SL* ps, SLDataType x)
{
	assert(ps);

	SLInsert(ps, ps->size, x);
}

//头删
void SLPopFront(SL* ps)
{
	assert(ps);

	//空表直接返回，Release下assert失效时也不会把size减成负数
	if (SLEmpty(ps))
	{
		return;
	}
	SLDelete(ps, 0);
}

//尾删
void SLPopBack(SL* ps)
{
	assert(ps);

	if (SLEmpty(ps))
	{
		return;
	}
	SLDelete(ps, ps->size - 1);
}
