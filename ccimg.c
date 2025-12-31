#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 4096

// 解析十六进制字符串为长整型
long parse_hex(const char *hex_str) {
    char *endptr;
    long value = strtol(hex_str, &endptr, 16);  // 使用16进制解析
    
    // 检查是否解析成功
    if (endptr == hex_str) {
        return -1;  // 没有解析到任何数字
    }
    
    // 跳过可能的前缀（0x或0X）
    while (*endptr != '\0') {
        if (!isspace((unsigned char)*endptr) && 
            !(endptr == hex_str + 1 && hex_str[0] == '0' && 
              (hex_str[1] == 'x' || hex_str[1] == 'X'))) {
            return -1;  // 有非法字符
        }
        endptr++;
    }
    
    return value;
}

int main(int argc, char *argv[]) {
    FILE *file_a, *file_b;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long offset;
    
    // 检查参数数量
    if (argc != 4) {
        fprintf(stderr, "用法: %s <源文件> <目标文件> <目标位置(十六进制)>\n", argv[0]);
        fprintf(stderr, "示例:\n");
        fprintf(stderr, "  %s a.txt b.txt 0x100   # 复制到256字节处\n", argv[0]);
        fprintf(stderr, "  %s a.txt b.txt 0xFF    # 复制到255字节处\n", argv[0]);
        fprintf(stderr, "  %s a.txt b.txt 400     # 复制到1024字节处\n", argv[0]);
        fprintf(stderr, "  %s a.txt b.txt 0x1A3F  # 复制到6719字节处\n", argv[0]);
        return 1;
    }
    
    // 解析偏移量（十六进制）
    offset = parse_hex(argv[3]);
    if (offset < 0) {
        fprintf(stderr, "错误: 偏移量必须是有效的十六进制数\n");
        fprintf(stderr, "      可以使用格式: 0x100, 0xFF, 400, 0x1A3F\n");
        return 1;
    }
    
    // 显示解析结果
    printf("目标位置: 0x%lX (%ld 字节)\n", offset, offset);
    
    // 打开源文件（a文件）
    file_a = fopen(argv[1], "rb");
    if (file_a == NULL) {
        perror("打开源文件失败");
        return 1;
    }
    
    // 打开目标文件（b文件）
    file_b = fopen(argv[2], "rb+");  // 使用"rb+"模式，可读写二进制文件
    if (file_b == NULL) {
        // 如果文件不存在，创建新文件
        file_b = fopen(argv[2], "wb+");
        if (file_b == NULL) {
            perror("打开/创建目标文件失败");
            fclose(file_a);
            return 1;
        }
    }
    
    // 移动到目标位置
    if (fseek(file_b, offset, SEEK_SET) != 0) {
        perror("定位目标文件位置失败");
        fclose(file_a);
        fclose(file_b);
        return 1;
    }
    
    // 复制文件内容
    printf("正在复制文件内容...\n");
    size_t total_bytes = 0;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file_a)) > 0) {
        size_t bytes_written = fwrite(buffer, 1, bytes_read, file_b);
        if (bytes_written != bytes_read) {
            perror("写入文件失败");
            fclose(file_a);
            fclose(file_b);
            return 1;
        }
        total_bytes += bytes_written;
    }
    
    if (ferror(file_a)) {
        perror("读取源文件失败");
        fclose(file_a);
        fclose(file_b);
        return 1;
    }
    
    // 关闭文件
    fclose(file_a);
    fclose(file_b);
    
    printf("复制完成！\n");
    printf("成功复制 0x%zX (%zu 字节) 到 %s 的偏移量 0x%lX 处\n", 
           total_bytes, total_bytes, argv[2], offset);
    
    return 0;
}