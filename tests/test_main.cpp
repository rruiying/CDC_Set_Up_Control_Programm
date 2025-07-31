#include <gtest/gtest.h>
#include <QCoreApplication>

int main(int argc, char **argv) {
    // 初始化Qt应用（因为我们使用Qt的类）
    QCoreApplication app(argc, argv);
    
    // 初始化Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // 运行所有测试
    return RUN_ALL_TESTS();
}