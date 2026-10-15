// =============================================================================
// SeqListGTest.cpp
// 顺序表（SeqList）的 GoogleTest 单元测试
//
// 被测接口（SeqList.h 中声明的全部函数，共 13 个）：
//   SLInit      SLDestroy   GetElem      LocateElem   SLInsert     SLDelete
//   SLPrint     SLEmpty     SLSize       SLPushFront  SLPushBack
//   SLPopFront  SLPopBack
//
// 运行方式：
//   1) 在 Visual Studio 中打开"测试"->"测试资源管理器"，编译后即可看到并运行全部用例；
//   2) 也可以直接运行生成的可执行文件（GoogleTest 会把结果打印到控制台）。
//
// 说明：本工程使用 Visual Studio 自带的 GoogleTest 框架
//       （Test Adapter for Google Test + Microsoft.googletest NuGet 包），
//       不需要自己下载和编译 gtest 源码。
// =============================================================================

#include "pch.h"        // VS 的 GoogleTest 项目模板默认启用预编译头；pch.h 中已包含 gtest/gtest.h
#include "SeqList.h"    // 被测的顺序表接口

#include <string>
#include <vector>

#ifdef _MSC_VER
#include <crtdbg.h>     // MSVC 的内存泄漏检测（_CrtSetDbgFlag 等）
#endif

// -----------------------------------------------------------------------------
// 一、测试辅助工具
// -----------------------------------------------------------------------------

// 把顺序表中的有效元素拷贝成 std::vector，便于整体比较和打印。
static std::vector<SLDataType> ToVector(const SL* ps)
{
    return std::vector<SLDataType>(ps->arr, ps->arr + ps->size);
}

// 断言"顺序表的内容完全等于期望序列"，长度也必须一致。
// 用法：EXPECT_SEQ(&list_, 1, 2, 3);
// 之所以自己写这个宏，是因为本工程只引用了 gtest 这一个包（没有 GoogleMock），
// 用不了 EXPECT_THAT + ElementsAre 那套容器匹配器。
#define EXPECT_SEQ(ps, ...)                                                        \
    do {                                                                           \
        const std::vector<SLDataType> expect_ = { __VA_ARGS__ };                   \
        ASSERT_EQ(static_cast<size_t>(SLSize(ps)), expect_.size())                  \
            << "顺序表长度不符";                                                    \
        for (size_t i = 0; i < expect_.size(); ++i)                                \
        {                                                                          \
            EXPECT_EQ(GetElem(ps, static_cast<int>(i)), expect_[i])                 \
                << "第 " << i << " 个元素不符";                                      \
        }                                                                          \
    } while (0)

// -----------------------------------------------------------------------------
// 二、测试夹具（Test Fixture）
//     每个 TEST_F 用例执行前自动调用 SetUp 建表，执行后自动调用 TearDown 销毁，
//     保证每个用例都从"一个干净的空顺序表"开始，用例之间互不影响。
// -----------------------------------------------------------------------------
class SeqListTest : public ::testing::Test
{
protected:
    // 用例开始前的准备：初始化顺序表
    void SetUp() override
    {
        SLInit(&list_);
    }

    // 用例结束后的清理：销毁顺序表，释放动态数组（避免内存泄漏）
    void TearDown() override
    {
        SLDestroy(&list_);
    }

    // 便捷方法：把 1,2,...,n 依次尾插进去
    void FillOneToN(int n)
    {
        for (int i = 1; i <= n; ++i)
        {
            SLPushBack(&list_, i);
        }
    }

    SL list_{};     // 被测对象，在 SetUp 中被初始化
};

// 说明：内存泄漏检查没有采用"全局开启 _CRTDBG_LEAK_CHECK_DF"的做法，
//       因为那样会把 gtest 框架自身的长生命周期分配，以及死亡测试子进程
//       （它本来就是"故意崩溃"、不释放内存）一起算成泄漏，造成大量误报。
//       这里改用精确的"堆快照比较"方式，详见文末的 SeqListLeakTest 用例。

// =============================================================================
// 四、SLInit / SLDestroy：初始化与销毁
// =============================================================================

// 初始化后：动态数组已分配、没有有效元素、容量等于默认初始容量
TEST(SeqListInitDestroyTest, InitCreatesEmptyList)
{
    SL s;
    SLInit(&s);

    EXPECT_NE(s.arr, nullptr);                     // 动态数组已经分配出来
    EXPECT_EQ(SLSize(&s), 0);                      // 有效元素个数为 0
    EXPECT_EQ(s.capacity, SL_DEFAULT_CAPACITY);    // 容量等于默认初始容量
    EXPECT_TRUE(SLEmpty(&s));                      // 判定为空表

    SLDestroy(&s);
}

// 销毁后：指针置空、长度和容量归零，避免销毁后误用还能读到旧数据
TEST(SeqListInitDestroyTest, DestroyResetsAllFields)
{
    SL s;
    SLInit(&s);
    SLPushBack(&s, 1);      // 先放点数据，确认销毁确实会清掉
    SLDestroy(&s);

    EXPECT_EQ(s.arr, nullptr);
    EXPECT_EQ(s.size, 0);
    EXPECT_EQ(s.capacity, 0);
    EXPECT_EQ(SLSize(&s), 0);
    EXPECT_TRUE(SLEmpty(&s));

    SLDestroy(&s);          // 重复销毁也不应该崩溃（free(nullptr) 是安全的）
    EXPECT_EQ(s.arr, nullptr);
}

// =============================================================================
// 五、SLEmpty / SLSize：状态查询
// =============================================================================

// 空表 -> 非空 -> 再删回空表，状态应该正确地来回切换
TEST_F(SeqListTest, EmptyAndSizeFollowContent)
{
    EXPECT_TRUE(SLEmpty(&list_));       // 刚初始化，是空表
    EXPECT_EQ(SLSize(&list_), 0);

    SLPushBack(&list_, 10);
    EXPECT_FALSE(SLEmpty(&list_));      // 有元素了，不再是空表
    EXPECT_EQ(SLSize(&list_), 1);

    SLPopBack(&list_);
    EXPECT_TRUE(SLEmpty(&list_));       // 删空之后又是空表
    EXPECT_EQ(SLSize(&list_), 0);
}

// =============================================================================
// 六、SLPushBack：尾插
// =============================================================================

// 连续尾插，最终顺序应与插入顺序完全一致
TEST_F(SeqListTest, PushBackAppendsInOrder)
{
    FillOneToN(5);

    EXPECT_EQ(SLSize(&list_), 5);
    EXPECT_SEQ(&list_, 1, 2, 3, 4, 5);
}

// 尾插超过容量时应自动扩容，且原有数据不能丢、顺序不能乱
TEST_F(SeqListTest, PushBackGrowsCapacityWithoutLosingData)
{
    const int n = 100;                          // 远超默认容量 4，会触发多次扩容
    for (int i = 0; i < n; ++i)
    {
        SLPushBack(&list_, i);
    }

    EXPECT_EQ(SLSize(&list_), n);
    EXPECT_GE(list_.capacity, n);               // 容量必须足够装下所有元素

    std::vector<SLDataType> expect(n);
    for (int i = 0; i < n; ++i)
    {
        expect[i] = i;
    }
    EXPECT_EQ(ToVector(&list_), expect);        // 数据完整且顺序正确
}

// =============================================================================
// 七、SLPushFront：头插
// =============================================================================

// 头插是往前面塞，所以最终顺序与插入顺序相反
TEST_F(SeqListTest, PushFrontPrependsInReverseOrder)
{
    for (int i = 1; i <= 5; ++i)
    {
        SLPushFront(&list_, i);
    }

    EXPECT_SEQ(&list_, 5, 4, 3, 2, 1);
}

// 头插同样要能触发扩容，并且元素位置正确
TEST_F(SeqListTest, PushFrontGrowsCapacityAndKeepsOrder)
{
    const int n = 100;
    for (int i = 0; i < n; ++i)
    {
        SLPushFront(&list_, i);
    }

    EXPECT_EQ(SLSize(&list_), n);
    EXPECT_GE(list_.capacity, n);
    EXPECT_EQ(GetElem(&list_, 0), n - 1);       // 最后插入的在最前面
    EXPECT_EQ(GetElem(&list_, n - 1), 0);       // 最先插入的在最后面
}

// =============================================================================
// 八、SLInsert：任意位置插入
// =============================================================================

// 头、中、尾三个位置分别插入，验证插入后整体顺序都正确
TEST_F(SeqListTest, InsertAtHeadMiddleAndTail)
{
    FillOneToN(5);                              // 1 2 3 4 5

    SLInsert(&list_, 0, 100);                   // 在下标 0 插入
    EXPECT_SEQ(&list_, 100, 1, 2, 3, 4, 5);

    SLInsert(&list_, 3, 200);                   // 在中间插入
    EXPECT_SEQ(&list_, 100, 1, 2, 200, 3, 4, 5);

    SLInsert(&list_, SLSize(&list_), 300);      // 在下标 size 处插入（合法边界，等价于尾插）
    EXPECT_SEQ(&list_, 100, 1, 2, 200, 3, 4, 5, 300);
}

// 在下标 size 处插入，语义上应该等价于尾插
TEST_F(SeqListTest, InsertAtIndexEqualToSizeEqualsPushBack)
{
    FillOneToN(3);                              // 1 2 3

    SLInsert(&list_, 3, 4);

    EXPECT_SEQ(&list_, 1, 2, 3, 4);
}

// 往空表里插入
TEST_F(SeqListTest, InsertIntoEmptyList)
{
    SLInsert(&list_, 0, 42);

    EXPECT_EQ(SLSize(&list_), 1);
    EXPECT_EQ(GetElem(&list_, 0), 42);
}

// 每插入一次，长度应该增加 1
TEST_F(SeqListTest, InsertIncreasesSizeByOne)
{
    FillOneToN(5);
    SLInsert(&list_, 2, 99);
    EXPECT_EQ(SLSize(&list_), 6);
}

// 插入位置越靠前，需要后移的元素越多；这里是"反复在头部插入"的极端情况
TEST_F(SeqListTest, InsertAtHeadRepeatedly)
{
    for (int i = 1; i <= 20; ++i)
    {
        SLInsert(&list_, 0, i);
    }

    EXPECT_EQ(SLSize(&list_), 20);
    for (int i = 0; i < 20; ++i)
    {
        EXPECT_EQ(GetElem(&list_, i), 20 - i);  // 20,19,...,1
    }
}

// =============================================================================
// 九、SLDelete：按下标删除（同时校验返回值）
// =============================================================================

// 删首、删中、删尾：删完后后面的元素整体前移，返回值是被删掉的那个元素
TEST_F(SeqListTest, DeleteReturnsRemovedValueAndShiftsLeft)
{
    FillOneToN(5);                              // 1 2 3 4 5

    EXPECT_EQ(SLDelete(&list_, 0), 1);          // 删首
    EXPECT_SEQ(&list_, 2, 3, 4, 5);

    EXPECT_EQ(SLDelete(&list_, 1), 3);          // 删中间
    EXPECT_SEQ(&list_, 2, 4, 5);

    EXPECT_EQ(SLDelete(&list_, SLSize(&list_) - 1), 5);   // 删尾
    EXPECT_SEQ(&list_, 2, 4);
    EXPECT_EQ(SLSize(&list_), 2);
}

// 连续删除直到空表
TEST_F(SeqListTest, DeleteUntilEmpty)
{
    FillOneToN(2);                              // 1 2

    EXPECT_EQ(SLDelete(&list_, 0), 1);
    EXPECT_EQ(SLDelete(&list_, 0), 2);
    EXPECT_TRUE(SLEmpty(&list_));
    EXPECT_EQ(SLSize(&list_), 0);
}

// 删除后长度应该减少 1
TEST_F(SeqListTest, DeleteDecreasesSizeByOne)
{
    FillOneToN(5);
    SLDelete(&list_, 2);
    EXPECT_EQ(SLSize(&list_), 4);
}

// =============================================================================
// 十、SLPopFront / SLPopBack：头删 / 尾删
// =============================================================================

// 头删：去掉第一个元素
TEST_F(SeqListTest, PopFrontRemovesFirstElement)
{
    FillOneToN(4);                              // 1 2 3 4

    SLPopFront(&list_);

    EXPECT_SEQ(&list_, 2, 3, 4);
}

// 尾删：去掉最后一个元素
TEST_F(SeqListTest, PopBackRemovesLastElement)
{
    FillOneToN(4);                              // 1 2 3 4

    SLPopBack(&list_);

    EXPECT_SEQ(&list_, 1, 2, 3);
}

// 反复删除直到空表，长度必须精确归零
TEST_F(SeqListTest, PopUntilEmpty)
{
    FillOneToN(5);
    while (!SLEmpty(&list_))
    {
        SLPopBack(&list_);
    }
    EXPECT_EQ(SLSize(&list_), 0);

    FillOneToN(5);
    while (!SLEmpty(&list_))
    {
        SLPopFront(&list_);
    }
    EXPECT_EQ(SLSize(&list_), 0);
}

// 在空表上做删除：
//   Debug 配置下 assert 生效，会主动终止进程（由下面的死亡测试用例覆盖）；
//   Release 配置下 assert 失效，函数内部有兜底，必须保证程序不崩、长度不会被减成负数。
#ifdef NDEBUG
TEST_F(SeqListTest, PopOnEmptyListIsSafeInReleaseBuild)
{
    SLPopBack(&list_);          // 空表尾删
    SLPopFront(&list_);         // 空表头删
    SLPopBack(&list_);

    EXPECT_EQ(SLSize(&list_), 0);   // 长度必须还是 0，不能变成 -1
    EXPECT_EQ(list_.size, 0);
    EXPECT_TRUE(SLEmpty(&list_));
}
#endif

// 越界入参在 Release 构建（assert 失效）下，应该被函数内部的兜底逻辑安全拒绝：
// 不能越界写、不能把 size 改成负数、也不能崩溃。
// 这组用例与上面的死亡测试正好互补——Debug 下验"拦得住"（assert 终止进程），
// Release 下验"退得干净"（优雅返回且数据不变）。
#ifdef NDEBUG
TEST_F(SeqListTest, OutOfRangeInputIsRejectedSafely)
{
    FillOneToN(3);                                  // 1 2 3

    // ---- SLInsert 的越界兜底：i 为负数 / i 超出 size ----
    SLInsert(&list_, -1, 100);                      // 负数下标
    EXPECT_SEQ(&list_, 1, 2, 3);                    // 内容不受影响
    SLInsert(&list_, 99, 100);                      // 超过 size
    EXPECT_SEQ(&list_, 1, 2, 3);
    SLInsert(&list_, 3, 4);                         // 再来一个合法的，确认状态没被弄坏
    EXPECT_SEQ(&list_, 1, 2, 3, 4);

    // ---- SLDelete 的越界兜底：i 为负数 / i 超出 size ----
    EXPECT_EQ(SLDelete(&list_, -1), 0);             // 越界时返回 0
    EXPECT_SEQ(&list_, 1, 2, 3, 4);
    EXPECT_EQ(SLDelete(&list_, 99), 0);
    EXPECT_SEQ(&list_, 1, 2, 3, 4);
    EXPECT_EQ(SLDelete(&list_, SLSize(&list_)), 0); // i == size 同样属于越界
    EXPECT_SEQ(&list_, 1, 2, 3, 4);
    EXPECT_EQ(SLDelete(&list_, 0), 1);              // 合法删除仍然正常
    EXPECT_SEQ(&list_, 2, 3, 4);

    // ---- 空表上的越界删除 ----
    while (!SLEmpty(&list_))
    {
        SLPopBack(&list_);
    }
    EXPECT_EQ(SLDelete(&list_, 0), 0);
    EXPECT_EQ(SLDelete(&list_, -1), 0);
    EXPECT_EQ(SLSize(&list_), 0);                   // size 不能变成负数
    EXPECT_EQ(list_.size, 0);
    EXPECT_TRUE(SLEmpty(&list_));
    // ---- SLModify 的越界兜底：i 为负数 / i 超出 size ----
    SLModify(&list_, -1, 999);
    SLModify(&list_, 99, 999);
    EXPECT_EQ(SLSize(&list_), 0);                   // 空表不能被改出元素
}
#endif
// =============================================================================
// 十一、GetElem：按下标取值
// =============================================================================

TEST_F(SeqListTest, GetElemReturnsValueAtIndex)
{
    FillOneToN(5);                              // 1 2 3 4 5

    EXPECT_EQ(GetElem(&list_, 0), 1);           // 首元素
    EXPECT_EQ(GetElem(&list_, 2), 3);           // 中间元素
    EXPECT_EQ(GetElem(&list_, 4), 5);           // 尾元素
}

// =============================================================================
// 十二、SLModify：按位置修改元素
// =============================================================================

// 头、中、尾三个位置分别修改，校验改完的值正确、其它元素不受影响
TEST_F(SeqListTest, ModifyAtHeadMiddleAndTail)
{
    FillOneToN(5);                              // 1 2 3 4 5

    SLModify(&list_, 0, 100);                   // 改头
    EXPECT_SEQ(&list_, 100, 2, 3, 4, 5);

    SLModify(&list_, 2, 300);                   // 改中间
    EXPECT_SEQ(&list_, 100, 2, 300, 4, 5);

    SLModify(&list_, 4, 500);                   // 改尾
    EXPECT_SEQ(&list_, 100, 2, 300, 4, 500);
}

// 修改只是替换值，不应该改变长度
TEST_F(SeqListTest, ModifyDoesNotChangeSize)
{
    FillOneToN(5);

    SLModify(&list_, 1, 999);

    EXPECT_EQ(SLSize(&list_), 5);
}

// 改完之后按值查找应该查到新值、查不到旧值
TEST_F(SeqListTest, ModifyIsVisibleToLocateElem)
{
    FillOneToN(5);                              // 1 2 3 4 5

    SLModify(&list_, 2, 77);                    // 原来的 3 变成 77

    EXPECT_EQ(LocateElem(&list_, 77), 2);       // 新值能查到
    EXPECT_EQ(LocateElem(&list_, 3), -1);       // 旧值查不到了
}

// 改成和原来相同的值也不应该有问题
TEST_F(SeqListTest, ModifyWithSameValueIsIdempotent)
{
    FillOneToN(3);

    SLModify(&list_, 1, 2);

    EXPECT_SEQ(&list_, 1, 2, 3);
}
// =============================================================================
// 十三、LocateElem：按值查找下标
// =============================================================================

// 找得到：返回对应下标
TEST_F(SeqListTest, LocateElemFindsValue)
{
    FillOneToN(5);

    EXPECT_EQ(LocateElem(&list_, 1), 0);
    EXPECT_EQ(LocateElem(&list_, 3), 2);
    EXPECT_EQ(LocateElem(&list_, 5), 4);
}

// 找不到：返回 -1
TEST_F(SeqListTest, LocateElemReturnsMinusOneWhenNotFound)
{
    FillOneToN(5);

    EXPECT_EQ(LocateElem(&list_, 999), -1);
}

// 有重复元素时：返回第一个出现的下标
TEST_F(SeqListTest, LocateElemReturnsFirstMatchWhenDuplicated)
{
    SLPushBack(&list_, 7);
    SLPushBack(&list_, 7);
    SLPushBack(&list_, 7);

    EXPECT_EQ(LocateElem(&list_, 7), 0);
}

// 空表查找也应该安全地返回 -1
TEST_F(SeqListTest, LocateElemOnEmptyList)
{
    EXPECT_EQ(LocateElem(&list_, 1), -1);
}

// =============================================================================
// 十四、SLPrint：打印内容
//     gtest 提供了捕获标准输出的工具，可以把打印结果抓下来做精确比对，
//     这样连输出格式（元素后跟空格、结尾换行）也一起被测试覆盖了。
// =============================================================================

TEST_F(SeqListTest, PrintOutputsAllElements)
{
    FillOneToN(5);

    ::testing::internal::CaptureStdout();                              // 开始捕获标准输出
    SLPrint(&list_);
    const std::string out = ::testing::internal::GetCapturedStdout();   // 结束捕获并取出内容

    EXPECT_EQ(out, "1 2 3 4 5 \n");             // 每个元素后有一个空格，结尾一个换行
}

TEST_F(SeqListTest, PrintEmptyListOutputsOnlyNewline)
{
    ::testing::internal::CaptureStdout();
    SLPrint(&list_);
    EXPECT_EQ(::testing::internal::GetCapturedStdout(), "\n");
}

// 打印不应该改变顺序表的内容
TEST_F(SeqListTest, PrintDoesNotModifyList)
{
    FillOneToN(3);

    ::testing::internal::CaptureStdout();
    SLPrint(&list_);
    (void)::testing::internal::GetCapturedStdout();

    EXPECT_SEQ(&list_, 1, 2, 3);
}

// =============================================================================
// 十五、综合用例：多种操作交替执行，验证整体一致性
// =============================================================================

TEST_F(SeqListTest, MixedOperationsKeepListConsistent)
{
    FillOneToN(10);                             // 1..10
    SLPopFront(&list_);                         // 2..10
    SLPopBack(&list_);                          // 2..9
    SLPushFront(&list_, 0);                     // 0 2 3 4 5 6 7 8 9
    SLPushBack(&list_, 10);                     // 0 2 3 4 5 6 7 8 9 10
    SLInsert(&list_, 1, 1);                     // 0 1 2 3 ... 10

    EXPECT_EQ(SLDelete(&list_, 0), 0);          // 再把开头的 0 删掉，回到 1..10

    EXPECT_EQ(SLSize(&list_), 10);
    std::vector<SLDataType> expect(10);
    for (int i = 0; i < 10; ++i)
    {
        expect[i] = i + 1;
    }
    EXPECT_EQ(ToVector(&list_), expect);

    // 逐个查找，值 (i+1) 的下标应该正好是 i
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(LocateElem(&list_, i + 1), i) << "查找值 " << i + 1 << " 时下标不对";
    }
}

// 压力测试：大量插入/删除，验证扩容之后逻辑依然正确
TEST_F(SeqListTest, StressManyInsertsAndDeletes)
{
    const int n = 1000;
    for (int i = 0; i < n; ++i)
    {
        SLPushBack(&list_, i);
    }
    EXPECT_EQ(SLSize(&list_), n);
    EXPECT_GE(list_.capacity, n);

    // 每次都在中间插入一个 -1，长度翻倍
    for (int i = 0; i < n; ++i)
    {
        SLInsert(&list_, SLSize(&list_) / 2, -1);
    }
    EXPECT_EQ(SLSize(&list_), 2 * n);

    // 再把所有 -1 删掉，应该正好删了 n 个，并回到原来的序列
    int removed = 0;
    while (LocateElem(&list_, -1) != -1)
    {
        SLDelete(&list_, LocateElem(&list_, -1));
        ++removed;
    }
    EXPECT_EQ(removed, n);
    EXPECT_EQ(SLSize(&list_), n);

    std::vector<SLDataType> expect(n);
    for (int i = 0; i < n; ++i)
    {
        expect[i] = i;
    }
    EXPECT_EQ(ToVector(&list_), expect);
}

// =============================================================================
// 十六、越界访问的死亡测试（Death Test）
//     顺序表内部用 assert 做下标合法性检查，越界时会让程序主动终止。
//     EXPECT_DEATH 会在子进程中执行这段代码，用来验证"越界确实被拦住了"，
//     这样连"错误输入的防护"本身也变成了可自动验证的测试。
//     注意：assert 只在 Debug 配置（未定义 NDEBUG）下生效，
//           所以这组用例用 #ifndef NDEBUG 包起来，Release 下自动跳过。
// =============================================================================
#ifndef NDEBUG

// 取值时下标越界（合法范围是 [0, size-1]，这里传 i == size）
TEST(SeqListDeathTest, GetElemDiesWhenIndexOutOfRange)
{
    SL s;
    SLInit(&s);
    SLPushBack(&s, 1);

    EXPECT_DEATH(GetElem(&s, 1), "");   // 第二个参数为空表示"不关心输出内容，只要进程崩了就算通过"

    SLDestroy(&s);
}

// 删除时下标越界
TEST(SeqListDeathTest, DeleteDiesWhenIndexOutOfRange)
{
    SL s;
    SLInit(&s);
    SLPushBack(&s, 1);

    EXPECT_DEATH(SLDelete(&s, 5), "");

    SLDestroy(&s);
}

// 插入位置越界（合法范围是 [0, size]，这里传了 100）
TEST(SeqListDeathTest, InsertDiesWhenIndexGreaterThanSize)
{
    SL s;
    SLInit(&s);
    SLPushBack(&s, 1);

    EXPECT_DEATH(SLInsert(&s, 100, 0), "");

    SLDestroy(&s);
}


// 修改时下标越界（合法范围是 [0, size-1]）
TEST(SeqListDeathTest, ModifyDiesWhenIndexOutOfRange)
{
    SL s;
    SLInit(&s);
    SLPushBack(&s, 1);

    EXPECT_DEATH(SLModify(&s, 1, 0), "");

    SLDestroy(&s);
}
#endif  // !NDEBUG

// =============================================================================
// 十七、内存泄漏检查
//     用 CRT 的堆检查点（_CrtMemCheckpoint / _CrtMemDifference）在一段代码前后
//     各拍一张"内存快照"，比较两张快照的差异即可判断有没有泄漏。
//     这种方式比全局开启 _CRTDBG_LEAK_CHECK_DF 更精确：既不会把 gtest 框架
//     自身的长生命周期分配算进来，也不会被死亡测试的子进程干扰。
// =============================================================================
TEST(SeqListLeakTest, InitExpandAndDestroyDoesNotLeak)
{
#ifdef _MSC_VER
    // 让 CRT 的统计信息输出到控制台，便于泄漏时定位
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

    _CrtMemState before{};
    _CrtMemState after{};
    _CrtMemState diff{};

    _CrtMemCheckpoint(&before);         // 第一张内存快照

    {
        SL s;
        SLInit(&s);
        for (int i = 0; i < 1000; ++i)  // 插入 1000 个元素，覆盖多次 realloc 扩容的路径
        {
            SLPushBack(&s, i);
        }
        SLDestroy(&s);                  // 正确销毁，应该把内存全部还回去
    }

    _CrtMemCheckpoint(&after);          // 第二张内存快照

    // _CrtMemDifference 比较两张快照里"各类内存块的数量与总大小"，
    // 返回非 0 就说明这段代码净多出了内存块，即发生了内存泄漏
    if (_CrtMemDifference(&diff, &before, &after))
    {
        _CrtMemDumpStatistics(&diff);
        ADD_FAILURE() << "顺序表在 初始化 -> 多次扩容 -> 销毁 之后仍然存在内存泄漏";
    }
#endif
}