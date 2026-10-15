// =============================================================================
// SeqListMenu.cpp
// 顺序表菜单程序（控制台交互），支持增、删、查、改
//
// 依赖：SeqList.h / SeqList.cpp 中提供的顺序表接口
//      （增：SLPushBack / SLPushFront / SLInsert
//        删：SLPopBack / SLPopFront / SLDelete
//        查：GetElem / LocateElem
//        改：SLModify
//        其它：SLInit / SLDestroy / SLPrint / SLEmpty / SLSize）
//
// 运行方式：
//   1) 在 Visual Studio 中把 SeqListMenu 设为启动项目，然后按 Ctrl+F5；
//   2) 或直接运行生成的可执行文件 SeqListMenu.exe。
//
// 设计要点：
//   * 只使用顺序表的公开接口，不直接访问 ps->arr（保持封装）；
//   * 所有位置参数都先在菜单层做范围检查，越界只提示、不调用接口，
//     这样就不会触发顺序表内部的 assert，程序不会崩；
//   * scanf 的返回值必须检查并清空输入缓冲区，否则输入字母会导致死循环。
// =============================================================================

#define _CRT_SECURE_NO_WARNINGS

#include "SeqList.h"

// -----------------------------------------------------------------------------
// 辅助函数
// -----------------------------------------------------------------------------

// 打印操作菜单
static void PrintMenu(void)
{
	printf("\n========== 顺序表管理系统 ==========\n");
	printf("  1. 增加元素（尾插）\n");
	printf("  2. 增加元素（头插）\n");
	printf("  3. 增加元素（指定位置插入）\n");
	printf("  4. 删除元素（尾删）\n");
	printf("  5. 删除元素（头删）\n");
	printf("  6. 删除元素（指定位置）\n");
	printf("  7. 删除元素（按值删除）\n");
	printf("  8. 查找元素（按位置取值）\n");
	printf("  9. 查找元素（按值查下标）\n");
	printf(" 10. 修改元素（按位置）\n");
	printf(" 11. 打印顺序表\n");
	printf(" 12. 显示长度 / 是否为空\n");
	printf("  0. 退出\n");
	printf("------------------------------------\n");
}

// 清空输入缓冲区中剩余的字符。
// scanf 读到非法字符后会把它留在缓冲区里，不清掉就会一直读到同一个错误输入。
static void ClearInputBuffer(void)
{
	int c;
	while ((c = getchar()) != '\n' && c != EOF)
	{
		/* 逐个丢弃 */
	}
}

// 读入一个整数：成功返回 1；输入不是整数时返回 0（并清空缓冲区）
static int ReadInt(const char* prompt, int* out)
{
	printf("%s", prompt);

	if (scanf("%d", out) != 1)
	{
		ClearInputBuffer();
		return 0;
	}

	ClearInputBuffer();     // 吃掉本行多余的字符，避免影响下一次读取
	return 1;
}

// 读入一个"合法位置"，要求落在 [low, high] 区间内。
// 越界或输入非法时打印提示并返回 0，调用方直接跳过本次操作即可。
static int ReadPosition(const char* prompt, int low, int high, int* out)
{
	int value;

	if (!ReadInt(prompt, &value))
	{
		printf(">> 输入不合法，请输入一个整数\n");
		return 0;
	}

	if (value < low || value > high)
	{
		printf(">> 位置非法：合法范围是 [%d, %d]\n", low, high);
		return 0;
	}

	*out = value;
	return 1;
}

// -----------------------------------------------------------------------------
// 主程序
// -----------------------------------------------------------------------------
int main(void)
{
	SL list;
	SLInit(&list);

	printf("欢迎使用顺序表管理系统（元素类型：int）\n");

	for (;;)
	{
		// 每一步都先把当前内容显示出来，操作反馈更直观
		printf("\n当前顺序表：");
		SLPrint(&list);

		PrintMenu();

		int choice = 0;
		if (!ReadInt("请选择: ", &choice))
		{
			printf(">> 输入不合法，请输入菜单编号\n");
			continue;
		}

		if (choice == 0)
		{
			break;                                  // 退出
		}

		int value = 0;                              // 用于临时存放元素值
		int index = 0;                              // 用于临时存放下标

		switch (choice)
		{
		// ---------------------------------------------------------- 增
		case 1:                                     // 尾插
			if (!ReadInt("请输入要尾插的元素值: ", &value))
			{
				printf(">> 输入不合法\n");
				break;
			}
			SLPushBack(&list, value);
			printf(">> 尾插成功：%d\n", value);
			break;

		case 2:                                     // 头插
			if (!ReadInt("请输入要头插的元素值: ", &value))
			{
				printf(">> 输入不合法\n");
				break;
			}
			SLPushFront(&list, value);
			printf(">> 头插成功：%d\n", value);
			break;

		case 3:                                     // 指定位置插入
			// 插入位置的合法范围是 [0, size]：插到 size 处等价于尾插
			if (!ReadPosition("请输入插入位置（合法范围 0 ~ 表长）: ", 0, SLSize(&list), &index))
			{
				break;
			}
			if (!ReadInt("请输入要插入的元素值: ", &value))
			{
				printf(">> 输入不合法\n");
				break;
			}
			SLInsert(&list, index, value);
			printf(">> 在下标 %d 处插入成功：%d\n", index, value);
			break;

		// ---------------------------------------------------------- 删
		case 4:                                     // 尾删
			if (SLEmpty(&list))
			{
				printf(">> 顺序表已空，无法尾删\n");
				break;
			}
			SLPopBack(&list);
			printf(">> 尾删成功\n");
			break;

		case 5:                                     // 头删
			if (SLEmpty(&list))
			{
				printf(">> 顺序表已空，无法头删\n");
				break;
			}
			SLPopFront(&list);
			printf(">> 头删成功\n");
			break;

		case 6:                                     // 指定位置删除
			if (SLEmpty(&list))
			{
				printf(">> 顺序表已空，没有元素可删除\n");
				break;
			}
			// 删除位置的合法范围是 [0, size-1]
			if (!ReadPosition("请输入要删除的位置: ", 0, SLSize(&list) - 1, &index))
			{
				break;
			}
			printf(">> 删除成功：被删除的元素是 %d\n", SLDelete(&list, index));
			break;

		case 7:                                     // 按值删除（删第一个匹配的）
			if (SLEmpty(&list))
			{
				printf(">> 顺序表已空，没有元素可删除\n");
				break;
			}
			if (!ReadInt("请输入要删除的元素值: ", &value))
			{
				printf(">> 输入不合法\n");
				break;
			}
			index = LocateElem(&list, value);       // 先查下标
			if (index < 0)
			{
				printf(">> 没有找到值为 %d 的元素\n", value);
				break;
			}
			SLDelete(&list, index);
			printf(">> 已删除下标 %d 处的元素 %d\n", index, value);
			break;

		// ---------------------------------------------------------- 查
		case 8:                                     // 按位置取值
			if (SLEmpty(&list))
			{
				printf(">> 顺序表为空，没有元素可查\n");
				break;
			}
			if (!ReadPosition("请输入要查找的位置: ", 0, SLSize(&list) - 1, &index))
			{
				break;
			}
			printf(">> 下标 %d 处的元素是 %d\n", index, GetElem(&list, index));
			break;

		case 9:                                     // 按值查下标
			if (!ReadInt("请输入要查找的元素值: ", &value))
			{
				printf(">> 输入不合法\n");
				break;
			}
			index = LocateElem(&list, value);
			if (index < 0)
			{
				printf(">> 没有找到值为 %d 的元素\n", value);
			}
			else
			{
				printf(">> 值为 %d 的元素第一次出现在下标 %d\n", value, index);
			}
			break;

		// ---------------------------------------------------------- 改
		case 10:                                    // 按位置修改
			if (SLEmpty(&list))
			{
				printf(">> 顺序表为空，没有元素可修改\n");
				break;
			}
			// 修改位置的合法范围是 [0, size-1]
			if (!ReadPosition("请输入要修改的位置: ", 0, SLSize(&list) - 1, &index))
			{
				break;
			}
			if (!ReadInt("请输入新的元素值: ", &value))
			{
				printf(">> 输入不合法\n");
				break;
			}
			{
				const int oldValue = GetElem(&list, index);     // 先记下旧值，便于打印出"由谁变成谁"
				SLModify(&list, index, value);
				printf(">> 修改成功：下标 %d 由 %d 变成 %d\n", index, oldValue, value);
			}
			break;

		// ---------------------------------------------------------- 其它
		case 11:                                    // 打印
			printf(">> 当前顺序表：");
			SLPrint(&list);
			break;

		case 12:                                    // 显示长度与空状态
			printf(">> 长度 = %d，是否为空 = %s\n",
				SLSize(&list), SLEmpty(&list) ? "是" : "否");
			break;

		default:
			printf(">> 无效的菜单编号：%d\n", choice);
			break;
		}
	}

	SLDestroy(&list);                               // 退出前释放动态数组，避免内存泄漏
	printf("\n已退出系统，内存已释放。\n");

	return 0;
}
