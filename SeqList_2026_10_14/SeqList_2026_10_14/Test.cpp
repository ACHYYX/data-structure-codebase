#define _CRT_SECURE_NO_WARNINGS

#include "SeqList.h"

//MSVC下用来自动检测内存泄漏
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

//==================== 极简测试框架 ====================

//统计测试项的通过/失败数量
static int g_pass = 0;
static int g_fail = 0;

//CHECK(表达式)：表达式成立记一次通过，否则打印行号和表达式本身
#define CHECK(expr)                                                     \
	do {                                                                \
		if (expr)                                                       \
		{                                                               \
			g_pass++;                                                   \
		}                                                               \
		else                                                            \
		{                                                               \
			g_fail++;                                                   \
			printf("  [失败] 第 %d 行: %s\n", __LINE__, #expr);         \
		}                                                               \
	} while (0)

//CHECK_ARR(ps, 期望值列表)：校验顺序表的内容和长度与期望序列完全一致
#define CHECK_ARR(ps, ...)                                              \
	do {                                                                \
		SLDataType expect[] = { __VA_ARGS__ };                          \
		int n = (int)(sizeof(expect) / sizeof(expect[0]));              \
		int actual = SLSize(ps);                                        \
		bool ok = (actual == n);                                        \
		for (int k = 0; k < n && ok; k++)                               \
		{                                                               \
			if (GetElem(ps, k) != expect[k])                            \
			{                                                           \
				ok = false;                                             \
			}                                                           \
		}                                                               \
		if (ok)                                                         \
		{                                                               \
			g_pass++;                                                   \
		}                                                               \
		else                                                            \
		{                                                               \
			g_fail++;                                                   \
			printf("  [失败] 第 %d 行: 期望 [", __LINE__);              \
			for (int k = 0; k < n; k++)                                 \
			{                                                           \
				printf("%d%s", expect[k], (k + 1 < n) ? "," : "");      \
			}                                                           \
			printf("]  实际 [");                                        \
			for (int k = 0; k < actual; k++)                            \
			{                                                           \
				printf("%d%s", GetElem(ps, k), (k + 1 < actual) ? "," : ""); \
			}                                                           \
			printf("]  长度 期望 %d 实际 %d\n", n, actual);             \
		}                                                               \
	} while (0)

//打印用例标题
#define CASE_TITLE(name) printf("\n===== 用例: %s =====\n", name)

//越界入参的用例默认关闭：这些用例在Debug配置下会触发assert中断，
//想手动验证时把下面的 0 改成 1 再运行
#define RUN_ILLEGAL_CASES 0

//MSVC下开启内存泄漏检测，程序正常退出时会打印泄漏信息
static void EnableLeakCheck(void)
{
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#endif
}

//==================== 各接口的测试用例 ====================

//用例1：初始化、判空、取长度
static void Test_Init(void)
{
	CASE_TITLE("SLInit / SLEmpty / SLSize");
	SL s;
	SLInit(&s);

	CHECK(s.arr != NULL);                      //动态数组已分配
	CHECK(s.size == 0);                        //初始没有有效元素
	CHECK(s.capacity == SL_DEFAULT_CAPACITY);  //初始容量
	CHECK(SLEmpty(&s));                        //空表返回true
	CHECK(SLSize(&s) == 0);                    //长度为0
	CHECK(!SLEmpty(&s) == false);              //取反验证一下判空逻辑

	SLDestroy(&s);
}

//用例2：尾插
static void Test_PushBack(void)
{
	CASE_TITLE("SLPushBack 尾插");
	SL s;
	SLInit(&s);

	for (int i = 1; i <= 5; i++)
	{
		SLPushBack(&s, i);
	}

	CHECK_ARR(&s, 1, 2, 3, 4, 5);         //顺序与插入顺序一致
	CHECK(SLSize(&s) == 5);
	CHECK(!SLEmpty(&s));
	CHECK(LocateElem(&s, 1) == 0);        //首元素下标为0
	CHECK(LocateElem(&s, 5) == 4);        //尾元素下标为4

	SLDestroy(&s);
}

//用例3：头插（检验插入时元素是否正确后移）
static void Test_PushFront(void)
{
	CASE_TITLE("SLPushFront 头插");
	SL s;
	SLInit(&s);

	for (int i = 1; i <= 5; i++)
	{
		SLPushFront(&s, i);
	}

	CHECK_ARR(&s, 5, 4, 3, 2, 1);         //头插结果是逆序
	CHECK(SLSize(&s) == 5);
	CHECK(LocateElem(&s, 5) == 0);
	CHECK(LocateElem(&s, 1) == 4);

	SLDestroy(&s);
}

//用例4：任意位置插入（头、中、尾三个位置）
static void Test_Insert(void)
{
	CASE_TITLE("SLInsert 头/中/尾位置插入");
	SL s;
	SLInit(&s);

	for (int i = 1; i <= 5; i++)
	{
		SLPushBack(&s, i);                //1 2 3 4 5
	}

	SLInsert(&s, 0, 100);                 //头插
	CHECK_ARR(&s, 100, 1, 2, 3, 4, 5);

	SLInsert(&s, 3, 200);                 //中间插入
	CHECK_ARR(&s, 100, 1, 2, 200, 3, 4, 5);

	SLInsert(&s, SLSize(&s), 300);        //尾插：i等于size的边界情况
	CHECK_ARR(&s, 100, 1, 2, 200, 3, 4, 5, 300);

	CHECK(SLSize(&s) == 8);   //5个原有元素 + 插入3个
	CHECK(GetElem(&s, 3) == 200);

	SLDestroy(&s);
}

//用例5：扩容（插入个数超过初始容量）
static void Test_Expand(void)
{
	CASE_TITLE("SLInsert 扩容");
	SL s;
	SLInit(&s);

	int oldCapacity = s.capacity;
	for (int i = 0; i < 50; i++)
	{
		SLPushBack(&s, i);                //50个元素，会触发多次扩容
	}

	CHECK(SLSize(&s) == 50);
	CHECK(s.capacity > oldCapacity);      //容量确实增长了
	CHECK(s.capacity >= 50);              //容量足够放下所有元素

	for (int i = 0; i < 50; i++)          //扩容后数据不能丢、顺序不能乱
	{
		CHECK(GetElem(&s, i) == i);
	}

	SLInsert(&s, 25, 999);                //扩容后再做中间插入
	CHECK(GetElem(&s, 25) == 999);
	CHECK(GetElem(&s, 24) == 24);
	CHECK(GetElem(&s, 26) == 25);
	CHECK(SLSize(&s) == 51);

	SLPushFront(&s, -1);                  //扩容后再做头插
	CHECK(GetElem(&s, 0) == -1);
	CHECK(GetElem(&s, 1) == 0);
	CHECK(SLSize(&s) == 52);

	SLDestroy(&s);
}

//用例6：任意位置删除（首、中、尾）
static void Test_Delete(void)
{
	CASE_TITLE("SLDelete 删首/中/尾 + 返回值");
	SL s;
	SLInit(&s);

	for (int i = 1; i <= 5; i++)
	{
		SLPushBack(&s, i);                //1 2 3 4 5
	}

	CHECK(SLDelete(&s, 0) == 1);          //删首，返回被删的值
	CHECK_ARR(&s, 2, 3, 4, 5);

	CHECK(SLDelete(&s, 1) == 3);          //删中间
	CHECK_ARR(&s, 2, 4, 5);

	CHECK(SLDelete(&s, SLSize(&s) - 1) == 5);   //删尾
	CHECK_ARR(&s, 2, 4);
	CHECK(SLSize(&s) == 2);

	CHECK(SLDelete(&s, 0) == 2);          //删到空表
	CHECK(SLDelete(&s, 0) == 4);
	CHECK(SLSize(&s) == 0);
	CHECK(SLEmpty(&s));

	SLDestroy(&s);
}

//用例7：头删与尾删（含删到空表、空表再删）
static void Test_PopFrontBack(void)
{
	CASE_TITLE("SLPopFront / SLPopBack");
	SL s;
	SLInit(&s);

	for (int i = 1; i <= 5; i++)
	{
		SLPushBack(&s, i);                //1 2 3 4 5
	}

	SLPopFront(&s);                       //头删
	CHECK_ARR(&s, 2, 3, 4, 5);
	CHECK(s.arr[0] == 2);

	SLPopBack(&s);                        //尾删
	CHECK_ARR(&s, 2, 3, 4);

	while (!SLEmpty(&s))                  //连续尾删到空表
	{
		SLPopBack(&s);
	}
	CHECK(SLSize(&s) == 0);

	//空表再删：不应该崩，size也不能被改成负数
	SLPopBack(&s);
	SLPopFront(&s);
	SLPopBack(&s);
	CHECK(SLSize(&s) == 0);
	CHECK(s.size == 0);
	CHECK(SLEmpty(&s));

	SLPopFront(&s);                       //从头插尾删交替验证一次
	SLPushFront(&s, 7);
	CHECK_ARR(&s, 7);
	SLPopBack(&s);
	CHECK(SLEmpty(&s));

	SLDestroy(&s);
}

//用例8：按下标取元素、按值查找下标
static void Test_GetElem_LocateElem(void)
{
	CASE_TITLE("GetElem / LocateElem");
	SL s;
	SLInit(&s);

	for (int i = 1; i <= 5; i++)
	{
		SLPushBack(&s, i * 10);           //10 20 30 40 50
	}

	CHECK(GetElem(&s, 0) == 10);          //按下标取值
	CHECK(GetElem(&s, 2) == 30);
	CHECK(GetElem(&s, 4) == 50);

	CHECK(LocateElem(&s, 10) == 0);       //按值查下标
	CHECK(LocateElem(&s, 30) == 2);
	CHECK(LocateElem(&s, 50) == 4);
	CHECK(LocateElem(&s, 999) == -1);     //找不到返回-1

	SLPushBack(&s, 30);                   //有重复元素时返回第一个下标
	CHECK(LocateElem(&s, 30) == 2);

	SLDestroy(&s);
}

//用例9：打印
static void Test_Print(void)
{
	CASE_TITLE("SLPrint");
	SL s;
	SLInit(&s);

	printf("  空表打印(应为空行): ");
	SLPrint(&s);

	for (int i = 1; i <= 5; i++)
	{
		SLPushBack(&s, i);
	}
	printf("  非空打印(应为 1 2 3 4 5): ");
	SLPrint(&s);

	CHECK(SLSize(&s) == 5);               //打印不应影响数据

	SLDestroy(&s);
}

//用例10：销毁
static void Test_Destroy(void)
{
	CASE_TITLE("SLDestroy");
	SL s;
	SLInit(&s);
	SLPushBack(&s, 1);
	SLDestroy(&s);

	CHECK(s.arr == NULL);                 //数组指针已置空
	CHECK(s.size == 0);                   //长度已归零
	CHECK(s.capacity == 0);               //容量已归零
	CHECK(SLSize(&s) == 0);               //销毁后不会读到旧的长度
	CHECK(SLEmpty(&s));

	SLDestroy(&s);                        //重复销毁（free(NULL)是安全的）
	CHECK(s.arr == NULL);
}

#if RUN_ILLEGAL_CASES
//用例11：非法入参（Debug下会触发assert中断，仅手动验证时开启）
static void Test_Illegal(void)
{
	CASE_TITLE("非法入参（会触发assert）");
	SL s;
	SLInit(&s);
	for (int i = 1; i <= 3; i++)
	{
		SLPushBack(&s, i);
	}

	SLInsert(&s, -1, 0);                  //i为负数
	SLInsert(&s, 999, 0);                 //i超过size
	GetElem(&s, 3);                       //下标越界
	SLDelete(&s, 3);                      //下标越界

	SLDestroy(&s);
}
#endif

//==================== 测试入口 ====================

int main(void)
{
	EnableLeakCheck();

	printf("========== 顺序表接口测试开始 ==========\n");

	Test_Init();
	Test_PushBack();
	Test_PushFront();
	Test_Insert();
	Test_Expand();
	Test_Delete();
	Test_PopFrontBack();
	Test_GetElem_LocateElem();
	Test_Print();
	Test_Destroy();

#if RUN_ILLEGAL_CASES
	Test_Illegal();
#endif

	printf("\n========== 测试结束: 通过 %d 项, 失败 %d 项 ==========\n", g_pass, g_fail);
	if (g_fail == 0)
	{
		printf("全部通过\n");
	}
	else
	{
		printf("存在失败项，请看上面的 [失败] 输出\n");
	}

	return g_fail == 0 ? 0 : 1;
}
